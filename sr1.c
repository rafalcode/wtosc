/* This is for use as an oscillator, short values are put in a ring of certain size and
 * then this is fed a total number of values requires and it loops around the ring
 * until it gets them.
 *
 * sr0.c is the proof of concept  */

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>

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

short *prtimesring2asynco(srn_t *mou, unsigned ntimes) /* a chance your arm" function, splits up a second into 30 equal parts and a llows a "silence recipe" */
{
    /* this doesn't work because of the little endian */
    unsigned i=0, j=0;
    unsigned syncoint=ntimes/6;
    unsigned shiftrecipe=0xFFFFFFFE;
    char bit=shiftrecipe&0x01;
    srn_t *m2=creasrn(1, 0x00);
    short *sbuf=malloc(ntimes*sizeof(short));
    srn_t *st=mou;
    srn_t *st2=m2;
    do {
        if(bit) {
            sbuf[i]=st->s;
            st=st->nx;
        } else {
            sbuf[i]=st2->s;
            st2=st2->nx;
        }
        if(i++==syncoint) {
            j++;
            bit=(shiftrecipe>>j)&0x01;
            syncoint+=syncoint;
        }
    } while (i !=ntimes);
    freering(m2);
    return sbuf;
}

int main(int argc, char *argv[])
{
    if(argc != 4) {
        printf("Testing a short integer ring, Please give size of ring and the total number of values you want generated loopwise on the ringi, and output wav\n");
        exit(EXIT_FAILURE);
    }
    srn_t *m=creasrn(atoi(argv[1]), 0x7FFF);
    // prtring(m);
    unsigned soulen=atoi(argv[2]); /* length of sound, in samples! */
    short *sbuf=prtimesring2asynco(m, soulen);
    wh_t *hdr=hdr4chunk(44100, 1, soulen);

    FILE *fout=fopen(argv[3], "wb");
    fwrite(hdr, sizeof(char), 44, fout);
    fwrite(sbuf, sizeof(short), soulen, fout);
    fclose(fout);
    free(hdr);
    free(sbuf);

    freering(m);

    return 0;
}
