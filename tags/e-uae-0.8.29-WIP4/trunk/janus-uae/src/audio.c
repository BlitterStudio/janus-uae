 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Paula audio emulation
  *
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  * Copyright 1996 Marcus Sundberg
  * Copyright 1996 Manfred Thole
  * Copyright 2006 Toni Wilen
  *
  * new filter algorithm and anti&sinc interpolators by Antti S. Lankila
  *
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "custom_private.h"
#include "newcpu.h"
#include "gensound.h"
#include "sounddep/sound.h"
#include "events.h"
#include "audio.h"
#include "savestate.h"
#include "driveclick.h"
#ifdef AVIOUTPUT
# include "avioutput.h"
#endif
#include "sinctable.h"
#include "gui.h" /* for gui_ledstate */

#define MAX_EV ~0ul
//#define DEBUG_AUDIO
#define DEBUG_CHANNEL_MASK 15

int audio_channel_mask = 15;

static int debugchannel (unsigned int ch)
{
    if ((1 << ch) & DEBUG_CHANNEL_MASK) return 1;
    return 0;
}

/* periods less than this value are replaced by this value. */
#define MIN_ALLOWED_PERIOD 16
/* reserve ~20 extra slots in sinc queue for cpu volume or some such updates
 * even at maximum period. This avoids sinc queue overflow on games like
 * battle squadron that write these low period values and do cpu-based
 * updates on paula registers, probably volume. */
#define NUMBER_OF_CPU_UPDATES_ALLOWED 20

#define SINC_QUEUE_LENGTH (SINC_QUEUE_MAX_AGE / MIN_ALLOWED_PERIOD + NUMBER_OF_CPU_UPDATES_ALLOWED)

typedef struct {
    int age;
    int output;
} sinc_queue_t;

struct audio_channel_data {
    unsigned long adk_mask;
    unsigned long evtime;
    uae_u8 dmaen, intreq2;
    uaecptr lc, pt;
    int current_sample, last_sample;
#ifndef MULTIPLICATION_PROFITABLE
    int *voltbl;
#endif
    int state;
    unsigned long per;
    int vol;
    int len, wlen;
    uae_u16 dat, dat2;
    int request_word, request_word_skip;
    int sinc_output_state;
    sinc_queue_t sinc_queue[SINC_QUEUE_LENGTH];
    int sinc_queue_length;
};

STATIC_INLINE unsigned int current_hpos (void)
{
    return (get_cycles () - eventtab[ev_hsync].oldcycles) / CYCLE_UNIT;
}

static struct audio_channel_data audio_channel[4];
int sound_available = 0;
#ifndef MULTIPLICATION_PROFITABLE
static int sound_table[64][256];
#endif
void (*sample_handler) (void);
static void (*sample_prehandler) (unsigned long best_evtime);

static unsigned long scaled_sample_evtime;
static unsigned long last_cycles, next_sample_evtime;

unsigned int obtainedfreq;


#ifndef MULTIPLICATION_PROFITABLE
void init_sound_table16 (void)
{
    int i,j;

    for (i = 0; i < 256; i++)
	for (j = 0; j < 64; j++)
	    sound_table[j][i] = j * (uae_s8)i * (currprefs.sound_stereo ? 2 : 1);
}

#ifdef HAVE_8BIT_AUDIO_SUPPORT
void init_sound_table8 (void)
{
    int i,j;

    for (i = 0; i < 256; i++)
	for (j = 0; j < 64; j++)
	    sound_table[j][i] = (j * (uae_s8)i * (currprefs.sound_stereo ? 2 : 1)) / 256;
}
#endif
#endif

#ifdef MULTIPLICATION_PROFITABLE
typedef uae_s8 sample8_t;
#define DO_CHANNEL_1(v, c) do { (v) *= audio_channel[c].vol; } while (0)
#define SBASEVAL8(logn) ((logn) == 1 ? SOUND8_BASE_VAL << 7 : SOUND8_BASE_VAL << 8)
#define SBASEVAL16(logn) ((logn) == 1 ? SOUND16_BASE_VAL >> 1 : SOUND16_BASE_VAL)
#define FINISH_DATA(data,b,logn) do { if (14 - (b) + (logn) > 0) (data) >>= 14 - (b) + (logn); else (data) <<= (b) - 14 - (logn); } while (0);
#else
typedef uae_u8 sample8_t;
#define DO_CHANNEL_1(v, c) do { (v) = audio_channel[c].voltbl[(v)]; } while (0)
#define SBASEVAL8(logn) SOUND8_BASE_VAL
#define SBASEVAL16(logn) SOUND16_BASE_VAL
#define FINISH_DATA(data,b,logn)
#endif

/* Always put the right word before the left word.  */
#define MAX_DELAY_BUFFER 1024
static uae_u32 right_word_saved[MAX_DELAY_BUFFER];
static uae_u32 left_word_saved[MAX_DELAY_BUFFER];
static int saved_ptr;

#define MIXED_STEREO_MAX 32
static int mixed_on, mixed_stereo_size, mixed_mul1, mixed_mul2;

#ifdef HAVE_STEREO_SUPPORT
STATIC_INLINE void put_sound_word_right (uae_u32 w)
{
    if (mixed_on) {
	right_word_saved[saved_ptr] = w;
	return;
    }
    PUT_SOUND_WORD_RIGHT (w);
}

STATIC_INLINE void put_sound_word_left (uae_u32	w)
{
    if (mixed_on) {
	uae_u32 rold, lold, rnew, lnew, tmp;

	left_word_saved[saved_ptr] = w;
	lnew = w - SOUND16_BASE_VAL;
	rnew = right_word_saved[saved_ptr] - SOUND16_BASE_VAL;

	saved_ptr = (saved_ptr + 1) & mixed_stereo_size;

	lold = left_word_saved[saved_ptr] - SOUND16_BASE_VAL;
	tmp = (rnew * mixed_mul1 + lold * mixed_mul2) / MIXED_STEREO_MAX;
	tmp += SOUND16_BASE_VAL;
	PUT_SOUND_WORD_RIGHT (tmp);

	rold = right_word_saved[saved_ptr] - SOUND16_BASE_VAL;
	w = (lnew * mixed_mul1 + rold * mixed_mul2) / MIXED_STEREO_MAX;
    }
    PUT_SOUND_WORD_LEFT (w);
}
#endif

#define DO_CHANNEL(v, c) do { (v) &= audio_channel[c].adk_mask; data += v; } while (0);



