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

extern unsigned char d_15to8table[65536];

/*
cvar_t		gl_nobind = {"gl_nobind", "0"};
cvar_t		gl_max_size = {"gl_max_size", "1024"};
cvar_t		gl_picmip = {"gl_picmip", "0"};
cvar_t          gl_gloss = {"gl_gloss", "0.3"};
cvar_t          gl_compress_textures = {"gl_compress_textures", "0"};
cvar_t          willi_gray_colormaps = {"willi_gray_colormaps", "0"};
*/
cvar_t          cg_conclock = {"cg_conclock", "1", true};

byte		*draw_chars;				// 8*8 graphic characters
shader_t	*draw_disc;
shader_t	*draw_backtile;

int		translate_texture;
int		char_texture;
/*
int			glow_texture_object;	//PENTA: gl texture object of the glow texture
int			normcube_texture_object; //PENTA: normalization cubemap
int			atten1d_texture_object;
int			atten2d_texture_object;
int			atten3d_texture_object;
int			halo_texture_object;
*/

/*
typedef struct
{
	int		texnum;
	float	sl, tl, sh, th;
} glpic_t;
*/

//byte		conback_buffer[sizeof(shader_t)];
//shader_t	*conback = (shader_t *)&conback_buffer;
shader_t        *conback;
/*
int		gl_lightmap_format = 4;
int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;


int		texels;

typedef struct
{
	int		texnum;
	char	identifier[64];
	int		width, height;
	qboolean	mipmap;
	int		type; //gl texture type like GL_TEXTURE_2D etc...
} shader_t;

#define	MAX_GLTEXTURES	1024
shader_t	gltextures[MAX_GLTEXTURES];
int			numgltextures;

qboolean is_overriden;

// <AWE> added local prototypes.
int		GL_Load2DAttenTexture (void);
int		GL_Load3DAttenTexture (void);
int		GL_LoadBumpTexture (void);
int		GL_LoadNormalizationCubemap (void);
int		GL_LoadPicTexture (qpic_t *pic);
*/
void GL_Bind (int texnum)
{
	if (gl_nobind.value)
		texnum = char_texture;
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


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up stupid hardware / drivers

=============================================================================
*/

#define	MAX_SCRAPS		2
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT*4];
qboolean	scrap_dirty;
int			scrap_texnum;

// returns a texture number and the position inside it
int Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error ("Scrap_AllocBlock: full");
	return 0;
}

int	scrap_uploads;

void Scrap_Upload (void)
{
	int		texnum;

	scrap_uploads++;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++) {
		GL_Bind(scrap_texnum + texnum);
		GL_Upload8 (scrap_texels[texnum], "scrap", BLOCK_WIDTH, BLOCK_HEIGHT, false, true, false);
	}
	scrap_dirty = false;
}

//=============================================================================
/* Support Routines */

void Draw_CharToConback (int num, byte *dest)
{
	int		row, col;
	byte	*source;
	int		drawline;
	int		x;

	row = num>>4;
	col = num&15;
	/*
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--)
	{
		for (x=0 ; x<8 ; x++)
			if (source[x] != 255)
				dest[x] = 0x60 + source[x];
		source += 128;
		dest += 320;
	}
	*/
}

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	int		i;
	shader_t	*cb;
	byte	*dest /*, *src*/;	// <AWE> unused because of "#if 0".
	int		x, y;
	char	ver[40];
	//glpic_t	*gl;
	int		start;
	byte	*ncdata;
