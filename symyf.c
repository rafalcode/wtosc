/* This is for use as an oscillator, short values are put in a ring of certain size and
 * then this is fed a total number of values requires and it loops around the ring
 * until it gets them.
 *
 * sr0.c is the proof of concept  */

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<math.h>

#define MAXSHORT 0x7FFF
#define AMPL 1.0*MAXSHORT
#define SRATE 44100.0
#define ISRATE 44100 /* integer version */
#define A440 440.

typedef struct
{
    float assocfl; /* float associated with this */
    float sonicv; /* sonic value */
} wavt_t;

struct s_r_n /* short int ring node */
{
    short s;
    struct s_r_n *nx;
};
typedef struct s_r_n srn_t;

typedef struct
{
    char id[4]; // should always contain "RIFF"
    int glen;    // general length: total file length minus 8, could because the types so far seen (id and glen itself) are actually 8 bytes
    char fstr[8]; // format string should be "WAVEfmt ": actually two 4chartypes here.
    int fmtnum; // format number 16 for PCM format
    short pcmnum; // PCM number 1 for PCM format
    short nchans; // num channels
    int sampfq; // sampling frequency: CD quality is 44100, 48000 is also common.
    int byps; // BYtes_Per_Second (aka, BlockAlign) = numchannels* bytes per sample*samplerate. stereo shorts at 44100 . should be 176k.
    short bypc; // BYtes_Per_Capture, bipsamp/8 most probably. A capture is the same as a sample if mono, but half a sample if stereo. bypc usually 2, a short.
    short bipsamp; // bits_per_sample: CD quality is 16.
    char datastr[4]; // should always contain "data"
    int byid; // BYtes_In_Data;
} wh_t; /* wav header type */

wh_t *hdr4chunk(int sfre, char nucha, int numsamps) /* a header for a file chunk of certain siez */
{
    wh_t *wh=malloc(sizeof(wh_t));
    strncpy(wh->id, "RIFF", 4);
    strncpy(wh->fstr, "WAVEfmt ", 8);
    strncpy(wh->datastr, "data", 4);
    wh->fmtnum=16;
    wh->pcmnum=1;
    wh->nchans=nucha; /* fed in to function */
    wh->sampfq=sfre; /* fed in to function */
    wh->bipsamp=16; /* shorts */
    wh->bypc=wh->bipsamp/8;
    wh->byid=numsamps*wh->nchans*wh->bypc;
    wh->glen=wh->byid+36;
    wh->byps=wh->nchans*wh->sampfq*wh->bypc;
    return wh;
}

srn_t *creasrn0(unsigned ssz) /* create empty ring of size ssz */
{
    int i;
    srn_t *mou /* mouth with a tendency to eat the tail*/, *ttmp /* type temporary */;

    mou=malloc(sizeof(srn_t));
    mou->nx=NULL;
    ttmp=mou;
    for(i=1;i<ssz;++i) {
        ttmp->nx=malloc(sizeof(srn_t));
        ttmp=ttmp->nx; /* with ->nmove on */
    }
    ttmp->nx=mou;
    return mou;
}

srn_t *creasrn(unsigned ssz, short uniformval) /* create empty ring of size ssz */
{
    int i;
    srn_t *mou /* mouth with a tendency to eat the tail*/, *ttmp /* type temporary */;

    mou=malloc(sizeof(srn_t));
    mou->s=0;
    mou->nx=NULL;
    ttmp=mou;
    for(i=1;i<ssz;++i) {
        ttmp->nx=malloc(sizeof(srn_t));
        ttmp->nx->s=uniformval;
        ttmp=ttmp->nx; /* with ->nmove on */
    }
    ttmp->nx=mou;
    return mou;
}

void prtring(srn_t *mou)
{
    srn_t *st=mou;
    do {
        printf("%d ", st->s);
        st=st->nx;
    } while (st !=mou);
    putchar('\n');
    return;
}

