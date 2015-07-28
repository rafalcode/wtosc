/* create a temporary directory which does not exist already.
 * The while() clause takes care of this. Tested */
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h> 

#define GBUF 64

char *mktmpd(void)
{
    struct timeval tsecs;
    gettimeofday(&tsecs, NULL);
    char *myt=calloc(14, sizeof(char));
    strncpy(myt, "tmpdir_", 7);
    sprintf(myt+7, "%lu", tsecs.tv_usec);

    DIR *d;
    while((d = opendir(myt)) != NULL) { /* see if a directory witht the same name exists ... very low likelihood though */
        gettimeofday(&tsecs, NULL);
        sprintf(myt+7, "%lu", tsecs.tv_usec);
        closedir(d);
    }
    closedir(d);
    mkdir(myt, S_IRWXU);

    return myt;
}

int main(void)
{
    char *tmpd=mktmpd();
    printf("A unique temp directory called \"%s\" has been created.\n", tmpd);

    int i;
    FILE *fp;
    char *fn=calloc(GBUF, sizeof(char));
    for(i=0;i<5;++i) {
        sprintf(fn, "%s/%i.wav", tmpd, i);
        fp=fopen(fn, "w");
        fclose(fp);
    }

    free(fn);
    free(tmpd);
    return EXIT_SUCCESS;
}
