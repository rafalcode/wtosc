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
/* hardcoded number of samples */
#define NCSAMPS 800 /* Number of Channel Samps: will need to be mutliplied by 2 if nhans is 2 */
#define SAMPF 220 /* in practice, we'll be clueless about the frequency, we would have to chose a nice one using ear */

typedef struct /* time point, tpt */
{
    int m, s, h;
} tpt;

struct svals_t /* a struct with 2 floats: assocfl, and sonicv */
{
    float assocfl; /* float associated with this */
    union {
        float sonicv; /* sonic value */
        float sonica[2];
    } sv;
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

char *xfw(char *inwf, char *tp, wh_t *inhdr) /* xfw: extract from wav */
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
    inhdr->byid = inhdr->bypc*inhdr->nchans*NSAMPS; /* number of bytes to extract */
    inhdr->glen = inhdr->byid+36;
    char *bf=malloc(inhdr->byid);
    fseek(inwavfp, point, SEEK_CUR);
    if ( fread(bf, inhdr->byid, sizeof(char), inwavfp) < 1 ) { /* Yup! we slurp in the baby! */
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

short *prtimesring2a(srp_t *sra, unsigned nnotes, unsigned csndlen, short nchans)
{
    unsigned i, ii, j;
    short *sbuf=malloc(nchans*csndlen*nnotes*sizeof(short));
    sr_t *st;

    if(nchans ==1) {

        i=0;
        for(j=0;j<nnotes;++j) {
            ii=0;
            st=sra[j].sr;
            do {
                sbuf[i++]=st->s;
                st=st->nx;
                ii++;
            } while (ii < csndlen);
        }

    } else if(nchans ==2) {

        /* chan 1*/
        i=0;
        for(j=0;j<nnotes;j++) {
            ii=0;
            st=sra[j].sr;
            do {
                sbuf[i]=st->s;
                i+=2;
                st=st->nx;
                st=st->nx;
                ii+=2;
            } while (ii < nchans*csndlen);
        }
        /* chan 2*/
        i=1;
        for(j=0;j<nnotes;j++) {
            ii=1;
            st=sra[j].sr;
            st=st->nx;
            do {
                sbuf[i]=st->s;
                i+=2;
                st=st->nx;
                st=st->nx;
                ii+=2;
            } while (ii < nchans*csndlen);
        }
    }

    return sbuf;
}

int main(int argc, char *argv[])
{
    if(argc != 4) {
        printf("Program to populate a wavetable from a certain point in a WAV, and generate different frequencies on it.\n");
        printf("3 Arguments: 1) filename, input wav 2) mm:ss.hh time at which to start sampling 3) output wav.\n");
        exit(EXIT_FAILURE);
    }
    int i, ilim;
    float fqa[NUMNOTES]={160., 220.25, 330.5, 441., 551.25, 800., 1200., 2300.};
    unsigned csndlen=11025; /* hard coded for the time being */

    /* get our sampels from the wav file */
    wh_t *twhdr=malloc(sizeof(wh_t));
    char *bf= xfw(argv[1], argv[2], twhdr);
    short *vals=malloc((twhdr->byid/2)*sizeof(short));
    ilim=twhdr->byid/2;
    for(i=0;i<ilim;i++) {
        vals[i]=bf[2*i+1]<<8;
        vals[i] |=bf[2*i];
    }
    free(bf);

    /* 1/3. setting up the "model wavetable" */
    wavt_t *wt=malloc(sizeof(wavt_t));
    wt->nsamps=(unsigned)NSAMPS;
    wt->d=malloc(wt->nsamps*sizeof(struct svals_t));
    wt->svalranges=malloc((wt->nsamps-1)*sizeof(struct svals_t));
    wt->stpsz=2.*M_PI/((float)wt->nsamps); /* distance in radians btwn each sampel int he wavelength */
    /* OK create the wavetable */
    if(twhdr->nchans ==1) {
        wt->d[0].assocfl = 0.;
        wt->d[0].sv.sonicv=vals[0];
        for(i=1;i<wt->nsamps;++i) {
            wt->d[i].assocfl =wt->stpsz*i;
            wt->d[i].sv.sonicv=vals[i];
            wt->svalranges[i-1]= wt->d[i].sv.sonicv - wt->d[i-1].sv.sonicv; /* the difference with the previous value, used for interpolating */
        }
    } else if(twhdr->nchans ==2) {
        wt->d[0].assocfl = 0.;
        wt->d[0].sv.sonicv=vals[0];
        for(i=2;i<wt->nsamps;i+=2) {
            wt->d[i].assocfl =wt->stpsz*i/2;
            wt->d[i].sv.sonicv=vals[i];
            wt->svalranges[i-2]= wt->d[i].sv.sonicv - wt->d[i-2].sv.sonicv; /* the difference with the previous value, used for interpolating */
        }
        wt->d[1].assocfl = 0.;
        wt->d[1].sv.sonicv=vals[1];
        for(i=3;i<wt->nsamps;i+=2) {
            wt->d[i].assocfl =wt->stpsz*i/2;
            wt->d[i].sv.sonicv=vals[i];
            wt->svalranges[i-2]= wt->d[i].sv.sonicv - wt->d[i-2].sv.sonicv; /* the difference with the previous value, used for interpolating */
        }
    }

    /* so our wavetable has been created, as you'll note it only has accurately representative values for one frequency,
     * but we're still going to use for other frequencies */

    float fsamps;
    sr_t *tsr;
    float stpsz;

    int j, k=0, m;
    float kincs, xprop;

    srp_t *sra=malloc(NUMNOTES*sizeof(srp_t)); /* array of rings */
    unsigned scsamps; /* Single Channel Samps */
    for(m=0;m<NUMNOTES;++m) { /* loop for our frequency range */
        fsamps=SRATE/fqa[m];
        scsamps=(unsigned)(.5+fsamps);
        sra[m].sz=twhdr->nchans*scsamps;
        sra[m].sr=creasrn0(sra[m].sz);
        tsr=sra[m].sr;
        stpsz=2.*M_PI/((float)scsamps);

        if(twhdr->nchans ==2) {

            /* first chan */
            for(i=0;i<sra[m].sz;i+=2) {
                kincs=stpsz*(i/2.);
                for(j=k; j<wt->nsamps-2; j+=2)
                    if(kincs>=wt->d[j].assocfl) /* d[0].assocfl always zero, so the next "continue" will alwsays increment j to 1, even if k=0 */
                        continue;
                    else
                        break;
                k=j-2; /* edge condition of k watched, the above continue will ensure it's never zero */
                xprop=(kincs-wt->d[k].assocfl)/wt->stpsz;
                tsr->s=(short)(.5+wt->d[k].sv.sonicv + xprop*wt->svalranges[k]);
                tsr=tsr->nx;
            }
            /* second chan */
            for(i=1;i<sra[m].sz;i+=2) {
                kincs=stpsz*i/2;
                for(j=k; j<wt->nsamps-2; j+=2)
                    if(kincs>=wt->d[j].assocfl) /* d[0].assocfl always zero, so the next "continue" will alwsays increment j to 1, even if k=0 */
                        continue;
                    else
                        break;
                k=j-2; /* edge condition of k watched, the above continue will ensure it's never zero */
                xprop=(kincs-wt->d[k].assocfl)/wt->stpsz;
                tsr->s=(short)(.5+wt->d[k].sv.sonicv + xprop*wt->svalranges[k]);
                tsr=tsr->nx;
            }

        } else if(twhdr->nchans ==1) {
            for(i=0;i<sra[m].sz;i++) {
                kincs=stpsz*i;
                for(j=k; j<wt->nsamps-1; j++)
                    if(kincs>=wt->d[j].assocfl) /* d[0].assocfl always zero, so the next "continue" will alwsays increment j to 1, even if k=0 */
                        continue;
                    else
                        break;
                k=j-1;
                xprop=(kincs-wt->d[k].assocfl)/wt->stpsz;
                tsr->s=(short)(.5+wt->d[k].sv.sonicv + xprop*wt->svalranges[k]);
                tsr=tsr->nx;
            }
        }
    }

    short *sbuf; /* our sound info buffer */
    sbuf=prtimesring2a(sra, NUMNOTES, csndlen, twhdr->nchans);
    wh_t *hdr=hdr4chunk((int)SRATE, twhdr->nchans, NUMNOTES*csndlen);
    FILE *fout=fopen(argv[3], "wb");
    fwrite(hdr, sizeof(char), 44, fout);
    fwrite(sbuf, sizeof(short), twhdr->nchans*NUMNOTES*csndlen, fout); /* shorts will get written as small endian bytes in file in x86 system */
    fclose(fout);

    free(hdr);
    free(twhdr);
    free(vals);
    free(sbuf);
    free(wt->svalranges);
    free(wt->d);
    free(wt);
    for(i=0;i<NUMNOTES;++i) 
        freering(sra[i].sr);
    free(sra);

    return 0;
}
