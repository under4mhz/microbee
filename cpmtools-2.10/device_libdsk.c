#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>      /* for printf debugging - uBee 2009/09/28 */
#include <ctype.h>      /* for toupper() - uBee 2009/09/28 */
#ifdef _WIN32  /* uBee 2016/07/18 */
#include <ncurses/curses.h>
#else
#include <curses.h>
#endif
#ifdef __linux__
#include <sys/ioctl.h>  /* for ioctl - uBee 2009/09/28 */
#include <linux/fd.h>   /* for ioctl constants - uBee 2009/09/28 */
#endif

#include "cpmfs.h"      /* uBee 2014/02/03 */
#include "device.h"

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int datarate = -1;    /* uBee 2009/09/28 */
static int doublestep = -1;  /* uBee 2009/09/28 */
static char device_type[32]; /* uBee 2010/03/11 */

int rcpmfs;                  /* uBee 2016/11/11 */

/*
================================================================================
 Disk read ID field function - added by uBee 2009/09/28.

 This is used to get the sector value from the ID field of the first sector
 read from the requested track/head.  The value is needed by some formats
 that don't use the correct side value in the sector IDs.

 uBee 2015/01/13
 Added new this->sideoffs value to the head_result

 uBee 2014/02/16
 Added cyl_result return value.

 uBee 2010/03/11
 The dsk_psecid() may not be supported by the LibDsk driver.  In these cases this
 function returns *head_result equal to the side parameter. The function now always 
 returns DSK_ERR_OK.

   pass: struct Device *this
         int side
         int track
         int *head_result               result of read for the head ID
         int *cyl_result                result of read for the cylinder ID
 return: int				0 if no errors, else error number
================================================================================
*/
static int disk_read_idfield (const struct Device *this, int side, int track, int *head_result, int *cyl_result)
{
 int res = 0; 

 dsk_err_t dsk_err;
 DSK_FORMAT result;

#if 0
 printf("disk_read_idfield() track=%d, side=%d\n", track, side);
#endif

 /* get the side information from first available sector header ID */
 dsk_err = dsk_psecid(this->dev, &this->geom, track, side, &result);
  
 if (dsk_err == DSK_ERR_OK)
    {
     *head_result = result.fmt_head;
     *cyl_result = result.fmt_cylinder;
    }
 else                             /* else if DSK_ERR_NOTIMPL or read error */
    {
     *head_result = side + this->sideoffs;
     *cyl_result = track;
    }

#if 0
 printf("disk_read_idfield() *head_result=%d, *cyl_result=%d\n",
 *head_result, *cyl_result);
#endif

 return DSK_ERR_OK;
}

/*
================================================================================
 Device_open           -- Open an image file

 uBee 2010/03/11

 The code added 2009/12/11 and edited in patch 'c' has now been simplified
 as the original reason for having it is now being done using a better
 method. The code is now used to convert the type to lower case so that type
 testing can be made in other places.
 
 The driver_type variable has now been removed.
================================================================================
*/
const char *Device_open(struct Device *this, const char *filename, int mode, const char *deviceOpts)
{
 int i;
 dsk_err_t dsk_err;
 int fd;

 if (deviceOpts != NULL)
    {
     strncpy(device_type, deviceOpts, sizeof(device_type)-1);
     device_type[sizeof(device_type)-1] = 0;
     i = 0;
     /* This may be used to test for certain device types and assign driver_type accordingly. */
     while (device_type[i])
        {
         device_type[i] = tolower(device_type[i]);
         i++;
        }
     /* uBee 20161/11/11 - test if it's the 'rcpmfs' type and flag it */
     rcpmfs = (strcmp(device_type, "rcpmfs") == 0);
    }
 else
    strcpy(device_type, "raw");
 /* end added code - uBee 2009/12/11, 2010/02/21, 2010/03/11 */
    
 /* uBee 2009/09/28  dsk_err_t e = dsk_open(&this->dev, filename, deviceOpts, NULL); */
  dsk_err = dsk_open(&this->dev, filename, device_type, NULL); /* uBee 2009/09/28 */
  this->opened = 0;
  if (dsk_err)
     return dsk_strerror(dsk_err);
  this->opened = 1;
  dsk_getgeom(this->dev, &this->geom);

 /* uBee 2009/09/28
   number of retries, and double stepping
 */   
  
 dsk_set_retry(this->dev, 100);   /* set retry count high */

 /* set double stepping if double stepping parameter used */
 if (doublestep != -1 && this->dev)
    dsk_err = dsk_set_option(this->dev, "DOUBLESTEP", 1);
 /* end added code - uBee 2009/09/28 */

  return NULL;
}

