#ifndef CPMFS_H
#define CPMFS_H

#include <sys/stat.h>
#include <sys/types.h>

#include "build.h"  /* 2010/03/31 uBee */

#ifdef _WIN32
    #include <windows.h>
    #include <winioctl.h>
    /* To make it compile on NT: extracts from Linux 2.0 *
     * <statbuf.h> and <sys/stat.h>                      */
    #define __S_IFMT        0170000 /* These bits determine file type.  */
    #define __S_IFDIR       0040000 /* Directory.  */
    #define __S_IFREG       0100000 /* Regular file.  */
    #define __S_IWUSR       0000200 /* Writable for user.  */
    #define __S_IWGRP       0000200 /* Writable for group.  */
    #define __S_IWOTH       0000200 /* Writable for others.  */

    #define __S_ISTYPE(mode, mask)  (((mode) & __S_IFMT) == (mask))
    #define __S_ISTYPE(mode, mask)  (((mode) & __S_IFMT) == (mask))
    /* These bits are defined in Borland C++ 5 but not in MS Visual C++ */
    #ifndef S_ISDIR
    # define S_ISDIR(mode)   __S_ISTYPE((mode), __S_IFDIR)
    #endif
    #ifndef S_ISREG
    # define S_ISREG(mode)   __S_ISTYPE((mode), __S_IFREG)
    #endif
    #ifndef S_IWUSR
    #define S_IWUSR __S_IWUSR
    #endif
    #ifndef S_IWGRP
    #define S_IWGRP __S_IWGRP
    #endif
    #ifndef S_IWOTH
    #define S_IWOTH __S_IWOTH
    #endif

    #include <io.h>            /* For open(), lseek() etc. */
    #ifndef HAVE_MODE_T
    typedef int mode_t;
    #endif
#endif

#ifdef __cplusplus
        extern "C" {
#endif

#include "device.h"

/* CP/M file attributes */
#define CPM_ATTR_F1		1
#define CPM_ATTR_F2		2
#define CPM_ATTR_F3		4
#define CPM_ATTR_F4		8
/* F5-F8 are banned in CP/M 2 & 3, F7 is used by ZSDOS */
#define CPM_ATTR_RO		256     /* Read-only */
#define CPM_ATTR_SYS		512	/* System */
#define CPM_ATTR_ARCV		1024	/* Archive */
#define CPM_ATTR_PWDEL 		2048	/* Password required to delete */
#define CPM_ATTR_PWWRITE	4096	/* Password required to write */
#define CPM_ATTR_PWREAD		8192	/* Password required to read */

typedef int cpm_attr_t;

struct cpmInode
{
  ino_t ino;
  mode_t mode;
  off_t size;
  cpm_attr_t attr;
  time_t atime;
  time_t mtime;
  time_t ctime;
  struct cpmSuperBlock *sb;
};

struct cpmFile
{
  mode_t mode;
  off_t pos;
  struct cpmInode *ino;
};

struct cpmDirent
{
  ino_t ino;
  off_t off;
  size_t reclen;
  char name[2+8+1+3+1]; /* 00foobarxy.zzy\0 */
};

struct cpmStat
{
  ino_t ino;
  mode_t mode;
  off_t size;
  time_t atime;
  time_t mtime;
  time_t ctime;
};

#define CPMFS_DR22  0
#define CPMFS_P2DOS 1
#define CPMFS_DR3   2

struct cpmSuperBlock
{
  struct Device dev;

  int secLength;
  int tracks;
  int sectrk;
  int blksiz;
  int maxdir;
  int skew;
  int skewstart;    /* uBee 2009/09/28 */
  int datasect;     /* uBee 2009/09/28 */
  int heads;        /* uBee 2009/09/28 */
  int sidedness;    /* uBee 2014/02/03 */
  int sideoffs;     /* uBee 2015/01/13 */  
  int cylinders;    /* uBee 2009/09/28 */
  int fm;           /* uBee 2010/03/17 */
  int datarate;     /* uBee 2016/07/11 */
  int testside;     /* uBee 2009/09/28 */
  int boottrk;
  int type;
  int size;
  int extents; /* logical extents per physical extent */
  struct PhysDirectoryEntry *dir;
  int alvSize;
  int *alv;
  int *skewtab;
  int cnotatime;
  char *label;
  size_t labelLength;
  char *passwd;
  size_t passwdLength;
  struct cpmInode *root;
  int dirtyDirectory; /* uBee (MH 2.13) 2010/04/03 */
};

struct cpmStatFS
{
  long f_bsize;
  long f_blocks;
  long f_bfree;
  long f_bused;
  long f_bavail;
  long f_files;
  long f_ffree;
  long f_namelen;
};

extern const char cmd[];
extern const char *boo;

int match(const char *a, const char *pattern);
void cpmglob(int opti, int argc, char * const argv[], struct cpmInode *root, int *gargc, char ***gargv);

int cpmReadSuper(struct cpmSuperBlock *drive, struct cpmInode *root, const char *format);
int cpmNamei(const struct cpmInode *dir, const char *filename, struct cpmInode *i);
void cpmStatFS(const struct cpmInode *ino, struct cpmStatFS *buf);
int cpmUnlink(const struct cpmInode *dir, const char *fname);
int cpmRename(const struct cpmInode *dir, const char *old, const char *newname);
int cpmOpendir(struct cpmInode *dir, struct cpmFile *dirp);
int cpmReaddir(struct cpmFile *dir, struct cpmDirent *ent);
void cpmStat(const struct cpmInode *ino, struct cpmStat *buf);
int cpmAttrGet(struct cpmInode *ino, cpm_attr_t *attrib);
int cpmAttrSet(struct cpmInode *ino, cpm_attr_t attrib);
int cpmChmod(struct cpmInode *ino, mode_t mode);
int cpmOpen(struct cpmInode *ino, struct cpmFile *file, mode_t mode);
int cpmRead(struct cpmFile *file, char *buf, int count);
int cpmWrite(struct cpmFile *file, const char *buf, int count);
int cpmClose(struct cpmFile *file);
int cpmCreat(struct cpmInode *dir, const char *fname, struct cpmInode *ino, mode_t mode);
int cpmSync(struct cpmSuperBlock *sb);
void cpmUmount(struct cpmSuperBlock *sb);
int string_search (char *strg_array[], char *strg_find);
void get_physical_values (int tracks, int heads, int sidedness, int track, int *cylinder, int *head);

#ifdef __cplusplus
	}
#endif

#endif
