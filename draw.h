/*
Copyright (C) 1996-1997 Id Software, Inc.

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

*/

// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

#include "gl_model.h" // for shader_t

typedef struct {
	int ofsx;	//x offset into the texture
	int ofsy;	//y offset into the texture
	int A;		//Width of whitespace to the left of the char
	int B;		//Width of the char's pixels
	int C;		//Width of the whiretspace to the right of the char
} drawchar_t;

typedef struct drawfont_s {
	char name[64];
	drawchar_t metrics[256];
	shader_t *shader;
	int			width;	//width of the font texture
	int			height; //height of the font texture
	int			charHeight; //height of a single character
} drawfont_t;


void Draw_Init (void);
void Draw_Character (int x, int y, int num);
void Draw_DebugChar (char num);
void Draw_ConsoleBackground (int lines);
void Draw_BeginDisc (void);
void Draw_EndDisc (void);
void Draw_TileClear (int x, int y, int w, int h);
void Draw_Fill (int x, int y, int w, int h, int c);
void Draw_FillRGB (int x, int y, int w, int h, float r, float g, float b);
void Draw_FadeScreen (void);
void Draw_String (int x, int y, char *str);
drawfont_t *Draw_FontForName(const char *name);
drawfont_t *Draw_DefaultFont(void);
int Draw_StringWidth(char *str, drawfont_t *font);
int Draw_StringHeight(char *str, drawfont_t *font);
void Draw_StringFont(int x, int y, char *str, drawfont_t *font);