static void sinc_prehandler (unsigned long best_evtime)
{
    int i, j, output;
    struct audio_channel_data *acd;

    for (i = 0; i < 4; i++) {
	acd = &audio_channel[i];
	output = (acd->current_sample * acd->vol) & acd->adk_mask;

	/* age the sinc queue and truncate it when necessary */
	for (j = 0; j < acd->sinc_queue_length; j += 1) {
	    acd->sinc_queue[j].age += best_evtime;
	    if (acd->sinc_queue[j].age >= SINC_QUEUE_MAX_AGE) {
		acd->sinc_queue_length = j;
		break;
	    }
	}
	/* if output state changes, record the state change and also
	 * write data into sinc queue for mixing in the BLEP */
	if (acd->sinc_output_state != output) {
	    if (acd->sinc_queue_length > SINC_QUEUE_LENGTH - 1) {
		write_log ("warning: sinc queue truncated. Last age: %d.\n",
			   acd->sinc_queue[SINC_QUEUE_LENGTH-1].age);
		acd->sinc_queue_length = SINC_QUEUE_LENGTH - 1;
	    }
	    /* make room for new and add the new value */
	    memmove (&acd->sinc_queue[1], &acd->sinc_queue[0],
	             sizeof(acd->sinc_queue[0]) * acd->sinc_queue_length);
	    acd->sinc_queue_length += 1;
	    acd->sinc_queue[0].age = best_evtime;
	    acd->sinc_queue[0].output = output - acd->sinc_output_state;
	    acd->sinc_output_state = output;
	}
    }
}


/* this interpolator performs BLEP mixing (bleps are shaped like integrated sinc
 * functions) with a type of BLEP that matches the filtering configuration. */
STATIC_INLINE void samplexx_sinc_handler (int *datasp)
{
    int i, n;
    int const *winsinc;

#if 1
    /* Amiga 500 filter model is default for now. Put n=2 for A1200. */
    n = 0;
    if (gui_ledstate & 1) /* power led */
	n += 1;
#else
    if (sound_use_filter_sinc) {
	n = (sound_use_filter_sinc == FILTER_MODEL_A500) ? 0 : 2;
	if (led_filter_on)
	    n += 1;
    } else {
	n = 4;
    }
#endif
    winsinc = winsinc_integral[n];

    for (i = 0; i < 4; i += 1) {
        int j, v;
        struct audio_channel_data *acd = &audio_channel[i];
        /* The sum rings with harmonic components up to infinity... */
	int sum = acd->sinc_output_state << 17;
        /* ...but we cancel them through mixing in BLEPs instead */
        for (j = 0; j < acd->sinc_queue_length; j += 1)
            sum -= winsinc[acd->sinc_queue[j].age] * acd->sinc_queue[j].output;
        v = sum >> 17;
	if (v > 32767)
	    v = 32767;
	else if (v < -32768)
	    v = -32768;
	datasp[i] = v;
    }
}

static void sample16i_sinc_handler (void)
{
    int datas[4], data1;

    samplexx_sinc_handler (datas);
    data1 = datas[0] + datas[3] + datas[1] + datas[2];
    FINISH_DATA (data1, 16, 2);
    PUT_SOUND_WORD_MONO (data1);
    check_sound_buffers ();
}

void sample16_handler (void)
{
    uae_u32 data0 = audio_channel[0].current_sample;
    uae_u32 data1 = audio_channel[1].current_sample;
    uae_u32 data2 = audio_channel[2].current_sample;
    uae_u32 data3 = audio_channel[3].current_sample;
    DO_CHANNEL_1 (data0, 0);
    DO_CHANNEL_1 (data1, 1);
    DO_CHANNEL_1 (data2, 2);
    DO_CHANNEL_1 (data3, 3);
    data0 &= audio_channel[0].adk_mask;
    data1 &= audio_channel[1].adk_mask;
    data2 &= audio_channel[2].adk_mask;
    data3 &= audio_channel[3].adk_mask;
    data0 += data1;
    data0 += data2;
    data0 += data3;
    {
	uae_u32 data = SBASEVAL16(2) + data0;
	FINISH_DATA (data, 16, 2);
	PUT_SOUND_WORD_MONO (data);
    }
    check_sound_buffers ();
}

static void sample16i_rh_handler (void)
{
    unsigned long delta, ratio;

    uae_u32 data0 = audio_channel[0].current_sample;
    uae_u32 data1 = audio_channel[1].current_sample;
    uae_u32 data2 = audio_channel[2].current_sample;
    uae_u32 data3 = audio_channel[3].current_sample;
    uae_u32 data0p = audio_channel[0].last_sample;
    uae_u32 data1p = audio_channel[1].last_sample;
    uae_u32 data2p = audio_channel[2].last_sample;
    uae_u32 data3p = audio_channel[3].last_sample;
    DO_CHANNEL_1 (data0, 0);
    DO_CHANNEL_1 (data1, 1);
    DO_CHANNEL_1 (data2, 2);
    DO_CHANNEL_1 (data3, 3);
    DO_CHANNEL_1 (data0p, 0);
    DO_CHANNEL_1 (data1p, 1);
    DO_CHANNEL_1 (data2p, 2);
    DO_CHANNEL_1 (data3p, 3);

    data0 &= audio_channel[0].adk_mask;
    data0p &= audio_channel[0].adk_mask;
    data1 &= audio_channel[1].adk_mask;
    data1p &= audio_channel[1].adk_mask;
    data2 &= audio_channel[2].adk_mask;
    data2p &= audio_channel[2].adk_mask;
    data3 &= audio_channel[3].adk_mask;
    data3p &= audio_channel[3].adk_mask;

    /* linear interpolation and summing up... */
    delta = audio_channel[0].per;
    ratio = ((audio_channel[0].evtime % delta) << 8) / delta;
    data0 = (data0 * (256 - ratio) + data0p * ratio) >> 8;
    delta = audio_channel[1].per;
    ratio = ((audio_channel[1].evtime % delta) << 8) / delta;
    data0 += (data1 * (256 - ratio) + data1p * ratio) >> 8;
    delta = audio_channel[2].per;
    ratio = ((audio_channel[2].evtime % delta) << 8) / delta;
    data0 += (data2 * (256 - ratio) + data2p * ratio) >> 8;
    delta = audio_channel[3].per;
    ratio = ((audio_channel[3].evtime % delta) << 8) / delta;
    data0 += (data3 * (256 - ratio) + data3p * ratio) >> 8;
    {
	uae_u32 data = SBASEVAL16(2) + data0;
	FINISH_DATA (data, 16, 2);
	PUT_SOUND_WORD_MONO (data);
    }
    check_sound_buffers ();
}