void prtimesring(srn_t *mou, unsigned ntimes)
{
    unsigned i=0;
    srn_t *st=mou;
    do {
        printf("%d ", st->s);
        st=st->nx;
        i++;
    } while (i !=ntimes);
    putchar('\n');
    return;
}

void freerestring(srn_t *lastmou, unsigned numrest) /* free the rest of the ring .. but what happens if we eant to maeke it begger? */
{
    unsigned i=0;
    srn_t *st=lastmou->nx, *nxt;
    while(i<numrest) {
        nxt=st->nx; /* we store the nx of this nx */
        free(st); /* now we can delete it */
        i++;
        st=nxt;
    }
    lastmou->nx=st;
}

void freering(srn_t *mou)
{
    srn_t *st=mou->nx, *nxt;
    while (st !=mou) {
        nxt=st->nx; /* we store the nx of this nx */
        free(st); /* now we can delete it */
        st=nxt;
    }
    free(mou);
}

short *prtimesring2a(srn_t *mou, unsigned ntimes)
{
    unsigned i=0;
    short *sbuf=malloc(ntimes*sizeof(short));
    srn_t *st=mou;
    do {
        sbuf[i]=st->s;
        st=st->nx;
        i++;
    } while (i !=ntimes);
    return sbuf;
}

int main(int argc, char *argv[])
{
    if(argc != 4) {
        printf("Program to issue a note via wavetable synthesis. The primary wavetable is calculated using A440.\n");
        printf("Wav table produced will be mono and at the standard 44100Hz sampling rate.\n");
        printf("Usage: 3 arguments: 1) floating pt frequency value 2) seconds' duration (fp fine) 3) output wavfle name.\n");
        exit(EXIT_FAILURE);
    }
    int i;
    unsigned soulen=(unsigned)(.5+atof(argv[2])*SRATE); /* length of sound, in samples! */

    float wtsamps=SRATE/A440;
    unsigned iwtsamps=(unsigned)(.5+wtsamps); /* integer number of samples for our wavetable */
    wavt_t *wt=malloc(iwtsamps*sizeof(wavt_t));
    float wtstpsz=2.*M_PI/wtsamps; /* the increment size ... yes it can be a float */
    /*OK create the wavetable */
    for(i=0;i<iwtsamps;++i) {
        wt[i].assocfl =wtstpsz*i;
        wt[i].sonicv=AMPL*sin(wt[i].assocfl);
    }

    /* Another frequency */
    float nsamps=SRATE/atof(argv[1]);
    unsigned insamps=(unsigned)(.5+nsamps);
    srn_t *m=creasrn0(insamps);
    srn_t *tsrn=m;
    float stpsz=2.*M_PI/((float)insamps); /* here we recognise that insamps is a different number to nsamps */

    int j, k=0;
    float kincs, xprop;
    for(i=0;i<insamps;++i) {
        kincs=stpsz*i;
        for(j=k; j<iwtsamps-1; ++j)
            if(kincs>=wt[j].assocfl)
                continue;
            else
                break;
        k=j-1;
        // xprop=(kincs-wt[k].assocfl)/(wt[j].assocfl-wt[k].assocfl);
        xprop=(kincs-wt[k].assocfl)/wtstpsz;
        tsrn->s=(short)(.5+wt[k].sonicv + xprop*(wt[j].sonicv-wt[k].sonicv));
        tsrn=tsrn->nx;
    }

    short *sbuf=prtimesring2a(m, soulen); /* nsamps are looped over to produce soulen's worth */
    wh_t *hdr=hdr4chunk((int)SRATE, 1, soulen);

    FILE *fout=fopen(argv[3], "wb");
    fwrite(hdr, sizeof(char), 44, fout);
    fwrite(sbuf, sizeof(short), soulen, fout);
    fclose(fout);
    free(hdr);
    free(sbuf);
    free(wt);

    freering(m);

    return 0;
}
