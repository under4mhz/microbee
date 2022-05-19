#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "device.h"

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif


/* Device_open           -- Open an image file                      */
const char *Device_open(struct Device *this, const char *filename, int mode, const char *deviceOpts)
{
  this->fd=open(filename,mode);
  this->opened=(this->fd==-1?0:1);
  return ((this->fd==-1)?strerror(errno):(const char*)0);
}

/* Device_setGeometry    -- Set disk geometry                       */
/* uBee 2009/09/28 void Device_setGeometry(struct Device *this, int secLength, int sectrk, int tracks) */

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
}

/* Device_close          -- Close an image file                     */
const char *Device_close(struct Device *this)
{
  this->opened=0;
  return ((close(this->fd)==-1)?strerror(errno):(const char*)0);
}

/*
================================================================================
 uBee 2016/07/18 rev i
 Added 'flags' parameter, won't be used here.
 
 uBee 2010/03/17 rev d
 Added logical sector (lsector) for drivers that need a logical sector
 number and handle's it's own skew translation i.e the 'remote' driver on
 CP/M 3, CP/M 2 not tried. (Probably not by this driver)
================================================================================
*/
/* Device_readSector     -- read a physical sector                  */
const char *Device_readSector(const struct Device *this, int track, int sector, int lsector, int flags, char *buf)
{
  int res;

  sector -= this->datasect; /* make the sector 0 based uBee 2009/09/28 */
  assert(sector>=0);
  assert(sector<this->sectrk);
  assert(track>=0);
  assert(track<this->tracks);
  if (lseek(this->fd,(off_t)(sector+track*this->sectrk)*this->secLength,SEEK_SET)==-1) 
  {
    return strerror(errno);
  }
  if ((res=read(this->fd, buf, this->secLength)) != this->secLength) 
  {
    if (res==-1)
    {
      return strerror(errno);
    }
    else memset(buf+res,0,this->secLength-res); /* hit end of disk image */
  }
  return (const char*)0;
}

/*
================================================================================
 uBee 2016/07/18 rev i
 Added 'flags' parameter, won't be used here.

 uBee 2010/03/17 rev d
 Added logical sector (lsector) for drivers that need a logical sector
 number and handle's it's own skew translation i.e the 'remote' driver on
 CP/M 3, CP/M 2 not tried. (Probably not by this driver)
================================================================================ 
*/
/* Device_writeSector    -- write physical sector                   */
const char *Device_writeSector(const struct Device *this, int track, int sector, int lsector, int flags, const char *buf)
{
  sector -= this->datasect; /* make the sector 0 based - uBee 2009/09/28 */
  assert(sector>=0);
  assert(sector<this->sectrk);
  assert(track>=0);
  assert(track<this->tracks);
  if (lseek(this->fd,(off_t)(sector+track*this->sectrk)*this->secLength, SEEK_SET)==-1)
  {
    return strerror(errno);
  }
  if (write(this->fd, buf, this->secLength) == this->secLength) return (const char*)0;
  return strerror(errno);
}
