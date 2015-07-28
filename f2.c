/* doin the sawtooth. I seem to have problems! */

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<math.h>

#define MAXSHORT 0x7FFF
#define AMPL 1.0*MAXSHORT
#define	AFREQ 441.0
#define SRATE 44100.0

int main(int argc, char *argv[])
{
    if(argc != 2) {
        printf("Usage: simple sine synthesis. Stereo shorts presumed: Arg is prefix for outputfilename.\n");
        exit(EXIT_FAILURE);
    }
    unsigned a1sz=strlen(argv[1]);
    char *ofn=calloc(a1sz+7, sizeof(char));
    strcpy(ofn, argv[1]);
    strcat(ofn, ".wvals");
    FILE *outfp= fopen(ofn, "w");
    float fqa[4]={220.25, 330.5, 441., 551.25};

    float nsamps=SRATE/fqa[2];
    int  insamps=(int)(.5+nsamps);

    short *bf=malloc(insamps*sizeof(short));
    float incs=2*M_PI/nsamps;
    float tf;
    int k;

    for (k = 0 ; k < nsamps ; k++) {
        tf=sin(incs*k);
        bf[k] = AMPL*tf;
    }
    float s0rate=nsamps*fqa[0];
    float n0samps=s0rate/fqa[2];
    printf("n0: %f\n", n0samps); 
    incs=2*M_PI/n0samps;
    for (k = 0 ; k < insamps*2 ; k++)
        fprintf(outfp, "%d\n", bf[k%insamps]);

    fclose(outfp);
    free(bf);
    free(ofn);

    return 0;
}