static void sample16i_crux_handler (void)
{
    uae_u32 data0 = audio_channel[0].current_sample;
    uae_u32 data1 = audio_channel[1].current_sample;
    uae_u32 data2 = audio_channel[2].current_sample;
    uae_u32 data3 = audio_channel[3].current_sample;
    uae_u32 data0p = audio_channel[0].last_sample;
    uae_u32 data1p = audio_channel[1].last_sample;
    uae_u32 data2p = audio_channel[2].last_sample;
    uae_u32 data3p = audio_channel[3].last_sample;
    DO_CHANNEL_1 (data0, 0);
    DO_CHANNEL_1 (data1, 1);
    DO_CHANNEL_1 (data2, 2);
    DO_CHANNEL_1 (data3, 3);
    DO_CHANNEL_1 (data0p, 0);
    DO_CHANNEL_1 (data1p, 1);
    DO_CHANNEL_1 (data2p, 2);
    DO_CHANNEL_1 (data3p, 3);

    data0 &= audio_channel[0].adk_mask;
    data0p &= audio_channel[0].adk_mask;
    data1 &= audio_channel[1].adk_mask;
    data1p &= audio_channel[1].adk_mask;
    data2 &= audio_channel[2].adk_mask;
    data2p &= audio_channel[2].adk_mask;
    data3 &= audio_channel[3].adk_mask;
    data3p &= audio_channel[3].adk_mask;

    {
	struct audio_channel_data *cdp;
	unsigned long ratio, ratio1;
#define INTERVAL (scaled_sample_evtime * 3)
	cdp = audio_channel + 0;
	ratio1 = cdp->per - cdp->evtime;
	ratio = (ratio1 << 12) / INTERVAL;
	if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
	data0 = (data0 * ratio + data0p * (4096 - ratio)) >> 12;

	cdp = audio_channel + 1;
	ratio1 = cdp->per - cdp->evtime;
	ratio = (ratio1 << 12) / INTERVAL;
	if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
	data1 = (data1 * ratio + data1p * (4096 - ratio)) >> 12;

	cdp = audio_channel + 2;
	ratio1 = cdp->per - cdp->evtime;
	ratio = (ratio1 << 12) / INTERVAL;
	if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
	data2 = (data2 * ratio + data2p * (4096 - ratio)) >> 12;

	cdp = audio_channel + 3;
	ratio1 = cdp->per - cdp->evtime;
	ratio = (ratio1 << 12) / INTERVAL;
	if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
	data3 = (data3 * ratio + data3p * (4096 - ratio)) >> 12;
    }
    data1 += data2;
    data0 += data3;
    data0 += data1;
    {
	uae_u32 data = SBASEVAL16(2) + data0;
	FINISH_DATA (data, 16, 2);
	PUT_SOUND_WORD_MONO (data);
    }
    check_sound_buffers ();
}

#ifdef HAVE_8BIT_AUDIO_SUPPORT
void sample8_handler (void)
{
    uae_u32 data0 = audio_channel[0].current_sample;
    uae_u32 data1 = audio_channel[1].current_sample;
    uae_u32 data2 = audio_channel[2].current_sample;
    uae_u32 data3 = audio_channel[3].current_sample;
    DO_CHANNEL_1 (data0, 0);
    DO_CHANNEL_1 (data1, 1);
    DO_CHANNEL_1 (data2, 2);
    DO_CHANNEL_1 (data3, 3);
    data0 &= audio_channel[0].adk_mask;
    data1 &= audio_channel[1].adk_mask;
    data2 &= audio_channel[2].adk_mask;
    data3 &= audio_channel[3].adk_mask;
    data0 += data1;
    data0 += data2;
    data0 += data3;
    {
        uae_u32 data = SBASEVAL8(2) + data0;
        FINISH_DATA (data, 8, 2);
        PUT_SOUND_BYTE (data);
    }
    check_sound_buffers ();
}
#endif

#ifdef HAVE_STEREO_SUPPORT
void sample16ss_handler (void)
{
    uae_u32 data0 = audio_channel[0].current_sample;
    uae_u32 data1 = audio_channel[1].current_sample;
    uae_u32 data2 = audio_channel[2].current_sample;
    uae_u32 data3 = audio_channel[3].current_sample;
    DO_CHANNEL_1 (data0, 0);
    DO_CHANNEL_1 (data1, 1);
    DO_CHANNEL_1 (data2, 2);
    DO_CHANNEL_1 (data3, 3);

    data0 &= audio_channel[0].adk_mask;
    data1 &= audio_channel[1].adk_mask;
    data2 &= audio_channel[2].adk_mask;
    data3 &= audio_channel[3].adk_mask;

    PUT_SOUND_WORD (data0 << 2);
    PUT_SOUND_WORD (data1 << 2);
    PUT_SOUND_WORD (data3 << 2);
    PUT_SOUND_WORD (data2 << 2);

    check_sound_buffers ();
}

static void sample16si_sinc_handler (void)
{
    int datas[4], data1, data2;

    samplexx_sinc_handler (datas);
    data1 = datas[0] + datas[3];
    data2 = datas[1] + datas[2];
    FINISH_DATA (data1, 16, 1);
    put_sound_word_left (data1);
    FINISH_DATA (data2, 16, 1);
    put_sound_word_right (data2);
    check_sound_buffers ();
}

void sample16s_handler (void)
{
    uae_u32 data0 = audio_channel[0].current_sample;
    uae_u32 data1 = audio_channel[1].current_sample;
    uae_u32 data2 = audio_channel[2].current_sample;
    uae_u32 data3 = audio_channel[3].current_sample;
    DO_CHANNEL_1 (data0, 0);
    DO_CHANNEL_1 (data1, 1);
    DO_CHANNEL_1 (data2, 2);
    DO_CHANNEL_1 (data3, 3);

    data0 &= audio_channel[0].adk_mask;
    data1 &= audio_channel[1].adk_mask;
    data2 &= audio_channel[2].adk_mask;
    data3 &= audio_channel[3].adk_mask;

    data0 += data3;
    {
	uae_u32 data = SBASEVAL16(1) + data0;
	FINISH_DATA (data, 16, 1);
	put_sound_word_left (data);
    }

    data1 += data2;
    {
	uae_u32 data = SBASEVAL16(1) + data1;
	FINISH_DATA (data, 16, 1);
	put_sound_word_right (data);
    }

    check_sound_buffers ();
}

