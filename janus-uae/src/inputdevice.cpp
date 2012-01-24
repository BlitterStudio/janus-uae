/*
* UAE - The Un*x Amiga Emulator
*
* joystick/mouse emulation
*
* Copyright 2001-2010 Toni Wilen
*
* new fetures:
* - very configurable (and very complex to configure :)
* - supports multiple native input devices (joysticks and mice)
* - supports mapping joystick/mouse buttons to keys and vice versa
* - joystick mouse emulation (supports both ports)
* - supports parallel port joystick adapter
* - full cd32 pad support (supports both ports)
* - fully backward compatible with old joystick/mouse configuration
*
*/

//#define DONGLE_DEBUG

#ifdef __AROS__
#include <exec/types.h>
#endif

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "keyboard.h"
#include "inputdevice.h"
#include "inputrecord.h"
#include "keybuf.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "memory.h"
#include "events.h"
#include "newcpu.h"
#include "uae.h"
#include "picasso96.h"
#include "catweasel.h"
#include "debug.h"
#include "ar.h"
#include "gui.h"
#include "disk.h"
#include "audio.h"
#ifndef __AROS__
#include "sound.h"
#else
#include "od-aros/sounddep/sound.h"
#endif
#include "savestate.h"
#include "arcadia.h"
#include "zfile.h"
#include "cia.h"
#include "autoconf.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif
#include "dongle.h"
#include "cdtv.h"

extern int bootrom_header, bootrom_items;

// 01 = host events
// 02 = joystick
// 04 = cia buttons
// 16 = potgo
// 32 = vsync

int inputdevice_logging = 0;

#define IE_INVERT 0x80
#define IE_CDTV 0x100

#define INPUTEVENT_JOY1_CD32_FIRST INPUTEVENT_JOY1_CD32_PLAY
#define INPUTEVENT_JOY2_CD32_FIRST INPUTEVENT_JOY2_CD32_PLAY
#define INPUTEVENT_JOY1_CD32_LAST INPUTEVENT_JOY1_CD32_BLUE
#define INPUTEVENT_JOY2_CD32_LAST INPUTEVENT_JOY2_CD32_BLUE

/* event masks */
#define AM_KEY 1 /* keyboard allowed */
#define AM_JOY_BUT 2 /* joystick buttons allowed */
#define AM_JOY_AXIS 4 /* joystick axis allowed */
#define AM_MOUSE_BUT 8 /* mouse buttons allowed */
#define AM_MOUSE_AXIS 16 /* mouse direction allowed */
#define AM_AF 32 /* supports autofire */
#define AM_INFO 64 /* information data for gui */
#define AM_DUMMY 128 /* placeholder */
#define AM_CUSTOM 256 /* custom event */
#define AM_K (AM_KEY|AM_JOY_BUT|AM_MOUSE_BUT|AM_AF) /* generic button/switch */

#define JOYMOUSE_CDTV 8

#define DEFEVENT(A, B, C, D, E, F) {_T(#A), B, C, D, E, F },
static struct inputevent events[] = {
	{0, 0, AM_K,0,0,0},
#include "inputevents.def"
	{0, 0, 0, 0, 0, 0}
};
#undef DEFEVENT

static int sublevdir[2][MAX_INPUT_SUB_EVENT];

struct uae_input_device2 {
	uae_u32 buttonmask;
	int states[MAX_INPUT_DEVICE_EVENTS / 2];
};

static struct uae_input_device2 joysticks2[MAX_INPUT_DEVICES];
static struct uae_input_device2 mice2[MAX_INPUT_DEVICES];
static uae_u8 scancodeused[MAX_INPUT_DEVICES][256];

// fire/left mouse button pullup resistors enabled?
static bool mouse_pullup = true;

static int joymodes[MAX_JPORTS];
static int *joyinputs[MAX_JPORTS];

static int input_acquired;
static int testmode, testmode_read, testmode_toggle;
struct teststore
{
	int testmode_type;
	int testmode_num;
	int testmode_wtype;
	int testmode_wnum;
	int testmode_state;
};
#define TESTMODE_MAX 2
static int testmode_count;
static struct teststore testmode_data[TESTMODE_MAX];
static struct teststore testmode_wait[TESTMODE_MAX];

static int bouncy;
static signed long bouncy_cycles;

static int handle_input_event (int nr, int state, int max, int autofire, bool canstoprecord, bool playbackevent);


static int isdevice (struct uae_input_device *id)
{
	int i, j;
	for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (id->eventid[i][j] > 0)
				return 1;
		}
	}
	return 0;
}

int inputdevice_uaelib (const TCHAR *s, const TCHAR *parm)
{
	int i;

	for (i = 1; events[i].name; i++) {
		if (!_tcscmp (s, events[i].confname)) {
			handle_input_event (i, _tstol (parm), 1, 0, false, false);
			return 1;
		}
	}
	return 0;
}

static struct uae_input_device *joysticks;
static struct uae_input_device *mice;
static struct uae_input_device *keyboards;
static struct uae_input_device_kbr_default *keyboard_default;

#define KBR_DEFAULT_MAP_FIRST 0
#ifndef __AROS__
#define KBR_DEFAULT_MAP_LAST 5
#else
#define KBR_DEFAULT_MAP_LAST 0
#endif
#define KBR_DEFAULT_MAP_CD32_FIRST 6
#define KBR_DEFAULT_MAP_CD32_LAST 8

#define KBR_DEFAULT_MAP_NP 0
#define KBR_DEFAULT_MAP_CK 1
#define KBR_DEFAULT_MAP_SE 2
#define KBR_DEFAULT_MAP_NP3 3
#define KBR_DEFAULT_MAP_CK3 4
#define KBR_DEFAULT_MAP_SE3 5
#define KBR_DEFAULT_MAP_CD32_NP 6
#define KBR_DEFAULT_MAP_CD32_CK 7
#define KBR_DEFAULT_MAP_CD32_SE 8
#define KBR_DEFAULT_MAP_XA1 9
#define KBR_DEFAULT_MAP_XA2 10
#define KBR_DEFAULT_MAP_ARCADIA 11
#define KBR_DEFAULT_MAP_ARCADIA_XA 12
#define KBR_DEFAULT_MAP_CDTV 13
static int **keyboard_default_kbmaps;

static int mouse_axis[MAX_INPUT_DEVICES][MAX_INPUT_DEVICE_EVENTS];
static int oldm_axis[MAX_INPUT_DEVICES][MAX_INPUT_DEVICE_EVENTS];

#define MOUSE_AXIS_TOTAL 4

static uae_s16 mouse_x[MAX_JPORTS], mouse_y[MAX_JPORTS];
static uae_s16 mouse_delta[MAX_JPORTS][MOUSE_AXIS_TOTAL];
static uae_s16 mouse_deltanoreset[MAX_JPORTS][MOUSE_AXIS_TOTAL];
static int joybutton[MAX_JPORTS];
static int joydir[MAX_JPORTS];
static int joydirpot[MAX_JPORTS][2];
static uae_s16 mouse_frame_x[MAX_JPORTS], mouse_frame_y[MAX_JPORTS];

static int mouse_port[NORMAL_JPORTS];
static int cd32_shifter[NORMAL_JPORTS];
static int cd32_pad_enabled[NORMAL_JPORTS];
static int parport_joystick_enabled;
static int oldmx[MAX_JPORTS], oldmy[MAX_JPORTS];
static int oleft[MAX_JPORTS], oright[MAX_JPORTS], otop[MAX_JPORTS], obot[MAX_JPORTS];
static int horizclear[MAX_JPORTS], vertclear[MAX_JPORTS];

uae_u16 potgo_value;
static int pot_cap[NORMAL_JPORTS][2];
static uae_u8 pot_dat[NORMAL_JPORTS][2];
static int pot_dat_act[NORMAL_JPORTS][2];
static int analog_port[NORMAL_JPORTS][2];
static int digital_port[NORMAL_JPORTS][2];
#define POTDAT_DELAY_PAL 8
#define POTDAT_DELAY_NTSC 7

static int use_joysticks[MAX_INPUT_DEVICES];
static int use_mice[MAX_INPUT_DEVICES];
static int use_keyboards[MAX_INPUT_DEVICES];

#define INPUT_QUEUE_SIZE 16
struct input_queue_struct {
	int event, storedstate, state, max, linecnt, nextlinecnt;
};
static struct input_queue_struct input_queue[INPUT_QUEUE_SIZE];

uae_u8 *restore_input (uae_u8 *src)
{
	restore_u32 ();
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			pot_cap[i][j] = restore_u16 ();
		}
	}
	return src;
}
uae_u8 *save_input (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	save_u32 (0);
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			save_u16 (pot_cap[i][j]);
		}
	}
	*len = dst - dstbak;
	return dstbak;
}

static void freejport (struct uae_prefs *dst, int num)
{
	memset (&dst->jports[num], 0, sizeof (struct jport));
	dst->jports[num].id = -1;
}
static void copyjport (const struct uae_prefs *src, struct uae_prefs *dst, int num)
{
	freejport (dst, num);
	_tcscpy (dst->jports[num].configname, src->jports[num].configname);
	_tcscpy (dst->jports[num].name, src->jports[num].name);
	dst->jports[num].id = src->jports[num].id;
	dst->jports[num].mode = src->jports[num].mode;
	dst->jports[num].autofire = src->jports[num].autofire;
}

static void out_config (struct zfile *f, int id, int num, TCHAR *s1, TCHAR *s2)
{
	TCHAR tmp[MAX_DPATH];
	_stprintf (tmp, _T("input.%d.%s%d"), id, s1, num);
	cfgfile_write_str (f, tmp, s2);
}

static bool write_config_head (struct zfile *f, int idnum, int devnum, TCHAR *name, struct uae_input_device *id,  struct inputdevice_functions *idf)
{
	TCHAR tmp2[MAX_DPATH];

	if (idnum == GAMEPORT_INPUT_SETTINGS) {
		if (!isdevice (id))
			return false;
		if (!id->enabled)
			return false;
	}

	TCHAR *s = NULL;
	if (id->name)
		s = id->name;
	else if (devnum < idf->get_num ())
		s = idf->get_friendlyname (devnum);
	if (s) {
		_stprintf (tmp2, _T("input.%d.%s.%d.friendlyname"), idnum + 1, name, devnum);
		cfgfile_write_str (f, tmp2, s);
	}

	s = NULL;
	if (id->configname)
		s = id->configname;
	else if (devnum < idf->get_num ())
		s = idf->get_uniquename (devnum);
	if (s) {
		_stprintf (tmp2, _T("input.%d.%s.%d.name"), idnum + 1, name, devnum);
		cfgfile_write_str (f, tmp2, s);
	}

	if (!isdevice (id)) {
		_stprintf (tmp2, _T("input.%d.%s.%d.empty"), idnum + 1, name, devnum);
		cfgfile_write_bool (f, tmp2, true);
		if (id->enabled) {
			_stprintf (tmp2, _T("input.%d.%s.%d.disabled"), idnum + 1, name, devnum);
			cfgfile_write (f, tmp2, _T("%d"), id->enabled ? 0 : 1);
		}
		return false;
	}

	if (idnum == GAMEPORT_INPUT_SETTINGS) {
		_stprintf (tmp2, _T("input.%d.%s.%d.custom"), idnum + 1, name, devnum);
		cfgfile_write_bool (f, tmp2, true);
	} else {
		_stprintf (tmp2, _T("input.%d.%s.%d.empty"), idnum + 1, name, devnum);
		cfgfile_write_bool (f, tmp2, false);
		_stprintf (tmp2, _T("input.%d.%s.%d.disabled"), idnum + 1, name, devnum);
		cfgfile_write_bool (f, tmp2, id->enabled ? false : true);
	}
	return true;
}

static bool write_slot (TCHAR *p, struct uae_input_device *uid, int i, int j)
{
	bool ok = false;
	if (uid->custom[i][j] && _tcslen (uid->custom[i][j]) > 0) {
		_stprintf (p, _T("'%s'.%d"), uid->custom[i][j], uid->flags[i][j] & ID_FLAG_SAVE_MASK);
		ok = true;
	} else if (uid->eventid[i][j] > 0) {
		_stprintf (p, _T("%s.%d"), events[uid->eventid[i][j]].confname, uid->flags[i][j] & ID_FLAG_SAVE_MASK);
		ok = true;
	} else {
		_tcscat (p, _T("NULL"));
	}
	return ok;
}

static struct inputdevice_functions *getidf (int devnum);

static void kbrlabel (TCHAR *s)
{
	while (*s) {
		*s = _totupper (*s);
		if (*s == ' ')
			*s = '_';
		s++;
	}
}

static void write_config2 (struct zfile *f, int idnum, int i, int offset, TCHAR *tmp1, struct uae_input_device *id)
{
	TCHAR tmp2[200], tmp3[200], *p;
	int evt, got, j, k;
	TCHAR *custom;
	int slotorder1[] = { 0, 1, 2, 3 };
	int slotorder2[] = { 4, 1, 2, 3 };
	int *slotorder;
	int io = i + offset;

	p = tmp2;
	got = 0;

	slotorder = slotorder1;
	// if gameports non-custom mapping in slot0 -> save slot4 as slot0
	if (id->port[io][0] && !(id->flags[io][0] & ID_FLAG_GAMEPORTSCUSTOM))
		slotorder = slotorder2;

	for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {

		evt = id->eventid[io][slotorder[j]];
		custom = id->custom[io][slotorder[j]];
		if (custom == NULL && evt <= 0) {
			for (k = j + 1; k < MAX_INPUT_SUB_EVENT; k++) {
				if (id->eventid[io][slotorder[k]] > 0 || id->custom[io][slotorder[k]] != NULL)
					break;
			}
			if (k == MAX_INPUT_SUB_EVENT)
				break;
		}
		if (id->port[io][0] > 0) {
			if (!(id->flags[io][0] & ID_FLAG_GAMEPORTSCUSTOM) && id->port[io][SPARE_SUB_EVENT] == 0)
				break;
		}

		if (p > tmp2) {
			*p++ = ',';
			*p = 0;
		}
		bool ok = write_slot (p, id, io, slotorder[j]);
		p += _tcslen (p);
		if (ok) {
			if (id->port[io][slotorder[j]] > 0) {
				_stprintf (p, _T(".%d"), id->port[io][slotorder[j]] - 1);
				p += _tcslen (p);
				if (idnum != GAMEPORT_INPUT_SETTINGS && j == 0 && id->port[io][SPARE_SUB_EVENT] && slotorder == slotorder1) {
					*p++ = '.';
					write_slot (p, id, io, SPARE_SUB_EVENT);
					p += _tcslen (p);
				}
			}
		}
	}
	if (p > tmp2) {
		_stprintf (tmp3, _T("input.%d.%s%d"), idnum + 1, tmp1, i);
		cfgfile_write_str (f, tmp3, tmp2);
	}
}

static void write_kbr_config (struct zfile *f, int idnum, int devnum, struct uae_input_device *kbr, struct inputdevice_functions *idf)
{
	TCHAR tmp1[200], tmp2[200], tmp3[200], tmp4[200], tmp5[200], *p;
	int i, j, k, evt, skip;
	int slotorder1[] = { 0, 1, 2, 3 };
	int slotorder2[] = { 4, 1, 2, 3 };
	int *slotorder;

	if (!keyboard_default)
		return;

	if (!write_config_head (f, idnum, devnum, _T("keyboard"), kbr, idf))
		return;

	i = 0;
	while (i < MAX_INPUT_DEVICE_EVENTS && kbr->extra[i] >= 0) {

		slotorder = slotorder1;
		// if gameports non-custom mapping in slot0 -> save slot4 as slot0
		if (kbr->port[i][0] && !(kbr->flags[i][0] & ID_FLAG_GAMEPORTSCUSTOM))
			slotorder = slotorder2;

		skip = 0;
		k = 0;
		while (keyboard_default[k].scancode >= 0) {
			if (keyboard_default[k].scancode == kbr->extra[i]) {
				skip = 1;
				for (j = 1; j < MAX_INPUT_SUB_EVENT; j++) {
					if ((kbr->flags[i][slotorder[j]] & ID_FLAG_SAVE_MASK) != 0 || kbr->eventid[i][slotorder[j]] > 0)
						skip = 0;
				}
				if (keyboard_default[k].evt != kbr->eventid[i][slotorder[0]] || keyboard_default[k].flags != (kbr->flags[i][slotorder[0]] & ID_FLAG_SAVE_MASK))
					skip = 0;
				break;
			}
			k++;
		}
		bool isdefaultspare =
			kbr->port[i][SPARE_SUB_EVENT] &&
			keyboard_default[k].evt == kbr->eventid[i][SPARE_SUB_EVENT] && keyboard_default[k].flags == (kbr->flags[i][SPARE_SUB_EVENT] & ID_FLAG_SAVE_MASK);

		if (kbr->port[i][0] > 0 && !(kbr->flags[i][0] & ID_FLAG_GAMEPORTSCUSTOM) && 
			(kbr->eventid[i][1] <= 0 && kbr->eventid[i][2] <= 0 && kbr->eventid[i][3] <= 0) &&
			(kbr->port[i][SPARE_SUB_EVENT] == 0 || isdefaultspare))
			skip = 1;
		if (kbr->eventid[i][0] == 0 && (kbr->flags[i][0] & ID_FLAG_SAVE_MASK) == 0 && keyboard_default[k].scancode < 0)
			skip = 1;
		if (skip) {
			i++;
			continue;
		}
		p = tmp2;
		p[0] = 0;
		for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			TCHAR *custom = kbr->custom[i][slotorder[j]];
			evt = kbr->eventid[i][slotorder[j]];
			if (custom == NULL && evt <= 0) {
				for (k = j + 1; k < MAX_INPUT_SUB_EVENT; k++) {
					if (kbr->eventid[i][slotorder[k]] > 0 || kbr->custom[i][slotorder[k]] != NULL)
						break;
				}
				if (k == MAX_INPUT_SUB_EVENT)
					break;
			}
			if (p > tmp2) {
				*p++ = ',';
				*p = 0;
			}
			bool ok = write_slot (p, kbr, i, slotorder[j]);
			p += _tcslen (p);
			if (ok) {
				// save port number + SPARE SLOT if needed
				if (kbr->port[i][slotorder[j]] > 0 && (kbr->flags[i][slotorder[j]] & ID_FLAG_GAMEPORTSCUSTOM)) {
					_stprintf (p, _T(".%d"), kbr->port[i][slotorder[j]] - 1);
					p += _tcslen (p);
					if (idnum != GAMEPORT_INPUT_SETTINGS && j == 0 && kbr->port[i][SPARE_SUB_EVENT] && !isdefaultspare && slotorder == slotorder1) {
						*p++ = '.';
						write_slot (p, kbr, i, SPARE_SUB_EVENT);
						p += _tcslen (p);
					}
				}
			}
		}
		idf->get_widget_type (devnum, i, tmp5, NULL);
		_stprintf (tmp3, _T("%d%s%s"), kbr->extra[i], tmp5[0] ? _T(".") : _T(""), tmp5[0] ? tmp5 : _T(""));
		kbrlabel (tmp3);
		_stprintf (tmp1, _T("keyboard.%d.button.%s"), devnum, tmp3);
		_stprintf (tmp4, _T("input.%d.%s"), idnum + 1, tmp1);
		cfgfile_write_str (f, tmp4, tmp2[0] ? tmp2 : _T("NULL"));
		i++;
	}
}

static void write_config (struct zfile *f, int idnum, int devnum, TCHAR *name, struct uae_input_device *id, struct uae_input_device2 *id2, struct inputdevice_functions *idf)
{
	TCHAR tmp1[MAX_DPATH];
	int i;

	if (!write_config_head (f, idnum, devnum, name, id, idf))
		return;

	_stprintf (tmp1, _T("%s.%d.axis."), name, devnum);
	for (i = 0; i < ID_AXIS_TOTAL; i++)
		write_config2 (f, idnum, i, ID_AXIS_OFFSET, tmp1, id);
	_stprintf (tmp1, _T("%s.%d.button.") ,name, devnum);
	for (i = 0; i < ID_BUTTON_TOTAL; i++)
		write_config2 (f, idnum, i, ID_BUTTON_OFFSET, tmp1, id);
}

void write_inputdevice_config (struct uae_prefs *p, struct zfile *f)
{
	int i, id;

	cfgfile_write (f, _T("input.config"), _T("%d"), p->input_selected_setting == GAMEPORT_INPUT_SETTINGS ? 0 : p->input_selected_setting + 1);
	cfgfile_write (f, _T("input.joymouse_speed_analog"), _T("%d"), p->input_joymouse_multiplier);
	cfgfile_write (f, _T("input.joymouse_speed_digital"), _T("%d"), p->input_joymouse_speed);
	cfgfile_write (f, _T("input.joymouse_deadzone"), _T("%d"), p->input_joymouse_deadzone);
	cfgfile_write (f, _T("input.joystick_deadzone"), _T("%d"), p->input_joystick_deadzone);
	cfgfile_write (f, _T("input.analog_joystick_multiplier"), _T("%d"), p->input_analog_joystick_mult);
	cfgfile_write (f, _T("input.analog_joystick_offset"), _T("%d"), p->input_analog_joystick_offset);
	cfgfile_write (f, _T("input.mouse_speed"), _T("%d"), p->input_mouse_speed);
	cfgfile_write (f, _T("input.autofire_speed"), _T("%d"), p->input_autofire_linecnt);
	cfgfile_dwrite (f, _T("input.contact_bounce"), _T("%d"), p->input_contact_bounce);
	for (id = 0; id < MAX_INPUT_SETTINGS; id++) {
		for (i = 0; i < MAX_INPUT_DEVICES; i++)
			write_config (f, id, i, _T("joystick"), &p->joystick_settings[id][i], &joysticks2[i], &idev[IDTYPE_JOYSTICK]);
		for (i = 0; i < MAX_INPUT_DEVICES; i++)
			write_config (f, id, i, _T("mouse"), &p->mouse_settings[id][i], &mice2[i], &idev[IDTYPE_MOUSE]);
		for (i = 0; i < MAX_INPUT_DEVICES; i++)
			write_kbr_config (f, id, i, &p->keyboard_settings[id][i], &idev[IDTYPE_KEYBOARD]);
	}
}

static int getnum (const TCHAR **pp)
{
	const TCHAR *p = *pp;
	int v;

	if (!_tcsnicmp (p, _T("false"), 5))
		v = 0;
	if (!_tcsnicmp (p, _T("true"), 4))
		v = 1;
	else
		v = _tstol (p);

	while (*p != 0 && *p !='.' && *p != ',')
		p++;
	if (*p == '.' || *p == ',')
		p++;
	*pp = p;
	return v;
}
static TCHAR *getstring (const TCHAR **pp)
{
	int i;
	static TCHAR str[1000];
	const TCHAR *p = *pp;

	if (*p == 0)
		return 0;
	i = 0;
	while (*p != 0 && *p !='.' && *p != ',' && i < 1000 - 1)
		str[i++] = *p++;
	if (*p == '.' || *p == ',')
		p++;
	str[i] = 0;
	*pp = p;
	return str;
}

static void reset_inputdevice_settings (struct uae_input_device *uid)
{
	for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
		for (int i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
			uid->eventid[l][i] = 0;
			uid->flags[l][i] = 0;
			xfree (uid->custom[l][i]);
			uid->custom[l][i] = NULL;
		}
	}
}
static void reset_inputdevice_slot (struct uae_prefs *prefs, int slot)
{
	for (int m = 0; m < MAX_INPUT_DEVICES; m++) {
		reset_inputdevice_settings (&prefs->joystick_settings[slot][m]);
		reset_inputdevice_settings (&prefs->mouse_settings[slot][m]);
		reset_inputdevice_settings (&prefs->keyboard_settings[slot][m]);
	}
}
void reset_inputdevice_config (struct uae_prefs *prefs)
{
	for (int i = 0; i< MAX_INPUT_SETTINGS; i++)
		reset_inputdevice_slot (prefs, i);
}


static void set_kbr_default_event (struct uae_input_device *kbr, struct uae_input_device_kbr_default *trans, int num)
{
	for (int i = 0;  trans[i].scancode >= 0; i++) {
		if (kbr->extra[num] == trans[i].scancode) {
			int k;
			for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {
				if (kbr->eventid[num][k] == 0)
					break;
			}
			if (k == MAX_INPUT_SUB_EVENT) {
				write_log (_T("corrupt default keyboard mappings\n"));
				return;
			}
			kbr->eventid[num][k] = trans[i].evt;
			kbr->flags[num][k] = trans[i].flags;
			break;
		}
	}
}

static void set_kbr_default (struct uae_prefs *p, int index, int devnum)
{
	int i, j;
	struct uae_input_device_kbr_default *trans = keyboard_default;
	struct uae_input_device *kbr;
	struct inputdevice_functions *id = &idev[IDTYPE_KEYBOARD];
	uae_u32 scancode;

	if (!trans)
		return;
	for (j = 0; j < MAX_INPUT_DEVICES; j++) {
		if (devnum >= 0 && devnum != j)
			continue;
		kbr = &p->keyboard_settings[index][j];
		for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
			memset (kbr, 0, sizeof (struct uae_input_device));
			kbr->extra[i] = -1;
		}
		if (j < id->get_num ()) {
			if (input_get_default_keyboard (j))
				kbr->enabled = 1;
			for (i = 0; i < id->get_widget_num (j); i++) {
				id->get_widget_type (j, i, 0, &scancode);
				kbr->extra[i] = scancode;
				set_kbr_default_event (kbr, trans, i);
			}
		}
	}
}


static void clear_id (struct uae_input_device *id)
{
#ifndef	_DEBUG
	int i, j;
	for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (j = 0; j < MAX_INPUT_SUB_EVENT; j++)
			xfree (id->custom[i][j]);
	}
#endif
	TCHAR *cn = id->configname;
	TCHAR *n = id->name;
	memset (id, 0, sizeof (struct uae_input_device));
	id->configname = cn;
	id->name = n;
}


