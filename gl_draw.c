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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"
#include <stdlib.h>

#define GL_COLOR_INDEX8_EXT     0x80E5

cvar_t          cg_conclock = {"cg_conclock", "1", true};

shader_t	*draw_disc = NULL;		//Hard disc activity shader
shader_t	*draw_backtile = NULL;	//Shader to tile when screen is sized down
shader_t	*draw_conback = NULL;	//Console Backbround shader
int			char_texture;			//the font (one of the only things left that doesn't have shaders)

void GL_Bind (int texnum)
{
	if (currenttexture == texnum)
		return;
	currenttexture = texnum;
//#ifdef _WIN32
//	bindTexFunc (GL_TEXTURE_2D, texnum);
//#else
	glBindTexture(GL_TEXTURE_2D, texnum);
//#endif

/*
	Don't do this anisotropy is part of the texture state, it is set when you bind the texture

        if (gl_texturefilteranisotropic)	// <AWE> anisotropic texture filtering
        {
            glTexParameterfv (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &gl_textureanisotropylevel);
        }
*/
}

void GL_BindAdvanced(gltexture_t *tex) {
	if (tex->dynamic) {
		Roq_UpdateTexture(tex);
	}
	GL_Bind(tex->texnum);
}

void Print_Tex_Cache_f(void);
void R_ReloadShaders_f(void);
void ReloadTextures_f(void);
void ToggleStatistics_f(void);

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	int		i;
	int		maxsize;
	char	buf[64];

	Cvar_RegisterVariable (&gl_max_size);
	Cvar_RegisterVariable (&gl_picmip);
	Cvar_RegisterVariable (&gl_gloss);
	Cvar_RegisterVariable (&gl_compress_textures);
	Cvar_RegisterVariable (&willi_gray_colormaps);
	Cvar_RegisterVariable (&cg_conclock);

	//Check maximum texture size
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
	sprintf(buf,"%i",maxsize);
	Cvar_Set ("gl_max_size", buf);
	Con_Printf("GL_MAX_TEXTURE_SIZE %i\n", maxsize);

	Cmd_AddCommand ("gl_texturemode", &Draw_TextureMode_f);
	Cmd_AddCommand ("roq_info", &Roq_Info_f);
	Cmd_AddCommand ("gl_texcache",Print_Tex_Cache_f);
	Cmd_AddCommand ("reloadTextures",ReloadTextures_f);
	Cmd_AddCommand ("reloadShaders", R_ReloadShaders_f); 
	Cmd_AddCommand ("bumptatistics", ToggleStatistics_f);

	// Load the console font
	char_texture = EasyTgaLoad("textures/system/charset.tga");

	//Global images
	glow_texture_object = GL_Load2DAttenTexture();
	atten3d_texture_object = GL_Load3DAttenTexture();
	normcube_texture_object = GL_LoadNormalizationCubemap();
	halo_texture_object = EasyTgaLoad("penta/utflare5.tga");

	for (i=0; i<8; i++) {
		char name[32];
		sprintf(name,"penta/caust%02.2i.tga",i*4);
		caustics_textures[i] = EasyTgaLoad(name);
	}

	//Load mirror dummys
	R_InitMirrorChains();

	R_InitGlare();
}



