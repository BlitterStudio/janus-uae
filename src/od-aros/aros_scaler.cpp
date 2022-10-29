/************************************************************************ 
 *
 * AROS scaler
 *
 * Copyright 2011 Oliver Brunner <aros<at>oliver-brunner.de>
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

//#define JUAE_DEBUG

#include "sysconfig.h"
#include "sysdeps.h"

#include "gfxfilter.h"

struct uae_filter uaefilters[] =
{
	{ UAE_FILTER_NULL, 0, 1, _T("Null filter"), _T("null"), UAE_FILTER_MODE_16_16 | UAE_FILTER_MODE_32_32 },

	{ UAE_FILTER_SCALE2X, 0, 2, _T("Scale2X"), _T("scale2x"), UAE_FILTER_MODE_16_16 | UAE_FILTER_MODE_32_32 },

	{ UAE_FILTER_HQ2X, 0, 2, _T("hq2x"), _T("hq2x"), UAE_FILTER_MODE_16_16 | UAE_FILTER_MODE_16_32, },

	{ UAE_FILTER_HQ3X, 0, 3, _T("hq3x"), _T("hq3x"), UAE_FILTER_MODE_16_16 | UAE_FILTER_MODE_16_32 },

	{ UAE_FILTER_HQ4X, 0, 4, _T("hq4x"), _T("hq4x"), UAE_FILTER_MODE_16_16 | UAE_FILTER_MODE_16_32 },

	{ UAE_FILTER_SUPEREAGLE, 0, 2, _T("SuperEagle"), _T("supereagle"), UAE_FILTER_MODE_16_16 | UAE_FILTER_MODE_16_32 },

	{ UAE_FILTER_SUPER2XSAI, 0, 2, _T("Super2xSaI"), _T("super2xsai"), UAE_FILTER_MODE_16_16 | UAE_FILTER_MODE_16_32 },

	{ UAE_FILTER_2XSAI, 0, 2, _T("2xSaI"), _T("2xsai"), UAE_FILTER_MODE_16_16 | UAE_FILTER_MODE_16_32 },

	{ UAE_FILTER_PAL, 1, 1, _T("PAL"), _T("pal"), UAE_FILTER_MODE_16_16 | UAE_FILTER_MODE_32_32 },

	{ 0 }
};

