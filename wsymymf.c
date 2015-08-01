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

#define SRATE 44100.0
#define NUMNOTES 8
/* hardcoded number of samples */
#define NCSAMPS 100 /* Number of Channel Samps: will need to be mutliplied by 2 if nhans is 2 */

typedef struct /* time point, tpt */
{
    int m, s, h;
} tpt;

struct svals_t /* a struct with 2 floats: rd, and sonicv */
{
    float rd; /* float associated with the radial distance */
    union {
        float sonicv; /* sonic value */
        float sonica[2];
    } sv;
};

typedef struct /* wavt_t */
{
    struct svals_t *d;
    float fsamps;
    unsigned nsamps;
    float stpsz;
} wavt_t;

struct s_r_n /* short int ring node struct */
{
    union {
        short s;
        short sa[2];
    } sv;
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

unsigned char *xfw(char *inwf, char *tp, wh_t *inhdr, unsigned ncsamps) /* xfw: extract from wav */
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

    printf("stamin=%u, stasec=%u, stahuns=%u\n", p->m, p->s, p->h);
    int point=inhdr->byps*(p->m*60 + p->s) + p->h*inhdr->byps/100;
    printf("point to is %i inside %i which is %.1f%% in.\n", point, inhdr->byid, (point*100.)/inhdr->byid); 
    if(point >= inhdr->byid) {
        printf("Timepoint at which to sample is over the size of the wav file. Aborting.\n");
        exit(EXIT_FAILURE);
    }

    /* recreate based on samples taken */
    inhdr->byid = inhdr->bypc*inhdr->nchans*ncsamps; /* number of bytes to extract */
    inhdr->glen = inhdr->byid+36;
    unsigned char *bf=malloc(inhdr->byid);
    fseek(inwavfp, point, SEEK_CUR);
    if ( fread(bf, inhdr->byid, sizeof(unsigned char), inwavfp) < 1 ) { /* Yup! we slurp in the baby! */
        printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
        exit(EXIT_FAILURE);
    }
    fclose(inwavfp);
    free(p);

    return bf;
}

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

void prtring(sr_t *mou, short ncha)
{
    sr_t *st=mou;
    do {
        if(ncha==2)
            printf("%d:%d ", st->sv.sa[0], st->sv.sa[1]);
        else if(ncha==1)
            printf("%d ", st->sv.s);
        st=st->nx;
    } while (st !=mou);
    putchar('\n');
    return;
}

