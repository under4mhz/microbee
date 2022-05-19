#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "getopt_.h"
#include "cpmfs.h"

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

const char cmd[]="cpmchmod";

int main(int argc, char *argv[])
{
  const char *err;
  const char *image;
  const char *format=FORMAT;
  const char *devopts=NULL;
  int c,i,usage=0,exitcode=0;
  struct cpmSuperBlock drive;
  struct cpmInode root;
  int gargc;
  char **gargv;
  unsigned int mode; 

  /* parse options */
#if HAVE_LIBDSK_H
  /* LibDsk option addition - uBee 2009/09/28 */
  while ((c=getopt(argc,argv,"T:L:f:h?v"))!=EOF) switch(c)
#else  
  while ((c=getopt(argc,argv,"T:f:h?v"))!=EOF) switch(c)
#endif  
  {
    case 'T': devopts=optarg; break;
    case 'f': format=optarg; break;
    case 'h':
    case '?': usage=1; break;
    case 'v': fprintf(stderr, APPVER"\n");    /* uBee 2010/03/31 */
              exit(1);
#if HAVE_LIBDSK_H
    case 'L': Device_libdsk_options(optarg);  /* uBee 2009/09/28 */
#endif    
  }

  if (optind>=(argc-2)) usage=1;
  else 
  {
    image=argv[optind++];
    if (!sscanf(argv[optind++], "%o", &mode)) usage=1;
  }    

  if (usage)
  {
    fprintf(stderr,"Usage: %s [-f format] image mode pattern ...\n",cmd);
    /* Add some more information about existing and new options - uBee 2009/09/28 */
    fprintf(stderr,"\nOther options:\n");
    fprintf(stderr," -v    Report build version.\n");  /* 2010/03/31 uBee */
#if HAVE_LIBDSK_H
    fprintf(stderr," -T    libdsk type.\n");
    fprintf(stderr," -L x  LibDsk options (x) separated by spaces in double quotes\n");
    fprintf(stderr,"             hd : data rate for 1.4Mb 3.5\" in 3.5\" drive.\n");
    fprintf(stderr,"             dd : data rate for 360k 5.25\" in 1.2Mb drive.\n");
    fprintf(stderr,"             sd : data rate for 720k 3.5\" in 3.5\" drive.\n");
    fprintf(stderr,"             ed : data rate for 2.8Mb 3.5\" in 3.5\" drive.\n");
    fprintf(stderr,"          dstep : double step (40T disk in 80T drive)\n");
#endif
    exit(1);
  }

  /* open image */
  if ((err=Device_open(&drive.dev, image, O_RDWR, devopts)))
  {
    fprintf(stderr,"%s: can not open %s (%s)\n",cmd,image,err);
    exit(1);
  }
  cpmReadSuper(&drive,&root,format);

  cpmglob(optind,argc,argv,&root,&gargc,&gargv);
  for (i=0; i<gargc; ++i)
  {
    struct cpmInode ino;

    if (cpmNamei(&root,gargv[i], &ino)==-1)
    {
      fprintf(stderr,"%s: can not find %s: %s\n",cmd,gargv[i],boo);
      exitcode=1;
    }
    else if (cpmChmod(&ino, mode) == -1)
    {
      fprintf(stderr,"%s: Failed to set attributes for %s: %s\n",cmd,gargv[i],boo);
      exitcode=1;
    }
  }
  cpmUmount(&drive); /* uBee (MH 2.13) 2010/04/03 */
  exit(exitcode);
}