static void sample16si_crux_handler (void)
{
    uae_u32 data0 = audio_channel[0].current_sample;
    uae_u32 data1 = audio_channel[1].current_sample;
    uae_u32 data2 = audio_channel[2].current_sample;
    uae_u32 data3 = audio_channel[3].current_sample;
    uae_u32 data0p = audio_channel[0].last_sample;
    uae_u32 data1p = audio_channel[1].last_sample;
    uae_u32 data2p = audio_channel[2].last_sample;
    uae_u32 data3p = audio_channel[3].last_sample;

    DO_CHANNEL_1 (data0, 0);
    DO_CHANNEL_1 (data1, 1);
    DO_CHANNEL_1 (data2, 2);
    DO_CHANNEL_1 (data3, 3);
    DO_CHANNEL_1 (data0p, 0);
    DO_CHANNEL_1 (data1p, 1);
    DO_CHANNEL_1 (data2p, 2);
    DO_CHANNEL_1 (data3p, 3);

    data0 &= audio_channel[0].adk_mask;
    data0p &= audio_channel[0].adk_mask;
    data1 &= audio_channel[1].adk_mask;
    data1p &= audio_channel[1].adk_mask;
    data2 &= audio_channel[2].adk_mask;
    data2p &= audio_channel[2].adk_mask;
    data3 &= audio_channel[3].adk_mask;
    data3p &= audio_channel[3].adk_mask;

    {
	struct audio_channel_data *cdp;
	unsigned long ratio, ratio1;
#define INTERVAL (scaled_sample_evtime * 3)
	cdp = audio_channel + 0;
	ratio1 = cdp->per - cdp->evtime;
	ratio = (ratio1 << 12) / INTERVAL;
	if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
	data0 = (data0 * ratio + data0p * (4096 - ratio)) >> 12;

	cdp = audio_channel + 1;
	ratio1 = cdp->per - cdp->evtime;
	ratio = (ratio1 << 12) / INTERVAL;
	if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
	data1 = (data1 * ratio + data1p * (4096 - ratio)) >> 12;

	cdp = audio_channel + 2;
	ratio1 = cdp->per - cdp->evtime;
	ratio = (ratio1 << 12) / INTERVAL;
	if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
	data2 = (data2 * ratio + data2p * (4096 - ratio)) >> 12;

	cdp = audio_channel + 3;
	ratio1 = cdp->per - cdp->evtime;
	ratio = (ratio1 << 12) / INTERVAL;
	if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
	data3 = (data3 * ratio + data3p * (4096 - ratio)) >> 12;
    }
    data1 += data2;
    data0 += data3;
    {
	uae_u32 data = SBASEVAL16(1) + data0;
	FINISH_DATA (data, 16, 1);
	put_sound_word_left (data);
    }

    {
	uae_u32 data = SBASEVAL16(1) + data1;
	FINISH_DATA (data, 16, 1);
	put_sound_word_right (data);
    }
    check_sound_buffers ();
}

static void sample16si_rh_handler (void)
{
    unsigned long delta, ratio;

    uae_u32 data0 = audio_channel[0].current_sample;
    uae_u32 data1 = audio_channel[1].current_sample;
    uae_u32 data2 = audio_channel[2].current_sample;
    uae_u32 data3 = audio_channel[3].current_sample;
    uae_u32 data0p = audio_channel[0].last_sample;
    uae_u32 data1p = audio_channel[1].last_sample;
    uae_u32 data2p = audio_channel[2].last_sample;
    uae_u32 data3p = audio_channel[3].last_sample;

    DO_CHANNEL_1 (data0, 0);
    DO_CHANNEL_1 (data1, 1);
    DO_CHANNEL_1 (data2, 2);
    DO_CHANNEL_1 (data3, 3);
    DO_CHANNEL_1 (data0p, 0);
    DO_CHANNEL_1 (data1p, 1);
    DO_CHANNEL_1 (data2p, 2);
    DO_CHANNEL_1 (data3p, 3);

    data0 &= audio_channel[0].adk_mask;
    data0p &= audio_channel[0].adk_mask;
    data1 &= audio_channel[1].adk_mask;
    data1p &= audio_channel[1].adk_mask;
    data2 &= audio_channel[2].adk_mask;
    data2p &= audio_channel[2].adk_mask;
    data3 &= audio_channel[3].adk_mask;
    data3p &= audio_channel[3].adk_mask;

    /* linear interpolation and summing up... */
    delta = audio_channel[0].per;
    ratio = ((audio_channel[0].evtime % delta) << 8) / delta;
    data0 = (data0 * (256 - ratio) + data0p * ratio) >> 8;
    delta = audio_channel[1].per;
    ratio = ((audio_channel[1].evtime % delta) << 8) / delta;
    data1 = (data1 * (256 - ratio) + data1p * ratio) >> 8;
    delta = audio_channel[2].per;
    ratio = ((audio_channel[2].evtime % delta) << 8) / delta;
    data1 += (data2 * (256 - ratio) + data2p * ratio) >> 8;
    delta = audio_channel[3].per;
    ratio = ((audio_channel[3].evtime % delta) << 8) / delta;
    data0 += (data3 * (256 - ratio) + data3p * ratio) >> 8;
    {
	uae_u32 data = SBASEVAL16(1) + data0;
	FINISH_DATA (data, 16, 1);
	put_sound_word_left (data);
    }

    {
	uae_u32 data = SBASEVAL16(1) + data1;
	FINISH_DATA (data, 16, 1);
	put_sound_word_right (data);
    }
    check_sound_buffers ();
}

#ifdef HAVE_8BIT_AUDIO_SUPPORT
void sample8s_handler (void)
{
    uae_u32 data0 = audio_channel[0].current_sample;
    uae_u32 data1 = audio_channel[1].current_sample;
    uae_u32 data2 = audio_channel[2].current_sample;
    uae_u32 data3 = audio_channel[3].current_sample;
    DO_CHANNEL_1 (data0, 0);
    DO_CHANNEL_1 (data1, 1);
    DO_CHANNEL_1 (data2, 2);
    DO_CHANNEL_1 (data3, 3);

    data0 &= audio_channel[0].adk_mask;
    data1 &= audio_channel[1].adk_mask;
    data2 &= audio_channel[2].adk_mask;
    data3 &= audio_channel[3].adk_mask;

    data0 += data3;
    {
        uae_u32 data = SBASEVAL8(1) + data0;
        FINISH_DATA (data, 8, 1);
        PUT_SOUND_BYTE_RIGHT (data);
    }
    data1 += data2;
    {
        uae_u32 data = SBASEVAL8(1) + data1;
        FINISH_DATA (data, 8, 1);
        PUT_SOUND_BYTE_LEFT (data);
    }
    check_sound_buffers ();
}
#endif
#else
#ifdef HAVE_8BIT_AUDIO_SUPPORT
void sample8s_handler (void)
{
    sample8_handler();
}
#endif

void sample16s_handler (void)
{
    sample16_handler ();
}
static void sample16si_crux_handler (void)
{
    sample16i_crux_handler ();
}
static void sample16si_rh_handler (void)
{
    sample16i_rh_handler ();
}
static void sample16si_sinc_handler (void)
{
    sample16i_sinc_handler ();
}
#endif

