/*
 * E-UAE - The portable Amiga Emulator
 *
 * MC68000 emulation - machine-dependent optimized operations
 *
 * (c) 2004-2007 Richard Drummond
 *
 * Based on code from UAE
 * Copyright 1996 Bernd Schmidt
 */

#ifndef EUAE_MACHDEP_M68KOPS_H
#define EUAE_MACHDEP_M68KOPS_H

/*
 * Optimized code which uses the host CPU's condition flags to evaluate
 * 68K CCR flags for certain operations.
 *
 * These are used by various opcode handlers when
 * gencpu has been built with OPTIMIZED_FLAGS defined
 */
/* GCC 2.95 doesn't understand the XER register */
#if __GNUC__ - 1 > 1
# define DEP_XER ,"xer"
#else
# define DEP_XER
#endif

/*
 * Test operations
 *
 * Evaluate operand and set Z and N flags. Always clear C and V.
 */
#define optflag_testl(regs, v) 				\
    do { 						\
	uae_u32 tmp; 					\
	asm (						\
		"cmpi cr0, %2, 0	\n\t" 		\
		"mfcr %1		\n\t" 		\
		"rlwinm %0, %1, 0, 0, 3 \n\t" 		\
							\
		: "=r" ((regs)->ccrflags.cznv),		\
		  "=r" (tmp)				\
		:  "r" (v)				\
		: "cr0"					\
	);						\
    } while (0)

#define optflag_testw(regs, v) optflag_testl(regs, (uae_s32)(v))
#define optflag_testb(regs, v) optflag_testl(regs, (uae_s32)(v))

/*
 * Add operations
 *
 * Perform v = s + d and set ZNCV accordingly
 */
#define optflag_addl(regs, v, s, d)			\
    do {						\
	asm (						\
		"addco. %1, %2, %3	\n\t"		\
		"mcrxr  cr2		\n\t"		\
		"mfcr   %0		\n\t"		\
							\
		: "=r" ((regs)->ccrflags.cznv), "=r" (v)	\
		: "r" (s), "r" (d)			\
		: "cr0", "cr2"  DEP_XER			\
	);						\
	COPY_CARRY(&(regs)->ccrflags);			\
    } while (0)

#define optflag_addw(regs, v, s, d) do { optflag_addl(regs, (v), (s) << 16, (d) << 16); v = v >> 16; } while (0)
#define optflag_addb(regs, v, s, d) do { optflag_addl(regs, (v), (s) << 24, (d) << 24); v = v >> 24; } while (0)

/*
 * Subtraction operations
 *
 * Perform v = d - s and set ZNCV accordingly
 */
#define optflag_subl(regs, v, s, d)				\
    do {						\
	asm (						\
		"subfco. %1, %2, %3	\n\t"		\
		"mcrxr  cr2		\n\t"		\
		"mfcr   %0		\n\t"		\
		"xoris  %0,%0,32	\n\t"		\
							\
		: "=r" ((regs)->ccrflags.cznv),			\
		  "=r" (v)				\
		:  "r" (s),				\
		   "r" (d)				\
		: "cr0", "cr2"  DEP_XER			\
	);						\
	COPY_CARRY(&(regs)->ccrflags);			\
    } while (0)

#define optflag_subw(regs, v, s, d) do { optflag_subl(regs, v, (s) << 16, (d) << 16); v = v >> 16; } while (0)
#define optflag_subb(regs, v, s, d) do { optflag_subl(regs, v, (s) << 24, (d) << 24); v = v >> 24; } while (0)

#define optflag_cmpl(regs, s, d) 				\
    do { 						\
	uae_s32 tmp; 					\
	asm (						\
		"subfco. %1, %2, %3	\n\t"		\
		"mcrxr  cr2		\n\t"		\
		"mfcr   %0		\n\t"		\
		"xoris  %0,%0,32	\n\t"		\
							\
		: "=r" ((regs)->ccrflags.cznv),			\
		  "=r" (tmp)				\
		:  "r" (s),				\
		   "r" (d) 				\
		: "cr0", "cr2"  DEP_XER			\
	);						\
    } while (0)

#define optflag_cmpw(regs, s, d) optflag_cmpl(regs, (s) << 16, (d) << 16)
#define optflag_cmpb(regs, s, d) optflag_cmpl(regs, (s) << 24, (d) << 24)

#endif /* EUAE_MACHDEP_M68KOPS_H */