static bool readslot (TCHAR *parm, int num, int joystick, int button, struct uae_input_device *id, int keynum, int subnum, struct inputevent *ie, int flags, int port, TCHAR *custom)
{
	int mask;

	if (custom == NULL && ie->name == NULL) {
		if (!_tcscmp (parm, _T("NULL"))) {
			if (joystick < 0) {
				id->eventid[keynum][subnum] = 0;
				id->flags[keynum][subnum] = 0;
			} else if (button) {
				id->eventid[num + ID_BUTTON_OFFSET][subnum] = 0;
				id->flags[num + ID_BUTTON_OFFSET][subnum] = 0;
			} else {
				id->eventid[num + ID_AXIS_OFFSET][subnum] = 0;
				id->flags[num + ID_AXIS_OFFSET][subnum] = 0;
			}
		}
		return false;
	}
	if (joystick < 0) {
		if (!(ie->allow_mask & AM_K))
			return false;
		id->eventid[keynum][subnum] = ie - events;
		id->flags[keynum][subnum] = flags;
		id->port[keynum][subnum] = port;
		xfree (id->custom[keynum][subnum]);
		id->custom[keynum][subnum] = custom;
	} else  if (button) {
		if (joystick)
			mask = AM_JOY_BUT;
		else
			mask = AM_MOUSE_BUT;
		if (!(ie->allow_mask & mask))
			return false;
		id->eventid[num + ID_BUTTON_OFFSET][subnum] = ie - events;
		id->flags[num + ID_BUTTON_OFFSET][subnum] = flags;
		id->port[num + ID_BUTTON_OFFSET][subnum] = port;
		xfree (id->custom[num + ID_BUTTON_OFFSET][subnum]);
		id->custom[num + ID_BUTTON_OFFSET][subnum] = custom;
	} else {
		if (joystick)
			mask = AM_JOY_AXIS;
		else
			mask = AM_MOUSE_AXIS;
		if (!(ie->allow_mask & mask))
			return false;
		id->eventid[num + ID_AXIS_OFFSET][subnum] = ie - events;
		id->flags[num + ID_AXIS_OFFSET][subnum] = flags;
		id->port[num + ID_AXIS_OFFSET][subnum] = port;
		xfree (id->custom[num + ID_AXIS_OFFSET][subnum]);
		id->custom[num + ID_AXIS_OFFSET][subnum] = custom;
	}
	return true;
}

static struct inputevent *readevent (const TCHAR *name, TCHAR **customp)
{
	int i = 1;
	while (events[i].name) {
		if (!_tcscmp (events[i].confname, name))
			return &events[i];
		i++;
	}
	if (_tcslen (name) > 2 && name[0] == '\'' && name[_tcslen (name) - 1] == '\'') {
		TCHAR *custom = my_strdup (name + 1);
		custom[_tcslen (custom) - 1] = 0;
		*customp = custom;
	}
	return &events[0];
}

void read_inputdevice_config (struct uae_prefs *pr, const TCHAR *option, TCHAR *value)
{
	struct uae_input_device *id = 0;
	struct inputevent *ie;
	int devnum, num, button, joystick, subnum, idnum, keynum;
	const TCHAR *p;
	TCHAR *p2, *custom;

	option += 6; /* "input." */
	p = getstring (&option);
	if (!strcasecmp (p, _T("config"))) {
		pr->input_selected_setting = _tstol (value) - 1;
		if (pr->input_selected_setting == -1)
			pr->input_selected_setting = GAMEPORT_INPUT_SETTINGS;
		if (pr->input_selected_setting < 0 || pr->input_selected_setting > MAX_INPUT_SETTINGS)
			pr->input_selected_setting = 0;
	}
	if (!strcasecmp (p, _T("joymouse_speed_analog")))
		pr->input_joymouse_multiplier = _tstol (value);
	if (!strcasecmp (p, _T("joymouse_speed_digital")))
		pr->input_joymouse_speed = _tstol (value);
	if (!strcasecmp (p, _T("joystick_deadzone")))
		pr->input_joystick_deadzone = _tstol (value);
	if (!strcasecmp (p, _T("joymouse_deadzone")))
		pr->input_joymouse_deadzone = _tstol (value);
	if (!strcasecmp (p, _T("mouse_speed")))
		pr->input_mouse_speed = _tstol (value);
	if (!strcasecmp (p, _T("autofire_speed")))
		pr->input_autofire_linecnt = _tstol (value);
	if (!strcasecmp (p, _T("analog_joystick_multiplier")))
		pr->input_analog_joystick_mult = _tstol (value);
	if (!strcasecmp (p, _T("analog_joystick_offset")))
		pr->input_analog_joystick_offset = _tstol (value);
	if (!strcasecmp (p, _T("contact_bounce")))
		pr->input_contact_bounce = _tstol (value);

	idnum = _tstol (p);
	if (idnum <= 0 || idnum > MAX_INPUT_SETTINGS)
		return;
	idnum--;

	if (_tcsncmp (option, _T("mouse."), 6) == 0) {
		p = option + 6;
	} else if (_tcsncmp (option, _T("joystick."), 9) == 0) {
		p = option + 9;
	} else if (_tcsncmp (option, _T("keyboard."), 9) == 0) {
		p = option + 9;
	} else
		return;

	devnum = getnum (&p);
	if (devnum < 0 || devnum >= MAX_INPUT_DEVICES)
		return;

	p2 = getstring (&p);
	if (!p2)
		return;

	if (_tcsncmp (option, _T("mouse."), 6) == 0) {
		id = &pr->mouse_settings[idnum][devnum];
		joystick = 0;
	} else if (_tcsncmp (option, _T("joystick."), 9) == 0) {
		id = &pr->joystick_settings[idnum][devnum];
		joystick = 1;
	} else if (_tcsncmp (option, _T("keyboard."), 9) == 0) {
		id = &pr->keyboard_settings[idnum][devnum];
		joystick = -1;
	}
	if (!id)
		return;

	if (!_tcscmp (p2, _T("name"))) {
		xfree (id->configname);
		id->configname = my_strdup (value);
		return;
	}
	if (!_tcscmp (p2, _T("friendlyname"))) {
		xfree (id->name);
		id->name = my_strdup (value);
		return;
	}

	if (!_tcscmp (p2, _T("custom"))) {
		int iscustom;
		p = value;
		iscustom = getnum (&p);
		if (idnum == GAMEPORT_INPUT_SETTINGS) {
			clear_id (id);
			if (joystick < 0)
				set_kbr_default (pr, idnum, devnum);
			id->enabled = iscustom;
		} else {
			id->enabled = false;
		}
		return;
	}

	if (!_tcscmp (p2, _T("empty"))) {
		int empty;
		p = value;
		empty = getnum (&p);
		clear_id (id);
		if (!empty) {
			if (joystick < 0)
				set_kbr_default (pr, idnum, devnum);
		}
		id->enabled = 1;
		if (idnum == GAMEPORT_INPUT_SETTINGS)
			id->enabled = 0;
		return;
	}

	if (!_tcscmp (p2, _T("disabled"))) {
		int disabled;
		p = value;
		disabled = getnum (&p);
		id->enabled = disabled == 0 ? 1 : 0;
		if (idnum == GAMEPORT_INPUT_SETTINGS)
			id->enabled = 0;
		return;
	}

	if (idnum == GAMEPORT_INPUT_SETTINGS && id->enabled == 0)
		return;

	button = 0;
	keynum = 0;
	if (joystick < 0) {
		num = getnum (&p);
		for (keynum = 0; keynum < MAX_INPUT_DEVICE_EVENTS; keynum++) {
			if (id->extra[keynum] == num)
				break;
		}
		if (keynum >= MAX_INPUT_DEVICE_EVENTS)
			return;
	} else {
		button = -1;
		if (!_tcscmp (p2, _T("axis")))
			button = 0;
		else if(!_tcscmp (p2, _T("button")))
			button = 1;
		if (button < 0)
			return;
		num = getnum (&p);
	}
	p = value;

	custom = NULL;
	for (subnum = 0; subnum < MAX_INPUT_SUB_EVENT; subnum++) {
		int flags, port;
		xfree (custom);
		custom = NULL;
		p2 = getstring (&p);
		if (!p2)
			break;
		ie = readevent (p2, &custom);
		flags = 0;
		port = 0;
		if (p[-1] == '.')
			flags = getnum (&p);
		if (p[-1] == '.') {
			port = getnum (&p) + 1;
		}
		if (idnum == GAMEPORT_INPUT_SETTINGS && port == 0)
			continue;
		if (p[-1] == '.' && idnum != GAMEPORT_INPUT_SETTINGS) {
			p2 = getstring (&p);
			if (p2) {
				int flags2 = 0;
				if (p[-1] == '.')
					flags2 = getnum (&p);
				TCHAR *custom2 = NULL;
				struct inputevent *ie2 = readevent (p2, &custom2);
				readslot (p2, num, joystick, button, id, keynum, SPARE_SUB_EVENT, ie2, flags2, MAX_JPORTS + 1, custom2);
			}
		}

		while (*p != 0) {
			if (p[-1] == ',')
				break;
			p++;
		}
		if (!readslot (p2, num, joystick, button, id, keynum, subnum, ie, flags, port, custom))
			continue;
		custom = NULL;
	}
	xfree (custom);
}

static int mouseedge_alive, mousehack_alive_cnt;
static int lastmx, lastmy;
static uaecptr magicmouse_ibase, magicmouse_gfxbase;
static int dimensioninfo_width, dimensioninfo_height, dimensioninfo_dbl;
static int vp_xoffset, vp_yoffset, mouseoffset_x, mouseoffset_y;
static int tablet_maxx, tablet_maxy, tablet_data;

int mousehack_alive (void)
{
	return mousehack_alive_cnt > 0 ? mousehack_alive_cnt : 0;
}

static uaecptr get_base (const uae_char *name)
{
	uaecptr v = get_long (4);
	addrbank *b = &get_mem_bank(v);

	if (!b || !b->check (v, 400) || b->flags != ABFLAG_RAM)
		return 0;
	v += 378; // liblist
	while (v = get_long (v)) {
		uae_u32 v2;
		uae_u8 *p;
		b = &get_mem_bank (v);
		if (!b || !b->check (v, 32) || b->flags != ABFLAG_RAM)
			goto fail;
		v2 = get_long (v + 10); // name
		b = &get_mem_bank (v2);
		if (!b || !b->check (v2, 20))
			goto fail;
		if (b->flags != ABFLAG_ROM && b->flags != ABFLAG_RAM)
			return 0;
		p = b->xlateaddr (v2);
		if (!memcmp (p, name, strlen (name) + 1)) {
			TCHAR *s = au (name);
			write_log (_T("get_base('%s')=%08x\n"), s, v);
			xfree (s);
			return v;
		}
	}
	return 0;
fail:
	{
		TCHAR *s = au (name);
		write_log (_T("get_base('%s') failed, invalid library list\n"), s);
		xfree (s);
	}
	return 0xffffffff;
}

static uaecptr get_intuitionbase (void)
{
	if (magicmouse_ibase == 0xffffffff)
		return 0;
	if (magicmouse_ibase)
		return magicmouse_ibase;
	magicmouse_ibase = get_base ("intuition.library");
	return magicmouse_ibase;
}
static uaecptr get_gfxbase (void)
{
	if (magicmouse_gfxbase == 0xffffffff)
		return 0;
	if (magicmouse_gfxbase)
		return magicmouse_gfxbase;
	magicmouse_gfxbase = get_base ("graphics.library");
	return magicmouse_gfxbase;
}

#define MH_E 0
#define MH_CNT 2
#define MH_MAXX 4
#define MH_MAXY 6
#define MH_MAXZ 8
#define MH_X 10
#define MH_Y 12
#define MH_Z 14
#define MH_RESX 16
#define MH_RESY 18
#define MH_MAXAX 20
#define MH_MAXAY 22
#define MH_MAXAZ 24
#define MH_AX 26
#define MH_AY 28
#define MH_AZ 30
#define MH_PRESSURE 32
#define MH_BUTTONBITS 34
#define MH_INPROXIMITY 38
#define MH_ABSX 40
#define MH_ABSY 42

#define MH_END 44
#define MH_START 4

int inputdevice_is_tablet (void)
{
	int v;
	if (!uae_boot_rom)
		return 0;
	if (currprefs.input_tablet == TABLET_OFF)
		return 0;
	if (currprefs.input_tablet == TABLET_MOUSEHACK)
		return -1;
	v = is_tablet ();
	if (!v)
		return 0;
	if (kickstart_version < 37)
		return v ? -1 : 0;
	return v ? 1 : 0;
}

static int getmhoffset (void)
{
	if (!uae_boot_rom)
		return 0;
	return get_long (rtarea_base + bootrom_header + 7 * 4) + bootrom_header;
}

static void mousehack_reset (void)
{
	int off;

	dimensioninfo_width = dimensioninfo_height = 0;
	mouseoffset_x = mouseoffset_y = 0;
	dimensioninfo_dbl = 0;
	mousehack_alive_cnt = 0;
	vp_xoffset = vp_yoffset = 0;
	tablet_data = 0;
	off = getmhoffset ();
	if (off)
		rtarea[off + MH_E] = 0;
}

static void mousehack_enable (void)
{
	int off, mode;

	if (!uae_boot_rom || currprefs.input_tablet == TABLET_OFF)
		return;
	off = getmhoffset ();
	if (rtarea[off + MH_E])
		return;
	mode = 0x80;
	if (currprefs.input_tablet == TABLET_MOUSEHACK)
		mode |= 1;
	if (inputdevice_is_tablet () > 0)
		mode |= 2;
	write_log (_T("Mouse driver enabled (%s)\n"), ((mode & 3) == 3 ? _T("tablet+mousehack") : ((mode & 3) == 2) ? _T("tablet") : _T("mousehack")));
	rtarea[off + MH_E] = 0x80;
}

void input_mousehack_mouseoffset (uaecptr pointerprefs)
{
	mouseoffset_x = (uae_s16)get_word (pointerprefs + 28);
	mouseoffset_y = (uae_s16)get_word (pointerprefs + 30);
}

void input_mousehack_status (int mode, uaecptr diminfo, uaecptr dispinfo, uaecptr vp, uae_u32 moffset)
{
	if (mode == 0) {
		uae_u8 v = rtarea[getmhoffset ()];
		v |= 0x40;
		rtarea[getmhoffset ()] = v;
		write_log (_T("Tablet driver running (%02x)\n"), v);
	} else if (mode == 1) {
		int x1 = -1, y1 = -1, x2 = -1, y2 = -1;
		uae_u32 props = 0;
		dimensioninfo_width = -1;
		dimensioninfo_height = -1;
		vp_xoffset = 0;
		vp_yoffset = 0;
		if (diminfo) {
			x1 = get_word (diminfo + 50);
			y1 = get_word (diminfo + 52);
			x2 = get_word (diminfo + 54);
			y2 = get_word (diminfo + 56);
			dimensioninfo_width = x2 - x1 + 1;
			dimensioninfo_height = y2 - y1 + 1;
		}
		if (vp) {
			vp_xoffset = get_word (vp + 28);
			vp_yoffset = get_word (vp + 30);
		}
		if (dispinfo)
			props = get_long (dispinfo + 18);
		dimensioninfo_dbl = (props & 0x00020000) ? 1 : 0;
		write_log (_T("%08x %08x %08x (%dx%d)-(%dx%d) d=%dx%d %s\n"),
			diminfo, props, vp, x1, y1, x2, y2, vp_xoffset, vp_yoffset,
			(props & 0x00020000) ? _T("dbl") : _T(""));
	} else if (mode == 2) {
		if (mousehack_alive_cnt == 0)
			mousehack_alive_cnt = -100;
		else if (mousehack_alive_cnt > 0)
			mousehack_alive_cnt = 100;
	}
}

void get_custom_mouse_limits (int *w, int *h, int *dx, int *dy, int dbl);

void inputdevice_tablet_strobe (void)
{
	uae_u8 *p;
	uae_u32 off;

	mousehack_enable ();
	if (!uae_boot_rom)
		return;
	if (!tablet_data)
		return;
	off = getmhoffset ();
	p = rtarea + off;
	p[MH_CNT]++;
}

void inputdevice_tablet (int x, int y, int z, int pressure, uae_u32 buttonbits, int inproximity, int ax, int ay, int az)
{
	uae_u8 *p;
	uae_u8 tmp[MH_END];
	uae_u32 off;

	mousehack_enable ();
	if (inputdevice_is_tablet () <= 0)
		return;
	//write_log (_T("%d %d %d %d %08X %d %d %d %d\n"), x, y, z, pressure, buttonbits, inproximity, ax, ay, az);
	off = getmhoffset ();
	p = rtarea + off;

	memcpy (tmp, p + MH_START, MH_END - MH_START); 
#if 0
	if (currprefs.input_magic_mouse) {
		int maxx, maxy, diffx, diffy;
		int dw, dh, ax, ay, aw, ah;
		float xmult, ymult;
		float fx, fy;

		fx = (float)x;
		fy = (float)y;
		desktop_coords (&dw, &dh, &ax, &ay, &aw, &ah);
		xmult = (float)tablet_maxx / dw;
		ymult = (float)tablet_maxy / dh;

		diffx = 0;
		diffy = 0;
		if (picasso_on) {
			maxx = gfxvidinfo.width;
			maxy = gfxvidinfo.height;
		} else {
			get_custom_mouse_limits (&maxx, &maxy, &diffx, &diffy);
		}
		diffx += ax;
		diffy += ah;

		fx -= diffx * xmult;
		if (fx < 0)
			fx = 0;
		if (fx >= aw * xmult)
			fx = aw * xmult - 1;
		fy -= diffy * ymult;
		if (fy < 0)
			fy = 0;
		if (fy >= ah * ymult)
			fy = ah * ymult - 1;

		x = (int)(fx * (aw * xmult) / tablet_maxx + 0.5);
		y = (int)(fy * (ah * ymult) / tablet_maxy + 0.5);

	}
#endif
	p[MH_X] = x >> 8;
	p[MH_X + 1] = x;
	p[MH_Y] = y >> 8;
	p[MH_Y + 1] = y;
	p[MH_Z] = z >> 8;
	p[MH_Z + 1] = z;

	p[MH_AX] = ax >> 8;
	p[MH_AX + 1] = ax;
	p[MH_AY] = ay >> 8;
	p[MH_AY + 1] = ay;
	p[MH_AZ] = az >> 8;
	p[MH_AZ + 1] = az;

	p[MH_MAXX] = tablet_maxx >> 8;
	p[MH_MAXX + 1] = tablet_maxx;
	p[MH_MAXY] = tablet_maxy >> 8;
	p[MH_MAXY + 1] = tablet_maxy;

	p[MH_PRESSURE] = pressure >> 8;
	p[MH_PRESSURE + 1] = pressure;

	p[MH_BUTTONBITS + 0] = buttonbits >> 24;
	p[MH_BUTTONBITS + 1] = buttonbits >> 16;
	p[MH_BUTTONBITS + 2] = buttonbits >>  8;
	p[MH_BUTTONBITS + 3] = buttonbits >>  0;

	if (inproximity < 0) {
		p[MH_INPROXIMITY] = p[MH_INPROXIMITY + 1] = 0xff;
	} else {
		p[MH_INPROXIMITY] = 0;
		p[MH_INPROXIMITY + 1] = inproximity ? 1 : 0;
	}

	if (!memcmp (tmp, p + MH_START, MH_END - MH_START))
		return;
	rtarea[off + MH_E] = 0xc0 | 2;
	p[MH_CNT]++;
}

void inputdevice_tablet_info (int maxx, int maxy, int maxz, int maxax, int maxay, int maxaz, int xres, int yres)
{
	uae_u8 *p;

	if (!uae_boot_rom)
		return;
	p = rtarea + getmhoffset ();

	tablet_maxx = maxx;
	tablet_maxy = maxy;
	p[MH_MAXX] = maxx >> 8;
	p[MH_MAXX + 1] = maxx;
	p[MH_MAXY] = maxy >> 8;
	p[MH_MAXY + 1] = maxy;
	p[MH_MAXZ] = maxz >> 8;
	p[MH_MAXZ + 1] = maxz;

	p[MH_RESX] = xres >> 8;
	p[MH_RESX + 1] = xres;
	p[MH_RESY] = yres >> 8;
	p[MH_RESY + 1] = yres;

	p[MH_MAXAX] = maxax >> 8;
	p[MH_MAXAX + 1] = maxax;
	p[MH_MAXAY] = maxay >> 8;
	p[MH_MAXAY + 1] = maxay;
	p[MH_MAXAZ] = maxaz >> 8;
	p[MH_MAXAZ + 1] = maxaz;
}


void getgfxoffset (int *dx, int *dy, int*,int*);

static void inputdevice_mh_abs (int x, int y)
{
	uae_u8 *p;
	uae_u8 tmp[4];
	uae_u32 off;

	mousehack_enable ();
	off = getmhoffset ();
	p = rtarea + off;

	memcpy (tmp, p + MH_ABSX, sizeof tmp);

	x -= mouseoffset_x + 1;
	y -= mouseoffset_y + 2;

	p[MH_ABSX] = x >> 8;
	p[MH_ABSX + 1] = x;
	p[MH_ABSY] = y >> 8;
	p[MH_ABSY + 1] = y;

	if (!memcmp (tmp, p + MH_ABSX, sizeof tmp))
		return;
	rtarea[off + MH_E] = 0xc0 | 1;
	p[MH_CNT]++;
	tablet_data = 1;
}

#if 0
static void inputdevice_mh_abs_v36 (int x, int y)
{
	uae_u8 *p;
	uae_u8 tmp[MH_END];
	uae_u32 off;
	int maxx, maxy, diffx, diffy;
	int fdy, fdx, fmx, fmy;

	mousehack_enable ();
	off = getmhoffset ();
	p = rtarea + off;

	memcpy (tmp, p + MH_START, MH_END - MH_START); 

	getgfxoffset (&fdx, &fdy, &fmx, &fmy);
	x -= fdx;
	y -= fdy;
	x += vp_xoffset;
	y += vp_yoffset;

	diffx = diffy = 0;
	maxx = maxy = 0;

	if (picasso_on) {
		maxx = picasso96_state.Width;
		maxy = picasso96_state.Height;
	} else if (dimensioninfo_width > 0 && dimensioninfo_height > 0) {
		maxx = dimensioninfo_width;
		maxy = dimensioninfo_height;
		get_custom_mouse_limits (&maxx, &maxy, &diffx, &diffy, dimensioninfo_dbl);
	} else {
		uaecptr gb = get_gfxbase ();
		maxx = 0; maxy = 0;
		if (gb) {
			maxy = get_word (gb + 216);
			maxx = get_word (gb + 218);
		}
		get_custom_mouse_limits (&maxx, &maxy, &diffx, &diffy, 0);
	}
#if 0
	{
		uaecptr gb = get_intuitionbase ();
		maxy = get_word (gb + 1344 + 2);
		maxx = get_word (gb + 1348 + 2);
		write_log (_T("%d %d\n"), maxx, maxy);
	}
#endif
#if 1
	{
		uaecptr gb = get_gfxbase ();
		uaecptr view = get_long (gb + 34);
		if (view) {
			uaecptr vp = get_long (view);
			if (vp) {
				int w, h, dw, dh;
				w = get_word (vp + 24);
				h = get_word (vp + 26) * 2;
				dw = get_word (vp + 28);
				dh = get_word (vp + 30);
				//write_log (_T("%d %d %d %d\n"), w, h, dw, dh);
				if (w < maxx)
					maxx = w;
				if (h < maxy)
					maxy = h;
				x -= dw;
				y -= dh;
			}
		}
		//write_log (_T("* %d %d\n"), get_word (gb + 218), get_word (gb + 216));
	}
	//write_log (_T("%d %d\n"), maxx, maxy);
#endif

	maxx = maxx * 1000 / fmx;
	maxy = maxy * 1000 / fmy;

	if (maxx <= 0)
		maxx = 1;
	if (maxy <= 0)
		maxy = 1;

	x -= diffx;
	if (x < 0)
		x = 0;
	if (x >= maxx)
		x = maxx - 1;

	y -= diffy;
	if (y < 0)
		y = 0;
	if (y >= maxy)
		y = maxy - 1;

	//write_log (_T("%d %d %d %d\n"), x, y, maxx, maxy);

	p[MH_X] = x >> 8;
	p[MH_X + 1] = x;
	p[MH_Y] = y >> 8;
	p[MH_Y + 1] = y;
	p[MH_MAXX] = maxx >> 8;
	p[MH_MAXX + 1] = maxx;
	p[MH_MAXY] = maxy >> 8;
	p[MH_MAXY + 1] = maxy;

	p[MH_Z] = p[MH_Z + 1] = 0;
	p[MH_MAXZ] = p[MH_MAXZ + 1] = 0;
	p[MH_AX] = p[MH_AX + 1] = 0;
	p[MH_AY] = p[MH_AY + 1] = 0;
	p[MH_AZ] = p[MH_AZ + 1] = 0;
	p[MH_PRESSURE] = p[MH_PRESSURE + 1] = 0;
	p[MH_INPROXIMITY] = p[MH_INPROXIMITY + 1] = 0xff;

	if (!memcmp (tmp, p + MH_START, MH_END - MH_START))
		return;
	p[MH_CNT]++;
	tablet_data = 1;
}
#endif

static void mousehack_helper (void)
{
	int x, y;
	int fdy, fdx, fmx, fmy;

	if (currprefs.input_magic_mouse == 0 && currprefs.input_tablet < TABLET_MOUSEHACK)
		return;
#if 0
	if (kickstart_version >= 36) {
		inputdevice_mh_abs_v36 (lastmx, lastmy);
		return;
	}
#endif
	x = lastmx;
	y = lastmy;
	getgfxoffset (&fdx, &fdy, &fmx, &fmy);


#ifdef PICASSO96
	if (picasso_on) {
		x -= picasso96_state.XOffset;
		y -= picasso96_state.YOffset;
		x = x * fmx / 1000;
		y = y * fmy / 1000;
		x -= fdx * fmx / 1000;
		y -= fdy * fmy / 1000;
	} else
#endif
	{
		x = x * fmx / 1000;
		y = y * fmy / 1000;
		x -= fdx * fmx / 1000 - 1;
		y -= fdy * fmy / 1000 - 2;
		if (x < 0)
			x = 0;
		if (x >= gfxvidinfo.width)
			x = gfxvidinfo.width - 1;
		if (y < 0)
			y = 0;
		if (y >= gfxvidinfo.height)
			y = gfxvidinfo.height - 1;
		x = coord_native_to_amiga_x (x);
		y = coord_native_to_amiga_y (y) << 1;
	}
	inputdevice_mh_abs (x, y);
}