#ifdef HAVE_ULAW_AUDIO_SUPPORT
static uae_u8 int2ulaw (int ch)
{
    int mask;

    if (ch < 0) {
	ch = -ch;
	mask = 0x7f;
    } else
	mask = 0xff;

    if (ch < 32)
	ch = 0xF0 | (15 - (ch / 2));
    else if (ch < 96)
	ch = 0xE0 | (15 - (ch - 32) / 4);
    else if (ch < 224)
	ch = 0xD0 | (15 - (ch - 96) / 8);
    else if (ch < 480)
	ch = 0xC0 | (15 - (ch - 224) / 16);
    else if (ch < 992 )
	ch = 0xB0 | (15 - (ch - 480) / 32);
    else if (ch < 2016)
	ch = 0xA0 | (15 - (ch - 992) / 64);
    else if (ch < 4064)
	ch = 0x90 | (15 - (ch - 2016) / 128);
    else if (ch < 8160)
	ch = 0x80 | (15 - (ch - 4064) / 256);
    else
	ch = 0x80;

    return (uae_u8)(mask & ch);
}

void sample_ulaw_handler (void)
{
    unsigned int nr;
    uae_u32 data = 0;

    for (nr = 0; nr < 4; nr++) {
	if (!(adkcon & (0x11 << nr))) {
	    uae_u32 d = audio_channel[nr].current_sample;
	    DO_CHANNEL_1 (d, nr);
	    data += d;
	}
    }
    PUT_SOUND_BYTE (int2ulaw (data));
    check_sound_buffers ();
}
#endif

void switch_audio_interpol (void)
{
#if defined HAVE_8BIT_AUDIO_SUPPORT || defined HAVE_ULAW_AUDIO_SUPPORT
    if (currprefs.sound_bits == 8)
	/* only supported for 16-bit audio */
	return;
#endif

    if (currprefs.sound_interpol == 0) {
	changed_prefs.sound_interpol = 1;
	write_log ("Interpol on: rh\n");
    } else if (currprefs.sound_interpol == 1) {
	changed_prefs.sound_interpol = 2;
	write_log ("Interpol on: crux\n");
    } else if (currprefs.sound_interpol == 2) {
	changed_prefs.sound_interpol = 3;
	write_log ("Interpol on: sinc\n");
    } else {
	changed_prefs.sound_interpol = 0;
	write_log ("Interpol off\n");
    }
    return;
}

void schedule_audio (void)
{
    unsigned long best = MAX_EV;
    int i;

    eventtab[ev_audio].active = 0;
    eventtab[ev_audio].oldcycles = get_cycles ();
    for (i = 0; i < 4; i++) {
	struct audio_channel_data *cdp = audio_channel + i;
	if (cdp->evtime != MAX_EV) {
	    if (best > cdp->evtime) {
		best = cdp->evtime;
		eventtab[ev_audio].active = 1;
	    }
	}
    }
    eventtab[ev_audio].evtime = get_cycles () + best;
}

/*
 * TODO: This function has been moved here from the audio back-end layer
 * since it was common to all.
 * Needs further cleaning up and a better name - or replacing entirely.
 */
void update_sound (unsigned int freq)
{
    if (obtainedfreq) {
	if (is_vsync ()) {
	    if (currprefs.ntscmode)
		scaled_sample_evtime = (unsigned long)(MAXHPOS_NTSC * MAXVPOS_NTSC * freq * CYCLE_UNIT + obtainedfreq - 1) / obtainedfreq;
	    else
		scaled_sample_evtime = (unsigned long)(MAXHPOS_PAL * MAXVPOS_PAL * freq * CYCLE_UNIT + obtainedfreq - 1) / obtainedfreq;
	} else {
	    scaled_sample_evtime = (unsigned long)(312.0 * 50 * CYCLE_UNIT / (obtainedfreq  / 227.0));
	}
    }
}

static int isirq (unsigned int nr)
{
    return INTREQR () & (0x80 << nr);
}

static void setirq (unsigned int nr, unsigned int debug)
{
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("SETIRQ %d %08.8X (%d)\n", nr, m68k_getpc (&regs), debug);
#endif
    INTREQ (0x8000 | (0x80 << nr));
}

static void newsample (unsigned int nr, sample8_t sample)
{
    struct audio_channel_data *cdp = audio_channel + nr;
#ifdef DEBUG_AUDIO
    if (!debugchannel (nr)) sample = 0;
#endif
    if (!(audio_channel_mask & (1 << nr)))
	sample = 0;
    cdp->last_sample = cdp->current_sample;
    cdp->current_sample = sample;
}

static void state23 (struct audio_channel_data *cdp)
{
    if (!cdp->dmaen)
	return;
    if (cdp->request_word >= 0)
	return;
    cdp->request_word = 0;
    if (cdp->wlen == 1) {
	cdp->wlen = cdp->len;
	cdp->pt = cdp->lc;
	cdp->intreq2 = 1;

#ifdef DEBUG_AUDIO
	if (debugchannel (cdp - audio_channel))
	    write_log ("Channel %d looped, LC=%08.8X LEN=%d\n", cdp - audio_channel, cdp->pt, cdp->wlen);
#endif
    } else {
	cdp->wlen = (cdp->wlen - 1) & 0xFFFF;
    }
}

