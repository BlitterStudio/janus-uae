 /*
  * UAE - The Un*x Amiga Emulator
  *
  * [n]curses output.
  *
  * There are 17 color modes:
  *  -H0/-H1 are black/white output
  *  -H2 through -H16 give you different color allocation strategies. On my
  *    system, -H14 seems to give nice results.
  *
  * Copyright 1997 Samuel Devulder, Bernd Schmidt
  *
  * Hatchet job to get it to build again: 2005 Richard Drummond
  */

/****************************************************************************/

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>
#include <signal.h>

/****************************************************************************/

#include "options.h"
#include "uae.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "keybuf.h"
#include "disk.h"
#include "debug.h"

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

/****************************************************************************/

#define MAXGRAYCHAR 128

enum {
    MYCOLOR_BLACK, MYCOLOR_RED, MYCOLOR_GREEN, MYCOLOR_BLUE,
    MYCOLOR_YELLOW, MYCOLOR_CYAN, MYCOLOR_MAGENTA, MYCOLOR_WHITE
};

static const int mycolor2curses_map [] = {
    COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_BLUE,
    COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA, COLOR_WHITE
};

static const int mycolor2pair_map[] = { 1,2,3,4,5,6,7,8 };

static chtype graychar[MAXGRAYCHAR];
static unsigned int maxc;
static int max_graychar;
static int curses_on;

static int *x2graymap;

/* Keyboard and mouse */

static int keystate[256];
static int keydelay = 20;

static void curses_exit(void);

/****************************************************************************/

static void curses_insert_disk (void)
{
    curses_exit();
    flush_screen(0,0);
}

/****************************************************************************/

/*
 * old:	fmt = " .,:=(Io^vM^vb*X^#M^vX*boI(=:. ^b^vobX^#M" doesn't work: "^vXb*oI(=:. ";
 * good:	fmt = " .':;=(IoJpgFPEB#^vgpJoI(=;:'. ^v^b=(IoJpgFPEB";
 *
 * 	fmt = " .,:=(Io*b^vM^vX^#M^vXb*oI(=:. ";
 */

static void init_graychar (void)
{
    chtype *p = graychar;
    chtype attrs;
    unsigned int i, j;
    char *fmt;

    attrs = termattrs();
    if ((currprefs.color_mode & 1) == 0 && (attrs & (A_REVERSE | A_BOLD)))
	fmt = " .':;=(IoJpgFPEB#^vgpJoI(=;:'. ^v^boJpgFPEB";
    else if ((currprefs.color_mode & 1) == 0 && (attrs & A_REVERSE))
	fmt = " .':;=(IoJpgFPEB#^vgpJoI(=;:'. ";
    else
	/* One could find a better pattern.. */
	fmt = " .`'^^\",:;i!1Il+=tfjxznuvyZYXHUOQ0MWB";
    attrs = A_NORMAL | COLOR_PAIR (0);
    while(*fmt) {
	if(*fmt == '^') {
	    ++fmt;
	    switch(*fmt) {
		case 's': case 'S': attrs ^= A_STANDOUT; break;
		case 'v': case 'V': attrs ^= A_REVERSE; break;
		case 'b': case 'B': attrs ^= A_BOLD; break;
		case 'd': case 'D': attrs ^= A_DIM; break;
		case 'u': case 'U': attrs ^= A_UNDERLINE; break;
		case 'p': case 'P': attrs  = A_NORMAL; break;
		case '#': if(ACS_CKBOARD == ':')
			       *p++ = (attrs | '#');
			  else *p++ = (attrs | ACS_CKBOARD); break;
		default:  *p++ = (attrs | *fmt); break;
	    }
	    ++fmt;
	} else *p++ = (attrs | *fmt++);
	if(p >= graychar + MAXGRAYCHAR) break;
    }
    max_graychar = (p - graychar) - 1;

    for (i = 0; i <= maxc; i++)
	x2graymap[i] = i * max_graychar / maxc;
#if 0
    for(j=0;j<LINES;++j) {
	move(j,0);
	for(i=0;i<COLS;++i) addch(graychar[i % (max_graychar+1)]);
    }
    refresh();
    sleep(3);
#endif
}

static int x_map[900], y_map[700], y_rev_map [700];


/****************************************************************************/

