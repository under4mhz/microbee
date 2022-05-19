#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt_.h"
#include "cpmfs.h"

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* 2016/11/01 uBee - for -e option */
extern int erased_files;

static const char * const month[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

/* namecmp -- compare two entries */
static int namecmp(const void *a, const void *b)
{
  if (**((const char * const *)a)=='[') return -1;
  return strcmp(*((const char * const *)a),*((const char * const *)b));
}

/* olddir  -- old style output */
/* 2016/11/01 uBee - **dirent are the command line parameters (gargv) and entries is gargc
   after cpmglob() has been run on argv and argc.  This also applies to functions oldddir(),
   old3dir(), lsattr() and ls() further below. (this just a comment, no code changes)
*/

static void olddir(char **dirent, int entries)
{
  int i,j,k,l,user,announce;

  announce=0;
  for (user=0; user<32; ++user)
  {
    for (i=l=0; i<entries; ++i)
    {
      if (dirent[i][0]=='0' + user/10 && dirent[i][1]=='0' + user%10)
      {
        if (announce==1)
        {
          printf("User %d\n",user);
        }
       
        announce=2;
        if (l%4) printf(" : ");
        for (j=2; dirent[i][j] && dirent[i][j] != '.'; ++j)
           putchar(toupper(dirent[i][j]));
      
        k=j;
        while (k<11)
           {
            putchar(' ');
            ++k;
           }
      
        if (dirent[i][j]=='.')
           ++j;
        for (k=0; dirent[i][j]; ++j,++k)
           putchar(toupper(dirent[i][j]));
        for (; k<3; ++k)
           putchar(' ');
        ++l;
      }
      
      if (l && (l%4) == 0)
       {
	l = 0;
	putchar('\n');
       }	
    }
    if (l%4) {
       putchar('\n');
    }

    if (announce==2) announce=1;
  }
  if (entries==0) printf("No files\n");
}

/* oldddir -- old style long output */
static void oldddir(char **dirent, int entries, struct cpmInode *ino)
{
  struct cpmStatFS buf;
  struct cpmStat statbuf;
  struct cpmInode file;

  if (entries)
  {
    int i,j,k,l,announce,user;

    qsort(dirent,entries,sizeof(char*),namecmp);
    cpmStatFS(ino,&buf);
    printf("     Name    Bytes   Recs  Attr     update             create\n");
    printf("------------ ------ ------ ---- -----------------  -----------------\n");
    announce=0;
    for (l=user=0; user<32; ++user)
    {
      for (i=0; i<entries; ++i)
      {
        struct tm *tmp;

        if (dirent[i][0]=='0'+user/10 && dirent[i][1]=='0'+user%10)
        {
          if (announce==1)
          {
            printf("\nUser %d:\n\n",user);
            printf("     Name    Bytes   Recs  Attr     update             create\n");
            printf("------------ ------ ------ ---- -----------------  -----------------\n");
          }
          announce=2;
          for (j=2; dirent[i][j] && dirent[i][j]!='.'; ++j) putchar(toupper(dirent[i][j]));
          k=j; while (k<10) { putchar(' '); ++k; }
          putchar('.');
          if (dirent[i][j]=='.') ++j;
          for (k=0; dirent[i][j]; ++j,++k) putchar(toupper(dirent[i][j]));
          for (; k<3; ++k) putchar(' ');

          cpmNamei(ino,dirent[i],&file);
          cpmStat(&file,&statbuf);
          printf(" %5.1ldK",(long) (statbuf.size+buf.f_bsize-1) /
			buf.f_bsize*(buf.f_bsize/1024));

          printf(" %6.1ld ",(long)(statbuf.size/128));
          putchar(statbuf.mode&0200 ? ' ' : 'R');
          putchar(statbuf.mode&01000 ? 'S' : ' ');
          putchar(' ');
          if (statbuf.mtime)
          {
            tmp=localtime(&statbuf.mtime);
            printf("  %02d-%s-%04d %02d:%02d",tmp->tm_mday,month[tmp->tm_mon],tmp->tm_year+1900,tmp->tm_hour,tmp->tm_min);
            tmp=localtime(&statbuf.ctime);
            printf("  %02d-%s-%04d %02d:%02d",tmp->tm_mday,month[tmp->tm_mon],tmp->tm_year+1900,tmp->tm_hour,tmp->tm_min);
          }
          putchar('\n');
          ++l;
        }
      }
      if (announce==2) announce=1;
    }
    printf("%5.1d Files occupying %6.1ldK",l,(buf.f_bused*buf.f_bsize)/1024);
    printf(", %7.1ldK Free.\n",(buf.f_bfree*buf.f_bsize)/1024);
  }
  else printf("No files found\n");
}

/* old3dir -- old CP/M Plus style long output */
static void old3dir(char **dirent, int entries, struct cpmInode *ino)
{
  struct cpmStatFS buf;
  struct cpmStat statbuf;
  struct cpmInode file;

  if (entries)
  {
    int i,j,k,l,announce,user, attrib;
    int totalBytes=0,totalRecs=0;

    qsort(dirent,entries,sizeof(char*),namecmp);
    cpmStatFS(ino,&buf);
    announce=1;
    for (l=0,user=0; user<32; ++user)
    {
      for (i=0; i<entries; ++i)
      {
        struct tm *tmp;

        if (dirent[i][0]=='0'+user/10 && dirent[i][1]=='0'+user%10)
        {
          cpmNamei(ino,dirent[i],&file);
          cpmStat(&file,&statbuf);
	  cpmAttrGet(&file, &attrib);
          if (announce==1)
          {
            if (user) putchar('\n');
            printf("Directory For Drive A:  User %2.1d\n\n",user);
            printf("    Name     Bytes   Recs   Attributes   Prot      Update          Create\n");
            printf("------------ ------ ------ ------------ ------ --------------  --------------\n\n");
          }
          announce=2;
          for (j=2; dirent[i][j] && dirent[i][j]!='.'; ++j) putchar(toupper(dirent[i][j]));
          k=j; while (k<10) { putchar(' '); ++k; }
          putchar(' ');
          if (dirent[i][j]=='.') ++j;
          for (k=0; dirent[i][j]; ++j,++k) putchar(toupper(dirent[i][j]));
          for (; k<3; ++k) putchar(' ');

          totalBytes+=statbuf.size;
          totalRecs+=(statbuf.size+127)/128;
          printf(" %5.1ldk",(long) (statbuf.size+buf.f_bsize-1) /
			buf.f_bsize*(buf.f_bsize/1024));
          printf(" %6.1ld ",(long)(statbuf.size/128));
          putchar((attrib & CPM_ATTR_F1)   ? '1' : ' ');
          putchar((attrib & CPM_ATTR_F2)   ? '2' : ' ');
          putchar((attrib & CPM_ATTR_F3)   ? '3' : ' ');          
          putchar((attrib & CPM_ATTR_F4)   ? '4' : ' ');
          putchar((statbuf.mode&(S_IWUSR|S_IWGRP|S_IWOTH)) ? ' ' : 'R');
          putchar((attrib & CPM_ATTR_SYS)  ? 'S' : ' ');
          putchar((attrib & CPM_ATTR_ARCV) ? 'A' : ' ');
          printf("      ");
          if      (attrib & CPM_ATTR_PWREAD)  printf("Read   ");
          else if (attrib & CPM_ATTR_PWWRITE) printf("Write  ");
          else if (attrib & CPM_ATTR_PWDEL)   printf("Delete "); 
          else printf("None   ");
          tmp=localtime(&statbuf.mtime);
          printf("%02d/%02d/%02d %02d:%02d  ",tmp->tm_mon+1,tmp->tm_mday,tmp->tm_year%100,tmp->tm_hour,tmp->tm_min);
          tmp=localtime(&statbuf.ctime);
          printf("%02d/%02d/%02d %02d:%02d",tmp->tm_mon+1,tmp->tm_mday,tmp->tm_year%100,tmp->tm_hour,tmp->tm_min);
          putchar('\n');
          ++l;
        }
      }
      if (announce==2) announce=1;
    }
    printf("\nTotal Bytes     = %6.1dk  ",(totalBytes+1023)/1024);
    printf("Total Records = %7.1d  ",totalRecs);
    printf("Files Found = %4.1d\n",l);
    printf("Total 1k Blocks = %6.1ld   ",(buf.f_bused*buf.f_bsize)/1024);
    printf("Used/Max Dir Entries For Drive A: %4.1ld/%4.1ld\n",buf.f_files-buf.f_ffree,buf.f_files);
  }
  else printf("No files found\n");
}

/* ls      -- UNIX style output */
static void ls(char **dirent, int entries, struct cpmInode *ino, int l, int c, int iflag)
{
  int i,user,announce,any;
  time_t now;
  struct cpmStat statbuf;
  struct cpmInode file;

  time(&now);
  qsort(dirent,entries,sizeof(char*),namecmp);
  announce=0;
  any=0;

  for (user=0; user<32; ++user)
  {
    announce=0;
    for (i=0; i<entries; ++i)
       if (dirent[i][0] != '.')
       {
         if (dirent[i][0]=='0'+user/10 && dirent[i][1]=='0'+user%10)
         {
           if (announce==0)
           {
             if (any)
                putchar('\n');

             printf("%d:\n",user);
             announce=1;
           }

           any=1;
           if (iflag || l)
           {
             cpmNamei(ino,dirent[i],&file);
             cpmStat(&file,&statbuf);
           }

           if (iflag)
              printf("%4ld ",(long) statbuf.ino);

           if (l)
           {
             struct tm *tmp;

             putchar(S_ISDIR(statbuf.mode) ? 'd' : '-');
             putchar(statbuf.mode&0400 ? 'r' : '-');
             putchar(statbuf.mode&0200 ? 'w' : '-');
             putchar(statbuf.mode&0100 ? 'x' : '-');
             putchar(statbuf.mode&0040 ? 'r' : '-');
             putchar(statbuf.mode&0020 ? 'w' : '-');
             putchar(statbuf.mode&0010 ? 'x' : '-');
             putchar(statbuf.mode&0004 ? 'r' : '-');
             putchar(statbuf.mode&0002 ? 'w' : '-');
             putchar(statbuf.mode&0001 ? 'x' : '-');
#if 0
             putchar(statbuf.flags&FLAG_PUBLIC ? 'p' : '-');
             putchar(dir[i].flags&FLAG_SYSTEM ? 's' : '-');
             printf(" %-2d ",dir[i].user);
#endif
             printf("%8.1ld ",(long)statbuf.size);
             tmp=localtime(c ? &statbuf.ctime : &statbuf.mtime);
             printf("%s %02d ",month[tmp->tm_mon],tmp->tm_mday);
             if ((c ? statbuf.ctime : statbuf.mtime)<(now-182*24*3600))
                printf("%04d  ",tmp->tm_year+1900);
             else
                printf("%02d:%02d ",tmp->tm_hour,tmp->tm_min);
           }

           printf("%s\n",dirent[i]+2);
         }
       }
  }
}

/* lsattr  -- output something like e2fs lsattr */
static void lsattr(char **dirent, int entries, struct cpmInode *ino)
{
  int i,user,announce,any;
  struct cpmStat statbuf;
  struct cpmInode file;
  cpm_attr_t attrib;

  qsort(dirent,entries,sizeof(char*),namecmp);
  announce=0;
  any=0;
  for (user=0; user<32; ++user)
  {
    announce=0;
    for (i=0; i<entries; ++i) if (dirent[i][0]!='.')
    {
      if (dirent[i][0]=='0'+user/10 && dirent[i][1]=='0'+user%10)
      {
        if (announce==0)
        {
          if (any) putchar('\n');
          printf("%d:\n",user);
          announce=1;
        }
        any=1;

        cpmNamei(ino,dirent[i],&file);
        cpmStat(&file,&statbuf);
        cpmAttrGet(&file, &attrib); 

        putchar ((attrib & CPM_ATTR_F1)      ? '1' : '-');
        putchar ((attrib & CPM_ATTR_F2)      ? '2' : '-');
        putchar ((attrib & CPM_ATTR_F3)      ? '3' : '-');
        putchar ((attrib & CPM_ATTR_F4)      ? '4' : '-');
        putchar ((attrib & CPM_ATTR_SYS)     ? 's' : '-');
        putchar ((attrib & CPM_ATTR_ARCV)    ? 'a' : '-');
        putchar ((attrib & CPM_ATTR_PWREAD)  ? 'r' : '-');
        putchar ((attrib & CPM_ATTR_PWWRITE) ? 'w' : '-');
        putchar ((attrib & CPM_ATTR_PWDEL)   ? 'e' : '-');

        printf(" %s\n",dirent[i]+2);
      }
    }
  }
}

const char cmd[]="cpmls";

int main(int argc, char *argv[])
{
  /* variables */
  const char *err;
  const char *image;
  const char *format=FORMAT;
  const char *devopts=NULL;
  int c,usage=0;
  struct cpmSuperBlock drive;
  struct cpmInode root;
  int style=0;
  int changetime=0;
  int inode=0;
  char **gargv;
  int gargc;
  static char starlit[2]="*";
  static char * const star[]={starlit};

  /* parse options */
#if HAVE_LIBDSK_H
  /* LibDsk option addition - uBee 2009/09/28 */
  /* New -e option to only work on erased_files - uBee 2016/11/01 */
  while ((c=getopt(argc,argv,"cT:L:f:ih?dDeFlAv"))!=EOF) switch(c)
#else  
  while ((c=getopt(argc,argv,"cT:f:ih?dDeFlAv"))!=EOF) switch(c)
#endif  
  {
    case 'f': format=optarg; break;
    case 'T': devopts=optarg; break;
    case 'h':
    case '?': usage=1; break;
    case 'd': style=1; break;
    case 'D': style=2; break;
    case 'e': erased_files = 1; break; /* uBee 2016/11/01 */
    case 'F': style=3; break;
    case 'l': style=4; break;
    case 'A': style=5; break;
    case 'c': changetime=1; break;
    case 'i': inode=1; break;
    case 'v': fprintf(stderr, APPVER"\n");    /* uBee 2010/03/31 */
              exit(1);
#if HAVE_LIBDSK_H
    case 'L': Device_libdsk_options(optarg);  /* uBee 2009/09/28 */
#endif    
  }

  if (optind == argc)
     usage = 1;
  else
     image = argv[optind++];
     
  /* 2016/11/01 uBee - at this point argv points to beginning of file parameters, i.e: 0:*.* 0:myfile.ext */

  if (usage)
  {
    fprintf(stderr,"Usage: %s [-ev] [-f format] [-T libdsk-type] [-d|-D|-F|-A|[-l][-c][-i]] image [file ...]\n",cmd);
    /* Add some more information about existing and new options - uBee 2009/09/28 */
    fprintf(stderr,"\nOther options:\n");
    fprintf(stderr," -c    Change times.\n");        /* 2016/11/02 uBee */
    fprintf(stderr," -d    Directory style 1.\n");   /* 2016/11/02 uBee */
    fprintf(stderr," -D    Directory style 2.\n");   /* 2016/11/02 uBee */
    fprintf(stderr," -F    Directory style 3.\n");   /* 2016/11/02 uBee */
    fprintf(stderr," -l    Directory style 4.\n");   /* 2016/11/02 uBee */    
    fprintf(stderr," -A    Directory style 5.\n");   /* 2016/11/02 uBee */
    fprintf(stderr," -i    Inode.\n");               /* 2016/11/02 uBee */
    fprintf(stderr," -e    Work on Erased files only (as user 0).\n");  /* 2016/11/01 uBee */
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
  if ((err=Device_open(&drive.dev,image,O_RDONLY,devopts))) 
  {
    fprintf(stderr,"%s: can not open %s (%s)\n",cmd,image,err);
    exit(1);
  }
  cpmReadSuper(&drive,&root,format);

  /* 2016/11/01 uBee - The CP/M file name entries are extract in 'cpmglob'
  to a higher level format */
  if (optind < argc)
     cpmglob(optind, argc, argv, &root, &gargc, &gargv);
  else
     cpmglob(0, 1, star, &root, &gargc, &gargv);

#if 0
 /* 2015/01/10, 2016/11/01 uBee */
 printf("cpmls: optind=%d argc=%d star=|%s| gargc=%d gargv=%s\n", optind, argc, star[0], gargc, gargv[0]);
#endif 

  if (style == 1)
     olddir(gargv, gargc);
  else if (style == 2)
     oldddir(gargv, gargc, &root);
  else if (style == 3)
     old3dir(gargv, gargc, &root);
  else if (style == 5)
     lsattr(gargv, gargc, &root); 
  else
     ls(gargv,gargc,&root,style==4,changetime,inode);
     
  exit(0);
}