/*
================================================================================
 Device_report - report values, added uBee 2016/07/14
================================================================================
*/
static void Device_report_1(struct Device *this, struct cpmSuperBlock *d)
{
 printf("device_libdsk: Device_setGeometry()\n"
        "d->cylinders            = %d\n"
        "d->secLength            = %d\n"
        "d->sectrk               = %d\n"
        "d->tracks               = %d\n"
        "d->datasect             = %d\n"
        "d->heads                = %d\n"
        "d->sidedness            = %d\n"
        "d->testside             = %d\n"
        "d->secLength            = %d\n"
        "d->fm                   = %d\n"
        "d->datarate             = %d\n"
        "this->geom.dg_heads     = %d\n"
        "this->geom.dg_cylinders = %d\n\n",
        d->cylinders,
        d->secLength,
        d->sectrk,
        d->tracks,
        d->datasect,
        d->heads,
        d->sidedness,
        d->testside,
        d->secLength,
        d->fm,
        d->datarate,
        this->geom.dg_heads,
        this->geom.dg_cylinders);
}             

/*
================================================================================
 Exit ncurses before reporting an error, added uBee 2016/07/18
================================================================================
*/
static void exit_ncurses(int flags)
{
 if (! (flags & 0x01))
    return;
    
 move(LINES-1,0);
 refresh();
 echo();
 noraw();
 endwin();
}

/*
================================================================================
 Set the minium and maximum sector value range, added uBee 2016/07/18
================================================================================
*/
static void sector_minmax(const struct Device *this, int flags, int *min_sector, int *max_sector)
{
 if (flags & 0x02)  /* system tracks? */
    {
     *min_sector = 1;
     *max_sector = this->sectrk;
    }
 else
    {
     *min_sector = this->datasect;
     *max_sector = this->datasect + (this->sectrk - 1);
    }
}

/*
================================================================================
 Device_setGeometry    -- Set disk geometry
================================================================================
*/
void Device_setGeometry(struct Device *this, struct cpmSuperBlock *d)  /* uBee 2016/07/12 */
{
  this->secLength=d->secLength;
  this->sectrk=d->sectrk;
  this->tracks=d->tracks;
  this->datasect=d->datasect;   /* uBee 2009/09/28 */
  this->heads=d->heads;         /* uBee 2009/09/28 */
  this->sidedness=d->sidedness; /* uBee 2014/02/04 */
  this->testside=d->testside;   /* uBee 2009/09/28 */

  this->geom.dg_secsize  = d->secLength;
  this->geom.dg_sectors  = d->sectrk;
  this->geom.dg_secbase  = d->datasect; /* uBee 2009/09/28 */
  this->geom.dg_fm       = d->fm;       /* uBee 2010/03/17 */
 
  /* uBee 2016/07/12 */
  if (datarate != -1)                   /* use the command line option? */
     this->geom.dg_datarate = datarate; /* set data rate */
  else
     if (d->datarate != -1)             /* use the diskdefs option? */
        this->geom.dg_datarate = d->datarate; /* set data rate */
  
  /* Did the autoprobe guess right about the number of sectors & cylinders? */
  if (this->geom.dg_cylinders * this->geom.dg_heads == d->tracks) return;

  /* Let the tracks and heads parameter decide these values if cylinders was specified - uBee 2009/09/28 */
  if (d->cylinders)  
     {
      this->geom.dg_heads     = d->heads;             /* uBee 2009/09/28 */
      this->geom.dg_cylinders = d->tracks / d->heads; /* uBee 2009/09/28 */
#if 0
      Device_report_1(this, d);
#endif      
      return;
     } 

  /* Otherwise we guess: <= 43 tracks: single-sided. Else double. This
   * fails for 80-track single-sided if there are any such beasts */

  if (d->tracks <= 43) 
  {
    this->geom.dg_cylinders = d->tracks;
    this->geom.dg_heads     = 1; 
  }
  else
  {
    this->geom.dg_cylinders = d->tracks/2;
    this->geom.dg_heads     = 2; 
  }
#if 0
      Device_report_1(this, d);
#endif  
}

/*
================================================================================
 Device_close          -- Close an image file
================================================================================ 
*/
const char *Device_close(struct Device *this)
{
  dsk_err_t dsk_err;
  this->opened=0;
  dsk_err = dsk_close(&this->dev);
  return (dsk_err?dsk_strerror(dsk_err):(const char*)0);
}