static void init_colors (void)
{
    unsigned int i;

    maxc = 0;

    for(i = 0; i < 4096; ++i) {
	int r,g,b,r1,g1,b1;
	int m, comp;
	int ctype;

	r =  i >> 8;
	g = (i >> 4) & 15;
	b =  i & 15;

	xcolors[i] = (77 * r + 151 * g + 28 * b)/16;
	if (xcolors[i] > maxc)
	    maxc = xcolors[i];
	m = r;
	if (g > m)
	    m = g;
	if (b > m)
	    m = b;
	if (m == 0) {
	    xcolors[i] |= MYCOLOR_WHITE << 8; /* to get gray instead of black in dark areas */
	    continue;
	}

	if ((currprefs.color_mode & ~1) != 0) {
	    r1 = r*15 / m;
	    g1 = g*15 / m;
	    b1 = b*15 / m;

	    comp = 8;
	    for (;;) {
		if (b1 < comp) {
		    if (r1 < comp)
			ctype = MYCOLOR_GREEN;
		    else if (g1 < comp)
			ctype = MYCOLOR_RED;
		    else
			ctype = MYCOLOR_YELLOW;
		} else {
		    if (r1 < comp) {
			if (g1 < comp)
			    ctype = MYCOLOR_BLUE;
			else
			    ctype = MYCOLOR_CYAN;
		    } else if (g1 < comp)
			    ctype = MYCOLOR_MAGENTA;
		    else {
			comp += 4;
			if (comp == 12 && (currprefs.color_mode & 2) != 0)
			    continue;
			ctype = MYCOLOR_WHITE;
		    }
		}
		break;
	    }
	    if (currprefs.color_mode & 8) {
		if (ctype == MYCOLOR_BLUE && xcolors[i] > /*27*/50)
		    ctype = r1 > (g1+2) ? MYCOLOR_MAGENTA : MYCOLOR_CYAN;
		if (ctype == MYCOLOR_RED && xcolors[i] > /*75*/ 90)
		    ctype = b1 > (g1+6) ? MYCOLOR_MAGENTA : MYCOLOR_YELLOW;
	    }
	    xcolors[i] |= ctype << 8;
	}
    }
    if (currprefs.color_mode & 4) {
	unsigned int j;
	for (j = MYCOLOR_RED; j < MYCOLOR_WHITE; j++) {
	    int best = 0;
	    unsigned int maxv = 0;
	    int multi, divi;

	    for (i = 0; i < 4096; i++)
		if ((xcolors[i] & 255) > maxv && (xcolors[i] >> 8) == j) {
		    best = i;
		    maxv = (xcolors[best] & 255);
		}
	    /* Now maxv is the highest intensity a color of type J is supposed to have.
	     * In  reality, it will most likely only have intensity maxv*multi/divi.
	     * We try to correct this. */
	    maxv = maxv * 256 / maxc;

	    divi = 256;
	    switch (j) {
	     case MYCOLOR_RED:     multi = 77; break;
	     case MYCOLOR_GREEN:   multi = 151; break;
	     case MYCOLOR_BLUE:    multi = 28; break;
	     case MYCOLOR_YELLOW:  multi = 228; break;
	     case MYCOLOR_CYAN:    multi = 179; break;
	     case MYCOLOR_MAGENTA: multi = 105; break;
	     default: abort();
	    }
#if 1 /* This makes the correction less extreme */
	    if (! (currprefs.color_mode & 8))
		multi = (multi + maxv) / 2;
#endif
	    for (i = 0; i < 4096; i++) {
		unsigned int v = xcolors[i];
		if ((v >> 8) != j)
		    continue;
		v &= 255;
		/* I don't think either of these is completely correct, but
		 * the first one produces rather good results. */
#if 1
		v = v * divi / multi;
		if (v > maxc)
		    v = maxc;
#else
		v = v * 256 / maxv);
		if (v > maxc)
		    /*maxc = v*/abort();
#endif
		xcolors[i] = v | (j << 8);
	    }
	}
    }
    x2graymap = (int *)malloc(sizeof(int) * (maxc+1));
}

