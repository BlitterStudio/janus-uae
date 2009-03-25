 /*
  * UAE - The Un*x Amiga Emulator
  *
  * CIA chip support
  *
  * (c) 1995 Bernd Schmidt
  */

extern void CIA_reset (void);
extern void CIA_vsync_handler (void);
extern void CIA_hsync_handler (void);
extern void CIA_handler (void);

extern void diskindex_handler (void);
extern void cia_diskindex (void);

extern void dumpcia (void);
extern void rethink_cias (void);

extern int parallel_direct_write_data (uae_u8, uae_u8);
extern int parallel_direct_read_data (uae_u8*);
extern int parallel_direct_write_status (uae_u8, uae_u8);
extern int parallel_direct_read_status (uae_u8*);

extern void CIA_inprec_prepare (void);
