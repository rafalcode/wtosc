/* gwa3: a multiharmnic version of gwa2 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<math.h>

#define ARBSZ 128
#define GBUF 64
#define AMP 0x7FFF

typedef struct /* wh_t, wavheader type */
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

wh_t *hdr4chunk2(int sfre, int nsamps, int numwl) /* version "2" is hardcoded version */
{
    // nsecs is seconds duration
    int totsamps= nsamps*numwl;

    wh_t *wh=malloc(sizeof(wh_t));
    memcpy(wh->id, "RIFF", 4*sizeof(char));
    memcpy(wh->fstr, "WAVEfmt ", 8*sizeof(char));
    memcpy(wh->datastr, "data", 4*sizeof(char));
    wh->fmtnum=16;
    wh->pcmnum=1;
    wh->nchans=1;
    wh->sampfq=sfre; /* fed in to function */
    wh->bipsamp=16;
    wh->bypc = wh->bipsamp/8;
    wh->byid = totsamps * wh->bypc * wh->nchans; /* this decides the true byte payload */
    wh->glen = wh->byid + 36;
    wh->byps = wh->nchans*wh->sampfq*wh->bypc;
    return wh;
}

int main(int argc, char *argv[])
{
    if(argc != 5) {
        printf("4 args please: 1) sample rate 2) number of samples per wavelength 3) num wavelengthsi 4) number of harmonics\n");
        exit(EXIT_FAILURE);
    }
    int i, k;

    /* create header:
     * with the time and the sampling frequency we can calculate how many actual samples we're going to need. They could be 1 2 4 byte or 1 or 2 channels though */
    int srate=atoi(argv[1]);
    int numsamps=atoi(argv[2]);
    int numwl=atoi(argv[3]); // num wavelengths
    int nh=atoi(argv[4]); 
    printf("Under a sampling rate of %i, with %i numsamps, output audio freq will be %2.6fHz\n", srate, numsamps, (float)srate/numsamps); 

#ifdef DBG
    printf("byid=%i\n", whd->byid); 
#endif
    float rstep=2*M_PI/numsamps; // radian step. yes, numsamps *is* per wavelength
#ifdef DBG
    printf("rstep=%2.6f\n", rstep); 
#endif

    double *mults=malloc(nh*sizeof(double));
    for(i=0;i<nh;++i)
        mults[i] = 1./(i+1);
    double *rvals=calloc(numsamps, sizeof(double));
    for(i=0;i<numsamps;++i)
       for(k=0;k<nh;++k)
           rvals[i] += mults[k]*sin(i*rstep*(k+1));
    // rvals are inflated, but no in regular manner, at least I don't wnat to depend on that. So just find it out.
    double rmax=0, rmin=1;
    for(i=0;i<numsamps;++i) {
        if(rvals[i]>rmax)
            rmax=rvals[i];
        if(rvals[i]< rmin)
            rmin=rvals[i];
    }
    double nmlizer=(rmax>(-rmin))?rmax:(-rmin);
    printf("rmax=%2.6f, rmin=%2.6f nmlizer=%2.6f\n", rmax, rmin, nmlizer); 

    short *svals=calloc(numsamps, sizeof(short));
    double rvaln; // rval normalized
    for(i=0;i<numsamps;++i) {
       rvaln = rvals[i]/nmlizer;
       svals[i] = (rvaln>0)? (short)(.5+AMP*rvaln): (short)(-.5+AMP*rvaln);
    }
    free(rvals);

    unsigned char *lends=malloc(numsamps*2*sizeof(unsigned char)); // little endians (dpay in other code)
    for(i=0;i<numsamps;++i) {
        lends[2*i] = svals[i]&0xFF;
        lends[2*i+1] = (svals[i]>>8)&0xFF; /* exclusively small endian, LSB's come out first */
    }
    free(svals);

    // at this stage we only have 1 wavelengths worth
    // time to prepare our output wav file:
    wh_t *whd=hdr4chunk2(srate, numsamps, numwl);
    FILE *outwavfp=fopen("gwa3out.wav", "wb");
    fwrite(whd, sizeof(char), sizeof(wh_t), outwavfp);
    for(i=0;i<numwl;++i) 
        fwrite(lends, sizeof(unsigned char), 2*numsamps, outwavfp);
    fclose(outwavfp);

    free(lends);
    free(whd);
    free(mults);
    return 0;
}