static void curses_init (void)
{
    initscr ();

    start_color ();
    if (! has_colors () || COLOR_PAIRS < 20 /* whatever */)
	currprefs.color_mode &= 1;
    else {
	init_pair (1, COLOR_BLACK, COLOR_BLACK);
	init_pair (2, COLOR_RED, COLOR_BLACK);
	init_pair (3, COLOR_GREEN, COLOR_BLACK);
	init_pair (4, COLOR_BLUE, COLOR_BLACK);
	init_pair (5, COLOR_YELLOW, COLOR_BLACK);
	init_pair (6, COLOR_CYAN, COLOR_BLACK);
	init_pair (7, COLOR_MAGENTA, COLOR_BLACK);
	init_pair (8, COLOR_WHITE, COLOR_BLACK);
    }
    write_log ("curses_init: %d pairs available\n", COLOR_PAIRS);

    cbreak(); noecho();
    nonl (); intrflush(stdscr, FALSE); keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    leaveok(stdscr, TRUE);

    attron (A_NORMAL | COLOR_PAIR (0));
    bkgd(' '|COLOR_PAIR(0));

#ifdef NCURSES_MOUSE_VERSION
    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED |
	      BUTTON2_PRESSED | BUTTON2_RELEASED |
	      BUTTON3_PRESSED | BUTTON3_RELEASED |
	      REPORT_MOUSE_POSITION, NULL);
#endif

    init_graychar();
    curses_on = 1;
}

static void curses_exit(void)
{
#ifdef NCURSES_MOUSE_VERSION
    mousemask(0, NULL);
#endif

    nocbreak(); echo(); nl(); intrflush(stdscr, TRUE);
    keypad(stdscr, FALSE); nodelay(stdscr, FALSE); leaveok(stdscr, FALSE);
    endwin();
    curses_on = 0;
}

/****************************************************************************/

static int getgraycol (int x, int y)
{
    uae_u8 *bufpt;
    int xs, xl, ys, yl, c, cm;

    xl = x_map[x+1] - (xs = x_map[x]);
    yl = y_map[y+1] - (ys = y_map[y]);

    bufpt = ((uae_u8 *)gfxvidinfo.bufmem) + ys*currprefs.gfx_width + xs;

    cm = c = 0;
    for(y = 0; y < yl; y++, bufpt += currprefs.gfx_width)
	for(x = 0; x < xl; x++) {
	    c += bufpt[x];
	    ++cm;
	}
    if (cm)
	c /= cm;
    if (! currprefs.curses_reverse_video)
	c = maxc - c;
    return graychar[x2graymap[c]];
}

static int getcol (int x, int y)
{
    uae_u16 *bufpt;
    int xs, xl, ys, yl, c, cm;
    int bestcol = MYCOLOR_BLACK, bestccnt = 0;
    unsigned char colcnt [8];

    memset (colcnt, 0 , sizeof colcnt);

    xl = x_map[x+1] - (xs = x_map[x]);
    yl = y_map[y+1] - (ys = y_map[y]);

    bufpt = ((uae_u16 *)gfxvidinfo.bufmem) + ys*currprefs.gfx_width + xs;

    cm = c = 0;
    for(y = 0; y < yl; y++, bufpt += currprefs.gfx_width)
	for(x = 0; x < xl; x++) {
	    int v = bufpt[x];
	    int cnt;

	    c += v & 0xFF;
	    cnt = ++colcnt[v >> 8];
	    if (cnt > bestccnt) {
		bestccnt = cnt;
		bestcol = v >> 8;
	    }
	    ++cm;
	}
    if (cm)
	c /= cm;
    if (! currprefs.curses_reverse_video)
	c = maxc - c;
    return (graychar[x2graymap[c]] & ~A_COLOR) | COLOR_PAIR (mycolor2pair_map[bestcol]);
}

STATIC_INLINE void flush_line_txt (struct vidbuf_description *gfxinfo, int row)
{
    int x;
    move (row, 0);
    if (currprefs.color_mode < 2)
	for (x = 0; x < COLS; ++x) {
	    int c;

	    c = getgraycol (x, row);
	    addch (c);
	}
    else
	for (x = 0; x < COLS; ++x) {
	    int c;

	    c = getcol (x, row);
	    addch (c);
	}
}

static void flush_line_curses (struct vidbuf_description *gfxinfo, int y)
{
    if(!curses_on)
	return;
    flush_line_txt (gfxinfo, y_rev_map[y]);
}

static void flush_block_curses (struct vidbuf_description *gfxinfo, int start_line, int end_line)
{
    int row;
    int start_row;
    int stop_row;

    if (!curses_on)
	return;

    start_row = y_rev_map[start_line];
    stop_row  = y_rev_map[end_line];

    for (row = start_row; row <= stop_row; row++)
	flush_line_txt (gfxinfo, row);
}