void prtimesring(sr_t *mou, unsigned ntimes, short ncha) /* print all values from CYCLING through the ring */
{
    unsigned i=0;
    sr_t *st=mou;
    do {
        if(ncha==2)
            printf("%d:%d ", st->sv.sa[0], st->sv.sa[1]);
        else if(ncha==1)
            printf("%d ", st->sv.s);
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

short *ring2buftimes(srp_t *sra, unsigned nnotes, unsigned ntimes, short nchans) /* take ntimes steps through ring */
{
    unsigned i=0, ii, j;
    unsigned ttimes=ntimes*nchans;
    short *sbuf=malloc(nnotes*ttimes*sizeof(short));
    sr_t *st;

    for(j=0;j<nnotes;j++) {
        ii=0;
        st=sra[j].sr;
        do {
            if(nchans ==2) {
                sbuf[i++]=st->sv.sa[0];
                sbuf[i++]=st->sv.sa[1];
            } else if(nchans ==1)
                sbuf[i++]=st->sv.s;
            ii++;
            st=st->nx;
        } while (ii <ntimes);
    }
    return sbuf;
}

int main(int argc, char *argv[])
{
    if(argc != 5) {
        printf("Program to populate a wavetable from a certain point in a WAV, and generate different frequencies on it.\n");
        printf("4 Arguments: 1) input wavfilename 2) mm:ss.hh time at which to start sampling 3) numsamps2extract 4) output wav.\n");
        exit(EXIT_FAILURE);
    }
    int i, ii;
    float fqa[NUMNOTES]={440., 480., 520., 551.25, 660., 800., 1100., 1500.};
//     float fqa[NUMNOTES]={440.};
    unsigned csndlen=11025; /* hard coded for the time being */

    /* get our sampels from the wav file */
    unsigned ncsamps=atoi(argv[3]);
    wh_t *twhdr=malloc(sizeof(wh_t));
    unsigned char *bf= xfw(argv[1], argv[2], twhdr, ncsamps);
    short *vals=malloc((twhdr->byid/2)*sizeof(short));
    for(i=0;i<twhdr->byid;i+=2) {
        ii=i/2;
        vals[ii]=((short)bf[i+1])<<8;
        vals[ii] |=(short)bf[i];
    }
    free(bf);

    /* 1/3. setting up the "model wavetable" */
    wavt_t *wt=malloc(sizeof(wavt_t));
    wt->nsamps=ncsamps;
    wt->d=malloc(wt->nsamps*sizeof(struct svals_t));
    wt->stpsz=2.*M_PI/((float)wt->nsamps); /* distance in radians btwn each sampel int he wavelength */
#ifdef DBG
    printf("wt->stpsz=%.2f / wt->nsamps=%u\n", wt->stpsz, wt->nsamps);
#endif
    /* OK create the wavetable: the shorts in vals[] must be changed to floats */
    if(twhdr->nchans ==1) {
        wt->d[0].rd = 0.;
        wt->d[0].sv.sonicv=(float)vals[0];
        for(i=1;i<wt->nsamps;++i) {
            wt->d[i].rd =wt->stpsz*i;
            wt->d[i].sv.sonicv=(float)vals[i];
        }
    } else if(twhdr->nchans ==2) {
        wt->d[0].rd = 0.;
        wt->d[0].sv.sonica[0]=(float)vals[0];
        wt->d[0].sv.sonica[1]=(float)vals[1];
        for(i=1;i<wt->nsamps;i++) {
            ii=2*i;
            wt->d[i].rd =wt->stpsz*i;
            wt->d[i].sv.sonica[0]=(float)vals[ii];
            wt->d[i].sv.sonica[1]=(float)vals[ii+1];
        }
    }

#ifdef DBG2
    printf("Values from wav file stored in wavtable (as floats):\n"); 
    if(twhdr->nchans ==1)
        for(i=0;i<wt->nsamps;++i)
            printf("%.1f ", wt->d[i].sv.sonicv);
    else if(twhdr->nchans ==2)
        for(i=0;i<wt->nsamps;++i)
            printf("%.1f:%.1f ", wt->d[i].sv.sonica[0], wt->d[i].sv.sonica[1]);
    printf("\n"); 
#endif

    /* 2/3: so our wavetable has been created, as you'll note it only has accurately representative values for one frequency,
     * but we're still going to use for other frequencies */
    float fsamps;
    sr_t *tsr;
    float stpsz;

    int j, k, m;
    float kincs, xprop;

    srp_t *sra=malloc(NUMNOTES*sizeof(srp_t)); /* array of rings */
    unsigned scsamps; /* Single Channel Samps */
    for(m=0;m<NUMNOTES;++m) { /* loop for our frequency range */
        fsamps=SRATE/fqa[m];
        scsamps=(unsigned)(.5+fsamps);
        sra[m].sz=scsamps;
        sra[m].sr=creasrn0(sra[m].sz);
        tsr=sra[m].sr;
        stpsz=2.*M_PI/((float)scsamps);
        k=0;

        for(i=0;i<sra[m].sz;i++) {
            kincs=stpsz*i;
            for(j=k; j<wt->nsamps-1; j++)
                if(kincs>=wt->d[j].rd)
                    continue;
                else
                    break;
            k=j-1;
            xprop=(kincs-wt->d[k].rd)/wt->stpsz;
            if(twhdr->nchans==2) {
                tsr->sv.sa[0]=(short)(.5+wt->d[k].sv.sonica[0] + xprop*(wt->d[j].sv.sonica[0]-wt->d[k].sv.sonica[0]));
                tsr->sv.sa[1]=(short)(.5+wt->d[k].sv.sonica[1] + xprop*(wt->d[j].sv.sonica[1]-wt->d[k].sv.sonica[1]));
            } else if(twhdr->nchans==1)
                tsr->sv.s=(short)(.5+wt->d[k].sv.sonicv + xprop*(wt->d[j].sv.sonicv-wt->d[k].sv.sonicv));
            tsr=tsr->nx;
        }
#ifdef DBG2
        printf("Ring values: these have been calculated from the wavetable:\n"); 
        prtring(tsr, twhdr->nchans);
#endif
    }

    /* 3/3: our cycle through ring and sound into buffer */
    short *sbuf; /* our sound info buffer */
    sbuf=ring2buftimes(sra, NUMNOTES, csndlen, twhdr->nchans);
    wh_t *hdr=hdr4chunk((int)SRATE, twhdr->nchans, NUMNOTES*csndlen);
    FILE *fout=fopen(argv[4], "wb");
    fwrite(hdr, sizeof(char), 44, fout);
    fwrite(sbuf, sizeof(short), twhdr->nchans*NUMNOTES*csndlen, fout); /* shorts will get written as small endian bytes in file in x86 system */
    fclose(fout);

    free(hdr);
    free(twhdr);
    free(vals);
    free(sbuf);
    free(wt->d);
    free(wt);
    for(i=0;i<NUMNOTES;++i) 
        freering(sra[i].sr);
    free(sra);

    return 0;
}
