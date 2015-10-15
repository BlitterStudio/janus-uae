// Implement MMAN and SHM functionality for Win32
// Copyright (C) 2000, Brian King
// GNU Public License

#ifndef _MMAN_H_
#define _MMAN_H_

//#include <windows.h>
#include <stdio.h> // for size_t
#include <time.h>
#include <signal.h>

#define MAX_SHMID 256

extern uae_u8 *natmem_offset, *natmem_offset_end;

typedef int uae_key_t;
typedef USHORT ushort;

/* One shmid data structure for each shared memory segment in the system. */
struct shmid_ds {
    uae_key_t key;
    size_t size;
    size_t rosize;
    void  *addr;
    TCHAR name[MAX_PATH];
    void *attached;
    int mode;
    void *natmembase;
    bool fake;
    int maprom;
};

//int mprotect (void *addr, size_t len, int prot);
void *uae_shmat (int shmid, void *shmaddr, int shmflg);
int uae_shmdt (const void *shmaddr);
int uae_shmget (uae_key_t key, size_t size, int shmflg, const TCHAR* name);
int uae_shmctl (int shmid, int cmd, struct shmid_ds *buf);
bool init_shm (void);

#define PROT_READ  0x01
#define PROT_WRITE 0x02
#define PROT_EXEC  0x04

#define IPC_PRIVATE 0x01
#define IPC_RMID    0x02
#define IPC_CREAT   0x04
#define IPC_STAT    0x08

#endif