static int mouseedge_x, mouseedge_y, mouseedge_time;
#define MOUSEEDGE_RANGE 100
#define MOUSEEDGE_TIME 2

extern void setmouseactivexy (int,int,int);

static int mouseedge (void)
{
	int x, y, dir;
	uaecptr ib;
	static int melast_x, melast_y;
	static int isnonzero;

	if (currprefs.input_magic_mouse == 0 || currprefs.input_tablet > 0)
		return 0;
	if (magicmouse_ibase == 0xffffffff)
		return 0;
	dir = 0;
	if (!mouseedge_time) {
		isnonzero = 0;
		goto end;
	}
	ib = get_intuitionbase ();
	if (!ib)
		return 0;
	x = get_word (ib + 70);
	y = get_word (ib + 68);
	if (x || y)
		isnonzero = 1;
	if (!isnonzero)
		return 0;
	if (melast_x == x) {
		if (mouseedge_x < -MOUSEEDGE_RANGE) {
			mouseedge_x = 0;
			dir |= 1;
			goto end;
		}
		if (mouseedge_x > MOUSEEDGE_RANGE) {
			mouseedge_x = 0;
			dir |= 2;
			goto end;
		}
	} else {
		mouseedge_x = 0;
		melast_x = x;
	}
	if (melast_y == y) {
		if (mouseedge_y < -MOUSEEDGE_RANGE) {
			mouseedge_y = 0;
			dir |= 4;
			goto end;
		}
		if (mouseedge_y > MOUSEEDGE_RANGE) {
			mouseedge_y = 0;
			dir |= 8;
			goto end;
		}
	} else {
		mouseedge_y = 0;
		melast_y = y;
	}
	return 1;

end:
	mouseedge_time = 0;
	if (dir) {
		if (!picasso_on) {
			int aw = 0, ah = 0, dx, dy;
			get_custom_mouse_limits (&aw, &ah, &dx, &dy, dimensioninfo_dbl);
			x += dx;
			y += dy;
		}
		if (!dmaen (DMA_SPRITE))
			setmouseactivexy (x, y, 0);
		else
			setmouseactivexy (x, y, dir);
	}
	return 1;
}

int magicmouse_alive (void)
{
	return mouseedge_alive > 0;
}

STATIC_INLINE int adjust (int val)
{
	if (val > 127)
		return 127;
	else if (val < -127)
		return -127;
	return val;
}

static int getbuttonstate (int joy, int button)
{
	return (joybutton[joy] & (1 << button)) ? 1 : 0;
}

static int getvelocity (int num, int subnum, int pct)
{
	int val;
	int v;

	if (pct > 1000)
		pct = 1000;
	val = mouse_delta[num][subnum];
	v = val * pct / 1000;
	if (!v) {
		if (val < -maxvpos / 2)
			v = -2;
		else if (val < 0)
			v = -1;
		else if (val > maxvpos / 2)
			v = 2;
		else if (val > 0)
			v = 1;
	}
	if (!mouse_deltanoreset[num][subnum]) {
		mouse_delta[num][subnum] -= v;
		gui_gameport_axis_change (num, subnum * 2 + 0, 0, -1);
		gui_gameport_axis_change (num, subnum * 2 + 1, 0, -1);
	}
	return v;
}

static void mouseupdate (int pct, int vsync)
{
	int v, i;
	int max = 120;
	static int mxd, myd;

	if (vsync) {
		if (mxd < 0) {
			if (mouseedge_x > 0)
				mouseedge_x = 0;
			else
				mouseedge_x += mxd;
			mouseedge_time = MOUSEEDGE_TIME;
		}
		if (mxd > 0) {
			if (mouseedge_x < 0)
				mouseedge_x = 0;
			else
				mouseedge_x += mxd;
			mouseedge_time = MOUSEEDGE_TIME;
		}
		if (myd < 0) {
			if (mouseedge_y > 0)
				mouseedge_y = 0;
			else
				mouseedge_y += myd;
			mouseedge_time = MOUSEEDGE_TIME;
		}
		if (myd > 0) {
			if (mouseedge_y < 0)
				mouseedge_y = 0;
			else
				mouseedge_y += myd;
			mouseedge_time = MOUSEEDGE_TIME;
		}
		if (mouseedge_time > 0) {
			mouseedge_time--;
			if (mouseedge_time == 0) {
				mouseedge_x = 0;
				mouseedge_y = 0;
			}
		}
		mxd = 0;
		myd = 0;
	}

	for (i = 0; i < 2; i++) {

		if (mouse_port[i]) {

			v = getvelocity (i, 0, pct);
			mxd += v;
			mouse_x[i] += v;

			v = getvelocity (i, 1, pct);
			myd += v;
			mouse_y[i] += v;

			v = getvelocity (i, 2, pct);
			if (v > 0)
				record_key (0x7a << 1);
			else if (v < 0)
				record_key (0x7b << 1);
			if (!mouse_deltanoreset[i][2])
				mouse_delta[i][2] = 0;

			if (mouse_frame_x[i] - mouse_x[i] > max)
				mouse_x[i] = mouse_frame_x[i] - max;
			if (mouse_frame_x[i] - mouse_x[i] < -max)
				mouse_x[i] = mouse_frame_x[i] + max;

			if (mouse_frame_y[i] - mouse_y[i] > max)
				mouse_y[i] = mouse_frame_y[i] - max;
			if (mouse_frame_y[i] - mouse_y[i] < -max)
				mouse_y[i] = mouse_frame_y[i] + max;
		}

		if (!vsync) {
			mouse_frame_x[i] = mouse_x[i];
			mouse_frame_y[i] = mouse_y[i];
		}

	}
}

static int input_vpos, input_frame;
extern int vpos;
static void readinput (void)
{
	uae_u32 totalvpos;
	int diff;

	totalvpos = input_frame * current_maxvpos () + vpos;
	diff = totalvpos - input_vpos;
	if (diff > 0) {
		if (diff < 10) {
			mouseupdate (0, 0);
		} else {
			mouseupdate (diff * 1000 / current_maxvpos (), 0);
		}
	}
	input_vpos = totalvpos;

}

static void joymousecounter (int joy)
{
	int left = 1, right = 1, top = 1, bot = 1;
	int b9, b8, b1, b0;
	int cntx, cnty, ocntx, ocnty;

	if (joydir[joy] & DIR_LEFT)
		left = 0;
	if (joydir[joy] & DIR_RIGHT)
		right = 0;
	if (joydir[joy] & DIR_UP)
		top = 0;
	if (joydir[joy] & DIR_DOWN)
		bot = 0;

	b0 = (bot ^ right) ? 1 : 0;
	b1 = (right ^ 1) ? 2 : 0;
	b8 = (top ^ left) ? 1 : 0;
	b9 = (left ^ 1) ? 2 : 0;

	cntx = b0 | b1;
	cnty = b8 | b9;
	ocntx = mouse_x[joy] & 3;
	ocnty = mouse_y[joy] & 3;

	if (cntx == 3 && ocntx == 0)
		mouse_x[joy] -= 4;
	else if (cntx == 0 && ocntx == 3)
		mouse_x[joy] += 4;
	mouse_x[joy] = (mouse_x[joy] & 0xfc) | cntx;

	if (cnty == 3 && ocnty == 0)
		mouse_y[joy] -= 4;
	else if (cnty == 0 && ocnty == 3)
		mouse_y[joy] += 4;
	mouse_y[joy] = (mouse_y[joy] & 0xfc) | cnty;
}

static uae_u16 getjoystate (int joy)
{
	uae_u16 v;

	v = (uae_u8)mouse_x[joy] | (mouse_y[joy] << 8);
#ifdef DONGLE_DEBUG
	if (notinrom ())
		write_log (_T("JOY%dDAT %04X %s\n"), joy, v, debuginfo (0));
#endif
	if (inputdevice_logging & 2)
		write_log (_T("JOY%dDAT=%04x %08x\n"), joy, v, M68K_GETPC);
	return v;
}

uae_u16 JOY0DAT (void)
{
	uae_u16 v;
	readinput ();
	v = getjoystate (0);
	v = dongle_joydat (0, v);
	return v;
}

uae_u16 JOY1DAT (void)
{
	uae_u16 v;
	readinput ();
	v = getjoystate (1);
	v = dongle_joydat (1, v);

	if (inputrecord_debug & 2) {
		if (input_record > 0)
			inprec_recorddebug_cia (v, -1, m68k_getpc ());
		else if (input_play > 0)
			inprec_playdebug_cia (v, -1, m68k_getpc ());
	}

	return v;
}

uae_u16 JOYGET (int num)
{
	uae_u16 v;
	v = getjoystate (num);
	v = dongle_joydat (num, v);
	return v;
}

void JOYSET (int num, uae_u16 dat)
{
	mouse_x[num] = dat & 0xff;
	mouse_y[num] = (dat >> 8) & 0xff;
	mouse_frame_x[num] = mouse_x[num];
	mouse_frame_y[num] = mouse_y[num];
}

void JOYTEST (uae_u16 v)
{
	mouse_x[0] &= 3;
	mouse_y[0] &= 3;
	mouse_x[1] &= 3;
	mouse_y[1] &= 3;
	mouse_x[0] |= v & 0xFC;
	mouse_x[1] |= v & 0xFC;
	mouse_y[0] |= (v >> 8) & 0xFC;
	mouse_y[1] |= (v >> 8) & 0xFC;
	mouse_frame_x[0] = mouse_x[0];
	mouse_frame_y[0] = mouse_y[0];
	mouse_frame_x[1] = mouse_x[1];
	mouse_frame_y[1] = mouse_y[1];
	dongle_joytest (v);
	if (inputdevice_logging & 2)
		write_log (_T("JOYTEST: %04X PC=%x\n"), v , M68K_GETPC);
}

static uae_u8 parconvert (uae_u8 v, int jd, int shift)
{
	if (jd & DIR_UP)
		v &= ~(1 << shift);
	if (jd & DIR_DOWN)
		v &= ~(2 << shift);
	if (jd & DIR_LEFT)
		v &= ~(4 << shift);
	if (jd & DIR_RIGHT)
		v &= ~(8 << shift);
	return v;
}

/* io-pins floating: dir=1 -> return data, dir=0 -> always return 1 */
uae_u8 handle_parport_joystick (int port, uae_u8 pra, uae_u8 dra)
{
	uae_u8 v;
	switch (port)
	{
	case 0:
		v = (pra & dra) | (dra ^ 0xff);
		if (parport_joystick_enabled) {
			v = parconvert (v, joydir[2], 0);
			v = parconvert (v, joydir[3], 4);
		}
		return v;
	case 1:
		v = ((pra & dra) | (dra ^ 0xff)) & 0x7;
		if (parport_joystick_enabled) {
			if (getbuttonstate (2, 0))
				v &= ~4;
			if (getbuttonstate (3, 0))
				v &= ~1;
		}
		return v;
	default:
		abort ();
		return 0;
	}
}

/* p5 is 1 or floating = cd32 2-button mode */
static bool cd32padmode (uae_u16 p5dir, uae_u16 p5dat)
{
	if (!(potgo_value & p5dir) || ((potgo_value & p5dat) && (potgo_value & p5dir)))
		return false;
	return true;
}

static bool is_joystick_pullup (int joy)
{
	return joymodes[joy] == JSEM_MODE_GAMEPAD;
}
static bool is_mouse_pullup (int joy)
{
	return mouse_pullup;
}

static void charge_cap (int joy, int idx, int charge)
{
	if (charge < -1 || charge > 1)
		charge = charge * 80;
	pot_cap[joy][idx] += charge;
	if (pot_cap[joy][idx] < 0)
		pot_cap[joy][idx] = 0;
	if (pot_cap[joy][idx] > 511)
		pot_cap[joy][idx] = 511;
}

static void cap_check (void)
{
	int joy, i;

	for (joy = 0; joy < 2; joy++) {
		for (i = 0; i < 2; i++) {
			int charge = 0, dong, joypot;
			uae_u16 pdir = 0x0200 << (joy * 4 + i * 2); /* output enable */
			uae_u16 pdat = 0x0100 << (joy * 4 + i * 2); /* data */
			uae_u16 p5dir = 0x0200 << (joy * 4);
			uae_u16 p5dat = 0x0100 << (joy * 4);
			int isbutton = getbuttonstate (joy, i == 0 ? JOYBUTTON_3 : JOYBUTTON_2);

			if (cd32_pad_enabled[joy]) {
				// only red and blue can be read if CD32 pad and only if it is in normal pad mode
				isbutton |= getbuttonstate (joy, JOYBUTTON_CD32_BLUE);
				// CD32 pad 3rd button line (P5) is always floating
				if (i == 0)
					isbutton = 0;
				if (cd32padmode (p5dir, p5dat))
					continue;
			}

			dong = dongle_analogjoy (joy, i);
			if (dong >= 0) {
				isbutton = 0;
				joypot = dong;
				if (pot_cap[joy][i] < joypot)
					charge = 1; // slow charge via dongle resistor
			} else {
				joypot = joydirpot[joy][i];
				if (analog_port[joy][i] && pot_cap[joy][i] < joypot)
					charge = 1; // slow charge via pot variable resistor
				if ((is_joystick_pullup (joy) && digital_port[joy][i]) || (is_mouse_pullup (joy) && mouse_port[joy]))
					charge = 1; // slow charge via pull-up resistor
			}
			if (!(potgo_value & pdir)) { // input?
				if (pot_dat_act[joy][i])
					pot_dat[joy][i]++;
				/* first 7 or 8 lines after potgo has been started = discharge cap */
				if (pot_dat_act[joy][i] == 1) {
					if (pot_dat[joy][i] < (currprefs.ntscmode ? POTDAT_DELAY_NTSC : POTDAT_DELAY_PAL)) {
						charge = -2; /* fast discharge delay */
					} else {
						pot_dat_act[joy][i] = 2;
						pot_dat[joy][i] = 0;
					}
				}
				if (dong >= 0) {
					if (pot_dat_act[joy][i] == 2 && pot_cap[joy][i] >= joypot)
						pot_dat_act[joy][i] = 0;
				} else {
					if (analog_port[joy][i] && pot_dat_act[joy][i] == 2 && pot_cap[joy][i] >= joypot)
						pot_dat_act[joy][i] = 0;
					if ((digital_port[joy][i] || mouse_port[joy]) && pot_dat_act[joy][i] == 2) {
						if (pot_cap[joy][i] >= 10 && !isbutton)
							pot_dat_act[joy][i] = 0;
					}
				}
			} else { // output?
				charge = (potgo_value & pdat) ? 2 : -2; /* fast (dis)charge if output */
				if (potgo_value & pdat)
					pot_dat_act[joy][i] = 0; // instant stop if output+high
				if (isbutton)
					pot_dat[joy][i]++; // "free running" if output+low
			}

			if (isbutton)
				charge = -2; // button press overrides everything

			if (currprefs.cs_cdtvcd) {
				/* CDTV P9 is not floating */
				if (!(potgo_value & pdir) && i == 1 && charge == 0)
					charge = 2;
			}
			// CD32 pad in 2-button mode: blue button is not floating
			if (cd32_pad_enabled[joy] && i == 1 && charge == 0)
				charge = 2;
		
			/* official Commodore mouse has pull-up resistors in button lines
			* NOTE: 3rd party mice may not have pullups! */
			if (dong < 0 && (is_mouse_pullup (joy) && mouse_port[joy] && digital_port[joy][i]) && charge == 0)
				charge = 2;
			/* emulate pullup resistor if button mapped because there too many broken
			* programs that read second button in input-mode (and most 2+ button pads have
			* pullups)
			*/
			if (dong < 0 && (is_joystick_pullup (joy) && digital_port[joy][i]) && charge == 0)
				charge = 2;

			charge_cap (joy, i, charge);
		}
	}
}


uae_u8 handle_joystick_buttons (uae_u8 pra, uae_u8 dra)
{
	uae_u8 but = 0;
	int i;

	cap_check ();
	for (i = 0; i < 2; i++) {
		int mask = 0x40 << i;
		if (cd32_pad_enabled[i]) {
			uae_u16 p5dir = 0x0200 << (i * 4);
			uae_u16 p5dat = 0x0100 << (i * 4);
			but |= mask;
			if (!cd32padmode (p5dir, p5dat)) {
				if (getbuttonstate (i, JOYBUTTON_CD32_RED) || getbuttonstate (i, JOYBUTTON_1))
					but &= ~mask;
			}
		} else {
			if (!getbuttonstate (i, JOYBUTTON_1))
				but |= mask;
			if (bouncy && cycles_in_range (bouncy_cycles)) {
				but &= ~mask;
				if (uaerand () & 1)
					but |= mask;
			}
			if (dra & mask)
				but = (but & ~mask) | (pra & mask);
		}
	}

	if (inputdevice_logging & 4) {
		static uae_u8 old;
		if (but != old)
			write_log (_T("BFE001: %02X:%02X %x\n"), dra, but, M68K_GETPC);
		old = but;
	}
	return but;
}

/* joystick 1 button 1 is used as a output for incrementing shift register */
void handle_cd32_joystick_cia (uae_u8 pra, uae_u8 dra)
{
	static int oldstate[2];
	int i;

	cap_check ();
	for (i = 0; i < 2; i++) {
		uae_u8 but = 0x40 << i;
		uae_u16 p5dir = 0x0200 << (i * 4); /* output enable P5 */
		uae_u16 p5dat = 0x0100 << (i * 4); /* data P5 */
		if (cd32padmode (p5dir, p5dat)) {
			if ((dra & but) && (pra & but) != oldstate[i]) {
				if (!(pra & but)) {
					cd32_shifter[i]--;
					if (cd32_shifter[i] < 0)
						cd32_shifter[i] = 0;
				}
			}
		}
		oldstate[i] = pra & but;
	}
}

/* joystick port 1 button 2 is input for button state */
static uae_u16 handle_joystick_potgor (uae_u16 potgor)
{
	int i;

	cap_check ();
	for (i = 0; i < 2; i++) {
		uae_u16 p9dir = 0x0800 << (i * 4); /* output enable P9 */
		uae_u16 p9dat = 0x0400 << (i * 4); /* data P9 */
		uae_u16 p5dir = 0x0200 << (i * 4); /* output enable P5 */
		uae_u16 p5dat = 0x0100 << (i * 4); /* data P5 */

		if (cd32_pad_enabled[i] && cd32padmode (p5dir, p5dat)) {

			/* p5 is floating in input-mode */
			potgor &= ~p5dat;
			potgor |= potgo_value & p5dat;
			if (!(potgo_value & p9dir))
				potgor |= p9dat;
			/* (P5 output and 1) or floating -> shift register is kept reset (Blue button) */
			if (!(potgo_value & p5dir) || ((potgo_value & p5dat) && (potgo_value & p5dir)))
				cd32_shifter[i] = 8;
			/* shift at 1 == return one, >1 = return button states */
			if (cd32_shifter[i] == 0)
				potgor &= ~p9dat; /* shift at zero == return zero */
			if (cd32_shifter[i] >= 2 && (joybutton[i] & ((1 << JOYBUTTON_CD32_PLAY) << (cd32_shifter[i] - 2))))
				potgor &= ~p9dat;

		} else  {

			potgor &= ~p5dat;
			if (pot_cap[i][0] > 100)
				potgor |= p5dat;


			if (!cd32_pad_enabled[i] || !cd32padmode (p5dir, p5dat)) {
				potgor &= ~p9dat;
				if (pot_cap[i][1] > 100)
					potgor |= p9dat;
			}

		}
	}
	return potgor;
}


static int inputdelay;

void inputdevice_read (void)
{
	do {
		handle_msgpump ();
		idev[IDTYPE_MOUSE].read ();
		idev[IDTYPE_JOYSTICK].read ();
		idev[IDTYPE_KEYBOARD].read ();
	} while (handle_msgpump ());
}

void inputdevice_hsync (void)
{
	static int cnt;
	cap_check ();

#ifdef CATWEASEL
	catweasel_hsync ();
#endif

	for (int i = 0; i < INPUT_QUEUE_SIZE; i++) {
		struct input_queue_struct *iq = &input_queue[i];
		if (iq->linecnt > 0) {
			iq->linecnt--;
			if (iq->linecnt == 0) {
				if (iq->state)
					iq->state = 0;
				else
					iq->state = iq->storedstate;
				handle_input_event (iq->event, iq->state, iq->max, 0, false, true);
				iq->linecnt = iq->nextlinecnt;
			}
		}
	}

	if (bouncy && get_cycles () > bouncy_cycles)
		bouncy = 0;

	if (input_record && input_record != INPREC_RECORD_PLAYING) {
		if (vpos == 0)
			inputdevice_read ();
		inputdelay = 0;
	}
	if (input_play) {
		inprec_playdiskchange ();
		int nr, state, max, autofire;
		while (inprec_playevent (&nr, &state, &max, &autofire))
			handle_input_event (nr, state, max, autofire, false, true);
		if (vpos == 0)
			handle_msgpump ();
	}
	if (!input_record && !input_play) {
		if ((++cnt & 63) == 63 ) {
			inputdevice_read ();
		} else if (inputdelay > 0) {
			inputdelay--;
			if (inputdelay == 0)
				inputdevice_read ();
		}
	}
}

static uae_u16 POTDAT (int joy)
{
	uae_u16 v = (pot_dat[joy][1] << 8) | pot_dat[joy][0];
	if (inputdevice_logging & 16)
		write_log (_T("POTDAT%d: %04X %08X\n"), joy, v, M68K_GETPC);
	return v;
}

uae_u16 POT0DAT (void)
{
	return POTDAT (0);
}
uae_u16 POT1DAT (void)
{
	return POTDAT (1);
}

/* direction=input, data pin floating, last connected logic level or previous status
*                  written when direction was ouput
*                  otherwise it is currently connected logic level.
* direction=output, data pin is current value, forced to zero if joystick button is pressed
* it takes some tens of microseconds before data pin changes state
*/

void POTGO (uae_u16 v)
{
	int i, j;

	if (inputdevice_logging & 16)
		write_log (_T("POTGO_W: %04X %08X\n"), v, M68K_GETPC);
#ifdef DONGLE_DEBUG
	if (notinrom ())
		write_log (_T("POTGO %04X %s\n"), v, debuginfo(0));
#endif
	dongle_potgo (v);
	potgo_value = potgo_value & 0x5500; /* keep state of data bits */
	potgo_value |= v & 0xaa00; /* get new direction bits */
	for (i = 0; i < 8; i += 2) {
		uae_u16 dir = 0x0200 << i;
		if (v & dir) {
			uae_u16 data = 0x0100 << i;
			potgo_value &= ~data;
			potgo_value |= v & data;
		}
	}
	for (i = 0; i < 2; i++) {
		if (cd32_pad_enabled[i]) {
			uae_u16 p5dir = 0x0200 << (i * 4); /* output enable P5 */
			uae_u16 p5dat = 0x0100 << (i * 4); /* data P5 */
			if (!(potgo_value & p5dir) || ((potgo_value & p5dat) && (potgo_value & p5dir)))
				cd32_shifter[i] = 8;
		}
	}
	if (v & 1) {
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 2; j++) {
				pot_dat_act[i][j] = 1;
				pot_dat[i][j] = 0;
			}
		}
	}
}

uae_u16 POTGOR (void)
{
	uae_u16 v;

	v = handle_joystick_potgor (potgo_value) & 0x5500;
	v = dongle_potgor (v);
#ifdef DONGLE_DEBUG
	if (notinrom ())
		write_log (_T("POTGOR %04X %s\n"), v, debuginfo(0));
#endif
	if (inputdevice_logging & 16)
		write_log (_T("POTGO_R: %04X %08X %d\n"), v, M68K_GETPC, cd32_shifter[1]);
	return v;
}

static int check_input_queue (int event)
{
	struct input_queue_struct *iq;
	int i;
	for (i = 0; i < INPUT_QUEUE_SIZE; i++) {
		iq = &input_queue[i];
		if (iq->event == event)
			return i;
	}
	return -1;
}

static void queue_input_event (int event, int state, int max, int linecnt, int autofire)
{
	struct input_queue_struct *iq;
	int i = check_input_queue (event);

	if (event <= 0)
		return;
	if (state < 0 && i >= 0) {
		iq = &input_queue[i];
		iq->nextlinecnt = -1;
		iq->linecnt = -1;
		iq->event = 0;
		if (iq->state == 0)
			handle_input_event (event, 0, 1, 0, false, false);
	} else if (i < 0) {
		for (i = 0; i < INPUT_QUEUE_SIZE; i++) {
			iq = &input_queue[i];
			if (iq->linecnt < 0)
				break;
		}
		if (i == INPUT_QUEUE_SIZE) {
			write_log (_T("input queue overflow\n"));
			return;
		}
		iq->event = event;
		iq->state = iq->storedstate = state;
		iq->max = max;
		iq->linecnt = linecnt;
		iq->nextlinecnt = autofire > 0 ? linecnt : -1;
	}
}

static uae_u8 keybuf[256];
static int inputcode_pending, inputcode_pending_state;

void inputdevice_release_all_keys (void)
{
	int i;

	for (i = 0; i < 0x80; i++) {
		if (keybuf[i] != 0) {
			keybuf[i] = 0;
			record_key (i << 1|1);
		}
	}
}


void inputdevice_add_inputcode (int code, int state)
{
	inputcode_pending = code;
	inputcode_pending_state = state;
}

