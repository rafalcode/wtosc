/* we'll be using shorts
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <math.h>
#include <cairo/cairo.h>

#define ONSZ 32 // output filename size.
#define GBUF 128
#define nr 2
#define nc 2
#define nb 3

#define CYCSZ 256 
#define NXPTS CYCSZ
#define LWD 0.5

typedef struct /* wavheader type: wh_t */
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
    int byid; // BYtes_In_Data (ection), sorry to those who interpret "by ID" no sense in this context */
} wh_t; /* wav header type */

char *mkon(char *inpfn)  /* make output filename ... some convenient */
{
    char *on=calloc(ONSZ, sizeof(char));

    struct timeval tsecs;
    gettimeofday(&tsecs, NULL);
    char lsns[7]={0}; // micseconds
    sprintf(lsns, "%lu", tsecs.tv_usec);
    sprintf(on, "t%.*s", 3, lsns); 

    char *per=strrchr(inpfn, '.');
    sprintf(on+4, "_%.*s.wav", 3, per-3); // 5 chars

    /* let's avoid overwriting same named file */
    struct stat fsta;
    while(stat(on, &fsta) != -1) {
        sprintf(lsns, "%lu", tsecs.tv_usec+1UL);
        sprintf(on+1, "%.*s", 3, lsns); 
        sprintf(on+4, "_%.*s.wav", 3, per-3); // 5 chars
    }

    return on;
}

int main(int argc, char *argv[])
{
    if(argc != 3) {
        printf("Usage: this program takes a wav bank file and prints out one of its wavtables. Args:\n");
        printf("       1) wavbank file 2) which (1st second etc) of the wavtables to print out\n");
        exit(EXIT_FAILURE);
    }

    /* The first check is to see if both these files are compatible. I.e. must have same sample frequencies and the like.
     * I was goign to check to see which was the smallest, but actually right now, we're going to pull them both into memory */
    int i;
    FILE *inrawfp;
    int whichwt=atoi(argv[2]);

    // cairo stuff
    int width=1024, height=480;
    cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create (surface);
    /*  we set the background */
    cairo_rectangle (cr, 0, 0, width, height); /* arg explan: topleftcorner and size of shape  */
    cairo_set_source_rgb(cr, 0, 0, 0); 
    cairo_fill (cr);
    /* vertical divider how ar ewe going to section off the screen vertically */
    double vbar=(double)width/NXPTS; // basis for x
    double *xpts=calloc(NXPTS, sizeof(double)); /* ori: origin, pt, last bar, */
    double *ypts=calloc(NXPTS, sizeof(double)); /* ori: origin, pt, last bar, */
    double midv=height/2;
    double span=midv-5;
    xpts[0]=vbar/2;
    for(i=1;i<NXPTS;i++)
        xpts[i] = xpts[i-1] +vbar;
    printf("vbar:%4.4f;xp0=%4.4f;xp1=%4.4f;xp2=%4.4f\n", vbar, xpts[0], xpts[1], xpts[2]); 

    /* Before opening input wavfile, let's use stat on the wav file */
    struct stat fsta;
    if(stat(argv[1], &fsta) == -1) {
        fprintf(stderr,"Can't stat input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    // printf("size of raw file is: %zu\n", fsta.st_size);
    char *buf=malloc(fsta.st_size);
    inrawfp = fopen(argv[1], "rb");
    if ( inrawfp == NULL ) {
        fprintf(stderr,"Can't open input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    if(fread(buf, fsta.st_size, sizeof(char), inrawfp) < 1) {
        printf("Sorry, trouble putting input file into buffer.\n"); 
        exit(EXIT_FAILURE);
    }
    fclose(inrawfp);

    unsigned whsz = sizeof(wh_t);

    int xtnt=(fsta.st_size - sizeof(wh_t))/2; // numbytes to numshorts,
    unsigned nchunks=xtnt/CYCSZ;
    unsigned rem=xtnt%CYCSZ;
    printf("num full chunks= %u; Remainder=%u\n", nchunks, rem); 

    // build the wav header
    wh_t *hd=calloc(1, sizeof(wh_t));
    memcpy(hd->fstr, "WAVEfmt ", 8*sizeof(char));
    memcpy(hd->id, "RIFF", 4*sizeof(char));
    memcpy(hd->datastr, "data", 4*sizeof(char));
    hd->fmtnum = 16;
    hd->pcmnum = 1;
    hd->bipsamp = 16;
    hd->bypc = hd->bipsamp/8;

    int tobeseen=whichwt;
    int jj=2*256*(tobeseen-1);

    short tmp;
    for(i=0;i<CYCSZ;i++)  {
        tmp=0x00FF&buf[2*i+jj+44];
        tmp|=0xFF00&(buf[2*i+1+jj+44]<<8);
        // if(tmp==0)
        ypts[i]=midv - span*tmp/0x7FFF;
        // printf("%5i, ", tmp); 
    }
    // the pic
    // cairo_set_source_rgba(cr, 0.65, 0.9, 0.45, 1);
    cairo_set_source_rgba(cr, 152/255.0, 251/255.0, 152/255.0, 1); // pale green
    cairo_set_line_width (cr, LWD); /* thinnest really possible */
    float tol=.2;
    for(i=0;i<NXPTS;i++){
        if( (ypts[i]> (midv-tol)) & (ypts[i] < (midv+tol)) ) {
            cairo_arc(cr, xpts[i], midv, .8, 0, 2 * M_PI);
            cairo_fill(cr);
        } else {
            cairo_move_to(cr, xpts[i], midv);
            cairo_line_to(cr, xpts[i], ypts[i]);
        }

        // cairo_move_to(cr,ori.h,ori.v);
        // cairo_line_to(cr,pt[i].h, pt[i].v);
        cairo_stroke(cr);
    }

    /* Write output and clean up */
    cairo_surface_write_to_png (surface, "wtbank0.png");
    cairo_destroy (cr);
    cairo_surface_destroy (surface);

    free(buf);
    free(hd);
    free(xpts);
    free(ypts);
    return 0;
}
