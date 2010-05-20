/************************************************************************ 
 *
 * Amiga Interface Inlcude
 *
 * Copyright 2009-2010 Oliver Brunner - aros<at>oliver-brunner.de
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

void init_pointer (void);
void free_pointer (void);
void o1i_Display_Update(int start,int i);
int do_inhibit_frame (int onoff);

typedef enum {
    DONT_KNOW = -1,
    INSIDE_WINDOW,
    OUTSIDE_WINDOW
} POINTER_STATE;

extern POINTER_STATE pointer_state;
extern int gMouseState;
extern int  screen_is_picasso;
extern char picasso_invalid_lines[];
extern int  picasso_invalid_start;
extern int  picasso_invalid_end;
extern int use_delta_buffer;
extern uae_u8 *oldpixbuf;
extern int usepub;

