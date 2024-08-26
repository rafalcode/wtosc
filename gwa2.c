/* gwa2: an easier gwa
 * accepts number of samples per wavelength */

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
    if(argc != 4) {
        printf("3 args please: 1) sample rate 2) number of samples per wavelength 3) num wavelengths\n");
        exit(EXIT_FAILURE);
    }
    int i, j;

    /* create header:
     * with the time and the sampling frequency we can calculate how many actual samples we're going to need. They could be 1 2 4 byte or 1 or 2 channels though */
    int srate=atoi(argv[1]);
    int numsamps=atoi(argv[2]);
    int numwl=atoi(argv[3]); // num wavelengths
    printf("Under a sampling rate of %i, with %i numsamps, output audio freq will be %2.6fHz\n", srate, numsamps, (float)srate/numsamps); 

    wh_t *whd=hdr4chunk2(srate, numsamps, numwl);

    FILE *outwavfp=fopen("gwa2out.wav", "wb");
    fwrite(whd, sizeof(char), sizeof(wh_t), outwavfp);
    unsigned char *dpay=malloc(whd->byid*sizeof(unsigned char)); /* data payload */
#ifdef DBG
    printf("byid=%i\n", whd->byid); 
#endif
    float rstep=2*M_PI/numsamps; // radian step. yes, numsamps *is* per wavelength
#ifdef DBG
    printf("rstep=%2.6f\n", rstep); 
#endif

    double rval; // value for raw radians.
    short sval;
    for(i=0;i<numwl;++i) {
        for(j=0;j<numsamps;++j) {
            rval = sin(i*2*M_PI+j*rstep);
            sval = (rval>0)? (short)(.5+AMP*rval): (short)(-.5+AMP*rval);
            dpay[2*(i*numsamps+j)] = sval&0xFF;
            dpay[2*(i*numsamps+j)+1] = (sval>>8)&0xFF; /* exclusively small endian, LSB's come out first */
#ifdef DBG2
            // check those pesky indices of dpay:
            printf("%i:%i\n", 2*(i*numsamps+j), 2*(i*numsamps+j)+1);
#endif
#ifdef DBG2
            printf("%i/%02x/%02x/%02x ", sval, sval, dpay[i*numsamps*2+j], dpay[i*numsamps*2+j+1]);
#endif
        }
    }
#ifdef DBG2
    printf("\n");
#endif

    fwrite(dpay, sizeof(char), whd->byid, outwavfp);
    fclose(outwavfp);

    free(dpay);
    free(whd);
    return 0;
}