static int lockscr_curses (struct vidbuf_description *gfxinfo)
{
    return 1;
}

static void unlockscr_curses (struct vidbuf_description *gfxinfo)
{
}

static void flush_screen_curses (struct vidbuf_description *gfxinfo, int start_line, int end_line)
{
    if (!debugging && !curses_on) {
	curses_init ();
	flush_block (0, gfxinfo->height - 1);
    }
    refresh ();
}

/****************************************************************************/

struct bstring *video_mode_menu = NULL;

void vidmode_menu_selected(int a)
{
}

int graphics_setup (void)
{
    return 1;
}

int graphics_init (void)
{
    unsigned int i;

    if (currprefs.color_mode > 16)
	write_log ("Bad color mode selected. Using default.\n"), currprefs.color_mode = 0;

    init_colors ();

    curses_init ();
    write_log ("Using %s.\n", longname ());

    if (debugging)
	curses_exit ();

    /* we have a 320x256x8 pseudo screen */

    currprefs.gfx_width = 320;
    currprefs.gfx_height = 256;
    currprefs.gfx_lores = 1;
    currprefs.gfx_linedbl = 0;

    gfxvidinfo.width = currprefs.gfx_width;
    gfxvidinfo.height = currprefs.gfx_height;
    gfxvidinfo.maxblocklines = MAXBLOCKLINES_MAX;
    gfxvidinfo.pixbytes = currprefs.color_mode < 2 ? 1 : 2;
    gfxvidinfo.rowbytes = gfxvidinfo.pixbytes * currprefs.gfx_width;
    gfxvidinfo.bufmem = (uae_u8 *)calloc(gfxvidinfo.rowbytes, currprefs.gfx_height+1);
    gfxvidinfo.linemem = 0;
    gfxvidinfo.emergmem = 0;
    switch (gfxvidinfo.pixbytes) {
     case 1:
	for (i = 0; i < 4096; i++)
	    xcolors[i] = xcolors[i] * 0x01010101;
	break;
     case 2:
	for (i = 0; i < 4096; i++)
	    xcolors[i] = xcolors[i] * 0x00010001;
	break;
    }
    if(!gfxvidinfo.bufmem) {
	write_log("Not enough memory.\n");
	return 0;
    }

    for (i = 0; i < sizeof x_map / sizeof *x_map; i++)
	x_map[i] = (i * currprefs.gfx_width) / COLS;
    for (i = 0; i < sizeof y_map / sizeof *y_map; i++)
	y_map[i] = (i * currprefs.gfx_height) / LINES;
    for (i = 0; i < sizeof y_map / sizeof *y_map - 1; i++) {
	int l1 = y_map[i];
	int l2 = y_map[i+1];
	int j;
	if (l2 >= sizeof y_rev_map / sizeof *y_rev_map)
	    break;
	for (j = l1; j < l2; j++)
	    y_rev_map[j] = i;
    }

    for(i=0; i<256; i++)
	keystate[i] = 0;

    gfxvidinfo.lockscr = lockscr_curses;
    gfxvidinfo.unlockscr = unlockscr_curses;
    gfxvidinfo.flush_line = flush_line_curses;
    gfxvidinfo.flush_block = flush_block_curses;
    gfxvidinfo.flush_screen = flush_screen_curses;

    reset_drawing ();

    return 1;
}

/****************************************************************************/

void graphics_leave (void)
{
    curses_exit ();
}

/****************************************************************************/

static int keycode2amiga (int ch)
{
    switch(ch) {
	case KEY_A1:    return AK_NP7;
	case KEY_UP:    return AK_NP8;
	case KEY_A3:    return AK_NP9;
	case KEY_LEFT:  return AK_NP4;
	case KEY_B2:    return AK_NP5;
	case KEY_RIGHT: return AK_NP6;
	case KEY_C1:    return AK_NP1;
	case KEY_DOWN:  return AK_NP2;
	case KEY_C3:    return AK_NP3;
	case KEY_ENTER: return AK_ENT;
	case 13:        return AK_RET;
	case ' ':       return AK_SPC;
	case 27:        return AK_ESC;
	default: return -1;
    }
}

/***************************************************************************/

void graphics_notify_state (int state)
{
}

/***************************************************************************/