//	int		fstep;		// <AWE> unused because of "#if 0".


	Cvar_RegisterVariable (&gl_nobind);
	Cvar_RegisterVariable (&gl_max_size);
	Cvar_RegisterVariable (&gl_picmip);
	Cvar_RegisterVariable (&gl_gloss);
	Cvar_RegisterVariable (&gl_compress_textures);
	Cvar_RegisterVariable (&willi_gray_colormaps);
	Cvar_RegisterVariable (&cg_conclock);

	// 3dfx can only handle 256 wide textures
	if (!Q_strncasecmp ((char *)gl_renderer, "3dfx",4) ||
		strstr((char *)gl_renderer, "Glide"))
		Cvar_Set ("gl_max_size", "256");

	Cmd_AddCommand ("gl_texturemode", &Draw_TextureMode_f);
	


	// load the console background and the charset
	// by hand, because we need to write the version
	// string into the background before turning
	// it into a texture

	char_texture = EasyTgaLoad ("charset.tga");

	conback = GL_ShaderForName ("screen/conback");
	/*
	// now turn them into textures
	
	start = Hunk_LowMark();

	cb = GL_CacheTexture ("gfx/conback.lmp");
	if (!cb)
		Sys_Error ("Couldn't load gfx/conback.lmp");

	// hack the version number directly into the pic
#if defined(__linux__)
	sprintf (ver, "(Linux %2.2f, gl %4.2f) %4.2f", (float)LINUX_VERSION, (float)GLQUAKE_VERSION, (float)VERSION);
#elif defined (__APPLE__) || defined (MACOSX)
	sprintf (ver, "(MACOS X %2.2f, gl %4.2f) %4.2f", (float)MACOSX_VERSION, (float)GLQUAKE_VERSION, (float)VERSION);
#else
	sprintf (ver, "(st %4.2f) %4.2f", (float)STQUAKE_VERSION, (float)VERSION);
#endif
	dest = cb->data + 320*186 + 320 - 11 - 8*strlen(ver);
	y = strlen(ver);
	for (x=0 ; x<y ; x++)
		Draw_CharToConback (ver[x], dest+(x<<3));

	conback->width = cb->width;
	conback->height = cb->height;
	ncdata = cb->data;

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	gl = (glpic_t *)conback->data;
	gl->texnum = GL_LoadTexture ("conback", conback->width, conback->height, ncdata, false, false, false);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;

	conback->width = vid.width;
	conback->height = vid.height;
	*/
	// free loaded console
	//Hunk_FreeToLowMark(start);

	// save a texture slot for translated picture
	translate_texture = texture_extension_number++;

	// save slots for scraps
	scrap_texnum = texture_extension_number;
	texture_extension_number += MAX_SCRAPS;

	//
	// get the other pics we need
	//
	draw_disc = GL_ShaderForName ("disc");
	draw_backtile = GL_ShaderForName ("backtile");

	//PENTA: load fallof glow
	glow_texture_object = GL_Load2DAttenTexture();
	atten3d_texture_object = GL_Load3DAttenTexture();

	//Load nomalization cube map
	normcube_texture_object = GL_LoadNormalizationCubemap();

	halo_texture_object = EasyTgaLoad("penta/utflare5.tga");

	for (i=0; i<8; i++) {
		char name[32];
		sprintf(name,"penta/caust%02.2i.tga",i*4);
		caustics_textures[i] = EasyTgaLoad(name);
	}

	//Load water shader textures
	InitShaderTex();
	//load mirror dummys
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

	if (num == 32)
		return;		// space

	if (num > 127) glColor3ub(255,50,10);
		
	nnum = num & 127;
	//nnum = nnum
	
	if (y <= -8)
		return;			// totally off screen

	row = nnum>>4;
	col = nnum&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

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

/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void Draw_DebugChar (char num)
{
}


/*
=============
Draw_AlphaPic
=============

void Draw_AlphaPic (int x, int y, shader_t *pic, float alpha)
{
	glDisable(GL_ALPHA_TEST);
	glEnable (GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glCullFace(GL_FRONT);
	glColor4f (1,1,1,alpha);
	Draw_GLPic (x, y, x+pic->width, y+pic->height, pic);
	glColor4f (1,1,1,1);
	glEnable(GL_ALPHA_TEST);
	glDisable (GL_BLEND);
}
*/
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
	float *lighmapscoords = NULL;
	/*
	float lighmapscoords[] = 
	{
		0,0,
		0,1,
		1,0,
		1,1
		};*/
	float *tangents = NULL;
	float *binormals = NULL;
	float *normals = NULL;
	unsigned char *colors = NULL;

	int indecies[]={0,1,2,2,1,3};

	vertexdef_t verts[]={
		vertices,0,              // vertices
		texcoords,0,             // texcoords
		lighmapscoords,0,        // lightmapcoords
		tangents,0,              // tangents
		binormals,0,             // binormals
		normals,0,               // normals
		colors,0                 // colors
	};
	int i;

	gl_bumpdriver.drawTriangleListBase ( verts, indecies, 6, shader);

}

/*
=============
Draw_TransPic
=============

void Draw_TransPic (int x, int y, shader_t *pic)
{
	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		 (unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error ("Draw_TransPic: bad coordinates");
	}
		
	Draw_Pic (x, y, pic);
}
*/