static void audio_handler (unsigned int nr, int timed)
{
    struct audio_channel_data *cdp = audio_channel + nr;

    unsigned int audav = adkcon & (0x01 << nr);
    unsigned int audap = adkcon & (0x10 << nr);
    unsigned int napnav = (!audav && !audap) || audav;
    unsigned long evtime = cdp->evtime;

    cdp->evtime = MAX_EV;
    switch (cdp->state)
    {
	case 0:
	    cdp->request_word = 0;
	    cdp->request_word_skip = 0;
	    cdp->intreq2 = 0;
	    if (cdp->dmaen) {
		cdp->state = 1;
		cdp->wlen = cdp->len;
		/* there are too many stupid sound routines that fail on "too" fast cpus.. */
		if (currprefs.cpu_level > 1)
		    cdp->pt = cdp->lc;
#ifdef DEBUG_AUDIO
		if (debugchannel (nr))
		    write_log ("%d:0>1: LEN=%d\n", nr, cdp->wlen);
#endif
		audio_handler (nr, timed);
		return;
	    } else if (!cdp->dmaen && cdp->request_word < 0 && !isirq (nr)) {
		cdp->evtime = 0;
		cdp->state = 2;
		setirq (nr, 0);
		audio_handler (nr, timed);
		return;
	    }
	return;

	case 1:
	    if (!cdp->dmaen) {
		cdp->state = 0;
		return;
	    }
	    cdp->state = 5;
	    if (cdp->wlen != 1)
		cdp->wlen = (cdp->wlen - 1) & 0xFFFF;
	    cdp->request_word = 2;
	    if (current_hpos () > maxhpos - 20)
		cdp->request_word_skip = 1;
	return;

	case 5:
	    if (!cdp->request_word) {
		cdp->request_word = 2;
		return;
	    }
	    setirq (nr, 1);
	    if (!cdp->dmaen) {
		cdp->state = 0;
		cdp->request_word = 0;
		return;
	    }
	    cdp->state = 2;
	    cdp->request_word = 3;
	    if (napnav)
		cdp->request_word = 2;
	    cdp->dat = cdp->dat2;
	return;

	case 2:
	    if (currprefs.produce_sound == 0)
		cdp->per = PERIOD_MAX;

	    if (!cdp->dmaen && isirq (nr) && ((cdp->per <= 30 ) || (evtime == 0 || evtime == MAX_EV || evtime == cdp->per))) {
		cdp->state = 0;
		cdp->evtime = MAX_EV;
		cdp->request_word = 0;
		return;
	    }

	    state23 (cdp);
	    cdp->state = 3;
	    cdp->evtime = cdp->per;
	    newsample (nr, (cdp->dat >> 8) & 0xff);
	    cdp->dat <<= 8;
	    /* Period attachment? */
	    if (audap) {
		if (cdp->intreq2 && cdp->dmaen)
		    setirq (nr, 2);
		cdp->intreq2 = 0;
		cdp->request_word = 1;
		cdp->dat = cdp->dat2;
		if (nr < 3) {
		    if (cdp->dat == 0)
			(cdp+1)->per = PERIOD_MAX;
		    else if (cdp->dat < maxhpos * CYCLE_UNIT / 2 && currprefs.produce_sound < 3)
			(cdp+1)->per = maxhpos * CYCLE_UNIT / 2;
		    else
			(cdp+1)->per = cdp->dat * CYCLE_UNIT;
		}
	    }
	return;

	case 3:
	    if (currprefs.produce_sound == 0)
		cdp->per = PERIOD_MAX;
	    state23 (cdp);
	    cdp->state = 2;
	    cdp->evtime = cdp->per;
	    newsample (nr, (cdp->dat >> 8) & 0xff);
	    cdp->dat <<= 8;
	    cdp->dat = cdp->dat2;
	    if (cdp->dmaen) {
		if (napnav)
		    cdp->request_word = 1;
		if (cdp->intreq2 && napnav)
		    setirq (nr, 3);
	    } else {
		if (napnav)
		    setirq (nr, 4);
	    }
	    cdp->intreq2 = 0;

	    /* Volume attachment? */
	    if (audav) {
		if (nr < 3) {
		    (cdp+1)->vol = cdp->dat;
#ifndef MULTIPLICATION_PROFITABLE
		    (cdp+1)->voltbl = sound_table[cdp->dat];
#endif
		}
	    }
	return;
    }
}

void audio_reset (void)
{
    int i;
    struct audio_channel_data *cdp;

#ifdef AHI
    ahi_close_sound ();
#endif
    reset_sound ();
    if (savestate_state != STATE_RESTORE) {
	for (i = 0; i < 4; i++) {
	    cdp = &audio_channel[i];
	    memset (cdp, 0, sizeof *audio_channel);
	    cdp->per = PERIOD_MAX - 1;
#ifndef MULTIPLICATION_PROFITABLE
	    cdp->voltbl = sound_table[0];
#endif
	    cdp->vol = 0;
	    cdp->evtime = MAX_EV;
	}
    } else {
	for (i = 0; i < 4; i++) {
	    cdp = &audio_channel[i];
	    cdp->dmaen = (dmacon & DMA_MASTER) && (dmacon & (1 << i));
	}
    }

#ifndef	MULTIPLICATION_PROFITABLE
    for (i = 0; i < 4; i++)
	audio_channel[i].voltbl = sound_table[audio_channel[i].vol];
#endif

    last_cycles = get_cycles ();
    next_sample_evtime = scaled_sample_evtime;
    schedule_audio ();
    events_schedule ();
}

STATIC_INLINE int sound_prefs_changed (void)
{
    return (changed_prefs.produce_sound != currprefs.produce_sound
	    || changed_prefs.sound_stereo != currprefs.sound_stereo
	    || changed_prefs.sound_stereo_separation != currprefs.sound_stereo_separation
	    || changed_prefs.sound_mixed_stereo != currprefs.sound_mixed_stereo
	    || changed_prefs.sound_latency != currprefs.sound_latency
	    || changed_prefs.sound_freq != currprefs.sound_freq
#if defined HAVE_8BIT_AUDIO_SUPPORT || defined HAVE_ULAW_AUDIO_SUPPORT
	    || changed_prefs.sound_bits != currprefs.sound_bits
#endif
	    || changed_prefs.sound_adjust != currprefs.sound_adjust
	    || changed_prefs.sound_interpol != currprefs.sound_interpol
	    || changed_prefs.sound_volume != currprefs.sound_volume);
}

