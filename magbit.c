/* magnify a certain bit */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct /* time point, tpt */
{
    int m, s, h;
} tpt;

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

tpt *s2tp(char *str)
{
    tpt *p=malloc(sizeof(tpt));
    char *tc, staminchar[5]={0};
    if( (tc=strchr(str, ':')) == NULL) {
        printf("There are no minutes \n"); 
        p->m=0;
        tc=str;
    } else {
        strncpy(staminchar, str, (int)(tc-str));
        p->m=atoi(staminchar);
        tc++;
    }

    char *ttc, stasecchar[5]={0};
    if( (ttc=strchr(tc, '.')) == NULL) {
        printf("There are no seconds\n"); 
        p->s=0;
        ttc=tc;
    } else {
        strncpy(stasecchar, tc, (int)(ttc-tc));
        p->s=atoi(stasecchar);
        ttc++;
    }
    char stahunschar[5]={0};
    strcpy(stahunschar, ttc);
    p->h=atoi(stahunschar);
    if((strlen(stahunschar))==1)
        p->h*=10;

    return p;
}

wh_t *hdr4chunk(int sfre, char nucha, int certainsz) /* a header for a file chunk of certain siez */
{
    wh_t *wh=malloc(sizeof(wh_t));
    strncpy(wh->id, "RIFF", 4);
    strncpy(wh->fstr, "WAVEfmt ", 8);
    strncpy(wh->datastr, "data", 4);
    wh->fmtnum=16;
    wh->pcmnum=1;
    wh->nchans=nucha; /* fed in to function */
    wh->sampfq=sfre; /* fed in to function */
    wh->glen=certainsz-8;
    wh->bipsamp=16;
    wh->bypc=wh->bipsamp/8;
    wh->byps=wh->nchans*wh->sampfq*wh->bypc;
    wh->byid=wh->glen-36;
    return wh;
}

int hdrchk(wh_t *inhdr)
{
    /* OK .. we test what sort of header we have */
    if ( inhdr->id[0] != 'R'
            || inhdr->id[1] != 'I' 
            || inhdr->id[2] != 'F' 
            || inhdr->id[3] != 'F' ) { 
        printf("ERROR: RIFF string problem\n"); 
        return 1;
    }

    if ( inhdr->fstr[0] != 'W'
            || inhdr->fstr[1] != 'A' 
            || inhdr->fstr[2] != 'V' 
            || inhdr->fstr[3] != 'E' 
            || inhdr->fstr[4] != 'f'
            || inhdr->fstr[5] != 'm' 
            || inhdr->fstr[6] != 't'
            || inhdr->fstr[7] != ' ' ) { 
        printf("ERROR: WAVEfmt string problem\n"); 
        return 1;
    }

    if ( inhdr->datastr[0] != 'd'
            || inhdr->datastr[1] != 'a' 
            || inhdr->datastr[2] != 't' 
            || inhdr->datastr[3] != 'a' ) { 
        printf("WARNING: header \"data\" string does not come up\n"); 
        return 1;
    }
    if ( inhdr->fmtnum != 16 ) {
        printf("WARNING: fmtnum is %i, while it's better off being %i\n", inhdr->fmtnum, 16); 
        return 1;
    }
    if ( inhdr->pcmnum != 1 ) {
        printf("WARNING: pcmnum is %i, while it's better off being %i\n", inhdr->pcmnum, 1); 
        return 1;
    }

    printf("There substantial evidence in the non-numerical fields of this file's header to think it is a wav file\n");

    printf("glen: %i\n", inhdr->glen);
    printf("byid: %i\n", inhdr->byid);
    printf("nchans: %d\n", inhdr->nchans);
    printf("sampfq: %i\n", inhdr->sampfq);
    printf("byps: %i\n", inhdr->byps);
    printf("bypc, bytes by capture (count of data in shorts): %d\n", inhdr->bypc);
    printf("bipsamp: %d\n", inhdr->bipsamp);

    if(inhdr->glen+8-44 == inhdr->byid)
        printf("Good, \"byid\" (%i) is 36 bytes smaller than \"glen\" (%i).\n", inhdr->byid, inhdr->glen);
    else {
        printf("WARNING: glen (%i) and byid (%i)do not show prerequisite normal relation(diff is %i).\n", inhdr->glen, inhdr->byid, inhdr->glen-inhdr->byid); 
    }
    // printf("Duration by glen is: %f\n", (float)(inhdr->glen+8-wh_tsz)/(inhdr->nchans*inhdr->sampfq*inhdr->byps));
    printf("Duration by byps is: %f\n", (float)(inhdr->glen+8-44)/inhdr->byps);

    if( (inhdr->bypc == inhdr->byps/inhdr->sampfq) && (inhdr->bypc == 2) )
        printf("bypc complies with being 2 and matching byps/sampfq. Data values can therefore be recorded as signed shorts.\n"); 

    return 0;
}

