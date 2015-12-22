/*
 * UAE - The Un*x Amiga Emulator
 *
 * Sound emulation stuff
 *
 * Copyright 1995, 1996, 1997 Bernd Schmidt
 */

#ifndef UAE_AUDIO_H
#define UAE_AUDIO_H

#include "uae/types.h"

#define PERIOD_MAX ULONG_MAX
#define MAX_EV ~0u

void aud0_handler (void);
void aud1_handler (void);
void aud2_handler (void);
void aud3_handler (void);

void AUDxDAT (int nr, uae_u16 value);
void AUDxDAT (int nr, uae_u16 value, uaecptr addr);
void AUDxVOL (int nr, uae_u16 value);
void AUDxPER (int nr, uae_u16 value);
void AUDxLCH (int nr, uae_u16 value);
void AUDxLCL (int nr, uae_u16 value);
void AUDxLEN (int nr, uae_u16 value);

uae_u16 audio_dmal (void);
void audio_state_machine (void);
uaecptr audio_getpt (int nr, bool reset);
int init_audio (void);
void ahi_install (void);
void audio_reset (void);
void update_audio (void);
void audio_evhandler (void);
void audio_hsync (void);
void audio_update_adkmasks (void);
void update_sound (double clk);
void update_cda_sound (double clk);
void led_filter_audio (void);
void set_audio (void);
int audio_activate (void);
void audio_deactivate (void);
void audio_vsync (void);
void audio_sampleripper(int);
void write_wavheader (struct zfile *wavfile, uae_u32 size, uae_u32 freq);

extern int sampleripper_enabled;

extern void audio_update_sndboard(unsigned int);
extern void audio_enable_sndboard(bool);
extern void audio_state_sndboard(int);
extern void audio_state_sndboard_state(int, int, unsigned int);

typedef void (*CDA_CALLBACK)(int);
extern void audio_cda_new_buffer(uae_s16 *buffer, int length, int userdata, CDA_CALLBACK next_cd_audio_buffer_callback);
extern void audio_cda_volume(int left, int right);

extern int sound_cd_volume[2];
extern int sound_paula_volume[2];

#define AUDIO_CHANNELS_PAULA 4
#define AUDIO_CHANNELS_MAX 8
#define AUDIO_CHANNEL_SNDBOARD_LEFT 4
#define AUDIO_CHANNEL_SNDBOARD_RIGHT 5
#define AUDIO_CHANNEL_CDA_LEFT 6
#define AUDIO_CHANNEL_CDA_RIGHT 7

enum {
	SND_MONO,
	SND_STEREO,
	SND_4CH_CLONEDSTEREO,
	SND_4CH,
	SND_6CH_CLONEDSTEREO,
	SND_6CH,
	SND_NONE
};

static inline int get_audio_stereomode (int channels)
{
	switch (channels)
	{
	case 1:
		return SND_MONO;
	case 2:
		return SND_STEREO;
	case 4:
		return SND_4CH;
	case 6:
		return SND_6CH;
	}
	return SND_STEREO;
}

STATIC_INLINE int get_audio_nativechannels (int stereomode)
{
	int ch[] = { 1, 2, 4, 4, 6, 6, 0 };
	return ch[stereomode];
}

STATIC_INLINE int get_audio_amigachannels (int stereomode)
{
	int ch[] = { 1, 2, 2, 4, 2, 4, 0 };
	return ch[stereomode];
}

STATIC_INLINE int get_audio_ismono (int stereomode)
{
	return stereomode == 0;
}

#define SOUND_MAX_DELAY_BUFFER 1024
#define SOUND_MAX_LOG_DELAY 10
#define MIXED_STEREO_MAX 16
#define MIXED_STEREO_SCALE 32

#endif /* UAE_AUDIO_H */