/*
================================================================================
 uBee 2016/07/18 rev i
 Added error checking for out of range tracks/sectors and check results
 of reads. If error then exit(1) to avoid segmentation issues.
 Note: No checking is done for 'head' values as it should be correct as it's
 determined by get_physical_values().
 Added a 'flags' parameter.

 uBee 2014/02/16
 Changes to also return the cylinder value found when calling
 disk_read_idfield() and making use of it when reading the sector.
   
 uBee 2009/09/28
 Converted this function to use physical reads instead of logical and some
 other additions.

 uBee 2015/01/08 rev h
 Added call to new get_physical_values() to return physical values based
 on 'sidedness' value.  Sidedness is done here from within cpmtools and
 does not use the LibDsk Sidedness option.
   
 uBee 2010/03/17 rev d
 Added logical sector (lsector) for drivers that need a logical sector
 number and handle's it's own skew translation i.e the 'remote' driver on
 CP/M 3, CP/M 2 or a method to use a modified remote AUXD.COM which is
 being here.
================================================================================
*/
/* Device_readSector     -- read a physical sector */
const char *Device_readSector(const struct Device *this, int track, int sector,
int lsector, int flags, char *buf)
{
 dsk_err_t dsk_err;
 int head;
 int cylinder;
 int head_result;
 int cyl_result;

 int min_sector;
 int max_sector;

 /* determine what the allowed sector range is - uBee 2016/07/18 */
 sector_minmax(this, flags, &min_sector, &max_sector);

 /* check sector value is legal for the geometry - uBee 2016/07/18 */
 if (sector < min_sector || sector > max_sector)
    {
     exit_ncurses(flags);
     printf("patched-cpmtools error: Device_readSector() - illegal sector value!\n");
     printf("sector=%d, allowed range=%d to %d\n", sector, min_sector, max_sector);
     exit (1);
    }

 /* convert to physical values according to sidedness */
 get_physical_values(this->geom.dg_cylinders, this->geom.dg_heads,
 this->sidedness, track, &cylinder, &head);

 /* check track (physical) value is legal for the geometry - uBee 2016/07/14 */
 if (cylinder >= this->geom.dg_cylinders)
    {
     exit_ncurses(flags);
     printf("patched-cpmtools error: Device_readSector() - illegal cylinder value!\n");
     printf("cylinder=%d, allowed range=0 to %d\n", cylinder, this->geom.dg_cylinders - 1);
     exit (1);
    }

 /* The original 'remote' AUXD program is based on logical and skewed values.
    If using an unmodified AUXD program the following code could be used to
    some limited degree, but instead a customised AUXD for the Microbee
    computer has been created, within that the dsk_pread() function uses
    physical and unskewed sector values. By doing it this way ubeedisk and
    this patched cpmtools will work for all floppy disks.
 */
#if 0
 if (strcmp(device_type, "remote") == 0)
    {
     sector = lsector;   /* use the logical sector number */
     cylinder = track;   /* back to the logical track number */
    }
#endif

#if 0
 printf("Device_readSector(): head=%d cyl=%02d sector=%d\n", head, cylinder, sector);
#endif

 /* Check side/cylinder value in first found sector ID.  This is done if requested by the disk format
 uBee 2009/12/11, 2010/03/11, 2014/02/16 */
 if (this->testside)
    {
     dsk_err = disk_read_idfield(this, head, cylinder, &head_result, &cyl_result);

#if 0
     printf("Device_readSector(): head=%d cyl=%02d head_result=%d cyl_result=%02d sector=%d\n",
     head, cylinder, head_result, cyl_result, sector);
#endif

     /* uBee 2014/02/16 - The 1st cylinder/head values are the physical
      * values, the 2nd cylinder/head values is what is expected to be
      * in the sector header. */

     dsk_err = dsk_xread(this->dev, &this->geom, buf, cylinder, head, cyl_result, head_result, sector, this->secLength, NULL);
     if (dsk_err == DSK_ERR_NOTIMPL)
        dsk_err = dsk_pread(this->dev, &this->geom, buf, cylinder, head, sector);
    }
 else
    dsk_err = dsk_pread(this->dev, &this->geom, buf, cylinder, head, sector);

 /* check result and exit if an error - uBee 2016/07/14 */
 if (dsk_err != DSK_ERR_OK)
    {
     exit_ncurses(flags);
     printf("patched-cpmtools error: Device_readSector(): head=%d cylinder=%d sector=%d\n", head, cylinder, sector);
     printf("Check \"-f format\" and all parameters are correct for this disk!\n");
     exit (1);
    }

 return (dsk_err?dsk_strerror(dsk_err):(const char*)0);
}