void handle_events (void)
{
    int ch;
    int kc;

    /* Hack to simulate key release */
    for(kc = 0; kc < 256; ++kc) {
	if(keystate[kc]) if(!--keystate[kc]) record_key((kc << 1) | 1);
    }

    if(!curses_on) return;

    while((ch = getch())!=ERR) {
	if(ch == 12) {clearok(stdscr,TRUE);refresh();}
#ifdef NCURSES_MOUSE_VERSION
	if(ch == KEY_MOUSE) {
	    MEVENT ev;
	    if(getmouse(&ev) == OK) {
		int mousex = (ev.x * gfxvidinfo.width) / COLS;
	        int mousey = (ev.y * gfxvidinfo.height) / LINES;
                setmousestate (0, 0, mousex, 1);
	        setmousestate (0, 1 ,mousey, 1);
#if 0
		if(ev.bstate & BUTTON1_PRESSED)  buttonstate[0] = keydelay;
		if(ev.bstate & BUTTON1_RELEASED) buttonstate[0] = 0;
		if(ev.bstate & BUTTON2_PRESSED)  buttonstate[1] = keydelay;
		if(ev.bstate & BUTTON2_RELEASED) buttonstate[1] = 0;
		if(ev.bstate & BUTTON3_PRESSED)  buttonstate[2] = keydelay;
		if(ev.bstate & BUTTON3_RELEASED) buttonstate[2] = 0;
#endif
	    }
	}
#endif
#if 0
	if (ch == 6)  ++lastmx; /* ^F */
	if (ch == 2)  --lastmx; /* ^B */
	if (ch == 14) ++lastmy; /* ^N */
	if (ch == 16) --lastmy; /* ^P */
	if (ch == 11) {buttonstate[0] = keydelay;ch = 0;} /* ^K */
	if (ch == 25) {buttonstate[2] = keydelay;ch = 0;} /* ^Y */
#endif
	if (ch == 15) uae_reset (0); /* ^O */
        if (ch == 23) uae_stop ();   /* ^W (Note: ^Q won't work) */
#if 0
	if (ch == KEY_F(1)) {
	  curses_insert_disk();
	  ch = 0;
	}
#endif
	if(isupper(ch)) {
	    keystate[AK_LSH] =
	    keystate[AK_RSH] = keydelay;
	    record_key(AK_LSH << 1);
	    record_key(AK_RSH << 1);
	    kc = keycode2amiga(tolower(ch));
	    keystate[kc] = keydelay;
	    record_key(kc << 1);
	} else if((kc = keycode2amiga(ch)) >= 0) {
	    keystate[kc] = keydelay;
	    record_key(kc << 1);
	}
    }
}

/***************************************************************************/

void target_specific_usage (void)
{
    printf("----------------------------------------------------------------------------\n");
    printf("[n]curses specific usage:\n");
    printf("  -x : Display reverse video.\n");
    printf("By default uae will assume a black on white display. If yours\n");
    printf("is light on dark, use -x. In case of graphics garbage, ^L will\n");
    printf("redisplay the screen. ^K simulate left mouse button, ^Y RMB.\n");
    printf("If you are using a xterm UAE can use the mouse. Else use ^F ^B\n");
    printf("^P ^N to emulate mouse mouvements.\n");
    printf("----------------------------------------------------------------------------\n");
}

/***************************************************************************/

int check_prefs_changed_gfx (void)
{
    return 0;
}

int debuggable (void)
{
    return 1;
}

int mousehack_allowed (void)
{
    return 0;
}

int is_fullscreen (void)
{
    return 1;
}

int is_vsync (void)
{
    return 0;
}

void toggle_fullscreen (void)
{
};

void toggle_mousegrab (void)
{
}

void screenshot (int mode)
{
   write_log ("Screenshot not supported yet\n");
}

/*
 * Mouse inputdevice functions
 */

/* Hardwire for 3 axes and 3 buttons - although SDL doesn't
 * currently support a Z-axis as such. Mousewheel events are supplied
 * as buttons 4 and 5
 */
#define MAX_BUTTONS	3
#define MAX_AXES	3
#define FIRST_AXIS	0
#define FIRST_BUTTON	MAX_AXES

static int init_mouse (void)
{
   return 1;
}

static void close_mouse (void)
{
   return;
}

static int acquire_mouse (unsigned int num, int flags)
{
   return 1;
}

static void unacquire_mouse (unsigned int num)
{
   return;
}

