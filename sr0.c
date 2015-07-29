/* This is for use as an oscillator, short values are put in a ring of certain size and
 * then this is fed a total number of values requires and it loops around the ring
 * until it gets them.
 *
 * sr0.c is the proof of concept  */

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>

struct s_r_n /* short int ring node */
{
    short s;
    struct s_r_n *nx;
};
typedef struct s_r_n srn_t;

srn_t *creasrn(unsigned ssz) /* create ring of size ssz, random values will be assigned to members of the ring */
{
    int i;
    srn_t *mou /* mouth with a tendency to eat the tail*/, *ttmp /* type temporary */;

    mou=malloc(sizeof(srn_t));
    mou->s=rand();
    mou->nx=NULL;
    ttmp=mou;
    for(i=1;i<ssz;++i) {
        ttmp->nx=malloc(sizeof(srn_t));
        ttmp->nx->s=rand();
        ttmp=ttmp->nx; /* with ->nmove on */
    }
    ttmp->nx=mou;
    return mou;
}

void prtring(srn_t *mou)
{
    srn_t *st=mou;
    do {
        printf("%d ", st->s);
        st=st->nx;
    } while (st !=mou);
    putchar('\n');
    return;
}

void prtimesring(srn_t *mou, unsigned ntimes)
{
    unsigned i=0;
    srn_t *st=mou;
    do {
        printf("%d ", st->s);
        st=st->nx;
        i++;
    } while (i !=ntimes);
    putchar('\n');
    return;
}

void freering(srn_t *mou)
{
    srn_t *st=mou->nx, *nxt;
    while (st !=mou) {
        nxt=st->nx; /* we store the nx of this nx */
        free(st); /* now we can delete it */
        st=nxt;
    }
    free(mou);
}

int main(int argc, char *argv[])
{
    if(argc != 3) {
        printf("Usage: program to test a ring of integers\n");
        printf("Arguments: 1) size of the ring 2) number of values required from ring (values will cycle through ring).\n");
        exit(EXIT_FAILURE);
    }
    srn_t *m=creasrn(atoi(argv[1]));
    prtring(m);
    prtimesring(m, atoi(argv[2]));
    freering(m);

    return 0;
}