/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character (int x, int y, int num)
{
	int		row, col, nnum;
	float		frow, fcol, size;

	/* use shaders for fonts ?
	float glyph_width, glyph_height;
	shader_t *shader;

	vec3_t vertices[4];
	float texcoords[8];

	int indecies[]={0,1,2,2,1,3};

	vertexdef_t verts[]={
		vertices,0,              // vertices
		texcoords,0,             // texcoords
		NULL,0,                  // lightmapcoords
		NULL,0,                  // tangents
		NULL,0,                  // binormals
		NULL,0,                  // normals
		NULL,0                   // colors
	};

	glyph_pix_width = 8;
	glyph_pix_height = 16;


	vertices[0][0] = x; 
	vertices[0][1] = y;
	vertices[0][3] = 0.0f;

	texcoords[0] = fcol;
	texcoords[1] = frow;
  
	vertices[1][0] = x + glyph_pix_width; 
	vertices[1][1] = y ;
	vertices[1][3] = 0.0f;  

	texcoords[2] = fcol + glyph_width;
	texcoords[3] = frow;

	vertices[2][0] = x;
	vertices[2][1] = y + glyph_pix_height;
	vertices[2][3] = 0.0f;  

	texcoords[4] = fcol;
	texcoords[5] = frow + glyph_height;

	vertices[3][0] = x + glyph_pix_width; 
	vertices[3][1] = y + glyph_pix_height;
	vertices[3][3] = 0.0f;  

	texcoords[6] = fcol + glyph_width;
	texcoords[7] = frow + glyph_height;
	*/


	if (num == 32)
		return;		// space

	if (num > 127) glColor3ub(255,50,10);
	else  glColor3ub(255,255,255);

	nnum = num & 127;

	if (y <= 0)
		return;			// totally off screen

	row = nnum>>4;
	col = nnum&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;


	glDisable(GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_Bind (char_texture);

	glBegin (GL_QUADS);
	glTexCoord2f (fcol, frow);
	glVertex2f (x, y);
	glTexCoord2f (fcol + size, frow);
	glVertex2f (x+8, y);
	glTexCoord2f (fcol + size, frow + size);
	glVertex2f (x+8, y+16);
	glTexCoord2f (fcol, frow + size);
	glVertex2f (x, y+16);
	glEnd ();

	glEnable(GL_ALPHA_TEST);
	glDisable (GL_BLEND);

	if (num > 127) glColor3ub(255,255,255);
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, char *str)
{
	while (*str)
	{
		Draw_Character (x, y, *str);
		str++;
		x += 8;
	}
}

#define MAX_FONT_CACHE 128
static drawfont_t fontCache[MAX_FONT_CACHE];
static int numFonts = 0;

static void Draw_FillDefaultFont(drawfont_t *defaultFont) {
	int i;
	strcpy(defaultFont->name,"DEFAULT");
	defaultFont->width = 128;
	defaultFont->height = 256;
	defaultFont->charHeight = 16;
	defaultFont->shader = GL_ShaderForName("screen/font");
	for (i=0; i<256; i++) {
		defaultFont->metrics[i].ofsx = (i&15)*8;
		defaultFont->metrics[i].ofsy = (i>>4)*defaultFont->charHeight;
		defaultFont->metrics[i].A = 0;
		defaultFont->metrics[i].B = 8;
		defaultFont->metrics[i].C = 0;
	}
}

/**
	Load a shader and the font metrics to go along with that shader.
*/
static void Draw_LoadFont(const char *name, drawfont_t *font) {
	char filename[128];
	short *data;
	int i;

	sprintf(filename,"%s.dat",name);
	data = (short *)COM_LoadTempFile(filename);

	//if the data was not found make it the default font
	if (!data) {
		Con_Printf("Error: Font file %s not found\n", filename);
		Draw_FillDefaultFont(font);
		strcpy(font->name,name);
		return;
	}

	strcpy(font->name,name);
	font->width = 256;
	font->height = 256;
	font->charHeight = 16;
	font->shader = GL_ShaderForName(name);
	for (i=0; i<256; i++) {
		font->metrics[i].ofsx = data[0];
		font->metrics[i].ofsy = (i>>4)*font->charHeight;
		font->metrics[i].A = data[1];
		font->metrics[i].B = data[2];
		font->metrics[i].C = data[3];
		data += 4;
	}
}

void Draw_InitFonts() {
	numFonts = 0;
	Draw_FillDefaultFont(&fontCache[0]);
	numFonts++;
}

drawfont_t *Draw_FontForName(const char *name) {
	int i;

	//see if it exists
	for (i=0; i<numFonts; i++) {
		if (!strcmp(name, fontCache[i].name)) {
			return &fontCache[i]; 
		}
	}

	//load a new font
	if (numFonts < MAX_FONT_CACHE) {
		Draw_LoadFont(name, &fontCache[numFonts]);
		numFonts++;
		return &fontCache[numFonts-1];
	}
	Sys_Error("To many fonts loaded\n");
	return NULL;
}

drawfont_t *Draw_DefaultFont(void) {
	return &fontCache[0];
}

#define CHAR_BATCH 64
#define MAX_CHARV CHAR_BATCH*4
#define MAX_CHARI CHAR_BATCH*6
static float vertBuff[MAX_CHARV*3];
static float texBuff[MAX_CHARV*2];
static int indexBuff[MAX_CHARI];
static int	numIndices;
static int	numVerts;
static float *verts;
static float *texcoords;
static int *indices;

static void Draw_EmitFontChar(drawfont_t *f, char ch, int y, int *linepos) {
	float uofs, vofs, usize, vsize;
	drawchar_t *c = &f->metrics[ch];

	//Advance cursor position
	(*linepos) += c->A;

	//Don't draw spaces
	if (ch == ' ') {
		(*linepos) += c->B;
		(*linepos) += c->C;
		return;
	}

	//Calculate texture coords
	uofs = c->ofsx / (float)f->width;
	vofs = c->ofsy / (float)f->height;

	usize = c->B / (float)f->width;
	vsize = f->charHeight / (float)f->height;

	//Add vertices to the buffer
	verts[0] = (*linepos);
	verts[1] = y;
	verts[2] = 0;
	verts += 3;
	texcoords[0] = uofs;
	texcoords[1] = vofs;
	texcoords += 2;

	verts[0] = (*linepos)+c->B;
	verts[1] = y;
	verts[2] = 0;
	verts += 3;
	texcoords[0] = uofs+usize;
	texcoords[1] = vofs;
	texcoords += 2;

	verts[0] = (*linepos)+c->B;
	verts[1] = y+f->charHeight;
	verts[2] = 0;
	verts += 3;
	texcoords[0] = uofs+usize;
	texcoords[1] = vofs+vsize;
	texcoords += 2;

	verts[0] = (*linepos);
	verts[1] = y+f->charHeight;
	verts[2] = 0;
	verts += 3;
	texcoords[0] = uofs;
	texcoords[1] = vofs+vsize;
	texcoords += 2;

	//Add to the index buffer
	indices[0] = numVerts+0; 
	indices[1] = numVerts+1;  
	indices[2] = numVerts+2;  
	indices[3] = numVerts+2;  
	indices[4] = numVerts+3;  
	indices[5] = numVerts+0;  
	indices += 6;

	numIndices += 6;
	numVerts +=4;

	//Advance cursor position
	(*linepos) += c->B;
	(*linepos) += c->C;
}

/**
	Draw a string with the given font
*/
void Draw_StringFont(int x, int y, char *str, drawfont_t *font) {
	int xofs;
	vertexdef_t vertDef;

	memset(&vertDef, 0, sizeof(vertDef));
	vertDef.vertices = GL_WrapUserPointer(&vertBuff[0]);
	vertDef.texcoords = GL_WrapUserPointer(&texBuff[0]);
    
	numIndices = 0;
	numVerts = 0;
	verts = &vertBuff[0];
	texcoords = &texBuff[0];
	indices = &indexBuff[0];

	xofs = x;
	while (*str) {
		//Newlines
		if ((*str) == '\n') {
			y+=font->charHeight;
			xofs = x;
			str++;
			continue;
		}

		//Add it to the vertex buffer
		Draw_EmitFontChar(font, (*str), y, &xofs);

		//Draw in 64 char batches
		if (numIndices >= MAX_CHARI) {
			gl_bumpdriver.drawTriangleListBase ( &vertDef, indexBuff, numIndices, font->shader, -1);
			numIndices = 0;
			numVerts = 0;
			verts = &vertBuff[0];
			texcoords = &texBuff[0];
			indices = &indexBuff[0];
		}
		str++;
	}

	if (numIndices) {
		gl_bumpdriver.drawTriangleListBase ( &vertDef, indexBuff, numIndices, font->shader, -1);
		numIndices = 0;
	}
}

/**
	Calculate the width of a given string in pixels based on the font metrics.
*/
int Draw_StringWidth(char *str, drawfont_t *font) {
	int width = 0;
	int maxwidth = 0;
	while (*str) {
		//Newlines
		if ((*str) == '\n') {
			if (width > maxwidth)
				maxwidth = width;
			width = 0;
		} else {
			width += font->metrics[(*str)].A;
			width += font->metrics[(*str)].B;
			width += font->metrics[(*str)].C;
		}
		str++;
	}
	if (width > maxwidth)
		maxwidth = width;
	return maxwidth;
}

/**
	Calculate the height of a given string in pixels based on the font metrics.
*/
int Draw_StringHeight(char *str, drawfont_t *font) {
	int height = font->charHeight;
	qboolean newline = false;
	while (*str) {
		//increase height if there are characters after the newline.
		if (newline) {
			height+=font->charHeight;
			newline = false;
		}
		//Newlines
		if ((*str) == '\n') {
			newline = true;
		}
		str++;
	}
	return height;
}

void Draw_StringDefault(int x, int y, char *str) {
	Draw_StringFont(x, y, str, &fontCache[0]);
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x1, int y1, int x2, int y2, shader_t *shader)
{
	vec3_t vertices[] = { 
		{x1,y1,0.0f}, 
		{x2,y1,0.0f}, 
		{x1,y2,0.0f}, 
		{x2,y2,0.0f} 
	}; 
	float texcoords[] = { 
		0.0f,0.0f, 
		1.0f,0.0f, 
		0.0f,1.0f, 
		1.0f,1.0f 
	}; 

	DriverPtr lighmapscoords = DRVNULL; 
	DriverPtr tangents = DRVNULL; 
	DriverPtr binormals = DRVNULL; 
	DriverPtr normals = DRVNULL; 
	DriverPtr colors = DRVNULL; 
    
	int indecies[]={0,1,2,2,1,3}; 
	vertexdef_t verts;

	memset(&verts, 0, sizeof(verts));
	verts.vertices = GL_WrapUserPointer(vertices);
	verts.texcoords = GL_WrapUserPointer(texcoords);

	gl_bumpdriver.drawTriangleListBase ( &verts, indecies, 6, shader, -1); 
}

/*
=============
Draw_Border
	A border is 4 quads, they fit inside the given rectangle.
=============
*/
void Draw_Border (int x1, int y1, int x2, int y2, int borderWidth, shader_t *shader)
{
	float texcoords[32];
	int indecies[24];
	int baseindecies[]={0,1,2,2,3,0};
	float basetexcoords[] = {0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 0.0f,1.0f}; 
	int i, j;

	vec3_t vertices[] = { 
		{x1,y1,0.0f}, 
		{x2,y1,0.0f}, 
		{x2-borderWidth,y1+borderWidth,0.0f},
		{x1+borderWidth,y1+borderWidth,0.0f}, 
		{x2,y1,0.0f}, 
		{x2,y2,0.0f}, 
		{x2-borderWidth,y2-borderWidth,0.0f}, 
		{x2-borderWidth,y1+borderWidth,0.0f}, 
		{x2,y2,0.0f},
		{x1,y2,0.0f},
		{x1+borderWidth,y2-borderWidth,0.0f},
		{x2-borderWidth,y2-borderWidth,0.0f},
		{x1,y2,0.0f},
		{x1,y1,0.0f},
		{x1+borderWidth,y1+borderWidth,0.0f},
		{x1+borderWidth,y2-borderWidth,0.0f}
	}; 

	DriverPtr lighmapscoords = DRVNULL; 
	DriverPtr tangents = DRVNULL; 
	DriverPtr binormals = DRVNULL; 
	DriverPtr normals = DRVNULL; 
	DriverPtr colors = DRVNULL; 
        
	vertexdef_t verts;
	memset(&verts, 0, sizeof(verts));
	verts.vertices = GL_WrapUserPointer(vertices);
	verts.texcoords = GL_WrapUserPointer(texcoords);


	//FIXME: Make some parts static so we don't have to redo for every border we draw
	for (i=0; i<4; i++) {
		for (j=0; j<6; j++) {
			indecies[i*6+j] = baseindecies[j]+i*4;
		}
		for (j=0; j<4; j++) {
			texcoords[i*8+j*2] = basetexcoords[j*2];
			texcoords[i*8+j*2+1] = basetexcoords[j*2+1];
		}

	}
    
	gl_bumpdriver.drawTriangleListBase ( &verts, indecies, 24, shader, -1); 
}

/*
=============
Draw_AlphaPic
=============
*/
void Draw_AlphaPic (int x1, int y1, int x2, int y2, shader_t *pic, float alpha)
{
	glDisable(GL_ALPHA_TEST);
	glEnable (GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glCullFace(GL_FRONT);
	glColor4f (1,1,1,alpha);
	Draw_Pic (x1, y1, x2, y2, pic);
	glColor4f (1,1,1,1);
	glEnable(GL_ALPHA_TEST);
	glDisable (GL_BLEND);
}

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground (int lines)
{
	if (!draw_conback) {
		Draw_FillRGB (0, lines - vid.height, vid.width, lines, 0.0f, 0.0f, 1.0f);
	} else {
		Draw_Pic (0, lines - vid.height, vid.width, lines, draw_conback);
	}
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{

	if (!draw_backtile) {
		Draw_FillRGB (x, y, w, h, 0.5f, 0.5f, 0.5f);
		return;
	}


	if (!draw_backtile->numcolorstages) {
		Draw_FillRGB (x, y, w, h, 0.5f, 0.5f, 0.5f);
		return;
	}

	//FIXME: This is not really a shader
	glColor3f (1,1,1);
	GL_Bind (*(int *)draw_backtile->colorstages[0].texture[0]->texnum);
	glBegin (GL_QUADS);
	glTexCoord2f (x/64.0, y/64.0);
	glVertex2f (x, y);
	glTexCoord2f ( (x+w)/64.0, y/64.0);
	glVertex2f (x+w, y);
	glTexCoord2f ( (x+w)/64.0, (y+h)/64.0);
	glVertex2f (x+w, y+h);
	glTexCoord2f ( x/64.0, (y+h)/64.0 );
	glVertex2f (x, y+h);
	glEnd ();
}

/*
=============
Draw_FillRGB

Fills a box of pixels with a single color
=============
*/
void Draw_FillRGB (int x, int y, int w, int h, float r, float g, float b)
{
	glDisable (GL_TEXTURE_2D);
	glColor3f (r,g,b);

	glBegin (GL_QUADS);

	glVertex2f (x,y);
	glVertex2f (x+w, y);
	glVertex2f (x+w, y+h);
	glVertex2f (x, y+h);

	glEnd ();
	glColor3f (1,1,1);
	glEnable (GL_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	glEnable (GL_BLEND);
	glDisable (GL_TEXTURE_2D);
	glColor4f (0, 0, 0, 0.8);
	glBegin (GL_QUADS);

	glVertex2f (0,0);
	glVertex2f (vid.width, 0);
	glVertex2f (vid.width, vid.height);
	glVertex2f (0, vid.height);

	glEnd ();
	glColor4f (1,1,1,1);
	glEnable (GL_TEXTURE_2D);
	glDisable (GL_BLEND);

	Sbar_Changed();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc (void)
{
	if (!draw_disc)
		return;
	glDrawBuffer  (GL_FRONT);
	Draw_Pic (vid.width - 24, 0, vid.width + 8, 32, draw_disc);
	glDrawBuffer  (GL_BACK);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc (void)
{
}

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
void GL_Set2D (void)
{
	glViewport (glx, gly, glwidth, glheight);

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	glOrtho  (0, vid.width, vid.height, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
//	glDisable (GL_BLEND);
	glEnable (GL_ALPHA_TEST);

//PENTA: fix console font
	glEnable(GL_BLEND);
	glAlphaFunc(GL_GREATER,0.01);
//	glDisable (GL_ALPHA_TEST);

	glColor4f (1,1,1,1);
}

//====================================================================