void inputdevice_do_keyboard (int code, int state)
{
	if (code >= 0x72 && code <= 0x77) { // CDTV keys
		if (cdtv_front_panel (-1)) {
			// front panel active
			if (!state)
				return;
			cdtv_front_panel (code - 0x72);
			return;
		}
	}
	if (code < 0x80) {
		uae_u8 key = code | (state ? 0x00 : 0x80);
		keybuf[key & 0x7f] = (key & 0x80) ? 0 : 1;
		if (key == AK_RESETWARNING) {
			resetwarning_do (0);
			return;
		} else if ((keybuf[AK_CTRL] || keybuf[AK_RCTRL]) && keybuf[AK_LAMI] && keybuf[AK_RAMI]) {
			int r = keybuf[AK_LALT] | keybuf[AK_RALT];
			if (!r && currprefs.cs_resetwarning && resetwarning_do (1))
				return;
			memset (keybuf, 0, sizeof (keybuf));
			uae_reset (r);
		}
		if (record_key ((uae_u8)((key << 1) | (key >> 7)))) {
			if (inputdevice_logging & 1)
				write_log (_T("Amiga key %02X %d\n"), key & 0x7f, key >> 7);
		}
		return;
	}
	inputdevice_add_inputcode (code, state);
}

// these need cpu trace data
static bool needcputrace (int code)
{
	switch (code)
	{
	case AKS_ENTERGUI:
	case AKS_STATECAPTURE:
	case AKS_STATESAVEQUICK:
	case AKS_STATESAVEQUICK1:
	case AKS_STATESAVEQUICK2:
	case AKS_STATESAVEQUICK3:
	case AKS_STATESAVEQUICK4:
	case AKS_STATESAVEQUICK5:
	case AKS_STATESAVEQUICK6:
	case AKS_STATESAVEQUICK7:
	case AKS_STATESAVEQUICK8:
	case AKS_STATESAVEQUICK9:
	case AKS_STATESAVEDIALOG:
		return true;
	}
	return false;
}

void inputdevice_handle_inputcode (void)
{
	static int swapperslot;
	int code = inputcode_pending;
	int state = inputcode_pending_state;
	static int tracer_enable;

	if (code == 0)
		goto end;
	if (needcputrace (code) && can_cpu_tracer () == true && is_cpu_tracer () == false && !input_play && !input_record && !debugging) {
		if (set_cpu_tracer (true)) {
			tracer_enable = 1;
			return; // wait for next frame
		}
	}

	inputcode_pending = 0;

	if (vpos != 0)
		write_log (_T("inputcode=%d but vpos = %d"), code, vpos);

#ifdef ARCADIA
	switch (code)
	{
	case AKS_ARCADIADIAGNOSTICS:
		arcadia_flag &= ~1;
		arcadia_flag |= state ? 1 : 0;
		break;
	case AKS_ARCADIAPLY1:
		arcadia_flag &= ~4;
		arcadia_flag |= state ? 4 : 0;
		break;
	case AKS_ARCADIAPLY2:
		arcadia_flag &= ~2;
		arcadia_flag |= state ? 2 : 0;
		break;
	case AKS_ARCADIACOIN1:
		if (state)
			arcadia_coin[0]++;
		break;
	case AKS_ARCADIACOIN2:
		if (state)
			arcadia_coin[1]++;
		break;
	}
#endif

	if (!state)
		return;
	switch (code)
	{
	case AKS_ENTERGUI:
		gui_display (-1);
		break;
	case AKS_SCREENSHOT_FILE:
		screenshot (1, 1);
		break;
	case AKS_SCREENSHOT_CLIPBOARD:
		screenshot (0, 1);
		break;
#ifdef ACTION_REPLAY
	case AKS_FREEZEBUTTON:
		action_replay_freeze ();
		break;
#endif
	case AKS_FLOPPY0:
		gui_display (0);
		break;
	case AKS_FLOPPY1:
		gui_display (1);
		break;
	case AKS_FLOPPY2:
		gui_display (2);
		break;
	case AKS_FLOPPY3:
		gui_display (3);
		break;
	case AKS_EFLOPPY0:
		disk_eject (0);
		break;
	case AKS_EFLOPPY1:
		disk_eject (1);
		break;
	case AKS_EFLOPPY2:
		disk_eject (2);
		break;
	case AKS_EFLOPPY3:
		disk_eject (3);
		break;
	case AKS_IRQ7:
		NMI_delayed ();
		break;
	case AKS_PAUSE:
		pausemode (-1);
		break;
	case AKS_WARP:
		warpmode (-1);
		break;
	case AKS_INHIBITSCREEN:
		toggle_inhibit_frame (IHF_SCROLLLOCK);
		break;
	case AKS_STATEREWIND:
		savestate_dorewind (-2);
		break;
	case AKS_STATECURRENT:
		savestate_dorewind (-1);
		break;
	case AKS_STATECAPTURE:
		savestate_capture (1);
		break;
	case AKS_VOLDOWN:
		sound_volume (-1);
		break;
	case AKS_VOLUP:
		sound_volume (1);
		break;
	case AKS_VOLMUTE:
		sound_mute (-1);
		break;
	case AKS_MVOLDOWN:
		master_sound_volume (-1);
		break;
	case AKS_MVOLUP:
		master_sound_volume (1);
		break;
	case AKS_MVOLMUTE:
		master_sound_volume (0);
		break;
	case AKS_QUIT:
		uae_quit ();
		break;
	case AKS_SOFTRESET:
		uae_reset (0);
		break;
	case AKS_HARDRESET:
		uae_reset (1);
		break;
	case AKS_STATESAVEQUICK:
	case AKS_STATESAVEQUICK1:
	case AKS_STATESAVEQUICK2:
	case AKS_STATESAVEQUICK3:
	case AKS_STATESAVEQUICK4:
	case AKS_STATESAVEQUICK5:
	case AKS_STATESAVEQUICK6:
	case AKS_STATESAVEQUICK7:
	case AKS_STATESAVEQUICK8:
	case AKS_STATESAVEQUICK9:
		savestate_quick ((code - AKS_STATESAVEQUICK) / 2, 1);
		break;
	case AKS_STATERESTOREQUICK:
	case AKS_STATERESTOREQUICK1:
	case AKS_STATERESTOREQUICK2:
	case AKS_STATERESTOREQUICK3:
	case AKS_STATERESTOREQUICK4:
	case AKS_STATERESTOREQUICK5:
	case AKS_STATERESTOREQUICK6:
	case AKS_STATERESTOREQUICK7:
	case AKS_STATERESTOREQUICK8:
	case AKS_STATERESTOREQUICK9:
		savestate_quick ((code - AKS_STATERESTOREQUICK) / 2, 0);
		break;
	case AKS_TOGGLEDEFAULTSCREEN:
		toggle_fullscreen (-1);
		break;
	case AKS_TOGGLEWINDOWEDFULLSCREEN:
		toggle_fullscreen (0);
		break;
	case AKS_TOGGLEFULLWINDOWFULLSCREEN:
		toggle_fullscreen (1);
		break;
	case AKS_TOGGLEWINDOWFULLWINDOW:
		toggle_fullscreen (2);
		break;
	case AKS_TOGGLEMOUSEGRAB:
		toggle_mousegrab ();
		break;
	case AKS_ENTERDEBUGGER:
		activate_debugger ();
		break;
	case AKS_STATESAVEDIALOG:
		gui_display (5);
		break;
	case AKS_STATERESTOREDIALOG:
		gui_display (4);
		break;
	case AKS_DECREASEREFRESHRATE:
	case AKS_INCREASEREFRESHRATE:
		{
			int dir = code == AKS_INCREASEREFRESHRATE ? 5 : -5;
			if (currprefs.chipset_refreshrate == 0)
				currprefs.chipset_refreshrate = currprefs.ntscmode ? 60 : 50;
			changed_prefs.chipset_refreshrate = currprefs.chipset_refreshrate + dir;
			if (changed_prefs.chipset_refreshrate < 10)
				changed_prefs.chipset_refreshrate = 10;
			if (changed_prefs.chipset_refreshrate > 900)
				changed_prefs.chipset_refreshrate = 900;
			config_changed = 1;
		}
		break;
	case AKS_DISKSWAPPER_NEXT:
		swapperslot++;
		if (swapperslot >= MAX_SPARE_DRIVES || currprefs.dfxlist[swapperslot][0] == 0)
			swapperslot = 0;
		break;
	case AKS_DISKSWAPPER_PREV:
		swapperslot--;
		if (swapperslot < 0)
			swapperslot = MAX_SPARE_DRIVES - 1;
		while (swapperslot > 0) {
			if (currprefs.dfxlist[swapperslot][0])
				break;
			swapperslot--;
		}
		break;
	case AKS_DISKSWAPPER_INSERT0:
	case AKS_DISKSWAPPER_INSERT1:
	case AKS_DISKSWAPPER_INSERT2:
	case AKS_DISKSWAPPER_INSERT3:
		_tcscpy (changed_prefs.floppyslots[code - AKS_DISKSWAPPER_INSERT0].df, currprefs.dfxlist[swapperslot]);
		config_changed = 1;
		break;

		break;
	case AKS_INPUT_CONFIG_1:
	case AKS_INPUT_CONFIG_2:
	case AKS_INPUT_CONFIG_3:
	case AKS_INPUT_CONFIG_4:
		changed_prefs.input_selected_setting = currprefs.input_selected_setting = code - AKS_INPUT_CONFIG_1;
		inputdevice_updateconfig (&currprefs);
		break;
	case AKS_DISK_PREV0:
	case AKS_DISK_PREV1:
	case AKS_DISK_PREV2:
	case AKS_DISK_PREV3:
		disk_prevnext (code - AKS_DISK_PREV0, -1);
		break;
	case AKS_DISK_NEXT0:
	case AKS_DISK_NEXT1:
	case AKS_DISK_NEXT2:
	case AKS_DISK_NEXT3:
		disk_prevnext (code - AKS_DISK_NEXT0, 1);
		break;
#ifdef CDTV
	case AKS_CDTV_FRONT_PANEL_STOP:
	case AKS_CDTV_FRONT_PANEL_PLAYPAUSE:
	case AKS_CDTV_FRONT_PANEL_PREV:
	case AKS_CDTV_FRONT_PANEL_NEXT:
	case AKS_CDTV_FRONT_PANEL_REW:
	case AKS_CDTV_FRONT_PANEL_FF:
		cdtv_front_panel (code - AKS_CDTV_FRONT_PANEL_STOP);
	break;
#endif
	}
end:
	if (tracer_enable) {
		set_cpu_tracer (false);
		tracer_enable = 0;
	}
}

int handle_custom_event (TCHAR *custom)
{
	TCHAR *p, *buf, *nextp;

	if (custom == NULL)
		return 0;
	p = buf = my_strdup (custom);
	while (p && *p) {
		TCHAR *p2;
		if (*p != '\"')
			break;
		p++;
		p2 = p;
		while (*p2 != '\"' && *p2 != 0)
			p2++;
		if (*p2 == '\"') {
			*p2++ = 0;
			nextp = p2 + 1;
			while (*nextp == ' ')
				nextp++;
		}
		cfgfile_parse_line (&changed_prefs, p, 0);
		p = nextp;
	}
	xfree(buf);
	return 0;
}

static int handle_input_event (int nr, int state, int max, int autofire, bool canstopplayback, bool playbackevent)
{
	struct inputevent *ie;
	int joy;
	bool isaks = false;

	if (nr <= 0)
		return 0;
	ie = &events[nr];
	if (ie->unit == 0 && ie->data >= 0x200)
		isaks = true;

	if (!isaks) {
		if (input_record && input_record != INPREC_RECORD_PLAYING)
			inprec_recordevent (nr, state, max, autofire);
		if (input_play && state && canstopplayback) {
			if (inprec_realtime ()) {
				if (input_record && input_record != INPREC_RECORD_PLAYING)
					inprec_recordevent (nr, state, max, autofire);
			}
		}
		if (!playbackevent && input_play)
			return 0;
	}

	if ((inputdevice_logging & 1) || input_record || input_play)
		write_log (_T("'%s' STATE=%d MAX=%d AF=%d\n"), ie->name, state, max, autofire);
	if (autofire) {
		if (state)
			queue_input_event (nr, state, max, currprefs.input_autofire_linecnt, 1);
		else
			queue_input_event (nr, -1, 0, 0, 1);
	}
	switch (ie->unit)
	{
	case 5: /* lightpen/gun */
		{
			if (lightpen_x < 0 && lightpen_y < 0) {
				lightpen_x = gfxvidinfo.width / 2;
				lightpen_y = gfxvidinfo.height / 2;
			}
			if (ie->type == 0) {
				int delta = 0;
				if (max == 0)
					delta = state * currprefs.input_mouse_speed / 100;
				else if (state > 0)
					delta = currprefs.input_joymouse_speed;
				else if (state < 0)
					delta = -currprefs.input_joymouse_speed;
				if (ie->data)
					lightpen_y += delta;
				else
					lightpen_x += delta;
			} else {
				int delta = currprefs.input_joymouse_speed;
				if (ie->data & DIR_LEFT)
					lightpen_x -= delta;
				if (ie->data & DIR_RIGHT)
					lightpen_x += delta;
				if (ie->data & DIR_UP)
					lightpen_y -= delta;
				if (ie->data & DIR_DOWN)
					lightpen_y += delta;
			}
		}
		break;
	case 1: /* ->JOY1 */
	case 2: /* ->JOY2 */
	case 3: /* ->Parallel port joystick adapter port #1 */
	case 4: /* ->Parallel port joystick adapter port #2 */
		joy = ie->unit - 1;
		if (ie->type & 4) {
			int old = joybutton[joy] & (1 << ie->data);

			if (state) {
				joybutton[joy] |= 1 << ie->data;
				gui_gameport_button_change (joy, ie->data, 1);
			} else {
				joybutton[joy] &= ~(1 << ie->data);
				gui_gameport_button_change (joy, ie->data, 0);
			}

			if (ie->data == 0 && old != (joybutton[joy] & (1 << ie->data)) && currprefs.cpu_cycle_exact) {
				if (!input_record && !input_play && currprefs.input_contact_bounce) {
					// emulate contact bounce, 1st button only, others have capacitors
					bouncy = 1;
					bouncy_cycles = get_cycles () + CYCLE_UNIT * currprefs.input_contact_bounce;
				}
			}


		} else if (ie->type & 8) {

			/* real mouse / analog stick mouse emulation */
			int delta;
			int deadzone = currprefs.input_joymouse_deadzone * max / 100;
			int unit = ie->data & 0x7f;

			if (max) {
				if (state <= deadzone && state >= -deadzone) {
					state = 0;
					mouse_deltanoreset[joy][unit] = 0;
				} else if (state < 0) {
					state += deadzone;
					mouse_deltanoreset[joy][unit] = 1;
				} else {
					state -= deadzone;
					mouse_deltanoreset[joy][unit] = 1;
				}
				max -= deadzone;
				delta = state * currprefs.input_joymouse_multiplier / max;
			} else {
				delta = state;
				mouse_deltanoreset[joy][unit] = 0;
			}
			if (ie->data & IE_CDTV) {
				delta = 0;
				if (state > 0)
					delta = JOYMOUSE_CDTV;
				else if (state < 0)
					delta = -JOYMOUSE_CDTV;
			}

			if (ie->data & IE_INVERT)
				delta = -delta;
			mouse_delta[joy][unit] += delta;

			max = 32;
			if (unit) {
				if (delta < 0) {
					gui_gameport_axis_change (joy, DIR_UP_BIT, abs (delta), max);
					gui_gameport_axis_change (joy, DIR_DOWN_BIT, 0, max);
				}
				if (delta > 0) {
					gui_gameport_axis_change (joy, DIR_DOWN_BIT, abs (delta), max);
					gui_gameport_axis_change (joy, DIR_UP_BIT, 0, max);
				}
			} else {
				if (delta < 0) {
					gui_gameport_axis_change (joy, DIR_LEFT_BIT, abs (delta), max);
					gui_gameport_axis_change (joy, DIR_RIGHT_BIT, 0, max);
				}
				if (delta > 0) {
					gui_gameport_axis_change (joy, DIR_RIGHT_BIT, abs (delta), max);
					gui_gameport_axis_change (joy, DIR_LEFT_BIT, 0, max);
				}
			}

		} else if (ie->type & 32) { /* button mouse emulation vertical */

			int speed = (ie->data & IE_CDTV) ? JOYMOUSE_CDTV : currprefs.input_joymouse_speed;

			if (state && (ie->data & DIR_UP)) {
				mouse_delta[joy][1] = -speed;
				mouse_deltanoreset[joy][1] = 1;
			} else if (state && (ie->data & DIR_DOWN)) {
				mouse_delta[joy][1] = speed;
				mouse_deltanoreset[joy][1] = 1;
			} else
				mouse_deltanoreset[joy][1] = 0;

		} else if (ie->type & 64) { /* button mouse emulation horizontal */

			int speed = (ie->data & IE_CDTV) ? JOYMOUSE_CDTV : currprefs.input_joymouse_speed;

			if (state && (ie->data & DIR_LEFT)) {
				mouse_delta[joy][0] = -speed;
				mouse_deltanoreset[joy][0] = 1;
			} else if (state && (ie->data & DIR_RIGHT)) {
				mouse_delta[joy][0] = speed;
				mouse_deltanoreset[joy][0] = 1;
			} else
				mouse_deltanoreset[joy][0] = 0;

		} else if (ie->type & 128) { /* analog joystick / paddle */

			int deadzone = currprefs.input_joymouse_deadzone * max / 100;
			int unit = ie->data & 0x7f;
			if (max) {
				if (state <= deadzone && state >= -deadzone) {
					state = 0;
				} else if (state < 0) {
					state += deadzone;
				} else {
					state -= deadzone;
				}
				state = state * max / (max - deadzone);
			}
			if (ie->data & IE_INVERT)
				state = -state;

			if (!unit) {
				if (state <= 0)
					gui_gameport_axis_change (joy, DIR_UP_BIT, abs (state), max);
				if (state >= 0)
					gui_gameport_axis_change (joy, DIR_DOWN_BIT, abs (state), max);
			} else {
				if (state <= 0)
					gui_gameport_axis_change (joy, DIR_LEFT_BIT, abs (state), max);
				if (state >= 0)
					gui_gameport_axis_change (joy, DIR_RIGHT_BIT, abs (state), max);
			}

			state = state * currprefs.input_analog_joystick_mult / max;
			state += (128 * currprefs.input_analog_joystick_mult / 100) + currprefs.input_analog_joystick_offset;
			if (state < 0)
				state = 0;
			if (state > 255)
				state = 255;
			joydirpot[joy][unit] = state;
			mouse_deltanoreset[joy][0] = 1;
			mouse_deltanoreset[joy][1] = 1;

		} else {

			int left = oleft[joy], right = oright[joy], top = otop[joy], bot = obot[joy];
			if (ie->type & 16) {
				/* button to axis mapping */
				if (ie->data & DIR_LEFT) {
					left = oleft[joy] = state ? 1 : 0;
					if (horizclear[joy] && left) {
						horizclear[joy] = 0;
						right = oright[joy] = 0;
					}
				}
				if (ie->data & DIR_RIGHT) {
					right = oright[joy] = state ? 1 : 0;
					if (horizclear[joy] && right) {
						horizclear[joy] = 0;
						left = oleft[joy] = 0;
					}
				}
				if (ie->data & DIR_UP) {
					top = otop[joy] = state ? 1 : 0;
					if (vertclear[joy] && top) {
						vertclear[joy] = 0;
						bot = obot[joy] = 0;
					}
				}
				if (ie->data & DIR_DOWN) {
					bot = obot[joy] = state ? 1 : 0;
					if (vertclear[joy] && bot) {
						vertclear[joy] = 0;
						top = otop[joy] = 0;
					}
				}
			} else {
				/* "normal" joystick axis */
				int deadzone = currprefs.input_joystick_deadzone * max / 100;
				int neg, pos;
				if (state < deadzone && state > -deadzone)
					state = 0;
				neg = state < 0 ? 1 : 0;
				pos = state > 0 ? 1 : 0;
				if (ie->data & DIR_LEFT) {
					left = oleft[joy] = neg;
					if (horizclear[joy] && left) {
						horizclear[joy] = 0;
						right = oright[joy] = 0;
					}
				}
				if (ie->data & DIR_RIGHT) {
					right = oright[joy] = pos;
					if (horizclear[joy] && right) {
						horizclear[joy] = 0;
						left = oleft[joy] = 0;
					}
				}
				if (ie->data & DIR_UP) {
					top = otop[joy] = neg;
					if (vertclear[joy] && top) {
						vertclear[joy] = 0;
						bot = obot[joy] = 0;
					}
				}
				if (ie->data & DIR_DOWN) {
					bot = obot[joy] = pos;
					if (vertclear[joy] && bot) {
						vertclear[joy] = 0;
						top = otop[joy] = 0;
					}
				}
			}
			mouse_deltanoreset[joy][0] = 1;
			mouse_deltanoreset[joy][1] = 1;
			joydir[joy] = 0;
			if (left)
				joydir[joy] |= DIR_LEFT;
			if (right)
				joydir[joy] |= DIR_RIGHT;
			if (top)
				joydir[joy] |= DIR_UP;
			if (bot)
				joydir[joy] |= DIR_DOWN;
			if (joy == 0 || joy == 1)
				joymousecounter (joy); 

			gui_gameport_axis_change (joy, DIR_LEFT_BIT, left, 0);
			gui_gameport_axis_change (joy, DIR_RIGHT_BIT, right, 0);
			gui_gameport_axis_change (joy, DIR_UP_BIT, top, 0);
			gui_gameport_axis_change (joy, DIR_DOWN_BIT, bot, 0);
		}
		break;
	case 0: /* ->KEY */
		inputdevice_do_keyboard (ie->data, state);
		break;
	}
	return 1;
}

static void inputdevice_checkconfig (void)
{
	if (currprefs.jports[0].id != changed_prefs.jports[0].id ||
		currprefs.jports[1].id != changed_prefs.jports[1].id ||
		currprefs.jports[2].id != changed_prefs.jports[2].id ||
		currprefs.jports[3].id != changed_prefs.jports[3].id ||
		currprefs.input_selected_setting != changed_prefs.input_selected_setting ||
		currprefs.input_joymouse_multiplier != changed_prefs.input_joymouse_multiplier ||
		currprefs.input_joymouse_deadzone != changed_prefs.input_joymouse_deadzone ||
		currprefs.input_joystick_deadzone != changed_prefs.input_joystick_deadzone ||
		currprefs.input_joymouse_speed != changed_prefs.input_joymouse_speed ||
		currprefs.input_autofire_linecnt != changed_prefs.input_autofire_linecnt ||
		currprefs.input_mouse_speed != changed_prefs.input_mouse_speed) {

			currprefs.input_selected_setting = changed_prefs.input_selected_setting;
			currprefs.input_joymouse_multiplier = changed_prefs.input_joymouse_multiplier;
			currprefs.input_joymouse_deadzone = changed_prefs.input_joymouse_deadzone;
			currprefs.input_joystick_deadzone = changed_prefs.input_joystick_deadzone;
			currprefs.input_joymouse_speed = changed_prefs.input_joymouse_speed;
			currprefs.input_autofire_linecnt = changed_prefs.input_autofire_linecnt;
			currprefs.input_mouse_speed = changed_prefs.input_mouse_speed;

			inputdevice_updateconfig (&currprefs);
	}
	if (currprefs.dongle != changed_prefs.dongle) {
		currprefs.dongle = changed_prefs.dongle;
		dongle_reset ();
	}
}

void inputdevice_vsync (void)
{
	if (inputdevice_logging & 32)
		write_log (_T("*\n"));

	input_frame++;
	mouseupdate (0, 1);

	if (!input_record) {
		inputdevice_read ();
		if (!input_play)
			inputdelay = uaerand () % (maxvpos <= 1 ? 1 : maxvpos - 1);
	}

	inputdevice_handle_inputcode ();
	if (mouseedge_alive > 0)
		mouseedge_alive--;
#ifdef ARCADIA
	if (arcadia_bios)
		arcadia_vsync ();
#endif
	if (mouseedge ())
		mouseedge_alive = 10;
	if (mousehack_alive_cnt > 0) {
		mousehack_alive_cnt--;
		if (mousehack_alive_cnt == 0)
			setmouseactive (-1);
	} else if (mousehack_alive_cnt < 0) {
		mousehack_alive_cnt++;
		if (mousehack_alive_cnt == 0) {
			mousehack_alive_cnt = 100;
			setmouseactive (0);
			setmouseactive (1);
		}
	}
	inputdevice_checkconfig ();
}

void inputdevice_reset (void)
{
	magicmouse_ibase = 0;
	magicmouse_gfxbase = 0;
	mousehack_reset ();
	if (inputdevice_is_tablet ())
		mousehack_enable ();
	bouncy = 0;
}

static int getoldport (struct uae_input_device *id)
{
	int i, j;

	for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			int evt = id->eventid[i][j];
			if (evt > 0) {
				int unit = events[evt].unit;
				if (unit >= 1 && unit <= 4)
					return unit;
			}
		}
	}
	return -1;
}