/*
================================================================================
 uBee 2016/07/18 rev i
 Added error checking for out of range tracks/sectors and check results
 of writes. If error then exit(1) to avoid segmentation issues.
 Note: No checking is done for 'head' values as it should be correct as it's
 determined by get_physical_values().
 Added a 'flags' parameter.

 uBee 2014/02/16
 Changes to also return the cylinder value found when calling
 disk_read_idfield() and making use of it when writing the sector.

 uBee 2009/09/28
 Converted this function to use physical writes instead of logical and some
 other additions.

 uBee 2015/01/08 rev h
 Added call to new get_physical_values() to return physical values based
 on 'sidedness' value.  Sidedness is done here from within cpmtools and
 does not use the LibDsk Sidedness option.

 uBee 2010/03/17 rev d
 Added logical sector (lsector) for drivers that need a logical sector
 number and handle's it's own skew translation i.e the 'remote' driver on
 CP/M 3, CP/M 2 or a method to use a modified remote AUXD.COM which is
 being here.
================================================================================
*/
/* Device_writeSector    -- write physical sector */
const char *Device_writeSector(const struct Device *this, int track, int sector,
int lsector, int flags, const char *buf)
{
 dsk_err_t dsk_err;
 int head;
 int cylinder;
 int head_result;
 int cyl_result;

 int min_sector;
 int max_sector;

 /* determine what the allowed sector range is - uBee 2016/07/18 */
 sector_minmax(this, flags, &min_sector, &max_sector);

 /* check sector value is legal for the geometry - uBee 2016/07/18 */
 if (sector < min_sector || sector > max_sector)
    {
     exit_ncurses(flags);
     printf("patched-cpmtools error: Device_writeSector() - illegal sector value!\n");
     printf("sector=%d, allowed range=%d to %d\n", sector, min_sector, max_sector);
     exit (1);
    }

 /* convert to physical values according to sidedness */
 get_physical_values(this->geom.dg_cylinders, this->geom.dg_heads,
 this->sidedness, track, &cylinder, &head);

 /* check track (physical) value is legal for the geometry - uBee 2016/07/14 */
 if (cylinder >= this->geom.dg_cylinders)
    {
     exit_ncurses(flags);
     printf("patched-cpmtools error: Device_writeSector() - illegal cylinder value!\n");
     printf("cylinder=%d, allowed range=0 to %d\n", cylinder, this->geom.dg_cylinders - 1);
     exit (1);
    }

 /* The original 'remote' AUXD program is based on logical and skewed values.
    If using an unmodified AUXD program the following code could be used to
    some limited degree, but instead a customised AUXD for the Microbee
    computer has been created, within that the dsk_pwrite() function uses
    physical and unskewed sector values. By doing it this way ubeedisk and
    this patched cpmtools will work for all floppy disks.
 */
#if 0
 if (strcmp(device_type, "remote") == 0)
    {
     sector = lsector;   /* use the logical sector number */
     cylinder = track;   /* back to the logical track number */
    }
#endif

#if 0
 printf("Device_writeSector(): head=%d cyl=%02d sector=%d\n", head, cylinder, sector);
#endif

 /* Check side/cylinder value in first found sector ID.  This is done if requested by the disk format
    uBee 2009/12/11, 2010/03/11, 2014/02/16 */
 if (this->testside)
    {
     dsk_err = disk_read_idfield(this, head, cylinder, &head_result, &cyl_result);

#if 0
     printf("Device_readSector(): head=%d cyl=%02d head_result=%d cyl_result=%02d sector=%d\n",
     head, cylinder, head_result, cyl_result, sector);
#endif

     dsk_err = dsk_xwrite(this->dev, &this->geom, buf, cylinder, head, cyl_result, head_result, sector, this->secLength, (int)NULL);
     if (dsk_err == DSK_ERR_NOTIMPL)
        dsk_err = dsk_pwrite(this->dev, &this->geom, buf, cylinder, head, sector);
    }
 else
    dsk_err = dsk_pwrite(this->dev, &this->geom, buf, cylinder, head, sector);

 /* check result and exit if an error - uBee 2016/07/14 */
 if (dsk_err != DSK_ERR_OK)
    {
     exit_ncurses(flags);
     printf("patched-cpmtools error: Device_writeSector(): head=%d cylinder=%d sector=%d\n", head, cylinder, sector);
     printf("Check \"-f format\" and all parameters are correct for this disk!\n");
     exit (1);
    }

  return (dsk_err?dsk_strerror(dsk_err):(const char*)0);
}

/*
================================================================================
 uBee 2009/09/28
 Set LibDsk options from one of the cpmtools utility programs.

 This function allows setting of various LibDsk options separated by spaces.
   pass: struct Device *this
 return: void
================================================================================
*/
void Device_libdsk_options (char *libdsk_opts)
{
 int i;
 char options[100];

 options[0] = ' '; /* leading space as delimiter */
 strncpy(&options[1], libdsk_opts, sizeof(options)-2);
 options[sizeof(options)-1] = 0;
 i = strlen(options);
 options[i] = ' '; /* trailing space */
 options[i+1] = 0;
 
 i = 0;
 while (options[i])
    {
     options[i] = toupper(options[i]);
     i++;
    }

 if (strstr(options, " HD "))
    datarate = 0;

 if (strstr(options, " DD "))
    datarate = 1;

 if (strstr(options, " SD "))
    datarate = 2;

 if (strstr(options, " ED "))
    datarate = 3;

 if (strstr(options, " DSTEP "))
    doublestep = 1;
}