static unsigned int get_mouse_num (void)
{
    return 1;
}

static const char *get_mouse_name (unsigned int mouse)
{
    return "Default mouse";
}

static unsigned int get_mouse_widget_num (unsigned int mouse)
{
    return MAX_AXES + MAX_BUTTONS;
}

static int get_mouse_widget_first (unsigned int mouse, int type)
{
    switch (type) {
	case IDEV_WIDGET_BUTTON:
	    return FIRST_BUTTON;
	case IDEV_WIDGET_AXIS:
	    return FIRST_AXIS;
    }
    return -1;
}

static int get_mouse_widget_type (unsigned int mouse, unsigned int num, char *name, uae_u32 *code)
{
    if (num >= MAX_AXES && num < MAX_AXES + MAX_BUTTONS) {
	if (name)
	    sprintf (name, "Button %d", num + 1 + MAX_AXES);
	return IDEV_WIDGET_BUTTON;
    } else if (num < MAX_AXES) {
	if (name)
	    sprintf (name, "Axis %d", num + 1);
	return IDEV_WIDGET_AXIS;
    }
    return IDEV_WIDGET_NONE;
}

static void read_mouse (void)
{
    /* We handle mouse input in handle_events() */
}

struct inputdevice_functions inputdevicefunc_mouse = {
    init_mouse,
    close_mouse,
    acquire_mouse,
    unacquire_mouse,
    read_mouse,
    get_mouse_num,
    get_mouse_name,
    get_mouse_widget_num,
    get_mouse_widget_type,
    get_mouse_widget_first
};

/*
 * Keyboard inputdevice functions
 */
static unsigned int get_kb_num (void)
{
    /* SDL supports only one keyboard */
    return 1;
}

static const char *get_kb_name (unsigned int kb)
{
    return "Default keyboard";
}

static unsigned  int get_kb_widget_num (unsigned int kb)
{
    return 255; // fix me
}

static int get_kb_widget_first (unsigned int kb, int type)
{
    return 0;
}

static int get_kb_widget_type (unsigned int kb, unsigned int num, char *name, uae_u32 *code)
{
    // fix me
    *code = num;
    return IDEV_WIDGET_KEY;
}

static int init_kb (void)
{
    return 1;
}

static void close_kb (void)
{
}

static int keyhack (int scancode, int pressed, int num)
{
    return scancode;
}

static void read_kb (void)
{
}

static int acquire_kb (unsigned int num, int flags)
{
    return 1;
}

static void unacquire_kb (unsigned int num)
{
}

struct inputdevice_functions inputdevicefunc_keyboard =
{
    init_kb,
    close_kb,
    acquire_kb,
    unacquire_kb,
    read_kb,
    get_kb_num,
    get_kb_name,
    get_kb_widget_num,
    get_kb_widget_type,
    get_kb_widget_first
};

int getcapslockstate (void)
{
    return 0;
}

void setcapslockstate (int state)
{
}

/*
 * Default inputdevice config for mouse
 */
void input_get_default_mouse (struct uae_input_device *uid)
{
    uid[0].eventid[ID_AXIS_OFFSET + 0][0]   = INPUTEVENT_MOUSE1_HORIZ;
    uid[0].eventid[ID_AXIS_OFFSET + 1][0]   = INPUTEVENT_MOUSE1_VERT;
    uid[0].eventid[ID_AXIS_OFFSET + 2][0]   = INPUTEVENT_MOUSE1_WHEEL;
    uid[0].eventid[ID_BUTTON_OFFSET + 0][0] = INPUTEVENT_JOY1_FIRE_BUTTON;
    uid[0].eventid[ID_BUTTON_OFFSET + 1][0] = INPUTEVENT_JOY1_2ND_BUTTON;
    uid[0].eventid[ID_BUTTON_OFFSET + 2][0] = INPUTEVENT_JOY1_3RD_BUTTON;
    uid[0].enabled = 1;
}

/*
 * Handle gfx specific cfgfile options
 */
void gfx_default_options (struct uae_prefs *p)
{
    p->curses_reverse_video = 0;
}

void gfx_save_options (FILE *f, const struct uae_prefs *p)
{
    fprintf (f, "curses.reverse_video=%s\n", p->curses_reverse_video ? "true" : "false");
}

int gfx_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
    return (cfgfile_yesno (option, value, "reverse_video", &p->curses_reverse_video));
}