static int switchdevice (struct uae_input_device *id, int num, int button)
{
	int i, j;
	int ismouse = 0;
	int newport = 0;
	int flags = 0;
	TCHAR *name = NULL;
	int otherbuttonpressed = 0;

	if (num >= 4)
		return 0;
	if (!button)
		return 0;
	for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		if (id == &joysticks[i]) {
			name = idev[IDTYPE_JOYSTICK].get_uniquename (i);
			newport = num == 0 ? 1 : 0;
			flags = idev[IDTYPE_JOYSTICK].get_flags (i);
			for (j = 0; j < MAX_INPUT_DEVICES; j++) {
				if (j != i) {
					struct uae_input_device2 *id2 = &joysticks2[j];
					if (id2->buttonmask)
						otherbuttonpressed = 1;
				}
			}
		}
		if (id == &mice[i]) {
			ismouse = 1;
			name = idev[IDTYPE_MOUSE].get_uniquename (i);
			newport = num == 0 ? 0 : 1;
			flags = idev[IDTYPE_MOUSE].get_flags (i);
		}
	}
	if (!name)
		return 0;
	if (num == 0 && otherbuttonpressed)
		newport = newport ? 0 : 1;
	if (currprefs.input_selected_setting == GAMEPORT_INPUT_SETTINGS) {
		if ((num == 0 || num == 1) && currprefs.jports[newport].id != JPORT_CUSTOM) {
			int om = jsem_ismouse (num, &currprefs);
			int om1 = jsem_ismouse (0, &currprefs);
			int om2 = jsem_ismouse (1, &currprefs);
			if ((om1 >= 0 || om2 >= 0) && ismouse)
				return 0;
			if (flags)
				return 0;
			if (name) {
				write_log (_T("inputdevice change '%s':%d->%d\n"), name, num, newport);
				inputdevice_joyport_config (&changed_prefs, name, newport, -1, 2);
				inputdevice_copyconfig (&changed_prefs, &currprefs);
				return 1;
			}
		}
		return 0;
	} else {
		int oldport = getoldport (id);
		int k, evt;
		struct inputevent *ie, *ie2;

		if (flags)
			return 0;
		if (oldport <= 0)
			return 0;
		newport++;
		/* do not switch if switching mouse and any "supermouse" mouse enabled */
		if (ismouse) {
			for (i = 0; i < MAX_INPUT_SETTINGS; i++) {
				if (mice[i].enabled && idev[IDTYPE_MOUSE].get_flags (i))
					return 0;
			}
		}
		for (i = 0; i < MAX_INPUT_SETTINGS; i++) {
			if (getoldport (&joysticks[i]) == newport)
				joysticks[i].enabled = 0;
			if (getoldport (&mice[i]) == newport)
				mice[i].enabled = 0;
		}   
		id->enabled = 1;
		for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
			for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
				evt = id->eventid[i][j];
				if (evt <= 0)
					continue;
				ie = &events[evt];
				if (ie->unit == oldport) {
					k = 1;
					while (events[k].confname) {
						ie2 = &events[k];
						if (ie2->type == ie->type && ie2->data == ie->data && ie2->allow_mask == ie->allow_mask && ie2->unit == newport) {
							id->eventid[i][j] = k;
							break;
						}
						k++;
					}
				} else if (ie->unit == newport) {
					k = 1;
					while (events[k].confname) {
						ie2 = &events[k];
						if (ie2->type == ie->type && ie2->data == ie->data && ie2->allow_mask == ie->allow_mask && ie2->unit == oldport) {
							id->eventid[i][j] = k;
							break;
						}
						k++;
					}
				}
			}
		}
		write_log (_T("inputdevice change '%s':%d->%d\n"), name, num, newport);
		inputdevice_copyconfig (&currprefs, &changed_prefs);
		inputdevice_copyconfig (&changed_prefs, &currprefs);
		return 1;
	}
	return 0;
}

static void process_custom_event (struct uae_input_device *id, int offset, int state)
{
	int idx = -1;
	int custompos = (id->flags[offset][0] >> 15) & 1;
	TCHAR *custom;

	if (state < 0) {
		idx = 0;
		custompos = 0;
	} else {
		idx = state > 0 ? 0 : 1;
		if (custompos)
			idx += 2;
		if (state == 0)
			custompos ^= 1;
	}
	custom = id->custom[offset][idx];
	if (custom == NULL) {
		if (idx >= 2)
			custom = id->custom[offset][idx - 2];
	}
	handle_custom_event (custom);
	id->flags[offset][0] &= ~0x8000;
	id->flags[offset][0] |= custompos << 15;
}

static void setbuttonstateall (struct uae_input_device *id, struct uae_input_device2 *id2, int button, int state)
{
	int i;
	uae_u32 mask = 1 << button;
	uae_u32 omask = id2->buttonmask & mask;
	uae_u32 nmask = (state ? 1 : 0) << button;

	if (input_play && state)
		inprec_realtime ();
	if (input_play)
		return;
	if (!id->enabled) {
		if (state)
			switchdevice (id, button, 1);
		return;
	}
	if (button >= ID_BUTTON_TOTAL)
		return;

	for (i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
		int evt = evt = id->eventid[ID_BUTTON_OFFSET + button][sublevdir[state <= 0 ? 1 : 0][i]];
		int autofire = (id->flags[ID_BUTTON_OFFSET + button][sublevdir[state <= 0 ? 1 : 0][i]] & ID_FLAG_AUTOFIRE) ? 1 : 0;
		int toggle = (id->flags[ID_BUTTON_OFFSET + button][sublevdir[state <= 0 ? 1 : 0][i]] & ID_FLAG_TOGGLE) ? 1 : 0;

		if (state < 0) {
			handle_input_event (evt, 1, 1, 0, true, false);
			queue_input_event (evt, 0, 1, 1, 0); /* send release event next frame */
			if (i == 0)
				process_custom_event (id, ID_BUTTON_OFFSET + button, state);
		} else if (toggle) {
			int toggled;
			if (!state)
				continue;
			if (omask & mask)
				continue;
			id->flags[ID_BUTTON_OFFSET + button][sublevdir[state <= 0 ? 1 : 0][i]] ^= ID_FLAG_TOGGLED;
			toggled = (id->flags[ID_BUTTON_OFFSET + button][sublevdir[state <= 0 ? 1 : 0][i]] & ID_FLAG_TOGGLED) ? 1 : 0;
			handle_input_event (evt, toggled, 1, autofire, true, false);
			if (i == 0)
				process_custom_event (id, ID_BUTTON_OFFSET + button, toggled);
		} else {
			if ((omask ^ nmask) & mask) {
				handle_input_event (evt, state, 1, autofire, true, false);
				if (i == 0)
					process_custom_event (id, ID_BUTTON_OFFSET + button, state);
			}
		}
	}

	if ((omask ^ nmask) & mask) {
		if (state)
			id2->buttonmask |= mask;
		else
			id2->buttonmask &= ~mask;
	}
}


/* - detect required number of joysticks and mice from configuration data
* - detect if CD32 pad emulation is needed
* - detect device type in ports (mouse or joystick)
*/

static int iscd32 (int ei)
{
	if (ei >= INPUTEVENT_JOY1_CD32_FIRST && ei <= INPUTEVENT_JOY1_CD32_LAST) {
		cd32_pad_enabled[0] = 1;
		return 1;
	}
	if (ei >= INPUTEVENT_JOY2_CD32_FIRST && ei <= INPUTEVENT_JOY2_CD32_LAST) {
		cd32_pad_enabled[1] = 1;
		return 2;
	}
	return 0;
}

static int isparport (int ei)
{
	if (ei > INPUTEVENT_PAR_JOY1_START && ei < INPUTEVENT_PAR_JOY_END) {
		parport_joystick_enabled = 1;
		return 1;
	}
	return 0;
}

static int ismouse (int ei)
{
	if (ei >= INPUTEVENT_MOUSE1_FIRST && ei <= INPUTEVENT_MOUSE1_LAST) {
		mouse_port[0] = 1;
		return 1;
	}
	if (ei >= INPUTEVENT_MOUSE2_FIRST && ei <= INPUTEVENT_MOUSE2_LAST) {
		mouse_port[1] = 1;
		return 2;
	}
	return 0;
}

static int isanalog (int ei)
{
	if (ei == INPUTEVENT_JOY1_HORIZ_POT || ei == INPUTEVENT_JOY1_HORIZ_POT_INV) {
		analog_port[0][0] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY1_VERT_POT || ei == INPUTEVENT_JOY1_VERT_POT_INV) {
		analog_port[0][1] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_HORIZ_POT || ei == INPUTEVENT_JOY2_HORIZ_POT_INV) {
		analog_port[1][0] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_VERT_POT || ei == INPUTEVENT_JOY2_VERT_POT_INV) {
		analog_port[1][1] = 1;
		return 1;
	}
	return 0;
}

static int isdigitalbutton (int ei)
{
	if (ei == INPUTEVENT_JOY1_2ND_BUTTON) {
		digital_port[0][1] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY1_3RD_BUTTON) {
		digital_port[0][0] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_2ND_BUTTON) {
		digital_port[1][1] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_3RD_BUTTON) {
		digital_port[1][0] = 1;
		return 1;
	}
	return 0;
}

static void scanevents (struct uae_prefs *p)
{
	int i, j, k, ei;
	const struct inputevent *e;
	int n_joy = idev[IDTYPE_JOYSTICK].get_num();
	int n_mouse = idev[IDTYPE_MOUSE].get_num();

	cd32_pad_enabled[0] = cd32_pad_enabled[1] = 0;
	parport_joystick_enabled = 0;
	mouse_port[0] = mouse_port[1] = 0;

	for (i = 0; i < NORMAL_JPORTS; i++) {
		for (j = 0; j < 2; j++) {
			digital_port[i][j] = 0;
			analog_port[i][j] = 0;
			joydirpot[i][j] = 128 / (312 * 100 / currprefs.input_analog_joystick_mult) + (128 * currprefs.input_analog_joystick_mult / 100) + currprefs.input_analog_joystick_offset;
		}
	}

	for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		use_joysticks[i] = 0;
		use_mice[i] = 0;
		for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {
			for (j = 0; j < ID_BUTTON_TOTAL; j++) {

				if ((joysticks[i].enabled && i < n_joy) || joysticks[i].enabled < 0) {
					ei = joysticks[i].eventid[ID_BUTTON_OFFSET + j][k];
					e = &events[ei];
					iscd32 (ei);
					isparport (ei);
					ismouse (ei);
					isdigitalbutton (ei);
					if (joysticks[i].eventid[ID_BUTTON_OFFSET + j][k] > 0)
						use_joysticks[i] = 1;
				}
				if ((mice[i].enabled && i < n_mouse) || mice[i].enabled < 0) {
					ei = mice[i].eventid[ID_BUTTON_OFFSET + j][k];
					e = &events[ei];
					iscd32 (ei);
					isparport (ei);
					ismouse (ei);
					isdigitalbutton (ei);
					if (mice[i].eventid[ID_BUTTON_OFFSET + j][k] > 0)
						use_mice[i] = 1;
				}

			}

			for (j = 0; j < ID_AXIS_TOTAL; j++) {

				if ((joysticks[i].enabled && i < n_joy) || joysticks[i].enabled < 0) {
					ei = joysticks[i].eventid[ID_AXIS_OFFSET + j][k];
					iscd32 (ei);
					isparport (ei);
					ismouse (ei);
					isanalog (ei);
					isdigitalbutton (ei);
					if (ei > 0)
						use_joysticks[i] = 1;
				}
				if ((mice[i].enabled && i < n_mouse) || mice[i].enabled < 0) {
					ei = mice[i].eventid[ID_AXIS_OFFSET + j][k];
					iscd32 (ei);
					isparport (ei);
					ismouse (ei);
					isanalog (ei);
					isdigitalbutton (ei);
					if (ei > 0)
						use_mice[i] = 1;
				}
			}
		}
	}
	memset (scancodeused, 0, sizeof scancodeused);
	for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		use_keyboards[i] = 0;
		if (keyboards[i].enabled && i < idev[IDTYPE_KEYBOARD].get_num()) {
			j = 0;
			while (j < MAX_INPUT_DEVICE_EVENTS && keyboards[i].extra[j] >= 0) {
				use_keyboards[i] = 1;
				for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {
					ei = keyboards[i].eventid[j][k];
					iscd32 (ei);
					isparport (ei);
					ismouse (ei);
					isdigitalbutton (ei);
					if (ei > 0)
						scancodeused[i][keyboards[i].extra[j]] = ei;
				}
				j++;
			}
		}
	}
}

static int axistable[] = {
	INPUTEVENT_MOUSE1_HORIZ, INPUTEVENT_MOUSE1_LEFT, INPUTEVENT_MOUSE1_RIGHT,
	INPUTEVENT_MOUSE1_VERT, INPUTEVENT_MOUSE1_UP, INPUTEVENT_MOUSE1_DOWN,
	INPUTEVENT_MOUSE2_HORIZ, INPUTEVENT_MOUSE2_LEFT, INPUTEVENT_MOUSE2_RIGHT,
	INPUTEVENT_MOUSE2_VERT, INPUTEVENT_MOUSE2_UP, INPUTEVENT_MOUSE2_DOWN,
	INPUTEVENT_JOY1_HORIZ, INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT,
	INPUTEVENT_JOY1_VERT, INPUTEVENT_JOY1_UP, INPUTEVENT_JOY1_DOWN,
	INPUTEVENT_JOY2_HORIZ, INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT,
	INPUTEVENT_JOY2_VERT, INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN,
	INPUTEVENT_LIGHTPEN_HORIZ, INPUTEVENT_LIGHTPEN_LEFT, INPUTEVENT_LIGHTPEN_RIGHT,
	INPUTEVENT_LIGHTPEN_VERT, INPUTEVENT_LIGHTPEN_UP, INPUTEVENT_LIGHTPEN_DOWN,
	INPUTEVENT_PAR_JOY1_HORIZ, INPUTEVENT_PAR_JOY1_LEFT, INPUTEVENT_PAR_JOY1_RIGHT,
	INPUTEVENT_PAR_JOY1_VERT, INPUTEVENT_PAR_JOY1_UP, INPUTEVENT_PAR_JOY1_DOWN,
	INPUTEVENT_PAR_JOY2_HORIZ, INPUTEVENT_PAR_JOY2_LEFT, INPUTEVENT_PAR_JOY2_RIGHT,
	INPUTEVENT_PAR_JOY2_VERT, INPUTEVENT_PAR_JOY2_UP, INPUTEVENT_PAR_JOY2_DOWN,
	INPUTEVENT_MOUSE_CDTV_HORIZ, INPUTEVENT_MOUSE_CDTV_LEFT, INPUTEVENT_MOUSE_CDTV_RIGHT,
	INPUTEVENT_MOUSE_CDTV_VERT, INPUTEVENT_MOUSE_CDTV_UP, INPUTEVENT_MOUSE_CDTV_DOWN,
	-1
};

int intputdevice_compa_get_eventtype (int evt, int **axistablep)
{
	for (int i = 0; axistable[i] >= 0; i += 3) {
		*axistablep = &axistable[i];
		if (axistable[i] == evt)
			return IDEV_WIDGET_AXIS;
		if (axistable[i + 1] == evt)
			return IDEV_WIDGET_BUTTONAXIS;
		if (axistable[i + 2] == evt)
			return IDEV_WIDGET_BUTTONAXIS;
	}
	*axistablep = NULL;
	return IDEV_WIDGET_BUTTON;
}

static int rem_port1[] = {
	INPUTEVENT_MOUSE1_HORIZ, INPUTEVENT_MOUSE1_VERT,
	INPUTEVENT_JOY1_HORIZ, INPUTEVENT_JOY1_VERT,
	INPUTEVENT_JOY1_HORIZ_POT, INPUTEVENT_JOY1_VERT_POT,
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON, INPUTEVENT_JOY1_3RD_BUTTON,
	INPUTEVENT_JOY1_CD32_RED, INPUTEVENT_JOY1_CD32_BLUE, INPUTEVENT_JOY1_CD32_GREEN, INPUTEVENT_JOY1_CD32_YELLOW,
	INPUTEVENT_JOY1_CD32_RWD, INPUTEVENT_JOY1_CD32_FFW, INPUTEVENT_JOY1_CD32_PLAY,
	INPUTEVENT_MOUSE_CDTV_HORIZ, INPUTEVENT_MOUSE_CDTV_VERT,
	INPUTEVENT_LIGHTPEN_HORIZ, INPUTEVENT_LIGHTPEN_VERT,
	-1
};
static int rem_port2[] = {
	INPUTEVENT_MOUSE2_HORIZ, INPUTEVENT_MOUSE2_VERT,
	INPUTEVENT_JOY2_HORIZ, INPUTEVENT_JOY2_VERT,
	INPUTEVENT_JOY2_HORIZ_POT, INPUTEVENT_JOY2_VERT_POT,
	INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_2ND_BUTTON, INPUTEVENT_JOY2_3RD_BUTTON,
	INPUTEVENT_JOY2_CD32_RED, INPUTEVENT_JOY2_CD32_BLUE, INPUTEVENT_JOY2_CD32_GREEN, INPUTEVENT_JOY2_CD32_YELLOW,
	INPUTEVENT_JOY2_CD32_RWD, INPUTEVENT_JOY2_CD32_FFW, INPUTEVENT_JOY2_CD32_PLAY,
	-1, -1,
	-1, -1,
	-1
};
static int rem_port3[] = {
	INPUTEVENT_PAR_JOY1_LEFT, INPUTEVENT_PAR_JOY1_RIGHT, INPUTEVENT_PAR_JOY1_UP, INPUTEVENT_PAR_JOY1_DOWN,
	INPUTEVENT_PAR_JOY1_FIRE_BUTTON,
	-1
};
static int rem_port4[] = {
	INPUTEVENT_PAR_JOY2_LEFT, INPUTEVENT_PAR_JOY2_RIGHT, INPUTEVENT_PAR_JOY2_UP, INPUTEVENT_PAR_JOY2_DOWN,
	INPUTEVENT_PAR_JOY2_FIRE_BUTTON,
	-1
};

static int *rem_ports[] = { rem_port1, rem_port2, rem_port3, rem_port4 };
static int af_port1[] = {
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_CD32_RED,
	-1
};
static int af_port2[] = {
	INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_CD32_RED,
	-1
};
static int af_port3[] = {
	INPUTEVENT_PAR_JOY1_FIRE_BUTTON,
	-1
};
static int af_port4[] = {
	INPUTEVENT_PAR_JOY2_FIRE_BUTTON,
	-1
};
static int *af_ports[] = { af_port1, af_port2, af_port3, af_port4 };
static int ip_joy1[] = {
	INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT, INPUTEVENT_JOY1_UP, INPUTEVENT_JOY1_DOWN,
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON,
	-1
};
static int ip_joy2[] = {
	INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT, INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN,
	INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_2ND_BUTTON,
	-1
};
static int ip_joypad1[] = {
	INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT, INPUTEVENT_JOY1_UP, INPUTEVENT_JOY1_DOWN,
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON, INPUTEVENT_JOY1_3RD_BUTTON,
	-1
};
static int ip_joypad2[] = {
	INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT, INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN,
	INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_2ND_BUTTON, INPUTEVENT_JOY2_3RD_BUTTON,
	-1
};
static int ip_joycd321[] = {
	INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT, INPUTEVENT_JOY1_UP, INPUTEVENT_JOY1_DOWN,
	INPUTEVENT_JOY1_CD32_RED, INPUTEVENT_JOY1_CD32_BLUE, INPUTEVENT_JOY1_CD32_GREEN, INPUTEVENT_JOY1_CD32_YELLOW,
	INPUTEVENT_JOY1_CD32_RWD, INPUTEVENT_JOY1_CD32_FFW, INPUTEVENT_JOY1_CD32_PLAY,
	-1
};
static int ip_joycd322[] = {
	INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT, INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN,
	INPUTEVENT_JOY2_CD32_RED, INPUTEVENT_JOY2_CD32_BLUE, INPUTEVENT_JOY2_CD32_GREEN, INPUTEVENT_JOY2_CD32_YELLOW,
	INPUTEVENT_JOY2_CD32_RWD, INPUTEVENT_JOY2_CD32_FFW, INPUTEVENT_JOY2_CD32_PLAY,
	-1
};
static int ip_parjoy1[] = {
	INPUTEVENT_PAR_JOY1_LEFT, INPUTEVENT_PAR_JOY1_RIGHT, INPUTEVENT_PAR_JOY1_UP, INPUTEVENT_PAR_JOY1_DOWN,
	INPUTEVENT_PAR_JOY1_FIRE_BUTTON,
	-1
};
static int ip_parjoy2[] = {
	INPUTEVENT_PAR_JOY2_LEFT, INPUTEVENT_PAR_JOY2_RIGHT, INPUTEVENT_PAR_JOY2_UP, INPUTEVENT_PAR_JOY2_DOWN,
	INPUTEVENT_PAR_JOY2_FIRE_BUTTON,
	-1
};
static int ip_mouse1[] = {
	INPUTEVENT_MOUSE1_LEFT, INPUTEVENT_MOUSE1_RIGHT, INPUTEVENT_MOUSE1_UP, INPUTEVENT_MOUSE1_DOWN,
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON,
	-1
};
static int ip_mouse2[] = {
	INPUTEVENT_MOUSE2_LEFT, INPUTEVENT_MOUSE2_RIGHT, INPUTEVENT_MOUSE2_UP, INPUTEVENT_MOUSE2_DOWN,
	INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_2ND_BUTTON,
	-1
};
static int ip_mousecdtv[] =
{
	INPUTEVENT_MOUSE_CDTV_LEFT, INPUTEVENT_MOUSE_CDTV_RIGHT, INPUTEVENT_MOUSE_CDTV_UP, INPUTEVENT_MOUSE_CDTV_DOWN,
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON,
	-1
};
static int ip_mediacdtv[] =
{
	INPUTEVENT_KEY_CDTV_PLAYPAUSE, INPUTEVENT_KEY_CDTV_STOP, INPUTEVENT_KEY_CDTV_PREV, INPUTEVENT_KEY_CDTV_NEXT,
	-1
};
static int ip_arcadia[] = {
	INPUTEVENT_SPC_ARCADIA_DIAGNOSTICS, INPUTEVENT_SPC_ARCADIA_PLAYER1, INPUTEVENT_SPC_ARCADIA_PLAYER2,
	INPUTEVENT_SPC_ARCADIA_COIN1, INPUTEVENT_SPC_ARCADIA_COIN2,
	-1
};
static int ip_lightpen1[] = {
	INPUTEVENT_LIGHTPEN_HORIZ, INPUTEVENT_LIGHTPEN_VERT, INPUTEVENT_JOY1_3RD_BUTTON,
	-1
};
static int ip_lightpen2[] = {
	INPUTEVENT_LIGHTPEN_HORIZ, INPUTEVENT_LIGHTPEN_VERT, INPUTEVENT_JOY2_3RD_BUTTON,
	-1
};
static int ip_analog1[] = {
	INPUTEVENT_JOY1_HORIZ_POT, INPUTEVENT_JOY1_VERT_POT, INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT,
	-1
};
static int ip_analog2[] = {
	INPUTEVENT_JOY2_HORIZ_POT, INPUTEVENT_JOY2_VERT_POT, INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT,
	-1
};

static int ip_arcadiaxa[] = {
	-1
};

static void checkcompakb (int *kb, int *srcmap)
{
	int found = 0, avail = 0;
	int j, k;

	DebOut("entered(%lx, %lx)\n", kb, srcmap);

	k = j = 0;
	while (kb[j] >= 0) {
		struct uae_input_device *uid = &keyboards[0];
		while (kb[j] >= 0 && srcmap[k] >= 0) {
			int id = kb[j];
			for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
				if (uid->extra[l] == id) {
					avail++;
					if (uid->eventid[l][0] == srcmap[k])
						found++;
					break;
				}
			}
			j++;
		}
		if (srcmap[k] < 0)
			break;
		j++;
		k++;
	}
	if (avail != found || avail == 0)
		return;
	k = j = 0;
	while (kb[j] >= 0) {
		struct uae_input_device *uid = &keyboards[0];
		while (kb[j] >= 0) {
			int id = kb[j];
			int evt = 0;
			k = 0;
			while (keyboard_default[k].scancode >= 0) {
				if (keyboard_default[k].scancode == kb[j]) {
					evt = keyboard_default[k].evt;
					break;
				}
				k++;
			}
			for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
				if (uid->extra[l] == id) {
					uid->eventid[l][0] = evt;
					break;
				}
			}
			j++;
		}
		j++;
	}
}

static void setautofireevent (struct uae_input_device *uid, int num, int sub, int af, int index)
{
	if (!af)
		return;
	int *afp = af_ports[index];
	for (int k = 0; afp[k] >= 0; k++) {
		if (afp[k] == uid->eventid[num][sub]) {
			uid->flags[num][sub] &= ~(ID_FLAG_AUTOFIRE | ID_FLAG_TOGGLE);
			if (af >= JPORT_AF_NORMAL)
				uid->flags[num][sub] |= ID_FLAG_AUTOFIRE;
			if (af == JPORT_AF_TOGGLE)
				uid->flags[num][sub] |= ID_FLAG_TOGGLE;
			return;
		}
	}
}

static void sparerestore (struct uae_input_device *uid, int num, int sub)
{
	uid->eventid[num][sub] = uid->eventid[num][SPARE_SUB_EVENT];
	uid->flags[num][sub] = uid->flags[num][SPARE_SUB_EVENT];
	uid->custom[num][sub] = uid->custom[num][SPARE_SUB_EVENT];
	uid->eventid[num][SPARE_SUB_EVENT] = 0;
	uid->flags[num][SPARE_SUB_EVENT] = 0;
	uid->port[num][SPARE_SUB_EVENT] = 0;
	uid->custom[num][SPARE_SUB_EVENT] = 0;
}

static void sparecopy (struct uae_input_device *uid, int num, int sub)
{
	uid->eventid[num][SPARE_SUB_EVENT] = uid->eventid[num][sub];
	uid->flags[num][SPARE_SUB_EVENT] = uid->flags[num][sub];
	uid->port[num][SPARE_SUB_EVENT] = MAX_JPORTS + 1;
	xfree (uid->custom[num][SPARE_SUB_EVENT]);
	uid->custom[num][SPARE_SUB_EVENT] = uid->custom[num][sub];
	uid->custom[num][sub] = NULL;
}

static void setcompakb (int *kb, int *srcmap, int index, int af)
{
	int j, k;
	k = j = 0;
	while (kb[j] >= 0 && srcmap[k] >= 0) {
		while (kb[j] >= 0) {
			int id = kb[j];
			for (int m = 0; m < MAX_INPUT_DEVICES; m++) {
				struct uae_input_device *uid = &keyboards[m];
				for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
					if (uid->extra[l] == id) {
						sparecopy (uid, l, 0);
						uid->eventid[l][0] = srcmap[k];
						uid->flags[l][0] = 0;
						uid->port[l][0] = index + 1;
						xfree (uid->custom[l][0]);
						uid->custom[l][0] = NULL;
						setautofireevent (uid, l, 0, af, index);
						break;
					}
				}
			}
			j++;
		}
		j++;
		k++;
	}
}

