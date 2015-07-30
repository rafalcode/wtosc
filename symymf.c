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
#define NUMNOTES 8

struct svals_t /* a struct with 2 floats: assocfl, and sonicv */
{
    float assocfl; /* float associated with this */
    float sonicv; /* sonic value */
};

typedef struct /* wavt_t */
{
    struct svals_t *d;
    float *svalranges;
    float fsamps;
    unsigned nsamps;
    float stpsz;
} wavt_t;

struct s_r_n /* short int ring node struct: not typedef'd */
{
    short s;
    struct s_r_n *nx;
};
typedef struct s_r_n sr_t;

typedef struct /* srp_t, a struct to hold a ptr to s_r_n and its size */
{
    sr_t *sr;
    unsigned sz;
} srp_t;

typedef struct /* WAV hdr struct */
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

wh_t *hdr4chunk(int sfre, char nucha, int numsamps) /* a header for a file chunk of certain size */
{
    wh_t *wh=malloc(sizeof(wh_t)); /* always 44 of course, but what the heck */
    strncpy(wh->id, "RIFF", 4);
    strncpy(wh->fstr, "WAVEfmt ", 8);
    strncpy(wh->datastr, "data", 4);
    wh->fmtnum=16;
    wh->pcmnum=1;
    wh->nchans=nucha; /* this fed in */
    wh->sampfq=sfre; /* fed in to function */

    wh->bipsamp=16; /* hard-coded to shorts, no 24bit here */
    wh->bypc=wh->bipsamp/8;
    wh->byid=numsamps*wh->nchans*wh->bypc;

    wh->glen=wh->byid+36;
    wh->byps=wh->nchans*wh->sampfq*wh->bypc;
    return wh;
}

sr_t *creasrn0(unsigned ssz) /* create empty ring of size ssz */
{
    int i;
    sr_t *mou /* mouth with a tendency to eat the tail*/, *ttmp /* type temporary */;

    mou=malloc(sizeof(sr_t));
    mou->nx=NULL;
    ttmp=mou;
    for(i=1;i<ssz;++i) {
        ttmp->nx=malloc(sizeof(sr_t));
        ttmp=ttmp->nx; /* with ->nmove on */
    }
    ttmp->nx=mou;
    return mou;
}

void prtring(sr_t *mou)
{
    sr_t *st=mou;
    do {
        printf("%d ", st->s);
        st=st->nx;
    } while (st !=mou);
    putchar('\n');
    return;
}

void prtimesring(sr_t *mou, unsigned ntimes)
{
    unsigned i=0;
    sr_t *st=mou;
    do {
        printf("%d ", st->s);
        st=st->nx;
        i++;
    } while (i !=ntimes);
    putchar('\n');
    return;
}

void freerestring(sr_t *lastmou, unsigned numrest) /* free the rest of the ring .. but what happens if we eant to maeke it begger? */
{
    unsigned i=0;
    sr_t *st=lastmou->nx, *nxt;
    while(i<numrest) {
        nxt=st->nx; /* we store the nx of this nx */
        free(st); /* now we can delete it */
        i++;
        st=nxt;
    }
    lastmou->nx=st;
}

void freering(sr_t *mou)
{
    sr_t *st=mou->nx, *nxt;
    while (st !=mou) {
        nxt=st->nx; /* we store the nx of this nx */
        free(st); /* now we can delete it */
        st=nxt;
    }
    free(mou);
}

short *prtimesring2a(srp_t *sra, unsigned nnotes, unsigned ntimes)
{
    unsigned i=0, ii, j;
    short *sbuf=malloc(ntimes*nnotes*sizeof(short));
    sr_t *st;
    for(j=0;j<nnotes;++j) {
        ii=0;
        st=sra[j].sr;
        do {
            sbuf[i++]=st->s;
            st=st->nx;
            ii++;
        } while (ii !=ntimes);
    }
    return sbuf;
}