void check_prefs_changed_audio (void)
{
#ifdef DRIVESOUND
    driveclick_check_prefs ();
#endif
    if (sound_available && sound_prefs_changed ()) {
	close_sound ();
#ifdef AVIOUTPUT
	AVIOutput_Restart ();
#endif

	currprefs.produce_sound = changed_prefs.produce_sound;
	currprefs.sound_stereo = changed_prefs.sound_stereo;
	currprefs.sound_stereo_separation = changed_prefs.sound_stereo_separation;
	currprefs.sound_mixed_stereo = changed_prefs.sound_mixed_stereo;
	currprefs.sound_adjust = changed_prefs.sound_adjust;
	currprefs.sound_interpol = changed_prefs.sound_interpol;
	currprefs.sound_freq = changed_prefs.sound_freq;
#if defined HAVE_8BIT_AUDIO_SUPPORT || defined HAVE_ULAW_AUDIO_SUPPORT
	currprefs.sound_bits = changed_prefs.sound_bits;
#endif
	currprefs.sound_latency = changed_prefs.sound_latency;
	currprefs.sound_volume = changed_prefs.sound_volume;
	if (currprefs.produce_sound >= 2) {
	    if (! audio_init ()) {
		if (! sound_available) {
		    write_log ("Sound is not supported.\n");
		} else {
		    write_log ("Sorry, can't initialize sound.\n");
		    currprefs.produce_sound = 0;
		    /* So we don't do this every frame */
		    changed_prefs.produce_sound = 0;
		}
	    }
	}
	last_cycles = get_cycles () - 1;
	next_sample_evtime = scaled_sample_evtime;
	compute_vsynctime ();
    }
    mixed_mul1 = MIXED_STEREO_MAX / 2 - ((currprefs.sound_stereo_separation * 3) / 2);
    mixed_mul2 = MIXED_STEREO_MAX / 2 + ((currprefs.sound_stereo_separation * 3) / 2);
    mixed_stereo_size = currprefs.sound_mixed_stereo > 0 ? (1 << (currprefs.sound_mixed_stereo - 1)) - 1 : 0;
    mixed_on = (currprefs.sound_stereo_separation > 0 || currprefs.sound_mixed_stereo > 0) ? 1 : 0;

    /* Select the right interpolation method.  */
    if (sample_handler == sample16_handler
	|| sample_handler == sample16i_crux_handler
	|| sample_handler == sample16i_rh_handler
	|| sample_handler == sample16i_sinc_handler) {
	sample_handler =   (currprefs.sound_interpol == 0 ? sample16_handler
			  : currprefs.sound_interpol == 1 ? sample16i_rh_handler
			  : currprefs.sound_interpol == 2 ? sample16i_crux_handler
			  :                                 sample16i_sinc_handler);
    } else if (sample_handler == sample16s_handler
	     || sample_handler == sample16si_crux_handler
	     || sample_handler == sample16si_rh_handler
	     || sample_handler == sample16si_sinc_handler) {
	sample_handler =   (currprefs.sound_interpol == 0 ? sample16s_handler
			  : currprefs.sound_interpol == 1 ? sample16si_rh_handler
			  : currprefs.sound_interpol == 2 ? sample16si_crux_handler
			  :                                 sample16si_sinc_handler);
    }
    sample_prehandler = NULL;
    if (sample_handler == sample16si_sinc_handler || sample_handler == sample16i_sinc_handler)
	sample_prehandler = sinc_prehandler;
    if (currprefs.produce_sound == 0) {
	eventtab[ev_audio].active = 0;
	events_schedule ();
    }
}

void update_audio (void)
{
    unsigned long int n_cycles;

    if (currprefs.produce_sound == 0 || savestate_state == STATE_RESTORE)
	return;

    n_cycles = get_cycles () - last_cycles;
    for (;;) {
	unsigned long int best_evtime = n_cycles + 1;

	if (audio_channel[0].evtime != MAX_EV && best_evtime > audio_channel[0].evtime)
	    best_evtime = audio_channel[0].evtime;
	if (audio_channel[1].evtime != MAX_EV && best_evtime > audio_channel[1].evtime)
	    best_evtime = audio_channel[1].evtime;
	if (audio_channel[2].evtime != MAX_EV && best_evtime > audio_channel[2].evtime)
	    best_evtime = audio_channel[2].evtime;
	if (audio_channel[3].evtime != MAX_EV && best_evtime > audio_channel[3].evtime)
	    best_evtime = audio_channel[3].evtime;

	if (currprefs.produce_sound > 1 && best_evtime > next_sample_evtime)
	    best_evtime = next_sample_evtime;

	if (best_evtime > n_cycles)
	    break;

	if (audio_channel[0].evtime != MAX_EV)
	    audio_channel[0].evtime -= best_evtime;
	if (audio_channel[1].evtime != MAX_EV)
	    audio_channel[1].evtime -= best_evtime;
	if (audio_channel[2].evtime != MAX_EV)
	    audio_channel[2].evtime -= best_evtime;
	if (audio_channel[3].evtime != MAX_EV)
	    audio_channel[3].evtime -= best_evtime;

	n_cycles -= best_evtime;
	if (currprefs.produce_sound > 1) {
	    next_sample_evtime -= best_evtime;
	    if (sample_prehandler)
		sample_prehandler (best_evtime / CYCLE_UNIT);
	    if (next_sample_evtime == 0) {
		next_sample_evtime = scaled_sample_evtime;
		(*sample_handler) ();
	    }
	}

	if (audio_channel[0].evtime == 0)
	    audio_handler (0, 1);
	if (audio_channel[1].evtime == 0)
	    audio_handler (1, 1);
	if (audio_channel[2].evtime == 0)
	    audio_handler (2, 1);
	if (audio_channel[3].evtime == 0)
	    audio_handler (3, 1);
    }
    last_cycles = get_cycles () - n_cycles;
}

void audio_evhandler (void)
{
    update_audio ();
    schedule_audio ();
}

#ifdef CPUEMU_6
extern uae_u8 cycle_line[];
#endif
uae_u16	dmacon;

void audio_hsync (int dmaaction)
{
    unsigned int nr, handle;

    if (currprefs.produce_sound == 0)
	return;

    update_audio ();
    handle = 0;
    /* Sound data is fetched at the beginning of each line */
    for (nr = 0; nr < 4; nr++) {
	struct audio_channel_data *cdp = audio_channel + nr;
	unsigned int chan_ena = (dmacon & DMA_MASTER) && (dmacon & (1 << nr));
	unsigned int handle2 = 0;

	if (dmaaction && cdp->request_word > 0) {

	    if (cdp->request_word_skip) {
		cdp->request_word_skip = 0;
		continue;
	    }

	    if (cdp->state == 5) {
		cdp->pt = cdp->lc;
#ifdef DEBUG_AUDIO
		if (debugchannel (nr))
		    write_log ("%d:>5: LEN=%d PT=%08.8X\n", nr, cdp->wlen, cdp->pt);
#endif
	    }
	    cdp->dat2 = chipmem_wget (cdp->pt);
	    if (cdp->request_word >= 2)
		handle2 = 1;
	    if (chan_ena) {
#ifdef CPUEMU_6
		cycle_line[13 + nr * 2] |= CYCLE_MISC;
#endif
		if (cdp->request_word == 1 || cdp->request_word == 2)
		    cdp->pt += 2;
	    }
	    cdp->request_word = -1;

	}

	if (cdp->dmaen != chan_ena) {
#ifdef DEBUG_AUDIO
	    if (debugchannel (nr))
		write_log ("AUD%dDMA %d->%d (%d) LEN=%d/%d %08.8X\n", nr, cdp->dmaen, chan_ena,
		    cdp->state, cdp->wlen, cdp->len, m68k_getpc());
#endif
	    cdp->dmaen = chan_ena;
	    if (cdp->dmaen)
		handle2 = 1;
	}
	if (handle2)
	    audio_handler (nr, 0);
	handle |= handle2;
    }
    if (handle) {
	schedule_audio ();
	events_schedule ();
    }
}

void AUDxDAT (unsigned int nr, uae_u16 v)
{
    struct audio_channel_data *cdp = audio_channel + nr;

#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dDAT: %04.4X STATE=%d IRQ=%d %08.8X\n", nr,
	    v, cdp->state, isirq(nr) ? 1 : 0, m68k_getpc (&regs));
