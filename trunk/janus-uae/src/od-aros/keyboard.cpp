/************************************************************************ 
 *
 * AROS keyboad handling
 *
 * Copyright 2011       Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
 *
 * Janus-UAE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-UAE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 *
 ************************************************************************/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "inputdevice.h"
#include "uae.h"
#include "aros.h"

static int get_kb_num (void)
{
    return 1;
}

static int get_kb_widget_num (int kb)
{
    return 128;
}

static int get_kb_widget_first (int kb, int type)
{
    return 0;
}

static int get_kb_widget_type (int kb, int num, TCHAR *name, uae_u32 *code)
{
    // fix me
    *code = num;
    return IDEV_WIDGET_KEY;
}

static int keyhack (int scancode, int pressed, int num)
{
    return scancode;
}

static void read_kb (void)
{
#ifdef CATWEASEL
		uae_u8 kc;
		if (catweasel_read_keyboard (&kc))
		{
			inputdevice_do_keyboard (kc & 0x7f, !(kc & 0x80));
		}
#endif
}

static int init_kb (void)
{
    return 1;
}

static void close_kb (void)
{
}

static int acquire_kb (int num, int flags)
{
    return 1;
}

static void unacquire_kb (int num)
{
}

static TCHAR *get_kb_name (int kb)
{
    return "Default AROS keyboard";
}


static TCHAR *get_kb_friendlyname (int kb)
{
	return get_kb_name(kb);
}

static TCHAR *get_kb_uniquename (int kb)
{
	return get_kb_name(kb);
}

static int get_kb_flags (int num)
{
	return 0;
}

#if 0
struct inputdevice_functions inputdevicefunc_keyboard =
{
    init_kb,
    close_kb,
    acquire_kb,
    unacquire_kb,
    read_kb,
    get_kb_num,
		get_kb_friendlyname, 
		get_kb_uniquename,
    get_kb_widget_num,
    get_kb_widget_type,
    get_kb_widget_first,
		get_kb_flags
};
#endif

#if 0
static struct uae_input_device_kbr_default keytrans_amiga[] = {
	/* TODO ?? */
	{ -1, 0 }
};

static struct uae_input_device_kbr_default *keytrans[] = {
	/* TODO ?? */
  keytrans_amiga,
  keytrans_amiga,
  keytrans_amiga


};

static int kb_np[] = { -1, 0 };

static int *kbmaps[] = {
	kb_np
};
#endif

static struct uae_input_device_kbr_default keytrans_amiga[] = {
  { -1, 0 }
};

static struct uae_input_device_kbr_default keytrans_pc1[] = {
  { -1, 0 }
};

static struct uae_input_device_kbr_default keytrans_pc2[] = {
  { -1, 0 }
};

static struct uae_input_device_kbr_default *keytrans[] = {
  keytrans_amiga,
  keytrans_pc1,
  keytrans_pc2
};

static int kb_np[] = { -1, -1 };
static int kb_ck[] = { -1, -1 };
static int kb_se[] = { -1, -1 };
static int kb_np3[] = { -1, -1 };
static int kb_ck3[] = { -1, -1 };
static int kb_se3[] = { -1, -1 };


static int kb_cd32_np[] = { -1, -1 };
static int kb_cd32_ck[] = { -1, -1 };
static int kb_cd32_se[] = { -1, -1 };

static int kb_cdtv[] = { -1, -1 };

static int kb_xa1[] = { -1, -1 };
static int kb_xa2[] = { -1, -1 };
static int kb_arcadia[] = { -1, -1 };
static int kb_arcadiaxa[] = { -1, -1 };

static int *kbmaps[] = {
	kb_np, kb_ck, kb_se, kb_np3, kb_ck3, kb_se3,
	kb_cd32_np, kb_cd32_ck, kb_cd32_se,
	kb_xa1, kb_xa2, kb_arcadia, kb_arcadiaxa, kb_cdtv
};

void keyboard_settrans (void)
{
	inputdevice_setkeytranslation (keytrans, kbmaps);
}

