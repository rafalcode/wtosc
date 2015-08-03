/* numbers associated with a wav file */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MXINTV 0x7FFFFFFF /* max int value */
#define ARBSZ 128
#define GBUF 8
/* hardcoded number of samples */
#define NCSAMPS 100 /*  A channel sample is a sample from one channel */

#define CONDREALLOC2(x, b, c, a1, t1); \
    if((x)==((b)-1)) { \
        (b) += (c); \
        (a1)=realloc((a1), (b)*sizeof(t1)); \
        memset((a1)+(b)-(c), '\0', (c)*sizeof(t1)); \
    }

typedef unsigned char boole;

typedef struct /* short occurences */
{
    short sa; /* array of shorts */
    unsigned oca; /* occurence counts (of the shorts) array */
} shocs_t;

typedef struct /* holder for shocs_t: occurence arrayr */
{
    unsigned b; /* buffer for our shorts array */
    unsigned n; /* actual size */
    shocs_t *a; /* occurence counts (of the shorts) array */
} shoca_t;

typedef struct /* da_t: data analysis type: just grouping interesting data in one struct */
{
    short mx; /* no point asking for min value in sound */
} da_t; 
typedef struct /* time point, tpt */
{
    int m, s, h;
} tpt;

typedef struct /* wh_t */
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

static int cmpsa(const void *p1, const void *p2)
{
    int ret;
    shocs_t *x1=(shocs_t*)p1;
    shocs_t *x2=(shocs_t*)p2;
    if(x1->sa > x2->sa)
        ret=1;
    else if(x1->sa == x2->sa)
        ret = 0;
    else
        ret=-1;
    return ret;
}

void prttopshoca(shoca_t *sha, unsigned top)
{
    int j;
    printf("top valuesand their occurences (shortval:occurences): %u\n", sha->n);
    for(j=sha->n-1; j>=sha->n-top;--j)
        printf("%hd:%u ", sha->a[j].sa, sha->a[j].oca);
    printf("\n"); 
    return;
}

void prtshoca(shoca_t *sha)
{
    int j;
    printf("number of different sequence lengths (shortval:occurences): %u\n", sha->n);
    for(j=0; j<sha->n;++j)
        printf("%hd:%u ", sha->a[j].sa, sha->a[j].oca);
    printf("\n"); 
    return;
}

shoca_t *uniquelens(short *vals, unsigned byidasshort)
{
    boole new;
    unsigned i, j;
    unsigned asz=0;
    shoca_t *sha=malloc(sizeof(shoca_t));
    sha->b=GBUF;
    sha->a=calloc(sha->b, sizeof(shocs_t));
    for(i=0; i<byidasshort;++i) {
        if(vals[i]<=0)
            continue;
        new=1;
        for(j=0; j<asz;++j) {
            if(sha->a[j].sa == vals[i]) {
                sha->a[j].oca++;
                new=0;
                break;
            }
        }
        if(new) {
            CONDREALLOC2(asz, sha->b, GBUF, sha->a, shocs_t);
            sha->a[asz].sa=vals[i];
            sha->a[asz].oca++;
            asz++;
        }
    }
#ifdef DBG
    printf("number of different sequence lengths: %u\n", asz);
    printf("vals: "); 
    for(j=0; j<asz;++j)
        printf("%5u ", sha->a[j].sa);
    printf("\n"); 
    printf("ocs: "); 
    for(j=0; j<asz;++j)
        printf("%5u ", sha->a[j].oca);
    printf("\n"); 
#endif
    sha->n=asz;
    sha->a=realloc(sha->a, asz*sizeof(shocs_t));
    return sha;
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

void prttheshorts(wh_t *twhdr, short *vals)
{
    int i;
    int byidasshort=twhdr->byid/2;

    if(twhdr->nchans ==2) {
        /* first channel */
        for(i=0;i<byidasshort;i+=2) 
            printf("%hd ", vals[i]);
        printf("\n"); 
        /* second channel */
        for(i=1;i<byidasshort;i+=2) 
            printf("%hd ", vals[i]);
        printf("\n"); 
    } else { /* it's in mono */
        for(i=0;i<byidasshort; i++)
            printf("%hd ", vals[i]);
        printf("\n"); 
    }
    return;
}

short *w2shorts(char *inwf, wh_t *inhdr, da_t *da) /* w2shorts: return an array of shorts which is the data in the wav file. */
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

    unsigned char *bf=malloc(inhdr->byid*sizeof(unsigned char));
    if ( fread(bf, inhdr->byid, sizeof(unsigned char), inwavfp) < 1 ) {
        printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
        exit(EXIT_FAILURE);
    }
    fclose(inwavfp);

    int byidasshort=inhdr->byid/2;
    short *vals=malloc(byidasshort*sizeof(short));

    int i, ii;
    printf("bytes in data from hdr: %d\n", inhdr->byid); 
    for(i=0;i<inhdr->byid;i+=2) {
        ii=i/2;
        vals[ii]=((short)bf[i+1])<<8; /* will also set lower bytes to zero */
        vals[ii] |=(short)bf[i];
        if(vals[ii] >da->mx)
            da->mx=vals[ii];
    }
    free(bf);

    return vals;
}

int main(int argc, char *argv[])
{
    if( argc != 3) {
        printf("Usage: calcs hits of a certain level: Args: 1) Name of wavfile 2) %% of mx to use as level.\n");
        exit(EXIT_FAILURE);
    }
    int i;
    da_t da={0}; /* our data analysis type */

    wh_t *twhdr=malloc(sizeof(wh_t)); /* empty as yet */
    short *vals=w2shorts(argv[1], twhdr, &da); /* in this call, twhdr gets set */
    int byidasshort=twhdr->byid/2;

#ifdef DBG2
    prttheshorts(twhdr, vals);
#endif
    boole uphit=0;
    unsigned hitlu=0, hitld=0; /* hit level up, hit level down */
    short l0=(short)(.5+atof(argv[2])*da.mx);
#ifdef DBG2
    if(twhdr->nchans ==2) {
        for(i=0;i<byidasshort; i+=2)
            if( (uphit==0) & ((vals[i] >= l0) | (vals[i+1] >= l0)) ) {
                uphit=1;
                hitlu++;
                printf("%i ", i); 
            } else if( (uphit==1) & ((vals[i] < l0) | (vals[i+1] < l0)) ) {
                uphit=0;
                hitld++;
            }
    } else if(twhdr->nchans ==1) {
        for(i=0;i<byidasshort; i++)
            if( (uphit==0) & (vals[i] >= l0)) {
                uphit=1;
                hitlu++;
                printf("%i ", i); 
            } else if( (uphit==1) & ((vals[i] < l0)) ) {
                uphit=0;
                hitld++;
            }
    }
    printf("\n"); 
#endif
    shoca_t *sha=uniquelens(vals, byidasshort);
#ifdef DBG
    qsort(sha->a, sha->n, sizeof(shocs_t), cmpsa);
    prttopshoca(sha, 100);
#endif
    printf("unique +ve amplitude values: %u\n", sha->n); 

    printf("Max short = %hd\n", da.mx);
    printf("#hit level upwards %u times. hit level downwards %u times.\n", hitlu, hitld); 

    free(sha->a);
    free(sha);
    free(twhdr);
    free(vals);
    return 0;
}
