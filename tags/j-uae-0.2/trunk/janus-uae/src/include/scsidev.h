 /*
  * UAE - The Un*x Amiga Emulator
  *
  * a SCSI device
  *
  * (c) 1995 Bernd Schmidt (hardfile.c)
  * (c) 1999 Patrick Ohly
  * (c) 2001-2002 Toni Wilen
  */

uaecptr scsidev_startup (uaecptr resaddr);
void scsidev_install (void);
void scsidev_exit (void);
void scsidev_reset (void);
void scsidev_start_threads (void);
void scsi_do_disk_change (int device_id, int insert);

extern int log_scsi;
