/* Copyright (C) 1999-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
 * mod rf.
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>

#include	<sndfile.h>
#define NDFQS 3 /* number of different frequencies */

int main (int argc, char *argv[])
{
    /* argument accounting */
    if(argc!=2) {
        printf("Error. Pls supply argument (name of the control file).\n");
        exit(EXIT_FAILURE);
    }
    /* parse edit file in very simple terms, that means using scanf */
    FILE *fpa=fopen(argv[1], "r");
    int i, k, vigs;
    float nf[NDFQS];
    SF_INFO	sfinfo={0};
    fscanf(fpa, "sampling freq: %i\nnvigsecs: %i\n", &sfinfo.samplerate, &vigs);

    fscanf(fpa, "note freqs: %f", nf);
    /*
    fscanf(fpa, "%f", &nf[1]);
    fscanf(fpa, "%f", &nf[2]);
    */

    for(i=1;i<NDFQS;++i) 
        fscanf(fpa, "%f ", nf+i);

    fclose(fpa); /* NO ERROR CHECKING */

    printf("%i, %i, %f, %f, %f\n", sfinfo.samplerate, vigs, nf[0], nf[1], nf[2]);

    for(i=0;i<NDFQS;++i) 
        nf[i] /= sfinfo.samplerate;

    printf("%i, %i, %f, %f, %f\n", sfinfo.samplerate, vigs, nf[0], nf[1], nf[2]);

    int sactpfq =  sfinfo.samplerate * vigs/20; /* samplecount per frequency/note */
    printf("smpct=%i\n", sactpfq); 
    sfinfo.channels	= 1;
    sfinfo.frames = NDFQS * sactpfq; /* do not multiply by sfinfo.channels: frame are multichannnel frames */
    sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);
    printf("%zu\n", sfinfo.frames);

    short *buffer;
    if (! (buffer = malloc(NDFQS*sactpfq*sfinfo.channels * sizeof(short)))) {
        printf ("Malloc failed.\n") ;
        exit(EXIT_FAILURE);
    }

    SNDFILE	*file ;
    if (! (file = sf_open ("sine0.wav", SFM_WRITE, &sfinfo))) {	/* incredibly, this call is changing the frame member! */
        printf ("Error : Not able to open output file.\n") ;
        exit(EXIT_FAILURE);
    }

    for(i=0;i<NDFQS;++i)
        for (k = 0 ; k < sactpfq ; k++)
            buffer[sactpfq*i+k] = 0x1FFF * sin (nf[i]* k * 2 * M_PI); /* multiple by max amplitude?: a bit loud, no? 0x6FFF shouldn't burst eardrums */

#ifdef DEBUG
    for(i=0;i<NDFQS;++i) 
        for (k = 0 ; k < sactpfq ; k++)
            printf("%hx", buffer[NDFQS*i+k]); 
    printf("\n"); 
#endif

    if (sf_write_short(file, buffer, NDFQS*sactpfq*sfinfo.channels) != NDFQS*sactpfq*sfinfo.channels)
        puts (sf_strerror (file));

    sf_close(file);
    free(buffer);
    return 0;
}