int main(int argc, char *argv[])
{
    if(argc != 3) {
        printf("Testing a short integer ring, 1) number of seconds, will be multiplied by 44100 to get samples 2) output wav\n");
        exit(EXIT_FAILURE);
    }
    int i;
    unsigned sndlen=44100*atoi(argv[1]); /* length of sound, for each notes: NUMNOTES of these will be generated */

    float fqa[NUMNOTES]={160., 220.25, 330.5, 441., 551.25, 800., 1200., 2300.};

    /* setting up the "model wavetable" */
    wavt_t *wt=malloc(sizeof(wavt_t));
    wt->fsamps=SRATE/fqa[2]; /* number of samples available for each wavelngth at this frequency. As a float */
    wt->nsamps=(unsigned)(.5+wt->fsamps); /* above as an unsigned int */
    wt->d=malloc(wt->nsamps*sizeof(struct svals_t));
    wt->svalranges=malloc((wt->nsamps-1)*sizeof(struct svals_t));
    wt->stpsz=2.*M_PI/((float)wt->nsamps); /* distance in radians btwn each sampel int he wavelength */
    /* OK create the wavetable */
    wt->d[0].assocfl = 0.;
    wt->d[0].sonicv=AMPL*sin(wt->d[0].assocfl);
    for(i=1;i<wt->nsamps;++i) {
        wt->d[i].assocfl =wt->stpsz*i;
        /* this is the key value allocation ... in this case, as it's a sine wave, it's easy */
        wt->d[i].sonicv=AMPL*sin(wt->d[i].assocfl);
        wt->svalranges[i-1]= wt->d[i].sonicv - wt->d[i-1].sonicv; /* the difference with the previous value, used for interpolating */
    }
    /* so our wavetable has been created, as you'll note it only has accurately representative values for one frequency,
     * but we're still going to use for other frequencies */

    float fsamps;
    sr_t *tsr;
    float stpsz;

    int j, k=0, m;
    float kincs, xprop;

    srp_t *sra=malloc(NUMNOTES*sizeof(srp_t)); /* array of rings */
    for(m=0;m<NUMNOTES;++m) { /* loop for our frequency range */
        fsamps=SRATE/fqa[m];
        sra[m].sz=(unsigned)(.5+fsamps);
        sra[m].sr=creasrn0(sra[m].sz);
        tsr=sra[m].sr;
        stpsz=2.*M_PI/((float)sra[m].sz);

        for(i=0;i<sra[m].sz;++i) {
            kincs=stpsz*i;
            for(j=k; j<wt->nsamps-1; ++j) /* here we cycle through the model wavetable samplepoints */
                if(kincs>=wt->d[j].assocfl) /* d[0].assocfl always zero, so the next "continue" will alwsays increment j to 1, even if k=0 */
                    continue;
                else
                    break;
            k=j-1; /* edge condition of k watched, the above continue will ensure it's never zero */
            xprop=(kincs-wt->d[k].assocfl)/wt->stpsz;
            tsr->s=(short)(.5+wt->d[k].sonicv + xprop*wt->svalranges[k]);
            tsr=tsr->nx;
        }
    }

    short *sbuf; /* our sound info buffer */
    sbuf=prtimesring2a(sra, NUMNOTES, sndlen); /* nsamps are looped over to produce sndlen's worth */
    wh_t *hdr=hdr4chunk((int)SRATE, 1, NUMNOTES*sndlen);
    FILE *fout=fopen(argv[2], "wb");
    fwrite(hdr, sizeof(char), 44, fout);
    fwrite(sbuf, sizeof(short), NUMNOTES*sndlen, fout);
    fclose(fout);

    free(hdr);
    free(sbuf);
    free(wt->svalranges);
    free(wt->d);
    free(wt);
    for(i=0;i<NUMNOTES;++i) 
        freering(sra[i].sr);
    free(sra);

    return 0;
}