int inputdevice_get_compatibility_input (struct uae_prefs *prefs, int index, int *typelist, int **inputlist, int **at)
{
	if (index >= MAX_JPORTS || joymodes[index] < 0)
		return 0;
	*typelist = joymodes[index];
	*inputlist = joyinputs[index];
	*at = axistable;
	int cnt = 0;
	for (int i = 0; joyinputs[index] && joyinputs[index][i] >= 0; i++, cnt++);
	return cnt;
}

static void clearevent (struct uae_input_device *uid, int evt)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (uid->eventid[i][j] == evt) {
				uid->eventid[i][j] = 0;
				uid->flags[i][j] = 0;
				xfree (uid->custom[i][j]);
				uid->custom[i][j] = NULL;
			}
		}
	}
}
static void clearkbrevent (struct uae_input_device *uid, int evt)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (uid->eventid[i][j] == evt) {
				uid->eventid[i][j] = 0;
				uid->flags[i][j] = 0;
				xfree (uid->custom[i][j]);
				uid->custom[i][j] = NULL;
				if (j == 0)
					set_kbr_default_event (uid, keyboard_default, i);
			}
		}
	}
}

static void resetjport (struct uae_prefs *prefs, int index)
{
	int *p = rem_ports[index];
	while (*p >= 0) {
		int evtnum = *p++;
		for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
			clearevent (&prefs->joystick_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
			clearevent (&prefs->mouse_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
			clearkbrevent (&prefs->keyboard_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
		}
		for (int i = 0; axistable[i] >= 0; i += 3) {
			if (evtnum == axistable[i] || evtnum == axistable[i + 1] || evtnum == axistable[i + 2]) {
				for (int j = 0; j < 3; j++) {
					int evtnum2 = axistable[i + j];
					for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
						clearevent (&prefs->joystick_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum2);
						clearevent (&prefs->mouse_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum2);
						clearkbrevent (&prefs->keyboard_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum2);
					}
				}
				break;
			}
		}
	}
}

static void remove_compa_config (struct uae_prefs *prefs, int index)
{
	int typelist, *inputlist, *atp;

	if (!inputdevice_get_compatibility_input (prefs, index, &typelist, &inputlist, &atp))
		return;
	for (int i = 0; inputlist[i] >= 0; i++) {
		int evtnum = inputlist[i];

		int atpidx = 0;
		while (*atp >= 0) {
			if (*atp == evtnum) {
				atp++;
				atpidx = 2;
				break;
			}
			if (atp[1] == evtnum || atp[2] == evtnum) {
				atpidx = 1;
				break;
			}
			atp += 3;
		}
		while (atpidx >= 0) {
			for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
				clearevent (&prefs->joystick_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
				clearevent (&prefs->mouse_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
				clearkbrevent (&prefs->keyboard_settings[GAMEPORT_INPUT_SETTINGS][l], evtnum);
			}
			evtnum = *atp++;
			atpidx--;
		}
	}
}

static void cleardevgp (struct uae_input_device *uid, int num, bool nocustom, int index)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (uid[num].port[i][j] == index + 1) {
				if (nocustom && (uid[num].flags[i][j] & ID_FLAG_GAMEPORTSCUSTOM))
					continue;
				uid[num].eventid[i][j] = 0;
				uid[num].flags[i][j] = 0;
				xfree (uid[num].custom[i][j]);
				uid[num].custom[i][j] = NULL;
				uid[num].port[i][j] = 0;
				if (uid[num].port[i][SPARE_SUB_EVENT])
					sparerestore (&uid[num], i, j);
			}
		}
	}
}
static void cleardevkbrgp (struct uae_input_device *uid, int num, bool nocustom, int index)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (uid[num].port[i][j] == index + 1) {
				if (nocustom && (uid[num].flags[i][j] & ID_FLAG_GAMEPORTSCUSTOM))
					continue;
				uid[num].eventid[i][j] = 0;
				uid[num].flags[i][j] = 0;
				xfree (uid[num].custom[i][j]);
				uid[num].custom[i][j] = NULL;
				uid[num].port[i][j] = 0;
				if (uid[num].port[i][SPARE_SUB_EVENT]) {
					sparerestore (&uid[num], i, j);
				} else if (j == 0) {
					set_kbr_default_event (&uid[num], keyboard_default, i);
				}
			}
		}
	}
}

// remove all gameports mappings mapped to port 'index'
static void remove_custom_config (struct uae_prefs *prefs, bool nocustom, int index)
{
	for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
		cleardevgp (joysticks, l, nocustom, index);
		cleardevgp (mice, l, nocustom, index);
		cleardevkbrgp (keyboards, l, nocustom, index);
	}
}

// prepare port for custom mapping, remove all current Amiga side device mappings
void inputdevice_compa_prepare_custom (struct uae_prefs *prefs, int index)
{
	int mode = prefs->jports[index].mode;
	freejport (prefs, index);
	resetjport (prefs, index);
	if (mode == 0)
		mode = index == 0 ? JSEM_MODE_MOUSE : (prefs->cs_cd32cd ? JSEM_MODE_JOYSTICK_CD32 : JSEM_MODE_JOYSTICK);
	prefs->jports[index].mode = mode;
	prefs->jports[index].id = -2;

	remove_compa_config (prefs, index);
	remove_custom_config (prefs, false, index);
}
// clear device before switching to new one
void inputdevice_compa_clear (struct uae_prefs *prefs, int index)
{
	freejport (prefs, index);
	resetjport (prefs, index);
	remove_compa_config (prefs, index);
}

static void cleardev (struct uae_input_device *uid, int num)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			uid[num].eventid[i][j] = 0;
			uid[num].flags[i][j] = 0;
			xfree (uid[num].custom[i][j]);
			uid[num].custom[i][j] = NULL;
		}
	}
}

static void enablejoydevice (struct uae_input_device *uid, bool gameportsmode, int evtnum)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if ((gameportsmode && uid->eventid[i][j] == evtnum) || uid->port[i][j] > 0) {
				uid->enabled = 1;
			}
		}
	}
}

static void setjoydevices (struct uae_prefs *prefs, bool gameportsmode, int port)
{
	for (int i = 0; joyinputs[port] && joyinputs[port][i] >= 0; i++) {
		int evtnum = joyinputs[port][i];
		for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
			enablejoydevice (&joysticks[l], gameportsmode, evtnum);
			enablejoydevice (&mice[l], gameportsmode, evtnum);
			enablejoydevice (&keyboards[l], gameportsmode, evtnum);
		}
		for (int k = 0; axistable[k] >= 0; k += 3) {
			if (evtnum == axistable[k] || evtnum == axistable[k + 1] || evtnum == axistable[k + 2]) {
				for (int j = 0; j < 3; j++) {
					int evtnum2 = axistable[k + j];
					for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
						enablejoydevice (&joysticks[l], gameportsmode, evtnum2);
						enablejoydevice (&mice[l], gameportsmode, evtnum2);
						enablejoydevice (&keyboards[l], gameportsmode, evtnum2);
					}
				}
				break;
			}
		}

	}
}

static void setjoyinputs (struct uae_prefs *prefs, int port)
{
	joyinputs[port] = NULL;
	switch (joymodes[port])
	{
		case JSEM_MODE_JOYSTICK:
			if (port >= 2)
				joyinputs[port] = port == 3 ? ip_parjoy2 : ip_parjoy1;
			else
				joyinputs[port] = port == 1 ? ip_joy2 : ip_joy1;
		break;
		case JSEM_MODE_GAMEPAD:
			joyinputs[port] = port ? ip_joypad2 : ip_joypad1;
		break;
		case JSEM_MODE_JOYSTICK_CD32:
			joyinputs[port] = port ? ip_joycd322 : ip_joycd321;
		break;
		case JSEM_MODE_JOYSTICK_ANALOG:
			joyinputs[port] = port ? ip_analog2 : ip_analog1;
		break;
		case JSEM_MODE_MOUSE:
			joyinputs[port] = port ? ip_mouse2 : ip_mouse1;
		break;
		case JSEM_MODE_LIGHTPEN:
			joyinputs[port] = port ? ip_lightpen2 : ip_lightpen1;
		break;
		case JSEM_MODE_MOUSE_CDTV:
			joyinputs[port] = ip_mousecdtv;
		break;
	}
}

static void setautofire (struct uae_input_device *uid, int port, int af)
{
	int *afp = af_ports[port];
	for (int k = 0; afp[k] >= 0; k++) {
		for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
			for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
				if (uid->eventid[i][j] == afp[k]) {
					uid->flags[i][j] &= ~(ID_FLAG_AUTOFIRE | ID_FLAG_TOGGLE);
					if (af >= JPORT_AF_NORMAL)
						uid->flags[i][j] |= ID_FLAG_AUTOFIRE;
					if (af == JPORT_AF_TOGGLE)
						uid->flags[i][j] |= ID_FLAG_TOGGLE;
				}
			}
		}
	}
}

static void setautofires (struct uae_prefs *prefs, int port, int af)
{
	for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
		setautofire (&joysticks[l], port, af);
		setautofire (&mice[l], port, af);
		setautofire (&keyboards[l], port, af);
	}
}

// merge gameport settings with current input configuration
static void compatibility_copy (struct uae_prefs *prefs, bool gameports)
{
	int used[MAX_INPUT_DEVICES] = { 0 };
	int i, joy;

	DebOut("entered(%lx, %d)\n", prefs, gameports);

	for (i = 0; i < MAX_JPORTS; i++) {
		joymodes[i] = prefs->jports[i].mode;
		joyinputs[i]= NULL;
		// remove all mappings from this port, except if custom
		if (prefs->jports[i].id != JPORT_CUSTOM) {
			if (gameports)
				remove_compa_config (prefs, i);
		}
		remove_custom_config (prefs, prefs->jports[i].id == JPORT_CUSTOM, i);
		setjoyinputs (prefs, i);
	}
	DebOut("ping..\n");

	for (i = 0; i < 2; i++) {
		int af = prefs->jports[i].autofire;
		if (prefs->jports[i].id >= 0 && joymodes[i] <= 0) {
			int mode = prefs->jports[i].mode;
			if (jsem_ismouse (i, prefs) >= 0) {
				switch (mode)
				{
					case JSEM_MODE_DEFAULT:
					case JSEM_MODE_MOUSE:
					default:
					joymodes[i] = JSEM_MODE_MOUSE;
					joyinputs[i] = i ? ip_mouse2 : ip_mouse1;
					break;
					case JSEM_MODE_LIGHTPEN:
					joymodes[i] = JSEM_MODE_LIGHTPEN;
					joyinputs[i] = i ? ip_lightpen2 : ip_lightpen1;
					break;
					case JSEM_MODE_MOUSE_CDTV:
					joymodes[i] = JSEM_MODE_MOUSE_CDTV;
					joyinputs[i] = ip_mousecdtv;
					break;
				}
			} else if (jsem_isjoy (i, prefs) >= 0) {
				switch (mode)
				{
					case JSEM_MODE_DEFAULT:
					case JSEM_MODE_JOYSTICK:
					case JSEM_MODE_GAMEPAD:
					case JSEM_MODE_JOYSTICK_CD32:
					default:
					{
						bool iscd32 = mode == JSEM_MODE_JOYSTICK_CD32 || (mode == JSEM_MODE_DEFAULT && prefs->cs_cd32cd);
						if (iscd32) {
							joymodes[i] = JSEM_MODE_JOYSTICK_CD32;
							joyinputs[i] = i ? ip_joycd322 : ip_joycd321;
						} else if (mode == JSEM_MODE_GAMEPAD) {
							joymodes[i] = JSEM_MODE_GAMEPAD;
							joyinputs[i] = i ? ip_joypad2 : ip_joypad1;
						} else {
							joymodes[i] = JSEM_MODE_JOYSTICK;
							joyinputs[i] = i ? ip_joy2 : ip_joy1;
						}
						break;
					}
					case JSEM_MODE_JOYSTICK_ANALOG:
						joymodes[i] = JSEM_MODE_JOYSTICK_ANALOG;
						joyinputs[i] = i ? ip_analog2 : ip_analog1;
						break;
					case JSEM_MODE_MOUSE:
						joymodes[i] = JSEM_MODE_MOUSE;
						joyinputs[i] = i ? ip_mouse2 : ip_mouse1;
						break;
					case JSEM_MODE_LIGHTPEN:
						joymodes[i] = JSEM_MODE_LIGHTPEN;
						joyinputs[i] = i ? ip_lightpen2 : ip_lightpen1;
						break;
					case JSEM_MODE_MOUSE_CDTV:
						joymodes[i] = JSEM_MODE_MOUSE_CDTV;
						joyinputs[i] = ip_mousecdtv;
						break;
				}
			} else if (prefs->jports[i].id >= 0) {
				joymodes[i] = i ? JSEM_MODE_JOYSTICK : JSEM_MODE_MOUSE;
				joyinputs[i] = i ? ip_joy2 : ip_mouse1;
			}
		}
	}
	DebOut("ping..\n");

	for (i = 2; i < MAX_JPORTS; i++) {
		if (prefs->jports[i].id >= 0 && joymodes[i] <= 0) {
			int mode = prefs->jports[i].mode;
			if (jsem_isjoy (i, prefs) >= 0) {
				joymodes[i] = JSEM_MODE_JOYSTICK;
				joyinputs[i] = i == 3 ? ip_parjoy2 : ip_parjoy1;
			} else if (prefs->jports[i].id >= 0) {
				prefs->jports[i].mode = joymodes[i] = JSEM_MODE_JOYSTICK;
				joyinputs[i] = i == 3 ? ip_parjoy2 : ip_parjoy1;
			}
		}
	}
	DebOut("ping..\n");

	for (i = 0; i < 2; i++) {
		int af = prefs->jports[i].autofire;
		if (prefs->jports[i].id >= 0) {
			int mode = prefs->jports[i].mode;
			if ((joy = jsem_ismouse (i, prefs)) >= 0) {
				if (gameports)
					cleardev (mice, joy);
				switch (mode)
				{
				case JSEM_MODE_DEFAULT:
				case JSEM_MODE_MOUSE:
				default:
					input_get_default_mouse (mice, joy, i, af);
					joymodes[i] = JSEM_MODE_MOUSE;
					break;
				case JSEM_MODE_LIGHTPEN:
					input_get_default_lightpen (mice, joy, i, af);
					joymodes[i] = JSEM_MODE_LIGHTPEN;
					break;
				}
				_tcsncpy (prefs->jports[i].name, idev[IDTYPE_MOUSE].get_friendlyname (joy), MAX_JPORTNAME - 1);
				_tcsncpy (prefs->jports[i].configname, idev[IDTYPE_MOUSE].get_uniquename (joy), MAX_JPORTNAME - 1);
			}
		}
	}
	DebOut("ping..\n");

	for (i = 1; i >= 0; i--) {
		int af = prefs->jports[i].autofire;
		if (prefs->jports[i].id >= 0) {
			int mode = prefs->jports[i].mode;
			joy = jsem_isjoy (i, prefs);
			if (joy >= 0) {
				if (gameports)
					cleardev (joysticks, joy);
				switch (mode)
				{
				case JSEM_MODE_DEFAULT:
				case JSEM_MODE_JOYSTICK:
				case JSEM_MODE_GAMEPAD:
				case JSEM_MODE_JOYSTICK_CD32:
				default:
				{
					bool iscd32 = mode == JSEM_MODE_JOYSTICK_CD32 || (mode == JSEM_MODE_DEFAULT && prefs->cs_cd32cd);
					input_get_default_joystick (joysticks, joy, i, af, mode);
					if (iscd32)
						joymodes[i] = JSEM_MODE_JOYSTICK_CD32;
					else if (mode == JSEM_MODE_GAMEPAD)
						joymodes[i] = JSEM_MODE_GAMEPAD;
					else
						joymodes[i] = JSEM_MODE_JOYSTICK;
					break;
				}
				case JSEM_MODE_JOYSTICK_ANALOG:
					input_get_default_joystick_analog (joysticks, joy, i, af);
					joymodes[i] = JSEM_MODE_JOYSTICK_ANALOG;
					break;
				case JSEM_MODE_MOUSE:
					input_get_default_mouse (joysticks, joy, i, af);
					joymodes[i] = JSEM_MODE_MOUSE;
					break;
				case JSEM_MODE_LIGHTPEN:
					input_get_default_lightpen (joysticks, joy, i, af);
					joymodes[i] = JSEM_MODE_LIGHTPEN;
					break;
				case JSEM_MODE_MOUSE_CDTV:
					joymodes[i] = JSEM_MODE_MOUSE_CDTV;
					input_get_default_joystick (joysticks, joy, i, af, mode);
					break;

				}
				_tcsncpy (prefs->jports[i].name, idev[IDTYPE_JOYSTICK].get_friendlyname (joy), MAX_JPORTNAME - 1);
				_tcsncpy (prefs->jports[i].configname, idev[IDTYPE_JOYSTICK].get_uniquename (joy), MAX_JPORTNAME - 1);
				used[joy] = 1;
			}
		}
	}
	DebOut("ping of death..\n");

	if (gameports) {
		// replace possible old mappings with default keyboard mapping
		for (i = KBR_DEFAULT_MAP_FIRST; i <= KBR_DEFAULT_MAP_LAST; i++) {
			DebOut("pong (%d)..\n",i);
			checkcompakb (keyboard_default_kbmaps[i], ip_joy2);
			checkcompakb (keyboard_default_kbmaps[i], ip_joy1);
			checkcompakb (keyboard_default_kbmaps[i], ip_joypad2);
			checkcompakb (keyboard_default_kbmaps[i], ip_joypad1);
			checkcompakb (keyboard_default_kbmaps[i], ip_parjoy2);
			checkcompakb (keyboard_default_kbmaps[i], ip_parjoy1);
			checkcompakb (keyboard_default_kbmaps[i], ip_mouse2);
			checkcompakb (keyboard_default_kbmaps[i], ip_mouse1);
		}
			DebOut("pong..\n");
#ifndef __AROS__
		for (i = KBR_DEFAULT_MAP_CD32_FIRST; i <= KBR_DEFAULT_MAP_CD32_LAST; i++) {
			DebOut("pong..KBR_DEFAULT_MAP_CD32_FIRST \n");
			checkcompakb (keyboard_default_kbmaps[i], ip_joycd321);
			checkcompakb (keyboard_default_kbmaps[i], ip_joycd322);
		}
#endif
	}
	DebOut("ping..\n");

	for (i = 0; i < 2; i++) {
		DebOut("prefs->jports[%d]\n", i);
		if (prefs->jports[i].id >= 0) {
			int *kb = NULL;
			int mode = prefs->jports[i].mode;
			int af = prefs->jports[i].autofire;
			for (joy = 0; used[joy]; joy++);
			if (JSEM_ISANYKBD (i, prefs)) {
				bool cd32 = mode == JSEM_MODE_JOYSTICK_CD32 || (mode == JSEM_MODE_DEFAULT && prefs->cs_cd32cd);
				if (JSEM_ISNUMPAD (i, prefs)) {
					if (cd32)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CD32_NP];
					else if (mode == JSEM_MODE_GAMEPAD)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_NP3];
					else
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_NP];
				} else if (JSEM_ISCURSOR (i, prefs)) {
					if (cd32)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CD32_CK];
					else if (mode == JSEM_MODE_GAMEPAD)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CK3];
					else
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CK];
				} else if (JSEM_ISSOMEWHEREELSE (i, prefs)) {
					if (cd32)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CD32_SE];
					else if (mode == JSEM_MODE_GAMEPAD)
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_SE3];
					else
						kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_SE];
				} else if (JSEM_ISXARCADE1 (i, prefs)) {
					kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_XA1];
				} else if (JSEM_ISXARCADE2 (i, prefs)) {
					kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_XA2];
				}
				if (kb) {
					switch (mode)
					{
					case JSEM_MODE_JOYSTICK:
					case JSEM_MODE_GAMEPAD:
					case JSEM_MODE_JOYSTICK_CD32:
					case JSEM_MODE_DEFAULT:
						if (cd32) {
							setcompakb (kb, i ? ip_joycd322 : ip_joycd321, i, af);
							joymodes[i] = JSEM_MODE_JOYSTICK_CD32;
						} else if (mode == JSEM_MODE_GAMEPAD) {
							setcompakb (kb, i ? ip_joypad2 : ip_joypad1, i, af);
							joymodes[i] = JSEM_MODE_GAMEPAD;
						} else {
							setcompakb (kb, i ? ip_joy2 : ip_joy1, i, af);
							joymodes[i] = JSEM_MODE_JOYSTICK;
						}
						break;
					case JSEM_MODE_MOUSE:
						setcompakb (kb, i ? ip_mouse2 : ip_mouse1, i, af);
						joymodes[i] = JSEM_MODE_MOUSE;
						break;
					}
					used[joy] = 1;
				}
			}
		}
	}
	DebOut("ping..\n");
	if (arcadia_bios) {
		setcompakb (keyboard_default_kbmaps[KBR_DEFAULT_MAP_ARCADIA], ip_arcadia, 0, 0);
		if (JSEM_ISXARCADE1 (i, prefs) || JSEM_ISXARCADE2 (i, prefs))
			setcompakb (keyboard_default_kbmaps[KBR_DEFAULT_MAP_ARCADIA_XA], ip_arcadiaxa, JSEM_ISXARCADE2 (i, prefs) ? 1 : 0, prefs->jports[i].autofire);
	}
	if (0 && currprefs.cs_cdtvcd) {
		setcompakb (keyboard_default_kbmaps[KBR_DEFAULT_MAP_CDTV], ip_mediacdtv, 0, 0);
	}
	DebOut("ping..\n");

	// parport
	for (i = 2; i < MAX_JPORTS; i++) {
		int af = prefs->jports[i].autofire;
		if (prefs->jports[i].id >= 0) {
			int *kb = NULL;
			joy = jsem_isjoy (i, prefs);
			if (joy >= 0) {
				if (gameports)
					cleardev (joysticks, joy);
				input_get_default_joystick (joysticks, joy, i, af, 0);
				_tcsncpy (prefs->jports[i].name, idev[IDTYPE_JOYSTICK].get_friendlyname (joy), MAX_JPORTNAME - 1);
				_tcsncpy (prefs->jports[i].configname, idev[IDTYPE_JOYSTICK].get_uniquename (joy), MAX_JPORTNAME - 1);
				used[joy] = 1;
				joymodes[i] = JSEM_MODE_JOYSTICK;
			}
		}
	}
	DebOut("ping..\n");
	for (i = 2; i < MAX_JPORTS; i++) {
		if (prefs->jports[i].id >= 0) {
			int *kb = NULL;
			for (joy = 0; used[joy]; joy++);
			if (JSEM_ISANYKBD (i, prefs)) {
				if (JSEM_ISNUMPAD (i, prefs))
					kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_NP];
				else if (JSEM_ISCURSOR (i, prefs))
					kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_CK];
				else if (JSEM_ISSOMEWHEREELSE (i, prefs))
					kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_SE];
				else if (JSEM_ISXARCADE1 (i, prefs))
					kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_XA1];
				else if (JSEM_ISXARCADE2 (i, prefs))
					kb = keyboard_default_kbmaps[KBR_DEFAULT_MAP_XA2];
				if (kb) {
					setcompakb (kb, i == 3 ? ip_parjoy2 : ip_parjoy1, i, prefs->jports[i].autofire);
					used[joy] = 1;
					joymodes[i] = JSEM_MODE_JOYSTICK;
				}
			}
		}
	}
	DebOut("ping..\n");

	for (i = 0; i < MAX_JPORTS; i++) {
		if (gameports)
			setautofires (prefs, i, prefs->jports[i].autofire);
	}
	DebOut("ping..\n");

	for (i = 0; i < MAX_JPORTS; i++) {
		setjoyinputs (prefs, i);
		setjoydevices (prefs, gameports, i);
	}
}

static void disableifempty2 (struct uae_input_device *uid)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			if (uid->eventid[i][j] > 0 || uid->custom[i][j] != NULL)
				return;
		}
	}
	uid->enabled = false;
}
static void disableifempty (struct uae_prefs *prefs)
{
	for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
		disableifempty2 (&joysticks[l]);
		disableifempty2 (&mice[l]);
		disableifempty2 (&keyboards[l]);
	}
}

static void matchdevices (struct inputdevice_functions *inf, struct uae_input_device *uid)
{
	int i, j;

	for (i = 0; i < inf->get_num (); i++) {
		TCHAR *aname1 = inf->get_friendlyname (i);
		TCHAR *aname2 = inf->get_uniquename (i);
		int match = -1;
		for (j = 0; j < MAX_INPUT_DEVICES; j++) {
			if (aname2 && uid[j].configname) {
				TCHAR bname[MAX_DPATH];
				TCHAR bname2[MAX_DPATH];
				TCHAR *p1 ,*p2;
				_tcscpy (bname, uid[j].configname);
				_tcscpy (bname2, aname2);
				p1 = _tcschr (bname, ' ');
				p2 = _tcschr (bname2, ' ');
				if (p1 && p2 && p1 - bname == p2 - bname2) {
					*p1 = 0;
					*p2 = 0;
					if (bname && !_tcscmp (bname2, bname)) {
						if (match >= 0)
							match = -2;
						else
							match = j;
					}
				}
				if (match == -2)
					break;
			}
		}
		// multiple matches -> use complete local-only id string for comparisons
		if (match == -2) {
			for (j = 0; j < MAX_INPUT_DEVICES; j++) {
				TCHAR *bname = uid[j].configname;
				if (aname2 && bname && !_tcscmp (aname2, bname))
					match = j;
			}
		}
		if (match < 0) {
			for (j = 0; j < MAX_INPUT_DEVICES; j++) {
				TCHAR *bname = uid[j].name;
				if (aname2 && bname && !_tcscmp (aname2, bname))
					match = j;
			}
		}
		if (match >= 0) {
			j = match;
			if (j != i) {
				struct uae_input_device *tmp = xmalloc (struct uae_input_device, 1);
				memcpy (tmp, &uid[j], sizeof (struct uae_input_device));
				memcpy (&uid[j], &uid[i], sizeof (struct uae_input_device));
				memcpy (&uid[i], tmp, sizeof (struct uae_input_device));
				xfree (tmp);
			}
		}
	}
	for (i = 0; i < inf->get_num (); i++) {
		if (uid[i].name == NULL)
			uid[i].name = my_strdup (inf->get_friendlyname (i));
		if (uid[i].configname == NULL)
			uid[i].configname = my_strdup (inf->get_uniquename (i));
	}
}

