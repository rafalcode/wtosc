/*  lopwav. the idea is to lop off either the start or the end of the file. All from the command line */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include	<math.h>

#define MXSHV 0x7FFF /* max short value */
#define AMPL 1.*MXSHV
#define		LEFT_FREQ		110.0
#define		RIGHT_FREQ		160.0

#define ARBSZ 128

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

long fszfind(FILE *fp)
{
    rewind(fp);
    fseek(fp, 0, SEEK_END);
    long fbytsz = ftell(fp);
    rewind(fp);
    return fbytsz;
}

wh_t *hdr4chunk(int sfre, char nucha, int secs) /* a header for a file chunk of certain siez */
{
    /* the following fields can almost be hard coded */
    wh_t *wh=malloc(sizeof(wh_t));
    strncpy(wh->id, "RIFF", 4);
    strncpy(wh->fstr, "WAVEfmt ", 8);
    strncpy(wh->datastr, "data", 4);
    /* for the canonical stereo CD quality you have this */
    wh->fmtnum=16;
    wh->pcmnum=1;
    wh->bipsamp=16;
    wh->bypc=wh->bipsamp/8;
    /* now the specifics */
    wh->nchans=nucha; /* fed in to function */
    wh->sampfq=sfre; /* fed in to function */
    wh->byps=wh->nchans*wh->sampfq*wh->bypc;

    int certainsz=sizeof(wh_t) + secs*wh->byps;
    wh->glen=certainsz-8;
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

int main(int argc, char *argv[])
{
    if(argc != 2) {
        printf("Usage: simple sine synthesis. Stereo shorts presumed: Arg is prefix for outputfilename.\n");
        exit(EXIT_FAILURE);
    }
    unsigned a1sz=strlen(argv[1]);
    char *ofn=calloc(a1sz+5, sizeof(char));
    strcpy(ofn, argv[1]);
    strcat(ofn, ".wav");
    FILE *outwavfp= fopen(ofn,"wb");
    const int times=3;
    int secs=3*times;

    wh_t *ohdr=hdr4chunk(44100, 2, secs);
    /* write we can already write out inhdr to there */
    fwrite(ohdr, sizeof(char), 44, outwavfp);

    int k;
    short *bf=malloc((ohdr->byid/ohdr->bypc)*sizeof(short));
    float lfq=LEFT_FREQ/ohdr->sampfq;
    float rfq=RIGHT_FREQ/ohdr->sampfq;
    float TWOPIL=lfq*2*M_PI;
    float TWOPIR=rfq*2*M_PI;
    int sample_count_per_time=secs*ohdr->sampfq;
    for(i=0;i<times;++i) 
    for (k = 0 ; k < sample_count ; k++) {
//         bf[2 * k] = AMPL * 2 * ( TWOPIL*k - floor(.5+TWOPIL*k));
//         bf[2 * k+1] = AMPL * 2 * ( TWOPIR*k - floor(.5+TWOPIR*k));
//        bf[2 * k] = AMPL * sin (TWOPIL*k);
//        bf[2 * k + 1] = AMPL * sin (TWOPIR*k);
        bf[i*sample_count_per_time+2 * k] = .5*AMPL - (AMPL/M_PI) * (sin (TWOPIL*k) + .5*sin (2*TWOPIL*k) + .3333*sin (3*TWOPIL*k));
        bf[i*sample_count_per_time+2 * k] = .5*AMPL - (AMPL/M_PI) * (sin (TWOPIR*k) + .5*sin (2*TWOPIR*k) + .3333*sin (3*TWOPIR*k));
    }

    fwrite(bf, sizeof(char), ohdr->byid, outwavfp);
    fclose(outwavfp);
    free(bf);
    free(ohdr);
    free(ofn);

    return 0;
}
