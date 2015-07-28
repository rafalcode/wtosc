/* Copyright (C) 1999-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
 * mod rf.
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>

#include	<sndfile.h>

#define		SAMPLE_RATE			44100
#define     SAMPD   2 /* sample duration */
#define     SAMCH   2 /* sample duration */
#define		SAMPLE_COUNT		(SAMPLE_RATE * SAMPD * SAMCH)

int main (int argc, char *argv[])
{
    /* argument accounting */
    if(argc!=2) {
        printf("Error. Pls supply argument (name of file with ints in\n");
        exit(EXIT_FAILURE);
    }

    FILE *fp=fopen(argv[1], "r");
    short *si=calloc(SAMPLE_COUNT, sizeof(short));
    int ct=0, j;
    while(ct<SAMPLE_COUNT) {
        j=fscanf(fp, "%hi", si+ct);
        ct +=j;
    }
    fclose(fp);
    printf("%hi\n", si[45]);

    SNDFILE	*file ;
    SF_INFO	sfinfo ;

    memset (&sfinfo, 0, sizeof (sfinfo)) ;
    /* take these are hard-coded for the time being */
    sfinfo.samplerate	= SAMPLE_RATE;
    sfinfo.frames		= SAMPLE_RATE*SAMPD;
    sfinfo.channels		= SAMCH;
    sfinfo.format		= (SF_FORMAT_WAV | SF_FORMAT_PCM_16);

    if (! (file = sf_open ("cut.wav", SFM_WRITE, &sfinfo))) {	
        printf ("Error : Not able to open output file.\n") ;
        exit(EXIT_FAILURE);
    }
    if (sf_write_short(file, si, SAMPLE_COUNT) != SAMPLE_COUNT)
        puts (sf_strerror (file));

    free(si);
    sf_close (file) ;
    return	 0 ;
}