static void matchdevices_all (struct uae_prefs *prefs)
{
	int i;
	for (i = 0; i < MAX_INPUT_SETTINGS; i++) {
		matchdevices (&idev[IDTYPE_MOUSE], prefs->mouse_settings[i]);
		matchdevices (&idev[IDTYPE_JOYSTICK], prefs->joystick_settings[i]);
		matchdevices (&idev[IDTYPE_KEYBOARD], prefs->keyboard_settings[i]);
	}
}

bool inputdevice_set_gameports_mapping (struct uae_prefs *prefs, int devnum, int num, const TCHAR *name, int port)
{
	joysticks = prefs->joystick_settings[GAMEPORT_INPUT_SETTINGS];
	mice = prefs->mouse_settings[GAMEPORT_INPUT_SETTINGS];
	keyboards = prefs->keyboard_settings[GAMEPORT_INPUT_SETTINGS];

	int sub = 0;
	if (inputdevice_get_widget_type (devnum, num, NULL) != IDEV_WIDGET_KEY) {
		for (sub = 0; sub < MAX_INPUT_SUB_EVENT; sub++) {
			if (!inputdevice_get_mapping (devnum, num, NULL, NULL, NULL, NULL, sub))
				break;
		}
	}
	if (sub >= MAX_INPUT_SUB_EVENT)
		sub = MAX_INPUT_SUB_EVENT - 1;
	inputdevice_set_mapping (devnum, num, name, NULL, IDEV_MAPPED_GAMEPORTSCUSTOM, port + 1, sub);

	joysticks = prefs->joystick_settings[prefs->input_selected_setting];
	mice = prefs->mouse_settings[prefs->input_selected_setting];
	keyboards = prefs->keyboard_settings[prefs->input_selected_setting];
	if (prefs->input_selected_setting != GAMEPORT_INPUT_SETTINGS) {
		int xflags, xport;
		TCHAR xname[MAX_DPATH], xcustom[MAX_DPATH];
		inputdevice_get_mapping (devnum, num, &xflags, &xport, xname, xcustom, 0);
		if (xport == 0)
			inputdevice_set_mapping (devnum, num, xname, xcustom, xflags, MAX_JPORTS + 1, SPARE_SUB_EVENT);
		inputdevice_set_mapping (devnum, num, name, NULL, IDEV_MAPPED_GAMEPORTSCUSTOM, port + 1, 0);
	}
	return true;
}

static void resetinput (void)
{
	if ((input_play || input_record) && hsync_counter > 0)
		return;
	cd32_shifter[0] = cd32_shifter[1] = 8;
	for (int i = 0; i < MAX_JPORTS; i++) {
		oleft[i] = 0;
		oright[i] = 0;
		otop[i] = 0;
		obot[i] = 0;
		oldmx[i] = -1;
		oldmy[i] = -1;
		joybutton[i] = 0;
		joydir[i] = 0;
		mouse_deltanoreset[i][0] = 0;
		mouse_delta[i][0] = 0;
		mouse_deltanoreset[i][1] = 0;
		mouse_delta[i][1] = 0;
		mouse_deltanoreset[i][2] = 0;
		mouse_delta[i][2] = 0;
	}
	memset (keybuf, 0, sizeof keybuf);
	for (int i = 0; i < INPUT_QUEUE_SIZE; i++)
		input_queue[i].linecnt = input_queue[i].nextlinecnt = -1;

	for (int i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
		sublevdir[0][i] = i;
		sublevdir[1][i] = MAX_INPUT_SUB_EVENT - i - 1;
	}
}

void inputdevice_updateconfig (struct uae_prefs *prefs)
{
	int i;

	DebOut("entered\n");

	copyjport (&changed_prefs, &currprefs, 0);
	copyjport (&changed_prefs, &currprefs, 1);
	copyjport (&changed_prefs, &currprefs, 2);
	copyjport (&changed_prefs, &currprefs, 3);

	DebOut("ping\n");
#ifdef RETROPLATFORM
	rp_input_change (0);
	rp_input_change (1);
	rp_input_change (2);
	rp_input_change (3);
	for (i = 0; i < MAX_JPORTS; i++)
		rp_update_gameport (i, -1, 0);
#endif

	DebOut("ping\n");
	resetinput ();

	DebOut("ping\n");
	joysticks = prefs->joystick_settings[prefs->input_selected_setting];
	mice = prefs->mouse_settings[prefs->input_selected_setting];
	keyboards = prefs->keyboard_settings[prefs->input_selected_setting];

	DebOut("ping\n");
	matchdevices_all (prefs);

	DebOut("ping\n");
	memset (joysticks2, 0, sizeof joysticks2);
	memset (mice2, 0, sizeof mice2);

	DebOut("ping\n");
	joysticks = prefs->joystick_settings[GAMEPORT_INPUT_SETTINGS];
	mice = prefs->mouse_settings[GAMEPORT_INPUT_SETTINGS];
	keyboards = prefs->keyboard_settings[GAMEPORT_INPUT_SETTINGS];
	for (i = 0; i < MAX_INPUT_SETTINGS; i++) {
		joysticks[i].enabled = 0;
		mice[i].enabled = 0;
	}
	DebOut("ping\n");
	compatibility_copy (prefs, true);
	joysticks = prefs->joystick_settings[prefs->input_selected_setting];
	mice = prefs->mouse_settings[prefs->input_selected_setting];
	keyboards = prefs->keyboard_settings[prefs->input_selected_setting];
	if (prefs->input_selected_setting != GAMEPORT_INPUT_SETTINGS) {
		compatibility_copy (prefs, false);
	}
	DebOut("ping\n");

	disableifempty (prefs);
	scanevents (prefs);

	config_changed = 1;
	DebOut("pong\n");
}

/* called when devices get inserted or removed
* store old devices temporarily, enumerate all devices
* restore old devices back (order may have changed)
*/
void inputdevice_devicechange (struct uae_prefs *prefs)
{
	int acc = input_acquired;
	int i, idx;
	TCHAR *jports[MAX_JPORTS];
	int jportskb[MAX_JPORTS], jportsmode[MAX_JPORTS];

	for (i = 0; i < MAX_JPORTS; i++) {
		jports[i] = NULL;
		jportskb[i] = -1;
		idx = inputdevice_getjoyportdevice (i, prefs->jports[i].id);
		if (idx >= JSEM_LASTKBD) {
			struct inputdevice_functions *idf;
			int devidx;
			idx -= JSEM_LASTKBD;
			idf = getidf (idx);
			devidx = inputdevice_get_device_index (idx);
			jports[i] = my_strdup (idf->get_uniquename (devidx));
		} else {
			jportskb[i] = idx;
		}
		jportsmode[i] = prefs->jports[i].mode;
	}

	inputdevice_unacquire ();
	idev[IDTYPE_JOYSTICK].close ();
	idev[IDTYPE_MOUSE].close ();
	idev[IDTYPE_KEYBOARD].close ();
	idev[IDTYPE_JOYSTICK].init ();
	idev[IDTYPE_MOUSE].init ();
	idev[IDTYPE_KEYBOARD].init ();
	matchdevices (&idev[IDTYPE_MOUSE], mice);
	matchdevices (&idev[IDTYPE_JOYSTICK], joysticks);
	matchdevices (&idev[IDTYPE_KEYBOARD], keyboards);

	for (i = 0; i < MAX_JPORTS; i++) {
		freejport (prefs, i);
		if (jports[i]) {
			inputdevice_joyport_config (prefs, jports[i], i, jportsmode[i], 2);
			xfree (jports[i]);
		} else if (jportskb[i] >= 0) {
			TCHAR tmp[10];
			_stprintf (tmp, _T("kbd%d"), jportskb[i]);
			inputdevice_joyport_config (prefs, tmp, i, jportsmode[i], 0);
		}
	}

	if (prefs == &changed_prefs)
		inputdevice_copyconfig (&changed_prefs, &currprefs);
	if (acc)
		inputdevice_acquire (TRUE);
	config_changed = 1;
}

// set default prefs to all input configuration settings
void inputdevice_default_prefs (struct uae_prefs *p)
{
	int i;

	inputdevice_init ();
	p->input_selected_setting = GAMEPORT_INPUT_SETTINGS;
	p->input_joymouse_multiplier = 20;
	p->input_joymouse_deadzone = 33;
	p->input_joystick_deadzone = 33;
	p->input_joymouse_speed = 10;
	p->input_analog_joystick_mult = 15;
	p->input_analog_joystick_offset = -1;
	p->input_mouse_speed = 100;
	p->input_autofire_linecnt = 600;
	for (i = 0; i < MAX_INPUT_SETTINGS; i++) {
		if (i == GAMEPORT_INPUT_SETTINGS) {
			if (p->jports[0].id != JPORT_CUSTOM || p->jports[0].id != JPORT_CUSTOM) {
				reset_inputdevice_slot (p, i);
			}
		}
		set_kbr_default (p, i, -1);
	}
}

// set default keyboard and keyboard>joystick layouts
void inputdevice_setkeytranslation (struct uae_input_device_kbr_default *trans, int **kbmaps)
{
	keyboard_default = trans;
	keyboard_default_kbmaps = kbmaps;
}

// return true if keyboard/scancode pair is mapped
int inputdevice_iskeymapped (int keyboard, int scancode)
{
	struct uae_input_device *na = &keyboards[keyboard];

	if (!keyboards || scancode < 0)
		return 0;
	return scancodeused[keyboard][scancode];
}

int inputdevice_synccapslock (int oldcaps, int *capstable)
{
	struct uae_input_device *na = &keyboards[0];
	int j, i;
	
	if (!keyboards)
		return -1;
	for (j = 0; na->extra[j]; j++) {
		if (na->extra[j] == INPUTEVENT_KEY_CAPS_LOCK) {
			for (i = 0; capstable[i]; i += 2) {
				if (na->extra[j] == capstable[i]) {
					if (oldcaps != capstable[i + 1]) {
						oldcaps = capstable[i + 1];
						inputdevice_translatekeycode (0, capstable[i], oldcaps ? -1 : 0);
					}
					return i;
				}
			}
		}
	}
	return -1;
}

static int inputdevice_translatekeycode_2 (int keyboard, int scancode, int state)
{
	struct uae_input_device *na = &keyboards[keyboard];
	int j, k;
	int handled = 0;

	if (!keyboards || scancode < 0)
		return handled;

	j = 0;
	while (j < MAX_INPUT_DEVICE_EVENTS && na->extra[j] >= 0) {
		if (na->extra[j] == scancode) {
			for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {/* send key release events in reverse order */
				int autofire = (na->flags[j][sublevdir[state == 0 ? 1 : 0][k]] & ID_FLAG_AUTOFIRE) ? 1 : 0;
				int toggle = (na->flags[j][sublevdir[state == 0 ? 1 : 0][k]] & ID_FLAG_TOGGLE) ? 1 : 0;
				int evt = na->eventid[j][sublevdir[state == 0 ? 1 : 0][k]];
				int toggled;

				// if evt == caps and scan == caps: sync with native caps led
				if (evt == INPUTEVENT_KEY_CAPS_LOCK) {
					int v;
					if (state < 0)
						state = 1;
					v = target_checkcapslock (scancode, &state);
					if (v < 0)
						continue;
					if (v > 0)
						toggle = 0;
				} else if (state < 0) {
					// it was caps lock resync, ignore, not mapped to caps
					continue;
				}

				if (toggle) {
					if (!state)
						continue;
					na->flags[j][sublevdir[state == 0 ? 1 : 0][k]] ^= ID_FLAG_TOGGLED;
					toggled = (na->flags[j][sublevdir[state == 0 ? 1 : 0][k]] & ID_FLAG_TOGGLED) ? 1 : 0;
					handled |= handle_input_event (evt, toggled, 1, autofire, true, false);
				} else {
					handled |= handle_input_event (evt, state, 1, autofire, true, false);
				}
			}
			process_custom_event (na, j, state);
			return handled;
		}
		j++;
	}
	return handled;
}

#define IECODE_UP_PREFIX 0x80
#define RAW_STEALTH 0x68
#define STEALTHF_E0KEY 0x08
#define STEALTHF_UPSTROKE 0x04
#define STEALTHF_SPECIAL 0x02
#define STEALTHF_E1KEY 0x01

static void sendmmcodes (int code, int newstate)
{
	uae_u8 b;

	b = RAW_STEALTH | IECODE_UP_PREFIX;
	record_key(((b << 1) | (b >> 7)) & 0xff);
	b = IECODE_UP_PREFIX;
	if ((code >> 8) == 0x01)
		b |= STEALTHF_E0KEY;
	if ((code >> 8) == 0x02)
		b |= STEALTHF_E1KEY;
	if (!newstate)
		b |= STEALTHF_UPSTROKE;
	record_key(((b << 1) | (b >> 7)) & 0xff);
	b = ((code >> 4) & 0x0f) | IECODE_UP_PREFIX;
	record_key(((b << 1) | (b >> 7)) & 0xff);
	b = (code & 0x0f) | IECODE_UP_PREFIX;
	record_key(((b << 1) | (b >> 7)) & 0xff);
}

// main keyboard press/release entry point
int inputdevice_translatekeycode (int keyboard, int scancode, int state)
{
	if (inputdevice_translatekeycode_2 (keyboard, scancode, state))
		return 1;
	if (currprefs.mmkeyboard && scancode > 0)
		sendmmcodes (scancode, state);
	return 0;
}

struct inputdevice_functions idev[3];

void inputdevice_init (void)
{
	idev[IDTYPE_JOYSTICK] = inputdevicefunc_joystick;
	idev[IDTYPE_JOYSTICK].init ();
	idev[IDTYPE_MOUSE] = inputdevicefunc_mouse;
	idev[IDTYPE_MOUSE].init ();
	idev[IDTYPE_KEYBOARD] = inputdevicefunc_keyboard;
	idev[IDTYPE_KEYBOARD].init ();
}

void inputdevice_close (void)
{
	idev[IDTYPE_JOYSTICK].close ();
	idev[IDTYPE_MOUSE].close ();
	idev[IDTYPE_KEYBOARD].close ();
	inprec_close (true);
}

static struct uae_input_device *get_uid (const struct inputdevice_functions *id, int devnum)
{
	struct uae_input_device *uid = 0;
	if (id == &idev[IDTYPE_JOYSTICK]) {
		uid = &joysticks[devnum];
	} else if (id == &idev[IDTYPE_MOUSE]) {
		uid = &mice[devnum];
	} else if (id == &idev[IDTYPE_KEYBOARD]) {
		uid = &keyboards[devnum];
	}
	return uid;
}

static int get_event_data (const struct inputdevice_functions *id, int devnum, int num, int *eventid, TCHAR **custom, int *flags, int *port, int sub)
{
	const struct uae_input_device *uid = get_uid (id, devnum);
	int type = id->get_widget_type (devnum, num, 0, 0);
	int i;
	if (type == IDEV_WIDGET_BUTTON || type == IDEV_WIDGET_BUTTONAXIS) {
		i = num - id->get_widget_first (devnum, IDEV_WIDGET_BUTTON) + ID_BUTTON_OFFSET;
		*eventid = uid->eventid[i][sub];
		if (flags)
			*flags = uid->flags[i][sub];
		if (port)
			*port = uid->port[i][sub];
		if (custom)
			*custom = uid->custom[i][sub];
		return i;
	} else if (type == IDEV_WIDGET_AXIS) {
		i = num - id->get_widget_first (devnum, type) + ID_AXIS_OFFSET;
		*eventid = uid->eventid[i][sub];
		if (flags)
			*flags = uid->flags[i][sub];
		if (port)
			*port = uid->port[i][sub];
		if (custom)
			*custom = uid->custom[i][sub];
		return i;
	} else if (type == IDEV_WIDGET_KEY) {
		i = num - id->get_widget_first (devnum, type);
		*eventid = uid->eventid[i][sub];
		if (flags)
			*flags = uid->flags[i][sub];
		if (port)
			*port = uid->port[i][sub];
		if (custom)
			*custom = uid->custom[i][sub];
		return i;
	}
	return -1;
}

static int put_event_data (const struct inputdevice_functions *id, int devnum, int num, int eventid, TCHAR *custom, int flags, int port, int sub)
{
	struct uae_input_device *uid = get_uid (id, devnum);
	int type = id->get_widget_type (devnum, num, 0, 0);
	int i;
	if (type == IDEV_WIDGET_BUTTON || type == IDEV_WIDGET_BUTTONAXIS) {
		i = num - id->get_widget_first (devnum, IDEV_WIDGET_BUTTON) + ID_BUTTON_OFFSET;
		uid->eventid[i][sub] = eventid;
		uid->flags[i][sub] = flags;
		uid->port[i][sub] = port;
		xfree (uid->custom[i][sub]);
		uid->custom[i][sub] = custom && _tcslen (custom) > 0 ? my_strdup (custom) : NULL;
		return i;
	} else if (type == IDEV_WIDGET_AXIS) {
		i = num - id->get_widget_first (devnum, type) + ID_AXIS_OFFSET;
		uid->eventid[i][sub] = eventid;
		uid->flags[i][sub] = flags;
		uid->port[i][sub] = port;
		xfree (uid->custom[i][sub]);
		uid->custom[i][sub] = custom && _tcslen (custom) > 0 ? my_strdup (custom) : NULL;
		return i;
	} else if (type == IDEV_WIDGET_KEY) {
		i = num - id->get_widget_first (devnum, type);
		uid->eventid[i][sub] = eventid;
		uid->flags[i][sub] = flags;
		uid->port[i][sub] = port;
		xfree (uid->custom[i][sub]);
		uid->custom[i][sub] = custom && _tcslen (custom) > 0 ? my_strdup (custom) : NULL;
		return i;
	}
	return -1;
}

static int is_event_used (const struct inputdevice_functions *id, int devnum, int isnum, int isevent)
{
	struct uae_input_device *uid = get_uid (id, devnum);
	int num, evt, sub;

	for (num = 0; num < id->get_widget_num (devnum); num++) {
		for (sub = 0; sub < MAX_INPUT_SUB_EVENT; sub++) {
			if (get_event_data (id, devnum, num, &evt, NULL, NULL, NULL, sub) >= 0) {
				if (evt == isevent && isnum != num)
					return 1;
			}
		}
	}
	return 0;
}

// device based index from global device index
int inputdevice_get_device_index (int devnum)
{
	if (devnum < idev[IDTYPE_JOYSTICK].get_num())
		return devnum;
	else if (devnum < idev[IDTYPE_JOYSTICK].get_num() + idev[IDTYPE_MOUSE].get_num())
		return devnum - idev[IDTYPE_JOYSTICK].get_num();
	else if (devnum < idev[IDTYPE_JOYSTICK].get_num() + idev[IDTYPE_MOUSE].get_num() + idev[IDTYPE_KEYBOARD].get_num())
		return devnum - idev[IDTYPE_JOYSTICK].get_num() - idev[IDTYPE_MOUSE].get_num();
	else
		return -1;
}

static int getdevnum (int type, int devnum)
{
	if (type == IDTYPE_JOYSTICK)
		return devnum;
	if (type == IDTYPE_MOUSE)
		return idev[IDTYPE_JOYSTICK].get_num() + devnum;
	if (type == IDTYPE_KEYBOARD)
		return idev[IDTYPE_JOYSTICK].get_num() + idev[IDTYPE_MOUSE].get_num() + devnum;
	return -1;
}

static int gettype (int devnum)
{
	if (devnum < idev[IDTYPE_JOYSTICK].get_num())
		return IDTYPE_JOYSTICK;
	else if (devnum < idev[IDTYPE_JOYSTICK].get_num() + idev[IDTYPE_MOUSE].get_num())
		return IDTYPE_MOUSE;
	else if (devnum < idev[IDTYPE_JOYSTICK].get_num() + idev[IDTYPE_MOUSE].get_num() + idev[IDTYPE_KEYBOARD].get_num())
		return IDTYPE_KEYBOARD;
	else
		return -1;
}

static struct inputdevice_functions *getidf (int devnum)
{
	int type = gettype (devnum);
	if (type < 0)
		return NULL;
	return &idev[type];
}

struct inputevent *inputdevice_get_eventinfo (int evt)
{
	return &events[evt];
}


/* returns number of devices of type "type" */
int inputdevice_get_device_total (int type)
{
	return idev[type].get_num ();
}
/* returns the name of device */
TCHAR *inputdevice_get_device_name (int type, int devnum)
{
	return idev[type].get_friendlyname (devnum);
}
/* returns the name of device */
TCHAR *inputdevice_get_device_name2 (int devnum)
{
	return getidf (devnum)->get_friendlyname (inputdevice_get_device_index (devnum));
}
/* returns machine readable name of device */
TCHAR *inputdevice_get_device_unique_name (int type, int devnum)
{
	return idev[type].get_uniquename (devnum);
}
/* returns state (enabled/disabled) */
int inputdevice_get_device_status (int devnum)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	if (idf == NULL)
		return -1;
	struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
	return uid->enabled;
}

/* set state (enabled/disabled) */
void inputdevice_set_device_status (int devnum, int enabled)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	int num = inputdevice_get_device_index (devnum);
	struct uae_input_device *uid = get_uid (idf, num);
	if (enabled) { // disable incompatible devices ("super device" vs "raw device")
		for (int i = 0; i < idf->get_num (); i++) {
			if (idf->get_flags (i) != idf->get_flags (num)) {
				struct uae_input_device *uid2 = get_uid (idf, i);
				uid2->enabled = 0;
			}
		}
	}
	uid->enabled = enabled;
}

/* returns number of axis/buttons and keys from selected device */
int inputdevice_get_widget_num (int devnum)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	return idf->get_widget_num (inputdevice_get_device_index (devnum));
}

// return name of event, do not use ie->name directly
void inputdevice_get_eventname (const struct inputevent *ie, TCHAR *out)
{
	if (!out)
		return;
	_tcscpy (out, ie->name);
}

int inputdevice_iterate (int devnum, int num, TCHAR *name, int *af)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	static int id_iterator;
	struct inputevent *ie;
	int mask, data, flags, type;
	int devindex = inputdevice_get_device_index (devnum);

	*af = 0;
	*name = 0;
	for (;;) {
		ie = &events[++id_iterator];
		if (!ie->confname) {
			id_iterator = 0;
			return 0;
		}
		mask = 0;
		type = idf->get_widget_type (devindex, num, NULL, NULL);
		if (type == IDEV_WIDGET_BUTTON || type == IDEV_WIDGET_BUTTONAXIS) {
			if (idf == &idev[IDTYPE_JOYSTICK]) {
				mask |= AM_JOY_BUT;
			} else {
				mask |= AM_MOUSE_BUT;
			}
		} else if (type == IDEV_WIDGET_AXIS) {
			if (idf == &idev[IDTYPE_JOYSTICK]) {
				mask |= AM_JOY_AXIS;
			} else {
				mask |= AM_MOUSE_AXIS;
			}
		} else if (type == IDEV_WIDGET_KEY) {
			mask |= AM_K;
		}
		if (ie->allow_mask & AM_INFO) {
			struct inputevent *ie2 = ie + 1;
			while (!(ie2->allow_mask & AM_INFO)) {
				if (is_event_used (idf, devindex, ie2 - ie, -1)) {
					ie2++;
					continue;
				}
				if (ie2->allow_mask & mask)
					break;
				ie2++;
			}
			if (!(ie2->allow_mask & AM_INFO))
				mask |= AM_INFO;
		}
		if (!(ie->allow_mask & mask))
			continue;
		get_event_data (idf, devindex, num, &data, NULL, &flags, NULL, 0);
		inputdevice_get_eventname (ie, name);
		*af = (flags & ID_FLAG_AUTOFIRE) ? 1 : 0;
		return 1;
	}
}

// return mapped event from devnum/num/sub
int inputdevice_get_mapping (int devnum, int num, int *pflags, int *pport, TCHAR *name, TCHAR *custom, int sub)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	const struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
	int flags = 0, flag, port, data;
	int devindex = inputdevice_get_device_index (devnum);
	TCHAR *customp = NULL;

	if (name)
		_tcscpy (name, _T("<none>"));
	if (custom)
		custom[0] = 0;
	if (pflags)
		*pflags = 0;
	if (pport)
		*pport = 0;
	if (uid == 0 || num < 0)
		return 0;
	if (get_event_data (idf, devindex, num, &data, &customp, &flag, &port, sub) < 0)
		return 0;
	if (customp && custom)
		_tcscpy (custom, customp);
	if (flag & ID_FLAG_AUTOFIRE)
		flags |= IDEV_MAPPED_AUTOFIRE_SET;
	if (flag & ID_FLAG_TOGGLE)
		flags |= IDEV_MAPPED_TOGGLE;
	if (flag & ID_FLAG_GAMEPORTSCUSTOM)
		flags |= IDEV_MAPPED_GAMEPORTSCUSTOM;
	if (!data)
		return 0;
	if (events[data].allow_mask & AM_AF)
		flags |= IDEV_MAPPED_AUTOFIRE_POSSIBLE;
	if (pflags)
		*pflags = flags;
	if (pport)
		*pport = port;
	inputdevice_get_eventname (&events[data], name);
	return data;
}

