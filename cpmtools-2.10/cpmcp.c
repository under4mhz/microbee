#include "config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <utime.h>

#include "getopt_.h"
#include "cpmfs.h"

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* 2016/11/01 uBee - for -e option */
extern int erased_files;

const char cmd[]="cpmcp";
static int text=0;
static int preserve=0;
static char substitute=0;

/* for reporting - uBee 2009/09/28 */
void cpm_set_report (int report, const char *s);
static int report_sides;

/**
 * Return the user number.
 * @param s CP/M filename in 0[0]:aaaaaaaa.bbb format.
 * @returns The user number or -1 for no match.
 */
static int userNumber(const char *s)
{
 if (isdigit(*s) && *(s+1)==':')
    return (*s-'0');
    
 if (isdigit(*s) && isdigit(*(s+1)) && *(s+2)==':')
    return (10*(*s-'0')+(*(s+1)));

 return -1;
}

/**
 * Copy one file from CP/M to UNIX.
 * @param root The inode for the root directory.
 * @param src  The CP/M filename in 00aaaaaaaabbb format.
 * @param dest The UNIX filename.
 * @returns 0 for success, 1 for error.
 */
static int cpmToUnix(const struct cpmInode *root, const char *src, const char *dest)
{
  struct cpmInode ino;
  int exitcode=0;
  int res;          /* uBee 2009/09/28 */
  char buf[4096];   /* uBee 2009/09/28 */

  if (cpmNamei(root,src,&ino)==-1) { fprintf(stderr,"%s: can not open `%s': %s\n",cmd,src,boo); exitcode=1; }
  else
  {
    struct cpmFile file;
    FILE *ufp;

    cpmOpen(&ino,&file,O_RDONLY);

/* skip the actual file copy to unix if we are after just a CP/M file report. - uBee 2009/09/28 */
  if (report_sides)
     {
      printf("\nSde Cyl Sec Usr Filename\n");
      printf(  "--- --- --- --- --------\n");
      cpm_set_report(report_sides, src);
      while ((res=cpmRead(&file,buf,sizeof(buf)))!=0)
         ;
     }
  else
     {
/* uBee 2009/09/28 end */





    if ((ufp=fopen(dest,text ? "w" : "wb"))==(FILE*)0) { fprintf(stderr,"%s: can not create %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; }
    else
    {
      int crpending=0;
      int ohno=0;
/* uBee 2009/09/28 int res; */
/* uBee 2009/09/28 char buf[4096]; */
      while ((res=cpmRead(&file,buf,sizeof(buf)))!=0)
      {
        int j;

        for (j=0; j<res; ++j)
        {
          if (text)
          {
            if (buf[j]=='\032') goto endwhile;
            if (crpending)
            {
              if (buf[j]=='\n') 
              {
                if (putc('\n',ufp)==EOF) { fprintf(stderr,"%s: can not write %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
                crpending=0;
              }
              else if (putc('\r',ufp)==EOF) { fprintf(stderr,"%s: can not write %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
              crpending=(buf[j]=='\r');
            }
            else
            {
              if (buf[j]=='\r') crpending=1;
              else if (putc(buf[j],ufp)==EOF) { fprintf(stderr,"%s: can not write %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
            }
          }
          else if (putc(buf[j],ufp)==EOF) { fprintf(stderr,"%s: can not write %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
        }
      }
      endwhile:
      if (fclose(ufp)==EOF && !ohno) { fprintf(stderr,"%s: can not close %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; ohno=1; }
      if (preserve && !ohno && (ino.atime || ino.mtime))
      {
        struct utimbuf ut;

        if (ino.atime) ut.actime=ino.atime; else time(&ut.actime);
        if (ino.mtime) ut.modtime=ino.mtime; else time(&ut.modtime);
        if (utime(dest,&ut)==-1) { fprintf(stderr,"%s: can change timestamps of %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; ohno=1; }
      }
    }
     }  /* uBee 2009/09/28 */    
    cpmClose(&file);
  }
 
  return exitcode;
}

static void usage(void)
{
  fprintf(stderr,
  "Usage: %s [-cersv] [-f format] [-p] [-t] image user:file file\n"
  "       %s [-cersv] [-f format] [-p] [-t] image user:file ... directory\n"
  "       %s [-cersv] [-f format] [-p] [-t] image file user:file\n"
  "       %s [-cersv] [-f format] [-p] [-t] image file ... user:\n",cmd,cmd,cmd,cmd);

  /* Add some more information about existing and new options - uBee 2009/09/28 2016/11/01 */
  fprintf(stderr,
  "\nOther options:\n"
  " -e    Work on Erased files only (as user 0).\n"
  " -r    No copying, instead outputs a CP/M file location report.\n"
  "       Requires a 'head' entry in diskdefs if heads > 1.\n"
  "       A dummy destination location is also required.\n"
  " -s c  Substitute slash characters in CP/M file names with\n" /* 2011/09/06 uBee */
  "       character 'c' when copying to host.\n"
  " -v    Report build version.\n");
#if HAVE_LIBDSK_H
  fprintf(stderr,
  " -T    libdsk type.\n"
  " -L x  LibDsk options (x) separated by spaces in double quotes\n"
  "             hd : data rate for 1.4Mb 3.5\" in 3.5\" drive.\n"
  "             dd : data rate for 360k 5.25\" in 1.2Mb drive.\n"
  "             sd : data rate for 720k 3.5\" in 3.5\" drive.\n"
  "             ed : data rate for 2.8Mb 3.5\" in 3.5\" drive.\n"
  "          dstep : double step (40T disk in 80T drive)\n");
#endif
  exit(1);
}

int main(int argc, char *argv[])
{
  const char *err;
  const char *image;
  const char *format=FORMAT;
  const char *devopts=NULL;
  int c,readcpm=-1,todir=-1;
  struct cpmInode root;
  struct cpmSuperBlock super;
  int exitcode=0;
  int gargc;
  char **gargv;
  char patched_cpm_file_name[20];

  /* parse options */
#if HAVE_LIBDSK_H
  /* LibDsk and CP/M file location report options (L and r) addition - uBee 2009/09/28 */
  /* New -e option to only work on erased_files - uBee 2016/11/01 */  
  while ((c=getopt(argc,argv,"eT:L:f:h?ps:trv"))!=EOF) switch(c)
#else  
  while ((c=getopt(argc,argv,"eT:f:h?ps:tv"))!=EOF) switch(c)
#endif  
  {
    case 'T': devopts=optarg; break;
    case 'e': erased_files = 1; /* uBee 2016/11/01 */
              readcpm = 1;
              break;
    case 'f': format=optarg; break;
    case 'h':
    case '?': usage(); break;
    case 'p': preserve=1; break;
    case 't': text=1; break;
    case 'r': report_sides = 1; break;        /* uBee 2009/09/28 */
    case 's': substitute=*optarg; break;
    case 'v': fprintf(stderr, APPVER"\n");    /* uBee 2010/03/31 */
              exit(1);
#if HAVE_LIBDSK_H
    case 'L': Device_libdsk_options(optarg);  /* uBee 2009/09/28 */
              break;
#endif
  }
  /* parse arguments */
  if ((optind+2)>=argc)
     usage();

  image=argv[optind++];

  if (userNumber(argv[optind])>=0) /* cpm -> unix? */
  {
    int i;
    struct stat statbuf;

    for (i=optind; i<(argc-1); ++i)
       if (userNumber(argv[i])==-1)
           usage();

    /* if we are making a CP/M file report then skip unix dir checking - uBee 2009/09/28 */
    if (report_sides)  /* uBee 2009/09/28 */
       todir = 1;  /* files won't really get copied! - uBee 2009/09/28 */
    else /* uBee 2009/09/28 */
       { /* uBee 2009/09/28 */
        todir=((argc-optind)>2);
        if (stat(argv[argc-1],&statbuf)==-1) { if (todir) usage(); }
        else if (S_ISDIR(statbuf.st_mode)) todir=1; else if (todir) usage();
       } /* uBee 2009/09/28 */
    readcpm=1;
  }

  else if (userNumber(argv[argc-1])>=0) /* unix -> cpm */
  {
    int i;

    todir=0;
    for (i=optind; i<(argc-1); ++i)
       if (userNumber(argv[i])>=0)
          usage();

    if ((argc-optind)>2 && *(strchr(argv[argc-1],':')+1)!='\0')
       usage();

    if (*(strchr(argv[argc-1],':')+1)=='\0')
       todir=1;

    readcpm=0;
  }
  else usage();

  /* open image file */
  if ((err=Device_open(&super.dev,image,readcpm ? O_RDONLY : O_RDWR, devopts)))
  {
    fprintf(stderr,"%s: can not open %s (%s)\n",cmd,image,err);
    exit(1);
  }
  cpmReadSuper(&super,&root,format);

  if (readcpm) /* copy from CP/M to UNIX */
  {
    int i;
    char *last=argv[argc-1];
    
    cpmglob(optind,argc-1,argv,&root,&gargc,&gargv);
    /* trying to copy multiple files to a file? */
    if (gargc>1 && !todir) usage();
    for (i=0; i<gargc; ++i)
    {
      char dest[_POSIX_PATH_MAX];

      if (todir)
      {
        strcpy(dest,last);

/* 2011/09/06, 2015/01/10 uBee - only use the slash character for unices */
#ifdef _WIN32
        strcat(dest,"\\");
#else
        if (*dest && dest[strlen(dest) - 1] != '/')
           strcat(dest,"/");
#endif        
        /* uBee 2011/09/06 substitute characters in host file name */
        strcpy(patched_cpm_file_name, gargv[i]+2);

        if (substitute)
           {
            char *p;
            p = patched_cpm_file_name;
            while (*p)
               {
                if (*p == '\\' || *p == '/')
                   *p = substitute;
                p++;
               }
           }

        strcat(dest, patched_cpm_file_name);
/*        strcat(dest,gargv[i]+2);  */
      }
      else
         strcpy(dest,last);
         
      /* uBee 2016/11/02 - If copying erased files append '_ERASED-000n' where n is incremented each time */
      if (erased_files)
         {
          int l;
          l = strlen(dest);
          sprintf(&dest[l], "_ERASED-%04d", erased_files);
          erased_files++;
#if 0
          printf("Copying: %s\n" ,dest);
#endif
         }

      if (cpmToUnix(&root,gargv[i],dest))
         exitcode=1;
    }
  }

  else /* copy from UNIX to CP/M */
  {
    int i;

    for (i=optind; i<(argc-1); ++i)
    {
      /* variables */
      char *dest=(char*)0;
      FILE *ufp;


/* Need to open in binary mode under win32 or the file gets truncated when EOF is found. - uBee 2010/02/21 */
/*    if ((ufp=fopen(argv[i],"r"))==(FILE*)0) */ /* cry a little */
      if ((ufp=fopen(argv[i],"rb"))==(FILE*)0) /* win32 needs binary mode - uBee 2010/02/21 */
      {
        fprintf(stderr,"%s: can not open %s: %s\n",cmd,argv[i],strerror(errno));
        exitcode=1;
      }

      else
      {
        struct cpmInode ino;

/* 2010/08/26 uBee - Must need 15 surely? but make it 20 to be safe */
/*        char cpmname[2+8+3+1]; */ /* 00foobarxy.zzy\0 */
        char cpmname[20]; /* 00foobarxy.zzy\0 */        

        if (todir)
        {
/* 2010/08/26, 2015/01/10 uBee - Use the correct slash character for win32 '\' (escaped) */
#ifdef _WIN32
         if ((dest = strrchr(argv[i], '\\')))
            ++dest;
         else
            dest = argv[i];
#else
         if ((dest = strrchr(argv[i], '/')))
            ++dest;
         else
            dest = argv[i];
#endif
          sprintf(cpmname,"%02d%s",userNumber(argv[argc-1]),dest);
        }
        else
        {
          sprintf(cpmname,"%02d%s",userNumber(argv[argc-1]),strchr(argv[argc-1],':')+1);
        }

        if (cpmCreat(&root,cpmname,&ino,0666)==-1) /* just cry */
        {
          fprintf(stderr,"%s: can not create %s: %s\n",cmd,cpmname,boo);
          exitcode=1;
        }

        else
        {
          struct cpmFile file;
          int ohno=0;
          char buf[4096+1];

          cpmOpen(&ino,&file,O_WRONLY);
          do
          {
            int j;

            for (j=0; j<(sizeof(buf)/2) && (c=getc(ufp))!=EOF; ++j)
            {
              if (text && c=='\n') buf[j++]='\r';
              buf[j]=c;
            }

            if (text && c==EOF) buf[j++]='\032';
            if (cpmWrite(&file,buf,j)!=j)
            {
              fprintf(stderr,"%s: can not write %s: %s\n",cmd,dest,boo);
              ohno=1;
              exitcode=1;
              break;
            }
          } while (c!=EOF);
          if (cpmClose(&file)==EOF && !ohno) /* I just can't hold back the tears */
          {
            fprintf(stderr,"%s: can not close %s: %s\n",cmd,dest,boo);
            exitcode=1;
          }

        }
        fclose(ufp);
      }
    }
  }

  cpmUmount(&super);

 /* 2015/04/26 uBee - Device_close() call missing causing IMD images to not be flushed */
 if (&super.dev)
    Device_close(&super.dev);
 
 exit(exitcode);
}
