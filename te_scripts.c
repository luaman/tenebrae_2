/*
Copyright (C) 2001-2002 Charles Hollemeersch

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Utility routines for the script parsing
*/

#include "te_scripts.h"
#include "quakedef.h"

typedef struct {
	char *name;
	int	value;
} glident_t;

static glident_t blendmodes[] = {
	{"GL_ONE", GL_ONE},
	{"GL_ZERO", GL_ZERO},
	{"GL_SRC_ALPHA", GL_SRC_ALPHA},
	{"GL_DST_ALPHA", GL_DST_ALPHA},
	{"GL_SRC_COLOR", GL_SRC_COLOR},
	{"GL_DST_COLOR", GL_DST_COLOR},
	{"GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA},
	{"GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA},
	{"GL_ONE_MINUS_SRC_COLOR", GL_ONE_MINUS_SRC_COLOR},
	{"GL_ONE_MINUS_DST_COLOR", GL_ONE_MINUS_DST_COLOR}
};

int SHADER_BlendModeForName(char *name) {
	int i;

	for (i=0 ; i< 10 ; i++)
	{
		if (!Q_strcasecmp (blendmodes[i].name, name) )
			return blendmodes[i].value;
	}
	if (i == 10)
	{
		return -1;
	}

	return -1;
}

int SC_BlendModeForName(char *name) {
	int i;

	for (i=0 ; i< 10 ; i++)
	{
		if (!Q_strcasecmp (blendmodes[i].name, name) )
			return blendmodes[i].value;
	}
	if (i == 10)
	{
		PARSERERROR("Bad blend mode name");
		return GL_ONE;
	}

	return GL_ONE;
}