// set event name/custom/flags to devnum/num/sub
int inputdevice_set_mapping (int devnum, int num, const TCHAR *name, TCHAR *custom, int flags, int port, int sub)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	const struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
	int eid, data, flag, portp, amask;
	TCHAR ename[256];
	int devindex = inputdevice_get_device_index (devnum);
	TCHAR *customp = NULL;

	if (uid == 0 || num < 0)
		return 0;
	if (name) {
		eid = 1;
		while (events[eid].name) {
			inputdevice_get_eventname (&events[eid], ename);
			if (!_tcscmp(ename, name))
				break;
			eid++;
		}
		if (!events[eid].name)
			return 0;
		if (events[eid].allow_mask & AM_INFO)
			return 0;
	} else {
		eid = 0;
	}
	if (get_event_data (idf, devindex, num, &data, &customp, &flag, &portp, sub) < 0)
		return 0;
	if (data >= 0) {
		amask = events[eid].allow_mask;
		flag &= ~(ID_FLAG_AUTOFIRE | ID_FLAG_TOGGLE | ID_FLAG_GAMEPORTSCUSTOM);
		if (amask & AM_AF) {
			flag |= (flags & IDEV_MAPPED_AUTOFIRE_SET) ? ID_FLAG_AUTOFIRE : 0;
			flag |= (flags & IDEV_MAPPED_TOGGLE) ? ID_FLAG_TOGGLE : 0;
		}
		flag |= (flags & IDEV_MAPPED_GAMEPORTSCUSTOM) ? ID_FLAG_GAMEPORTSCUSTOM : 0;
		if (port >= 0)
			portp = port;
		put_event_data (idf, devindex, num, eid, custom, flag, portp, sub);
		return 1;
	}
	return 0;
}

int inputdevice_get_widget_type (int devnum, int num, TCHAR *name)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	return idf->get_widget_type (inputdevice_get_device_index (devnum), num, name, 0);
}

static int config_change;

void inputdevice_config_change (void)
{
	config_change = 1;
}

int inputdevice_config_change_test (void)
{
	int v = config_change;
	config_change = 0;
	return v;
}

// copy configuration #src to configuration #dst
void inputdevice_copyconfig (const struct uae_prefs *src, struct uae_prefs *dst)
{
	int i, j;

	dst->input_selected_setting = src->input_selected_setting;
	dst->input_joymouse_multiplier = src->input_joymouse_multiplier;
	dst->input_joymouse_deadzone = src->input_joymouse_deadzone;
	dst->input_joystick_deadzone = src->input_joystick_deadzone;
	dst->input_joymouse_speed = src->input_joymouse_speed;
	dst->input_mouse_speed = src->input_mouse_speed;
	dst->input_autofire_linecnt = src->input_autofire_linecnt;
	copyjport (src, dst, 0);
	copyjport (src, dst, 1);
	copyjport (src, dst, 2);
	copyjport (src, dst, 3);

	for (i = 0; i < MAX_INPUT_SETTINGS; i++) {
		for (j = 0; j < MAX_INPUT_DEVICES; j++) {
			memcpy (&dst->joystick_settings[i][j], &src->joystick_settings[i][j], sizeof (struct uae_input_device));
			memcpy (&dst->mouse_settings[i][j], &src->mouse_settings[i][j], sizeof (struct uae_input_device));
			memcpy (&dst->keyboard_settings[i][j], &src->keyboard_settings[i][j], sizeof (struct uae_input_device));
		}
	}

	inputdevice_updateconfig (dst);
}

static void swapjoydevice (struct uae_input_device *uid, int **swaps)
{
	for (int i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (int j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			bool found = false;
			for (int k = 0; k < 2 && !found; k++) {
				int evtnum;
				for (int kk = 0; (evtnum = swaps[k][kk]) >= 0 && !found; kk++) {
					if (uid->eventid[i][j] == evtnum) {
						uid->eventid[i][j] = swaps[1 - k][kk];
						found = true;
					} else {
						for (int jj = 0; axistable[jj] >= 0; jj += 3) {
							if (evtnum == axistable[jj] || evtnum == axistable[jj + 1] || evtnum == axistable[jj + 2]) {
								for (int ii = 0; ii < 3; ii++) {
									if (uid->eventid[i][j] == axistable[jj + ii]) {
										int evtnum2 = swaps[1 - k][kk];
										for (int m = 0; axistable[m] >= 0; m += 3) {
											if (evtnum2 == axistable[m] || evtnum2 == axistable[m + 1] || evtnum2 == axistable[m + 2]) {
												uid->eventid[i][j] = axistable[m + ii];
												found = true;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

// swap gameports ports, remember to handle customized ports too
void inputdevice_swap_compa_ports (struct uae_prefs *prefs, int portswap)
{
	struct jport tmp;
	if ((prefs->jports[portswap].id == JPORT_CUSTOM || prefs->jports[portswap + 1].id == JPORT_CUSTOM)) {
		int *swaps[2];
		swaps[0] = rem_ports[portswap];
		swaps[1] = rem_ports[portswap + 1];
		for (int l = 0; l < MAX_INPUT_DEVICES; l++) {
			swapjoydevice (&prefs->joystick_settings[GAMEPORT_INPUT_SETTINGS][l], swaps);
			swapjoydevice (&prefs->mouse_settings[GAMEPORT_INPUT_SETTINGS][l], swaps);
			swapjoydevice (&prefs->keyboard_settings[GAMEPORT_INPUT_SETTINGS][l], swaps);
		}
	}
	memcpy (&tmp, &prefs->jports[portswap], sizeof (struct jport));
	memcpy (&prefs->jports[portswap], &prefs->jports[portswap + 1], sizeof (struct jport));
	memcpy (&prefs->jports[portswap + 1], &tmp, sizeof (struct jport));
	inputdevice_updateconfig (prefs);
}

// swap device "devnum" ports 0<>1 and 2<>3
void inputdevice_swap_ports (struct uae_prefs *p, int devnum)
{
	const struct inputdevice_functions *idf = getidf (devnum);
	struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
	int i, j, k, event, unit;
	const struct inputevent *ie, *ie2;

	for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
		for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			event = uid->eventid[i][j];
			if (event <= 0)
				continue;
			ie = &events[event];
			if (ie->unit <= 0)
				continue;
			unit = ie->unit;
			k = 1;
			while (events[k].confname) {
				ie2 = &events[k];
				if (ie2->type == ie->type && ie2->data == ie->data && ie2->unit - 1 == ((ie->unit - 1) ^ 1) &&
					ie2->allow_mask == ie->allow_mask && uid->port[i][j] == 0) {
					uid->eventid[i][j] = k;
					break;
				}
				k++;
			}
		}
	}
}

// copy whole configuration #x-slot to another
void inputdevice_copy_single_config (struct uae_prefs *p, int src, int dst, int devnum)
{
	if (src == dst)
		return;
	if (devnum < 0 || gettype (devnum) == IDTYPE_JOYSTICK)
		memcpy (p->joystick_settings[dst], p->joystick_settings[src], sizeof (struct uae_input_device) * MAX_INPUT_DEVICES);
	if (devnum < 0 || gettype (devnum) == IDTYPE_MOUSE)
		memcpy (p->mouse_settings[dst], p->mouse_settings[src], sizeof (struct uae_input_device) * MAX_INPUT_DEVICES);
	if (devnum < 0 || gettype (devnum) == IDTYPE_KEYBOARD)
		memcpy (p->keyboard_settings[dst], p->keyboard_settings[src], sizeof (struct uae_input_device) * MAX_INPUT_DEVICES);
}

void inputdevice_acquire (int allmode)
{
	int i;

	DebOut("entered\n");

	inputdevice_unacquire ();
	DebOut("allmode: %d\n", allmode);

	DebOut("0 (MAX_INPUT_DEVICES: %d)\n", MAX_INPUT_DEVICES);
	for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		DebOut("0.%d\n",i);
		if ((use_joysticks[i] && allmode >= 0) || (allmode && !idev[IDTYPE_JOYSTICK].get_flags (i)))
			idev[IDTYPE_JOYSTICK].acquire (i, 0);
	}

	for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		DebOut("1.%d\n",i);
		DebOut("1.%d: use_mice[%d]=%d\n",i,i,use_mice[i]);
		DebOut("1.%d: idev[IDTYPE_MOUSE]=%lx\n",i,idev[IDTYPE_MOUSE]);
		DebOut("1.%d: idev[IDTYPE_MOUSE].get_flags (%d)=%d\n",i,i,idev[IDTYPE_MOUSE].get_flags (i));
		if ((use_mice[i] && allmode >= 0) || (allmode && !idev[IDTYPE_MOUSE].get_flags (i)))
			idev[IDTYPE_MOUSE].acquire (i, allmode < 0);
	}
	DebOut("2\n");
	for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		if ((use_keyboards[i] && allmode >= 0) || (allmode <  0 && !idev[IDTYPE_KEYBOARD].get_flags (i)))
			idev[IDTYPE_KEYBOARD].acquire (i, allmode < 0);
	}
	//    if (!input_acquired)
	//	write_log (_T("input devices acquired (%s)\n"), allmode ? "all" : "selected only");
	input_acquired = 1;
	DebOut("done.\n");
}

void inputdevice_unacquire (void)
{
	int i;

	//    if (input_acquired)
	//	write_log (_T("input devices unacquired\n"));
	input_acquired = 0;
	for (i = 0; i < MAX_INPUT_DEVICES; i++)
		idev[IDTYPE_JOYSTICK].unacquire (i);
	for (i = 0; i < MAX_INPUT_DEVICES; i++)
		idev[IDTYPE_MOUSE].unacquire (i);
	for (i = 0; i < MAX_INPUT_DEVICES; i++)
		idev[IDTYPE_KEYBOARD].unacquire (i);
}

void inputdevice_testrecord (int type, int num, int wtype, int wnum, int state)
{
	if (wnum < 0) {
		testmode = -1;
		return;
	}
	if (testmode_count >= TESTMODE_MAX)
		return;
	if (type == IDTYPE_KEYBOARD) {
		if (wnum == 0x100) {
			wnum = -1;
		} else {
			struct uae_input_device *na = &keyboards[num];
			int j = 0;
			while (j < MAX_INPUT_DEVICE_EVENTS && na->extra[j] >= 0) {
				if (na->extra[j] == wnum) {
					wnum = j;
					break;
				}
				j++;
			}
			if (j >= MAX_INPUT_DEVICE_EVENTS || na->extra[j] < 0)
				type = -1;
		}
	}
	// wait until previous event is released before accepting new ones
	for (int i = 0; i < TESTMODE_MAX; i++) {
		struct teststore *ts2 = &testmode_wait[i];
		if (ts2->testmode_num < 0)
			continue;
		if (ts2->testmode_num != num || ts2->testmode_type != type || ts2->testmode_wtype != wtype || ts2->testmode_wnum != wnum)
			continue;
		if (state)
			continue;
		ts2->testmode_num = -1;
	}
	if (!state)
		return;

	//write_log (_T("%d %d %d %d %d\n"), type, num, wtype, wnum, state);
	struct teststore *ts = &testmode_data[testmode_count];
	ts->testmode_type = type;
	ts->testmode_num = num;
	ts->testmode_wtype = wtype;
	ts->testmode_wnum = wnum;
	ts->testmode_state = state;
	testmode_count++;
}

int inputdevice_istest (void)
{
	return testmode;
}
void inputdevice_settest (int set)
{
	testmode = set;
	testmode_count = 0;
	testmode_wait[0].testmode_num = -1;
	testmode_wait[1].testmode_num = -1;
}

int inputdevice_testread_count (void)
{
	inputdevice_read ();
	if (testmode != 1) {
		testmode = 0;
		return -1;
	}
	return testmode_count;
}

int inputdevice_testread (int *devnum, int *wtype, int *state)
{
	inputdevice_read ();
	if (testmode != 1) {
		testmode = 0;
		return -1;
	}
	if (testmode_count > 0) {
		testmode_count--;
		struct teststore *ts = &testmode_data[testmode_count];
		*devnum = getdevnum (ts->testmode_type, ts->testmode_num);
		*wtype = idev[ts->testmode_type].get_widget_first (ts->testmode_num, ts->testmode_wtype) + ts->testmode_wnum;
		*state = ts->testmode_state;
		if (ts->testmode_state)
			memcpy (&testmode_wait[testmode_count], ts, sizeof (struct teststore));
		return 1;
	}
	return 0;
}

/* Call this function when host machine's joystick/joypad/etc button state changes
* This function translates button events to Amiga joybutton/joyaxis/keyboard events
*/

/* button states:
* state = -1 -> mouse wheel turned or similar (button without release)
* state = 1 -> button pressed
* state = 0 -> button released
*/

void setjoybuttonstate (int joy, int button, int state)
{
	if (testmode) {
		inputdevice_testrecord (IDTYPE_JOYSTICK, joy, IDEV_WIDGET_BUTTON, button, state);
		if (state < 0)
			inputdevice_testrecord (IDTYPE_JOYSTICK, joy, IDEV_WIDGET_BUTTON, button, 0);
		return;
	}
#if 0
	if (ignoreoldinput (joy)) {
		if (state)
			switchdevice (&joysticks[joy], button, 1);
		return;
	}
#endif
	setbuttonstateall (&joysticks[joy], &joysticks2[joy], button, state ? 1 : 0);
}

/* buttonmask = 1 = normal toggle button, 0 = mouse wheel turn or similar
*/
void setjoybuttonstateall (int joy, uae_u32 buttonbits, uae_u32 buttonmask)
{
	int i;

#if 0
	if (ignoreoldinput (joy))
		return;
#endif
	for (i = 0; i < ID_BUTTON_TOTAL; i++) {
		if (buttonmask & (1 << i))
			setbuttonstateall (&joysticks[joy], &joysticks2[joy], i, (buttonbits & (1 << i)) ? 1 : 0);
		else if (buttonbits & (1 << i))
			setbuttonstateall (&joysticks[joy], &joysticks2[joy], i, -1);
	}
}
/* mouse buttons (just like joystick buttons)
*/
void setmousebuttonstateall (int mouse, uae_u32 buttonbits, uae_u32 buttonmask)
{
	int i;

	for (i = 0; i < ID_BUTTON_TOTAL; i++) {
		if (buttonmask & (1 << i))
			setbuttonstateall (&mice[mouse], &mice2[mouse], i, (buttonbits & (1 << i)) ? 1 : 0);
		else if (buttonbits & (1 << i))
			setbuttonstateall (&mice[mouse], &mice2[mouse], i, -1);
	}
}

void setmousebuttonstate (int mouse, int button, int state)
{
	if (testmode) {
		inputdevice_testrecord (IDTYPE_MOUSE, mouse, IDEV_WIDGET_BUTTON, button, state);
		return;
	}
	setbuttonstateall (&mice[mouse], &mice2[mouse], button, state);
}

/* same for joystick axis (analog or digital)
* (0 = center, -max = full left/top, max = full right/bottom)
*/
void setjoystickstate (int joy, int axis, int state, int max)
{
	struct uae_input_device *id = &joysticks[joy];
	struct uae_input_device2 *id2 = &joysticks2[joy];
	int deadzone = currprefs.input_joymouse_deadzone * max / 100;
	int i, v1, v2;

	if (testmode) {
		inputdevice_testrecord (IDTYPE_JOYSTICK, joy, IDEV_WIDGET_AXIS, axis, state);
		return;
	}
	v1 = state;
	v2 = id2->states[axis];
	if (v1 < deadzone && v1 > -deadzone)
		v1 = 0;
	if (v2 < deadzone && v2 > -deadzone)
		v2 = 0;
	if (v1 == v2)
		return;
	if (input_play && state)
		inprec_realtime ();
	if (input_play)
		return;
	if (!joysticks[joy].enabled) {
		if (v1)
			switchdevice (&joysticks[joy], axis * 2 + (v1 < 0 ? 0 : 1), 0);
		return;
	}
	for (i = 0; i < MAX_INPUT_SUB_EVENT; i++)
		handle_input_event (id->eventid[ID_AXIS_OFFSET + axis][i], state, max, id->flags[ID_AXIS_OFFSET + axis][i] & ID_FLAG_AUTOFIRE, true, false);
	id2->states[axis] = state;
}
int getjoystickstate (int joy)
{
	if (testmode)
		return 1;
	return joysticks[joy].enabled;
}

void setmousestate (int mouse, int axis, int data, int isabs)
{
	int i, v, diff;
	int *mouse_p, *oldm_p;
	double d;
	struct uae_input_device *id = &mice[mouse];
	static double fract[MAX_INPUT_DEVICES][MAX_INPUT_DEVICE_EVENTS];

	if (testmode) {
		inputdevice_testrecord (IDTYPE_MOUSE, mouse, IDEV_WIDGET_AXIS, axis, data);
		// fake "release" event
		inputdevice_testrecord (IDTYPE_MOUSE, mouse, IDEV_WIDGET_AXIS, axis, 0);
		return;
	}
	if (input_play)
		return;
	if (!mice[mouse].enabled) {
		if (isabs && currprefs.input_tablet > 0) {
			if (axis == 0)
				lastmx = data;
			else
				lastmy = data;
			if (axis)
				mousehack_helper ();
		}
		return;
	}
	d = 0;
	mouse_p = &mouse_axis[mouse][axis];
	oldm_p = &oldm_axis[mouse][axis];
	if (!isabs) {
		// eat relative movements while in mousehack mode
		if (currprefs.input_tablet == TABLET_MOUSEHACK && mousehack_alive ())
			return;
		*oldm_p = *mouse_p;
		*mouse_p += data;
		d = (*mouse_p - *oldm_p) * currprefs.input_mouse_speed / 100.0;
	} else {
		d = data - *oldm_p;
		*oldm_p = data;
		*mouse_p += d;
		if (axis == 0)
			lastmx = data;
		else
			lastmy = data;
		if (axis)
			mousehack_helper ();
		if (currprefs.input_tablet == TABLET_MOUSEHACK && mousehack_alive ())
			return;
	}
	v = (int)d;
	fract[mouse][axis] += d - v;
	diff = (int)fract[mouse][axis];
	v += diff;
	fract[mouse][axis] -= diff;
	for (i = 0; i < MAX_INPUT_SUB_EVENT; i++)
		handle_input_event (id->eventid[ID_AXIS_OFFSET + axis][i], v, 0, 0, true, false);
}

int getmousestate (int joy)
{
	if (testmode)
		return 1;
	return mice[joy].enabled;
}

void warpmode (int mode)
{
	int fr, fr2;

	fr = currprefs.gfx_framerate;
	if (fr == 0)
		fr = -1;
	fr2 = currprefs.turbo_emulation;
	if (fr2 == -1)
		fr2 = 0;

	if (mode < 0) {
		if (currprefs.turbo_emulation) {
			changed_prefs.gfx_framerate = currprefs.gfx_framerate = fr2;
			currprefs.turbo_emulation = 0;
		} else {
			currprefs.turbo_emulation = fr;
		}
	} else if (mode == 0 && currprefs.turbo_emulation) {
		if (currprefs.turbo_emulation > 0)
			changed_prefs.gfx_framerate = currprefs.gfx_framerate = fr2;
		currprefs.turbo_emulation = 0;
	} else if (mode > 0 && !currprefs.turbo_emulation) {
		currprefs.turbo_emulation = fr;
	}
	if (currprefs.turbo_emulation) {
		if (!currprefs.cpu_cycle_exact && !currprefs.blitter_cycle_exact)
			changed_prefs.gfx_framerate = currprefs.gfx_framerate = 10;
		pause_sound ();
	} else {
		resume_sound ();
	}
	compute_vsynctime ();
#ifdef RETROPLATFORM
	rp_turbo (currprefs.turbo_emulation);
#endif
	changed_prefs.turbo_emulation = currprefs.turbo_emulation;
	config_changed = 1;
}

void pausemode (int mode)
{
	if (mode < 0)
		pause_emulation = pause_emulation ? 0 : 9;
	else
		pause_emulation = mode;
	config_changed = 1;
}

int jsem_isjoy (int port, const struct uae_prefs *p)
{
	int v = JSEM_DECODEVAL (port, p);
	if (v < JSEM_JOYS)
		return -1;
	v -= JSEM_JOYS;
	if (v >= inputdevice_get_device_total (IDTYPE_JOYSTICK))
		return -1;
	return v;
}

int jsem_ismouse (int port, const struct uae_prefs *p)
{
	int v = JSEM_DECODEVAL (port, p);
	if (v < JSEM_MICE)
		return -1;
	v -= JSEM_MICE;
	if (v >= inputdevice_get_device_total (IDTYPE_MOUSE))
		return -1;
	return v;
}

int jsem_iskbdjoy (int port, const struct uae_prefs *p)
{
	int v = JSEM_DECODEVAL (port, p);
	if (v < JSEM_KBDLAYOUT)
		return -1;
	v -= JSEM_KBDLAYOUT;
	if (v >= JSEM_LASTKBD)
		return -1;
	return v;
}

int inputdevice_joyport_config (struct uae_prefs *p, TCHAR *value, int portnum, int mode, int type)
{
	switch (type)
	{
	case 1:
	case 2:
		{
			int i, j;
			for (j = 0; j < MAX_JPORTS; j++) {
				struct inputdevice_functions *idf;
				int type = IDTYPE_MOUSE;
				int idnum = JSEM_MICE;
				if (j > 0) {
					type = IDTYPE_JOYSTICK;
					idnum = JSEM_JOYS;
				}
				idf = &idev[type];
				for (i = 0; i < idf->get_num (); i++) {
					TCHAR *name1 = idf->get_friendlyname (i);
					TCHAR *name2 = idf->get_uniquename (i);
					if ((name1 && !_tcscmp (name1, value)) || (name2 && !_tcscmp (name2, value))) {
						p->jports[portnum].id = idnum + i;
						if (mode >= 0)
							p->jports[portnum].mode = mode;
						config_changed = 1;
						return 1;
					}
				}
			}
		}
		break;
	case 0:
		{
			int start = JPORT_NONE, got = 0, max = 0;
			TCHAR *pp = 0;
			if (_tcsncmp (value, _T("kbd"), 3) == 0) {
				start = JSEM_KBDLAYOUT;
				pp = value + 3;
				got = 1;
				max = JSEM_LASTKBD;
			} else if (_tcsncmp (value, _T("joy"), 3) == 0) {
				start = JSEM_JOYS;
				pp = value + 3;
				got = 1;
				max = idev[IDTYPE_JOYSTICK].get_num ();
			} else if (_tcsncmp (value, _T("mouse"), 5) == 0) {
				start = JSEM_MICE;
				pp = value + 5;
				got = 1;
				max = idev[IDTYPE_MOUSE].get_num ();
			} else if (_tcscmp (value, _T("none")) == 0) {
				got = 2;
			} else if (_tcscmp (value, _T("custom")) == 0) {
				got = 2;
				start = JPORT_CUSTOM;
			}
			if (got) {
				if (pp) {
					int v = _tstol (pp);
					if (start >= 0) {
						if (start == JSEM_KBDLAYOUT && v > 0)
							v--;
						if (v >= 0) {
							if (v >= max)
								v = 0;
							start += v;
							got = 2;
						}
					}
				}
				if (got == 2) {
					p->jports[portnum].id = start;
					if (mode >= 0)
						p->jports[portnum].mode = mode;
					config_changed = 1;
					return 1;
				}
			}
		}
		break;
	}
	return 0;
}

int inputdevice_getjoyportdevice (int port, int val)
{
	int idx;
	if (val == JPORT_CUSTOM) {
		idx = inputdevice_get_device_total (IDTYPE_JOYSTICK) + JSEM_LASTKBD;
		if (port < 2)
			idx += inputdevice_get_device_total (IDTYPE_MOUSE);
	} else if (val < 0) {
		idx = -1;
	} else if (val >= JSEM_MICE) {
		idx = val - JSEM_MICE;
		if (idx >= inputdevice_get_device_total (IDTYPE_MOUSE))
			idx = 0;
		else
			idx += inputdevice_get_device_total (IDTYPE_JOYSTICK);
		idx += JSEM_LASTKBD;
	} else if (val >= JSEM_JOYS) {
		idx = val - JSEM_JOYS;
		if (idx >= inputdevice_get_device_total (IDTYPE_JOYSTICK))
			idx = 0;
		idx += JSEM_LASTKBD;
	} else {
		idx = val - JSEM_KBDLAYOUT;
	}
	return idx;
}

// for state recorder use only!

uae_u8 *save_inputstate (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	for (int i = 0; i < MAX_JPORTS; i++) {
		save_u16 (joydir[i]);
		save_u16 (joybutton[i]);
		save_u16 (otop[i]);
		save_u16 (obot[i]);
		save_u16 (oleft[i]);
		save_u16 (oright[i]);
	}
	for (int i = 0; i < NORMAL_JPORTS; i++) {
		save_u16 (cd32_shifter[i]);
		for (int j = 0; j < 2; j++) {
			save_u16 (pot_cap[i][j]);
			save_u16 (joydirpot[i][j]);
		}
	}
	for (int i = 0; i < NORMAL_JPORTS; i++) {
		for (int j = 0; j < MOUSE_AXIS_TOTAL; j++) {
			save_u16 (mouse_delta[i][j]);
			save_u16 (mouse_deltanoreset[i][j]);
		}
		save_u16 (mouse_frame_x[i]);
		save_u16 (mouse_frame_y[i]);
	}
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_inputstate (uae_u8 *src)
{
	for (int i = 0; i < MAX_JPORTS; i++) {
		joydir[i] = restore_u16 ();
		joybutton[i] = restore_u16 ();
		otop[i] = restore_u16 ();
		obot[i] = restore_u16 ();
		oleft[i] = restore_u16 ();
		oright[i] = restore_u16 ();
	}
	for (int i = 0; i < NORMAL_JPORTS; i++) {
		cd32_shifter[i] = restore_u16 ();
		for (int j = 0; j < 2; j++) {
			pot_cap[i][j] = restore_u16 ();
			joydirpot[i][j] = restore_u16 ();
		}
	}
	for (int i = 0; i < NORMAL_JPORTS; i++) {
		for (int j = 0; j < MOUSE_AXIS_TOTAL; j++) {
			mouse_delta[i][j] = restore_u16 ();
			mouse_deltanoreset[i][j] = restore_u16 ();
		}
		mouse_frame_x[i] = restore_u16 ();
		mouse_frame_y[i] = restore_u16 ();
	}
	return src;
}

void clear_inputstate (void)
{
	return;
	for (int i = 0; i < MAX_JPORTS; i++) {
		horizclear[i] = 1;
		vertclear[i] = 1;
	}
}