#endif
    update_audio ();
    cdp->dat2 = v;
    cdp->request_word = -1;
    cdp->request_word_skip = 0;
    if (cdp->state == 0) {
	cdp->state = 2;
	audio_handler (nr, 0);
	schedule_audio ();
	events_schedule ();
    }
}

void AUDxLCH (unsigned int nr, uae_u16 v)
{
    update_audio ();
    audio_channel[nr].lc = (audio_channel[nr].lc & 0xffff) | ((uae_u32)v << 16);
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dLCH: %04.4X %08.8X\n", nr, v, m68k_getpc (&regs));
#endif
}

void AUDxLCL (unsigned int nr, uae_u16 v)
{
    update_audio ();
    audio_channel[nr].lc = (audio_channel[nr].lc & ~0xffff) | (v & 0xFFFE);
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dLCL: %04.4X %08.8X\n", nr, v, m68k_getpc (&regs));
#endif
}

void AUDxPER (unsigned int nr, uae_u16 v)
{
    unsigned long per = v * CYCLE_UNIT;
    update_audio ();

    if (per == 0)
	per = PERIOD_MAX - 1;

    if (per < maxhpos * CYCLE_UNIT / 2 && currprefs.produce_sound < 3)
	per = maxhpos * CYCLE_UNIT / 2;
    /* the sinc code registers paula output state changes, but has a finite
     * buffer in which to do so. Hence, we forbid very low values; this should
     * only limit the accurate rendering of supersonic sounds, which are
     * filtered away on the sinc output path anyway. */
    if (currprefs.produce_sound == 3 && sample_handler == sample16si_sinc_handler && per < MIN_ALLOWED_PERIOD * CYCLE_UNIT)
	per = MIN_ALLOWED_PERIOD * CYCLE_UNIT;

   if (audio_channel[nr].per == PERIOD_MAX - 1 && per != PERIOD_MAX - 1) {
	audio_channel[nr].evtime = CYCLE_UNIT;
	if (currprefs.produce_sound > 0) {
	    schedule_audio ();
	    events_schedule ();
	}
    }

    audio_channel[nr].per = per;
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dPER: %d %08.8X\n", nr, v, m68k_getpc (&regs));
#endif
}

void AUDxLEN (unsigned int nr, uae_u16 v)
{
    update_audio ();
    audio_channel[nr].len = v;
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dLEN: %d %08.8X\n", nr, v, m68k_getpc (&regs));
#endif
}

void AUDxVOL (unsigned int nr, uae_u16 v)
{
    unsigned int v2 = v & 64 ? 63 : v & 63;
    update_audio ();
    audio_channel[nr].vol = v2;
#ifndef MULTIPLICATION_PROFITABLE
    audio_channel[nr].voltbl = sound_table[v2];
#endif
#ifdef DEBUG_AUDIO
    if (debugchannel (nr))
	write_log ("AUD%dVOL: %d %08.8X\n", nr, v2, m68k_getpc (&regs));
#endif
}

void audio_update_irq (uae_u16 v)
{
#ifdef DEBUG_AUDIO
    uae_u16 v2 = intreq, v3 = intreq;
    unsigned int i;
    if (v & 0x8000)
	v2 |= v & 0x7FFF;
    else
	v2 &= ~v;
    v2 &= (0x80 | 0x100 | 0x200 | 0x400);
    v3 &= (0x80 | 0x100 | 0x200 | 0x400);
    for (i = 0; i < 4; i++) {
	if ((1 << i) & DEBUG_CHANNEL_MASK) {
	    uae_u16 mask = 0x80 << i;
	    if ((v2 & mask) != (v3 & mask))
		write_log ("AUD%dINTREQ %d->%d %08.8X\n", i, !!(v3 & mask), !!(v2 & mask), m68k_getpc (&regs));
	}
    }
#endif
}

void audio_update_adkmasks (void)
{
    unsigned long t = adkcon | (adkcon >> 4);
    audio_channel[0].adk_mask = (((t >> 0) & 1) - 1);
    audio_channel[1].adk_mask = (((t >> 1) & 1) - 1);
    audio_channel[2].adk_mask = (((t >> 2) & 1) - 1);
    audio_channel[3].adk_mask = (((t >> 3) & 1) - 1);
#ifdef DEBUG_AUDIO
    {
	static int prevcon = -1;
	if ((prevcon & 0xff) != (adkcon & 0xff)) {
	    write_log ("ADKCON=%02.2x %08.8X\n", adkcon & 0xff, m68k_getpc (&regs));
	    prevcon = adkcon;
	}
    }
#endif
}

int audio_setup (void)
{
    return setup_sound ();
}

int audio_init (void)
{
    int result = init_sound ();
    update_sound (vblank_hz);
    return result;
}

void audio_close (void)
{
    close_sound ();
}

void audio_pause (void)
{
    pause_sound ();
}

void audio_resume (void)
{
    resume_sound ();
}

void audio_volume (int volume)
{
    sound_volume (volume);
}

#ifdef SAVESTATE

const uae_u8 *restore_audio (unsigned int channel, const uae_u8 *src)
{
    struct audio_channel_data *acd = &audio_channel[channel];
    uae_u16 p;

    acd->state = restore_u8 ();
    acd->vol = restore_u8 ();
    acd->intreq2 = restore_u8 ();
    acd->request_word = restore_u8 ();
    acd->len = restore_u16 ();
    acd->wlen = restore_u16 ();
    p = restore_u16 ();
    acd->per = p ? (unsigned)p * CYCLE_UNIT : (unsigned)PERIOD_MAX;
    p = restore_u16 ();
    acd->lc = restore_u32 ();
    acd->pt = restore_u32 ();
    acd->evtime = restore_u32 ();
    return src;
}

#endif /* SAVESTATE */

#if defined SAVESTATE || defined DEBUGGER

uae_u8 *save_audio (unsigned int channel, uae_u32 *len, uae_u8 *dstptr)
{
    const struct audio_channel_data *acd = &audio_channel[channel];
    uae_u8 *dst, *dstbak;
    uae_u16 p;

    if (dstptr)
	dstbak = dst = dstptr;
    else
	dstbak = dst = malloc (100);

    save_u8 ((uae_u8)acd->state);
    save_u8 (acd->vol);
    save_u8 (acd->intreq2);
    save_u8 (acd->request_word);
    save_u16 (acd->len);
    save_u16 (acd->wlen);
    p = acd->per == PERIOD_MAX ? 0 : acd->per / CYCLE_UNIT;
    save_u16 (p);
    save_u16 (acd->dat2);
    save_u32 (acd->lc);
    save_u32 (acd->pt);
    save_u32 (acd->evtime);
    *len = dst - dstbak;
    return dstbak;
}

#endif /* SAVESTATE || DEBUGGER */