unsigned char *xfw(char *inwf, char *tp, wh_t *inhdr, unsigned ncsamps) /* xfw: extract from wav: the wav header can be used as the metadata for the returned buffer */
{
    FILE *inwavfp=fopen(inwf,"rb");
    if ( inwavfp == NULL ) {
        fprintf(stderr,"Can't open input file %s", inwf);
        exit(EXIT_FAILURE);
    }

    if ( fread(inhdr, sizeof(wh_t), sizeof(char), inwavfp) < 1 ) {
        printf("Can't read file header\n");
        exit(EXIT_FAILURE);
    }

    tpt *p=s2tp(tp);

    int point=inhdr->byps*(p->m*60 + p->s) + p->h*inhdr->byps/100;
#ifdef DBG
    printf("stamin=%u, stasec=%u, stahuns=%u\n", p->m, p->s, p->h);
    printf("timepoint is %i inside %i which is %.1f%% in.\n", point, inhdr->byid, (point*100.)/inhdr->byid); 
#endif
    if(point >= inhdr->byid) {
        printf("Specified timepoint at which to sample is over the size of the wav file. Aborting.\n");
        exit(EXIT_FAILURE);
    }

    /* recreate based on samples taken */
    inhdr->byid = inhdr->bypc*inhdr->nchans*ncsamps; /* number of bytes to extract */
    inhdr->glen = inhdr->byid+36;
    unsigned char *bf=malloc(inhdr->byid*sizeof(unsigned char));
    fseek(inwavfp, point, SEEK_CUR);
    if ( fread(bf, inhdr->byid, sizeof(unsigned char), inwavfp) < 1 ) {
        printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
        exit(EXIT_FAILURE);
    }
    fclose(inwavfp);
    free(p);

    return bf;
}

int main(int argc, char *argv[])
{
    if(argc != 6) {
        printf("Usage: samples from a wav's time point Args: 1) Name of wavfile 2) mm:ss.hh string say to where sampling should begin.\n");
        printf("\t3) numbsamps 4) number of times magnification is required 5) output wavfilename\n");
        exit(EXIT_FAILURE);
    }
    int i, ii;

    wh_t *twhdr=calloc(1, sizeof(wh_t));
    unsigned ncsamps=atoi(argv[3]);
    unsigned magtimes=atoi(argv[4]);
    unsigned char *bf= xfw(argv[1], argv[2], twhdr, ncsamps);
    /* need to convert bytes in smallendian to shorts */
    int byidasshort=twhdr->byid/2;
    short *vals=malloc(byidasshort*sizeof(short));

    printf("bytes in data from hdr: %d\n", twhdr->byid); 
    printf("nsamps in data: %d\n", twhdr->byid/twhdr->bypc); 
    for(i=0;i<twhdr->byid;i+=2) {
        ii=i/2;
        vals[ii]=((short)bf[i+1])<<8; /* will also set lower bytes to zero */
        vals[ii] |=(short)bf[i];
    }

#ifdef DBG
    if(twhdr->nchans ==2) {
        /* first channel */
        for(i=0;i<byidasshort;i+=2) 
            printf("%hx ", vals[i]);
        printf("\n"); 
        /* second channel */
        for(i=1;i<byidasshort;i+=2) 
            printf("%hx ", vals[i]);
        printf("\n"); 
    } else { /* it's in mono */
        for(i=0;i<byidasshort; i++)
            printf("%hx ", vals[i]);
        printf("\n"); 
    }
#endif
    twhdr->byid = magtimes*ncsamps*twhdr->nchans;
    twhdr->glen = twhdr->byid +36;
    short *sbuf=malloc((twhdr->byid/2)*sizeof(short)); /* our sound info buffer */
    for(i=0;i<magtimes;++i) 
        memcpy(sbuf+i*ncsamps*twhdr->nchans, vals, ncsamps*twhdr->nchans*sizeof(short));
    FILE *fout=fopen(argv[5], "wb");
    fwrite(twhdr, sizeof(char), 44, fout);
    fwrite(sbuf, sizeof(short), twhdr->nchans*ncsamps*magtimes, fout); /* shorts will get written as small endian bytes in file in x86 system */
    fclose(fout);

    free(twhdr);
    free(vals);
    free(bf);
    free(sbuf);
    return 0;
}