/*
==============
Draw_PicFilled
==============

void Draw_PicFilled (int x, int y, int xs, int ys, shader_t *pic)
{
        glColor4f (1,1,1,1);
	Draw_GLPic (x, y, xs, ys, pic);
}
*/
/*
===================
Draw_TransPicFilled
===================

void Draw_TransPicFilled (int x, int y, int xs, int ys, shader_t *pic)
{
	
        if (x < 0 || (unsigned)(xs) > vid.width || y < 0 ||
                 (unsigned)(ys) > vid.height)
        {
                Sys_Error ("Draw_TransPic: bad coordinates (%d ,%d )\n", x, y);
        }

        Draw_PicFilled (x, y, xs, ys, pic);
}
*/                                                                                
                                                                            
/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============

void Draw_TransPicTranslate (int x, int y, shader_t *pic, byte *translation)
{
	int				v, u, c;
	unsigned		trans[64*64], *dest;
	byte			*src;
	int				p;

	GL_Bind (translate_texture);

	c = pic->width * pic->height;

	dest = trans;
	for (v=0 ; v<64 ; v++, dest += 64)
	{
		src = &menuplyr_pixels[ ((v*pic->height)>>6) *pic->width];
		for (u=0 ; u<64 ; u++)
		{
			p = src[(u*pic->width)>>6];
			if (p == 255)
				dest[u] = p;
			else
				dest[u] =  d_8to24table[translation[p]];
		}
	}

	glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glColor3f (1,1,1);
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex2f (x, y);
	glTexCoord2f (1, 0);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (1, 1);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (0, 1);
	glVertex2f (x, y+pic->height);
	glEnd ();
}
*/

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground (int lines)
{

	int y = (vid.height * 3) >> 2;
	int x, i; 

	char tl[80]; //Console Clock - Eradicator
	char timebuf[20];

	Draw_Pic (0, lines - vid.height, vid.width, lines, conback);
	/*
	if (lines > y)
		Draw_Pic (0, lines - vid.height, conback);
	else
		Draw_AlphaPic (0, lines - vid.height, conback, (float)(1.2 * lines)/y);
	*/
	if ( cg_conclock.value )
	{
                Sys_Strtime( timebuf );
		y = lines-14; 
		sprintf (tl, "Time: %s",timebuf); //Console Clock - Eradicator
		x = vid.conwidth - (vid.conwidth*12/vid.width*12) + 30; 
		for (i=0 ; i < strlen(tl) ; i++) 
		   Draw_Character (x + i * 8, y, tl[i] | 0x80);
	}

}

void Draw_SpiralConsoleBackground (int lines) //Spiral Console - Eradicator
{ 
	/*
   int x, i; 
   int y; 
   static float xangle = 0, xfactor = .3f, xstep = .01f; 
   
   char tl[80]; //Console Clock - Eradicator
   char timebuf[20];
   Sys_Strtime( timebuf );


   glPushMatrix(); 
   glMatrixMode(GL_TEXTURE); 
   glPushMatrix(); 
   glLoadIdentity(); 
   xangle += 1.0f; 
   xfactor += xstep; 
   if (xfactor > 8 || xfactor < .3f) 
      xstep = -xstep; 
   glRotatef(xangle, 0, 0, 1); 
   glScalef(xfactor, xfactor, xfactor); 
   y = (vid.height * 3) >> 2;  
   if (lines > y) 
      Draw_Pic(0, lines-vid.height, conback); 
   else 
      Draw_AlphaPic (0, lines - vid.height, conback, (float)(1.2 * lines)/y); 
   glPopMatrix(); 
   glMatrixMode(GL_MODELVIEW); 
   glPopMatrix(); 

   	if ( cg_conclock.value )
	{
		y = lines-14; 
		sprintf (tl, "Time: %s",timebuf); //Console Clock - Eradicator
		x = vid.conwidth - (vid.conwidth*12/vid.width*12) + 30; 
		for (i=0 ; i < strlen(tl) ; i++) 
			Draw_Character (x + i * 8, y, tl[i] | 0x80);
	}
	*/
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
	/*
	glColor3f (1,1,1);
	GL_Bind (draw_backtile->texnum);
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
	*/
	Draw_Pic (x, y, x+w, y+h, draw_backtile);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	glDisable (GL_TEXTURE_2D);
	glColor3f (host_basepal[c*3]/255.0,
		host_basepal[c*3+1]/255.0,
		host_basepal[c*3+2]/255.0);

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

