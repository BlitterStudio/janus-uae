/*
 * The following functions do exactly the same thing as their
 * Amiga counterpart, but can be called in situation where calling
 * the exec.library functions is impossible.
 */

#ifndef UAE_NATIVE2AMIGA_API_H
#define UAE_NATIVE2AMIGA_API_H

#include "uae/types.h"

#ifdef SUPPORT_THREADS
void uae_Cause(uaecptr interrupt);
void uae_ReplyMsg(uaecptr msg);
void uae_PutMsg(uaecptr port, uaecptr msg);
void uae_Signal(uaecptr task, uae_u32 mask);
void uae_NotificationHack(uaecptr, uaecptr);
#endif
void uae_NewList(uaecptr list);
int native2amiga_isfree (void);

#endif /* UAE_NATIVE2AMIGA_API_H */
