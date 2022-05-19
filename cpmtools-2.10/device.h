#ifndef DEVICE_H
#define DEVICE_H

#ifdef _WIN32
/* The type of device the file system is on: */
#define CPMDRV_FILE  0 /* Regular file or Unix block device */
#define CPMDRV_WIN95 1 /* Windows 95 floppy drive accessed via VWIN32 */
#define CPMDRV_WINNT 2 /* Windows NT floppy drive accessed via CreateFile */
#endif

struct Device
{
  int opened;

  int secLength;
  int tracks;
  int sectrk;
#if HAVE_LIBDSK_H
  DSK_PDRIVER   dev;
  DSK_GEOMETRY geom; 
#endif
#if HAVE_WINDOWS_H
  int drvtype;
  HANDLE hdisk;
#endif
  int fd;
  int datasect;  /* uBee 2009/09/28 */
  int cylinders; /* uBee 2015/01/08 */
  int heads;     /* uBee 2009/09/28 */
  int sidedness; /* uBee 2014/02/04 */
  int sideoffs;  /* uBee 2015/01/13 */
  int testside;  /* uBee 2009/09/28 */
  int addoffs;   /* uBee 2009/09/28 */
};

const char *Device_open(struct Device *self, const char *filename, int mode, const char *deviceOpts);

struct cpmSuperBlock; /* uBee 2016/07/12 */
void Device_setGeometry(struct Device *this, struct cpmSuperBlock *d);

const char *Device_close(struct Device *self);
/* uBee 2016/07/18 rev i
   Added a 'flags' parameter to Device_readSector() and Device_writeSector()

   uBee 2010/10/17 rev d
   Added logical sector (lsector) for drivers that need a logical sector
   number and handle's it's own skew translation i.e the 'remote' driver on
   CP/M 3, CP/M 2. */

/*
const char *Device_readSector(const struct Device *self, int track, int sector, char *buf);
const char *Device_writeSector(const struct Device *self, int track, int sector, const char *buf);
*/

const char *Device_readSector(const struct Device *self, int track, int sector, int lsector, int flags, char *buf);
const char *Device_writeSector(const struct Device *self, int track, int sector, int lsector, int flags, const char *buf);

#if HAVE_LIBDSK_H
void Device_libdsk_options (char *libdsk_opts);
#endif

#endif
