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
// gl_warp.c -- sky and water polygons

//#define NO_PNG
#ifndef NO_PNG
#include <png.h>
#endif

#include "quakedef.h"

extern	model_t	*loadmodel;

int		skyshadernum;

int		solidskytexture;
int		alphaskytexture;
float	speedscale;		// for top sky and bottom sky

int		newwatertex,newslimetex,newlavatex,newteletex,newenvmap; //PENTA: texture id's for the new fluid textures

msurface_t	*warpface;

extern cvar_t gl_subdivide_size;


void BoundPoly (int numverts, float *verts, vec3_t mins, vec3_t maxs)
{
    int		i, j;
    float	*v;

    mins[0] = mins[1] = mins[2] = 9999;
    maxs[0] = maxs[1] = maxs[2] = -9999;
    v = verts;
    for (i=0 ; i<numverts ; i++)
	for (j=0 ; j<3 ; j++, v++)
	{
	    if (*v < mins[j])
		mins[j] = *v;
	    if (*v > maxs[j])
		maxs[j] = *v;
	}
}

void SubdividePolygon (int numverts, float *verts)
{
    int		i, j, k;
    vec3_t	mins, maxs;
    float	m;
    float	*v;
    vec3_t	front[64], back[64];
    int		f, b;
    float	dist[64];
    float	frac;
    glpoly_t	*poly;
    float	s, t;
    float	tex[2];
	byte	color[4];

    if (numverts > 60)
	Sys_Error ("numverts = %i", numverts);

    BoundPoly (numverts, verts, mins, maxs);

    for (i=0 ; i<3 ; i++)
    {
	m = (mins[i] + maxs[i]) * 0.5;
	m = gl_subdivide_size.value * floor (m/gl_subdivide_size.value + 0.5);
	if (maxs[i] - m < 8)
	    continue;
	if (m - mins[i] < 8)
	    continue;

	// cut it
	v = verts + i;
	for (j=0 ; j<numverts ; j++, v+= 3)
	    dist[j] = *v - m;

	// wrap cases
	dist[j] = dist[0];
	v-=i;
	VectorCopy (verts, v);

	f = b = 0;
	v = verts;
	for (j=0 ; j<numverts ; j++, v+= 3)
	{
	    if (dist[j] >= 0)
	    {
		VectorCopy (v, front[f]);
		f++;
	    }
	    if (dist[j] <= 0)
	    {
		VectorCopy (v, back[b]);
		b++;
	    }
	    if (dist[j] == 0 || dist[j+1] == 0)
		continue;
	    if ( (dist[j] > 0) != (dist[j+1] > 0) )
	    {
		// clip point
		frac = dist[j] / (dist[j] - dist[j+1]);
		for (k=0 ; k<3 ; k++)
		    front[f][k] = back[b][k] = v[k] + frac*(v[3+k] - v[k]);
		f++;
		b++;
	    }
	}

	SubdividePolygon (f, front[0]);
	SubdividePolygon (b, back[0]);
	return;
    }

    poly = Hunk_Alloc(sizeof(glpoly_t));
						
    poly->next = warpface->polys;
    warpface->polys = poly;
    poly->numverts = numverts;
    poly->firstvertex = R_GetNextVertexIndex();

	for (i=0; i<4; i++) color[i] = 255;

    for (i=0 ; i<numverts ; i++, verts+= 3)
    {
	//tex[0] = DotProduct (verts, warpface->texinfo->vecs[0]);
	//tex[1] = DotProduct (verts, warpface->texinfo->vecs[1]);
	//Penta: lighmap coords are ignored...
	R_AllocateVertexInTemp(verts,tex,tex,color);
    }
}

/*
=================================================================

  Quake 2 environment sky

=================================================================
*/

#ifdef QUAKE2


#define	SKY_TEX		2000

/*
=================================================================

  PCX Loading

=================================================================
*/

typedef struct
{
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
    unsigned 	data;			// unbounded
} pcx_t;

byte	*pcx_rgb;

/*
============
LoadPCX
============
*/
void LoadPCX (FILE *f)
{
    pcx_t	*pcx, pcxbuf;
    byte	palette[768];
    byte	*pix;
    int		x, y;
    int		dataByte, runLength;
    int		count;

//
// parse the PCX file
//
    fread (&pcxbuf, 1, sizeof(pcxbuf), f);

    pcx = &pcxbuf;

    if (pcx->manufacturer != 0x0a
	|| pcx->version != 5
	|| pcx->encoding != 1
	|| pcx->bits_per_pixel != 8
	|| pcx->xmax >= 320
	|| pcx->ymax >= 256)
    {
	Con_Printf ("Bad pcx file\n");
	return;
    }

    // seek to palette
    fseek (f, -768, SEEK_END);
    fread (palette, 1, 768, f);

    fseek (f, sizeof(pcxbuf) - 4, SEEK_SET);

    count = (pcx->xmax+1) * (pcx->ymax+1);
    pcx_rgb = malloc( count * 4);

    for (y=0 ; y<=pcx->ymax ; y++)
    {
	pix = pcx_rgb + 4*y*(pcx->xmax+1);
	for (x=0 ; x<=pcx->ymax ; )
	{
	    dataByte = fgetc(f);

	    if((dataByte & 0xC0) == 0xC0)
	    {
		runLength = dataByte & 0x3F;
		dataByte = fgetc(f);
	    }
	    else
		runLength = 1;

	    while(runLength-- > 0)
	    {
		pix[0] = palette[dataByte*3];
		pix[1] = palette[dataByte*3+1];
		pix[2] = palette[dataByte*3+2];
		pix[3] = 255;
		pix += 4;
		x++;
	    }
	}
    }
}

#endif

//PENTA: removed this from the ifdef QUAKE2
/*
=========================================================

TARGA LOADING

=========================================================
*/

/* -DC- moved typedef to targa.h  */

TargaHeader		targa_header;
byte			*targa_rgba;

int fgetLittleShort (FILE *f)
{
    byte	b1, b2;

    b1 = fgetc(f);
    b2 = fgetc(f);

    return (short)(b1 + b2*256);
}

int fgetLittleLong (FILE *f)
{
    byte	b1, b2, b3, b4;

    b1 = fgetc(f);
    b2 = fgetc(f);
    b3 = fgetc(f);
    b4 = fgetc(f);

    return b1 + (b2<<8) + (b3<<16) + (b4<<24);
}

static char argh[MAX_OSPATH];

// Proper replacement for LoadTGA and friends
int LoadTexture(char* filename, int size)
{
    FILE	*f;
    char* tmp;

    // Check for *.png first, then *.jpg, fall back to *.tga if not found...
    // first replace the last three letters with png (yeah, hack)
    strncpy(argh, filename,sizeof(argh));
    tmp = argh + strlen(filename) - 3;
#ifndef NO_PNG
    tmp[0] = 'p';
    tmp[1] = 'n';
    tmp[2] = 'g';
    COM_FOpenFile (argh, &f);
    if ( f )
    {
	png_infop info_ptr;
	png_structp png_ptr;
        char* mem;
        unsigned long width, height;
        
	int bit_depth;
	int color_type;
	unsigned char** rows;
        int i;
        png_color_16 background = {0, 0, 0};
        png_color_16p image_background;
	
	Con_DPrintf("Loading %s\n", argh);
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, f);
        png_set_sig_bytes(png_ptr, 0);
	
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height,
		     &bit_depth, &color_type, 0, 0, 0);
	
	
	// Allocate memory and get data there
	mem = malloc(size*height*width);
	rows = malloc(height*sizeof(char*));

        if ( bit_depth == 16)
            png_set_strip_16(png_ptr);
        
        if ( color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8 )
            png_set_gray_1_2_4_to_8(png_ptr);

        png_set_gamma(png_ptr, 1.0, 1.0);



        if ( size == 4 )
	{
            if ( color_type & PNG_COLOR_MASK_PALETTE )
                png_set_palette_to_rgb(png_ptr);

            if ( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) )
                png_set_tRNS_to_alpha(png_ptr);

            if ( (color_type & PNG_COLOR_MASK_COLOR) == 0 )
                png_set_gray_to_rgb(png_ptr);

            if ( (color_type & PNG_COLOR_MASK_ALPHA) == 0 )
                png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	}
	else
	{          
            if ( color_type & PNG_COLOR_MASK_ALPHA )
                png_set_strip_alpha(png_ptr);

            if ( color_type & PNG_COLOR_MASK_PALETTE )
            {
                png_set_palette_to_rgb(png_ptr);   

            }

            if ( color_type & PNG_COLOR_MASK_COLOR )
                png_set_rgb_to_gray_fixed(png_ptr, 1, -1, -1);
        }
	
	for ( i = 0; i < height; i++ )
	{
	    rows[i] = mem + size * width * i;
	}
	png_read_image(png_ptr, rows);
	
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	free(rows);        
	targa_header.width = width;
	targa_header.height = height;
        targa_rgba = mem;
        fclose(f);
	return 1;
    }
#endif

    tmp[0] = 't';
    tmp[1] = 'g';
    tmp[2] = 'a';
    COM_FOpenFile (argh, &f);
    if (!f)
    {
	return 0;
    }
    LoadTGA(f);
    return 1;
}


int LoadTextureInPlace(char* filename, int size, byte* mem, int* width, int* height)
{
    FILE	*f;
    char* tmp;

    // Check for *.png first, then *.jpg, fall back to *.tga if not found...
    // first replace the last three letters with png (yeah, hack)
    strncpy(argh, filename,sizeof(argh));
    tmp = argh + strlen(filename) - 3;
#ifndef NO_PNG
    tmp[0] = 'p';
    tmp[1] = 'n';
    tmp[2] = 'g';
    COM_FOpenFile (argh, &f);
    if ( f )
    {
	png_infop info_ptr;
	png_structp png_ptr;
	int bit_depth;
	int color_type;
        unsigned long mywidth, myheight;
        
	unsigned char** rows;
        int i;
        png_color_16 background = {0, 0, 0};
        png_color_16p image_background;
	
	Con_DPrintf("Loading %s\n", argh);
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, f);
        png_set_sig_bytes(png_ptr, 0);
	
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &mywidth, &myheight,
		     &bit_depth, &color_type, 0, 0, 0);
	*width=mywidth;
        *height=myheight;
        
	
	// Allocate memory and get data there
	rows = malloc(*height*sizeof(char*));

        if ( bit_depth == 16)
            png_set_strip_16(png_ptr);
        
        if ( color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8 )
            png_set_gray_1_2_4_to_8(png_ptr);

        png_set_gamma(png_ptr, 1.0, 1.0);



        if ( size == 4 )
	{
            if ( color_type & PNG_COLOR_MASK_PALETTE )
                png_set_palette_to_rgb(png_ptr);

            if ( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) )
                png_set_tRNS_to_alpha(png_ptr);

            if ( (color_type & PNG_COLOR_MASK_COLOR) == 0 )
                png_set_gray_to_rgb(png_ptr);

            if ( (color_type & PNG_COLOR_MASK_ALPHA) == 0 )
                png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	}
	else
	{          
            if ( color_type & PNG_COLOR_MASK_ALPHA )
                png_set_strip_alpha(png_ptr);

            if ( color_type & PNG_COLOR_MASK_PALETTE )
            {
                png_set_palette_to_rgb(png_ptr);   

            }

            if ( color_type & PNG_COLOR_MASK_COLOR )
                png_set_rgb_to_gray_fixed(png_ptr, 1, -1, -1);
        }
	
	for ( i = 0; i < *height; i++ )
	{
	    rows[i] = mem + size * *width * i;
	}
	png_read_image(png_ptr, rows);
	
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	free(rows);        
        fclose(f);
	return 1;
    }
#endif
    tmp[0] = 't';
    tmp[1] = 'g';
    tmp[2] = 'a';
    COM_FOpenFile (argh, &f);
    if (!f)
    {
//		Con_SafePrintf ("Couldn't load %s\n", argh);
	return 0;
    }
    if ( size == 4 )
	LoadColorTGA(f, mem, width, height);
    else
	LoadGrayTGA(f, mem, width, height);
    return 1;
}

/*
=============
LoadTGA
=============
*/
void LoadTGA (FILE *fin)
{
    int				columns, rows, numPixels;
    byte			*pixbuf;
    int				row, column;
	int				row_inc;

    targa_header.id_length = fgetc(fin);
    targa_header.colormap_type = fgetc(fin);
    targa_header.image_type = fgetc(fin);
	
    targa_header.colormap_index = fgetLittleShort(fin);
    targa_header.colormap_length = fgetLittleShort(fin);
    targa_header.colormap_size = fgetc(fin);
    targa_header.x_origin = fgetLittleShort(fin);
    targa_header.y_origin = fgetLittleShort(fin);
    targa_header.width = fgetLittleShort(fin);
    targa_header.height = fgetLittleShort(fin);
    targa_header.pixel_size = fgetc(fin);
    targa_header.attributes = fgetc(fin);

    if (targa_header.image_type!=2 
	&& targa_header.image_type!=10) 
	Sys_Error ("LoadTGA: Only type 2 and 10 targa RGB images supported\n");

    if (targa_header.colormap_type !=0 
	|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24))
	Sys_Error ("Texture_LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

    columns = targa_header.width;
    rows = targa_header.height;
    numPixels = columns * rows;

    targa_rgba = malloc (numPixels*4);
	
    if (targa_header.id_length != 0)
	fseek(fin, targa_header.id_length, SEEK_CUR);  // skip TARGA image comment
	
	// If bit 5 of attributes isn't set, the image has been stored from bottom to top
	if ((targa_header.attributes & 0x20) == 0)
	{
		pixbuf = targa_rgba + (rows - 1)*columns*4;
		row_inc = -columns*4*2;
	}
	else
	{
		pixbuf = targa_rgba;
		row_inc = 0;
	}

    if (targa_header.image_type==2) {  // Uncompressed, RGB images
	for(row=rows-1; row>=0; row--, pixbuf += row_inc) {
	    for(column=0; column<columns; column++) {
		unsigned char red,green,blue,alphabyte;
		switch (targa_header.pixel_size) {
		case 24:
							
		    blue = getc(fin);
		    green = getc(fin);
		    red = getc(fin);
		    *pixbuf++ = red;
		    *pixbuf++ = green;
		    *pixbuf++ = blue;
		    *pixbuf++ = 255;
		    break;
		case 32:
		    blue = getc(fin);
		    green = getc(fin);
		    red = getc(fin);
		    alphabyte = getc(fin);
		    *pixbuf++ = red;
		    *pixbuf++ = green;
		    *pixbuf++ = blue;
		    *pixbuf++ = alphabyte;
		    break;
		}
	    }
	}
    }
    else if (targa_header.image_type==10) {   // Runlength encoded RGB images
	unsigned char red = 0x00,green = 0x00,blue = 0x00,alphabyte = 0x00,packetHeader,packetSize,j;
	for(row=rows-1; row>=0; row--, pixbuf += row_inc) {
	    for(column=0; column<columns; ) {
		packetHeader=getc(fin);
		packetSize = 1 + (packetHeader & 0x7f);
		if (packetHeader & 0x80) {        // run-length packet
		    switch (targa_header.pixel_size) {
		    case 24:
			blue = getc(fin);
			green = getc(fin);
			red = getc(fin);
			alphabyte = 255;
			break;
		    case 32:
			blue = getc(fin);
			green = getc(fin);
			red = getc(fin);
			alphabyte = getc(fin);
			break;
		    }
	
		    for(j=0;j<packetSize;j++) {
			*pixbuf++=red;
			*pixbuf++=green;
			*pixbuf++=blue;
			*pixbuf++=alphabyte;
			column++;
			if (column==columns) { // run spans across rows
			    column=0;
			    if (row>0) {
				row--;
				pixbuf += row_inc;
				}
			    else
				goto breakOut;
			}
		    }
		}
		else {                            // non run-length packet
		    for(j=0;j<packetSize;j++) {
			switch (targa_header.pixel_size) {
			case 24:
			    blue = getc(fin);
			    green = getc(fin);
			    red = getc(fin);
			    *pixbuf++ = red;
			    *pixbuf++ = green;
			    *pixbuf++ = blue;
			    *pixbuf++ = 255;
			    break;
			case 32:
			    blue = getc(fin);
			    green = getc(fin);
			    red = getc(fin);
			    alphabyte = getc(fin);
			    *pixbuf++ = red;
			    *pixbuf++ = green;
			    *pixbuf++ = blue;
			    *pixbuf++ = alphabyte;
			    break;
			}
			column++;
			if (column==columns) { // pixel packet run spans across rows
			    column=0;
			    if (row>0) {
				row--;
				pixbuf += row_inc;
				}
			    else
				goto breakOut;
			}						
		    }
		}
	    }
	breakOut:;
	}
    }
	
    fclose(fin);
}

/*
=============
LoadColorTGA
PENTA: loads a color tga (rgb/rgba/or quake palette)
returns width & height in the references.
=============
*/
void LoadColorTGA (FILE *fin, byte *pixels, int *width, int *height)
{
    int				columns, rows, numPixels;
    byte			*pixbuf;
    int				row, column;
	int				row_inc;
	byte			palette[256][3];

    targa_header.id_length = fgetc(fin);
    targa_header.colormap_type = fgetc(fin);
    targa_header.image_type = fgetc(fin);
	
    targa_header.colormap_index = fgetLittleShort(fin);
    targa_header.colormap_length = fgetLittleShort(fin);
    targa_header.colormap_size = fgetc(fin);
    targa_header.x_origin = fgetLittleShort(fin);
    targa_header.y_origin = fgetLittleShort(fin);
    targa_header.width = fgetLittleShort(fin);
    targa_header.height = fgetLittleShort(fin);
    targa_header.pixel_size = fgetc(fin);
    targa_header.attributes = fgetc(fin);

    if (targa_header.image_type!=2 
	&& targa_header.image_type!=10 && targa_header.image_type!=1) 
	Sys_Error ("LoadColorTGA: Only type 1, 2 and 10 targa images supported, type was %i\n",targa_header.image_type);

    /*
      if (targa_header.colormap_type !=0 
      || (targa_header.pixel_size!=32 && targa_header.pixel_size!=24))
      Sys_Error ("Texture_LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");
    */
    if (targa_header.image_type==1 && targa_header.pixel_size != 8 && 
	targa_header.colormap_size != 24 && targa_header.colormap_length != 256)
	Sys_Error ("LoadGrayTGA: Strange palette type\n");

    columns = targa_header.width;
    rows = targa_header.height;
    numPixels = columns * rows;

    targa_rgba = pixels;
	
    if (targa_header.id_length != 0)
	fseek(fin, targa_header.id_length, SEEK_CUR);  // skip TARGA image comment
	
	// If bit 5 of attributes isn't set, the image has been stored from bottom to top
	if ((targa_header.attributes & 0x20) == 0)
	{
		pixbuf = targa_rgba + (rows - 1)*columns*4;
		row_inc = -columns*4*2;
	}
	else
	{
		pixbuf = targa_rgba;
		row_inc = 0;
	}

    if (targa_header.image_type==1) {  // Color mapped
	//fseek(fin, 768, SEEK_CUR);  // skip palette
	fread(palette, 1, 768, fin);
	for(row=rows-1; row>=0; row--, pixbuf += row_inc) {
	    for(column=0; column<columns; column++) {
		unsigned char index;
		index = getc(fin);
		*(int *)pixbuf = 0xFF000000 || (palette[index][0] << 16) || (palette[index][0] << 8) || (palette[index][0]);
		pixbuf+=4;
	    }
	}
    } else if (targa_header.image_type==2) {  // Uncompressed, RGB images
	for(row=rows-1; row>=0; row--, pixbuf += row_inc) {
	    for(column=0; column<columns; column++) {
		unsigned char red,green,blue,alphabyte;
		switch (targa_header.pixel_size) {
		case 24:
							
		    blue = getc(fin);
		    green = getc(fin);
		    red = getc(fin);
		    *pixbuf++ = red;
		    *pixbuf++ = green;
		    *pixbuf++ = blue;
		    *pixbuf++ = 255;
		    break;
		case 32:
		    blue = getc(fin);
		    green = getc(fin);
		    red = getc(fin);
		    alphabyte = getc(fin);
		    *pixbuf++ = red;
		    *pixbuf++ = green;
		    *pixbuf++ = blue;
		    *pixbuf++ = alphabyte;
		    break;
		}
	    }
	}
    }
    else if (targa_header.image_type==10) {   // Runlength encoded RGB images
	unsigned char red = 0x00,green = 0x00,blue = 0x00,alphabyte = 0x00,packetHeader,packetSize,j;
	for(row=rows-1; row>=0; row--, pixbuf += row_inc) {
	    for(column=0; column<columns; ) {
		packetHeader=getc(fin);
		packetSize = 1 + (packetHeader & 0x7f);
		if (packetHeader & 0x80) {        // run-length packet
		    switch (targa_header.pixel_size) {
		    case 24:
			blue = getc(fin);
			green = getc(fin);
			red = getc(fin);
			alphabyte = 255;
			break;
		    case 32:
			blue = getc(fin);
			green = getc(fin);
			red = getc(fin);
			alphabyte = getc(fin);
			break;
		    }
	
		    for(j=0;j<packetSize;j++) {
			*pixbuf++=red;
			*pixbuf++=green;
			*pixbuf++=blue;
			*pixbuf++=alphabyte;
			column++;
			if (column==columns) { // run spans across rows
			    column=0;
			    if (row>0) {
				row--;
				pixbuf += row_inc;
				}
			    else
				goto breakOut;
			}
		    }
		}
		else {                            // non run-length packet
		    for(j=0;j<packetSize;j++) {
			switch (targa_header.pixel_size) {
			case 24:
			    blue = getc(fin);
			    green = getc(fin);
			    red = getc(fin);
			    *pixbuf++ = red;
			    *pixbuf++ = green;
			    *pixbuf++ = blue;
			    *pixbuf++ = 255;
			    break;
			case 32:
			    blue = getc(fin);
			    green = getc(fin);
			    red = getc(fin);
			    alphabyte = getc(fin);
			    *pixbuf++ = red;
			    *pixbuf++ = green;
			    *pixbuf++ = blue;
			    *pixbuf++ = alphabyte;
			    break;
			}
			column++;
			if (column==columns) { // pixel packet run spans across rows
			    column=0;
			    if (row>0) {
				row--;
				pixbuf += row_inc;
				}
			    else
				goto breakOut;
			}						
		    }
		}
	    }
	breakOut:;
	}
    }
	
    //we didn't load a paletted image so we still have to fix the gamma
    /*
      Just let the uses specify the gamma in the image editor
      if (targa_header.image_type != 1) {
      int i;
      float f, inf;
      for (i=0 ; i<numPixels*4; i++)
      {
      f = pow ( (targa_rgba[i]+1)/256.0 , 0.7 );
      inf = f*255 + 0.5;
      if (inf < 0)
      inf = 0;
      if (inf > 255)
      inf = 255;
      targa_rgba[i] = inf;
      }
      }
    */
    fclose(fin);
    *width = targa_header.width;
    *height = targa_header.height;
}

/*
=============
LoadGrayTGA
PENTA: Load a grayscale tga for bump maps
Copies the result to pixbuf (bytes not rgb) and puts the width & height in the references.
=============
*/
void LoadGrayTGA (FILE *fin,byte *pixels,int *width, int *height)
{
    int				columns, rows, numPixels;
    int				row, column;
    byte			*pixbuf;
	int				row_inc;

    targa_header.id_length = fgetc(fin);
    targa_header.colormap_type = fgetc(fin);
    targa_header.image_type = fgetc(fin);
	
    targa_header.colormap_index = fgetLittleShort(fin);
    targa_header.colormap_length = fgetLittleShort(fin);
    targa_header.colormap_size = fgetc(fin);
    targa_header.x_origin = fgetLittleShort(fin);
    targa_header.y_origin = fgetLittleShort(fin);
    targa_header.width = fgetLittleShort(fin);
    targa_header.height = fgetLittleShort(fin);
    targa_header.pixel_size = fgetc(fin);
    targa_header.attributes = fgetc(fin);

    if (targa_header.image_type!=1 
	&& targa_header.image_type!=3) 
	Sys_Error ("LoadGrayTGA: Only type 1 and 3 targa images supported for bump maps.\n");

    if (targa_header.image_type==1 && targa_header.pixel_size != 8 && 
	targa_header.colormap_size != 24 && targa_header.colormap_length != 256)
	Sys_Error ("LoadGrayTGA: Strange palette type\n");

    columns = targa_header.width;
    rows = targa_header.height;
    numPixels = columns * rows;

	// If bit 5 of attributes isn't set, the image has been stored from bottom to top
	if ((targa_header.attributes & 0x20) == 0)
	{
		pixbuf = pixels + (rows - 1)*columns;
		row_inc = -columns*2;
	}
	else
	{
		pixbuf = pixels;
		row_inc = 0;
	}
	
    if (targa_header.id_length != 0)
	fseek(fin, targa_header.id_length, SEEK_CUR);  // skip TARGA image comment
	
    if (targa_header.image_type == 1) {
	fseek(fin, 768, SEEK_CUR);//skip palette 
    }

    //fread(pixbuf,1,numPixels,fin);	
    for(row=rows-1; row>=0; row--, pixbuf += row_inc) {
	for(column=0; column<columns; column++) {
	    *pixbuf++= getc(fin);
	}
    }

    fclose(fin);
    *width = targa_header.width;
    *height = targa_header.height;
}
/*
==================
PENTA:
Loads the tga in filename and returns the texture object
in gl texture object.
==================
*/
int EasyTgaLoad(char *filename)
{
    FILE	*f;
    int			texturemode;
    unsigned long width, height;
    char* tmp;
    char* mem;
        
    if ( gl_texcomp && ((int)gl_compress_textures.value) & 1 )
    {
	texturemode = GL_COMPRESSED_RGBA_ARB;
    }
    else
    {
	texturemode = GL_RGBA8;
    }

    GL_Bind (texture_extension_number);
    // Check for *.png first, then *.jpg, fall back to *.tga if not found...
    // first replace the last three letters with png (yeah, hack)
    strncpy(argh, filename,sizeof(argh));
    tmp = argh + strlen(filename) - 3;
#ifndef NO_PNG
    tmp[0] = 'p';
    tmp[1] = 'n';
    tmp[2] = 'g';
    COM_FOpenFile (argh, &f);
    if ( f )
    {
        png_infop info_ptr;
        png_structp png_ptr;
        int bit_depth;
        int color_type;
        unsigned char** rows;
        int i;
        png_color_16 background = {0, 0, 0};
        png_color_16p image_background;

        Con_DPrintf("Loading %s\n", argh);
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, f);
        png_set_sig_bytes(png_ptr, 0);
	
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height,
		     &bit_depth, &color_type, 0, 0, 0);
	
	
	// Allocate memory and get data there
	mem = malloc(4*height*width);
	rows = malloc(height*sizeof(char*));

        if ( bit_depth == 16)
            png_set_strip_16(png_ptr);
        
        if ( color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8 )
            png_set_gray_1_2_4_to_8(png_ptr);

        png_set_gamma(png_ptr, 1.0, 1.0);



        if ( color_type & PNG_COLOR_MASK_PALETTE )
            png_set_palette_to_rgb(png_ptr);

        if ( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) )
            png_set_tRNS_to_alpha(png_ptr);

        if ( (color_type & PNG_COLOR_MASK_COLOR) == 0 )
            png_set_gray_to_rgb(png_ptr);

        if ( (color_type & PNG_COLOR_MASK_ALPHA) == 0 )
            png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);


	for ( i = 0; i < height; i++ )
	{
	    rows[i] = mem + 4 * width * i;
	}
	png_read_image(png_ptr, rows);
	
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	free(rows);        
        fclose(f);
    }
    else
#endif
    {
        tmp[0] = 't';
        tmp[1] = 'g';
        tmp[2] = 'a';
	COM_FOpenFile (argh, &f);
	if (!f)
	{
	    Con_SafePrintf ("Couldn't load %s\n", argh);
	    return 0;
	}
	LoadTGA (f);
        width = targa_header.width;
        height = targa_header.height;
        mem = targa_rgba;
    } 
    glTexImage2D (GL_TEXTURE_2D, 0, texturemode, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mem);
    free (mem);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    texture_extension_number++;
    return texture_extension_number-1;
}

//#ifdef QUAKE2 PENTA: enable skyboxes again

#define SKY_TEX 10000

char	skybox_name[64];
float skybox_cloudspeed;
qboolean skybox_hasclouds;

/*
==================
R_LoadSkys
==================
*/
char	*suf[7] = {"rt", "bk", "lf", "ft", "up", "dn","tile"};
void R_LoadSkys (void)
{
    int		i;
    FILE	*f;
    char	name[64];

    skybox_hasclouds = true;

    for (i=0 ; i<7 ; i++)
    {
	sprintf (name, "env/%s%s.tga", skybox_name, suf[i]);
//		Con_Printf("Loading file: %s\n",name);

	if (!LoadTexture(name, 4))
	{
	    if (i == 6) {
		skybox_hasclouds = false;
	    } else {
		Con_Printf ("Couldn't load %s\n", name);
	    }
	    continue;
	}
	else
	{
	    GL_Bind (SKY_TEX + i);
	    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, targa_header.width, targa_header.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, targa_rgba);

	    free (targa_rgba);

	    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
    }
}


vec3_t	skyclip[6] =
{
    {1,1,0},
    {1,-1,0},
    {0,-1,1},
    {0,1,1},
    {1,0,1},
    {-1,0,1} 
};
int	c_sky;

// 1 = s, 2 = t, 3 = 2048
int	st_to_vec[6][3] =
{
    {3,-1,2},
    {-3,1,2},

    {1,3,2},
    {-1,-3,2},

    {-2,-1,3},		// 0 degrees yaw, look straight up
    {2,-1,-3}		// look straight down

//	{-1,2,3},
//	{1,2,-3}
};

// s = [0]/[2], t = [1]/[2]
int	vec_to_st[6][3] =
{
    {-2,3,1},
    {2,3,-1},

    {1,3,2},
    {-1,3,-2},

    {-2,-1,3},
    {-2,1,-3}

//	{-1,2,3},
//	{1,2,-3}
};

float	skymins[2][6], skymaxs[2][6];


void MakeSkyVec (float s, float t, int axis)
{
    vec3_t		v, b;
    int			j, k;

    b[0] = s*1024;
    b[1] = t*1024;
    b[2] = 1024;

    for (j=0 ; j<3 ; j++)
    {
	k = st_to_vec[axis][j];
	if (k < 0)
	    v[j] = -b[-k - 1];
	else
	    v[j] = b[k - 1];
	v[j] += r_origin[j];
    }

    // avoid bilerp seam
    s = (s+1)*0.5;
    t = (t+1)*0.5;

    if (s < 1.0/512)
	s = 1.0/512;
    else if (s > 511.0/512)
	s = 511.0/512;
    if (t < 1.0/512)
	t = 1.0/512;
    else if (t > 511.0/512)
	t = 511.0/512;

    t = 1.0 - t;
    glTexCoord2f (s, t);
    glVertex3fv (v);
}

/*
==============
R_DrawSkyBox
==============
*/
int	skytexorder[6] = {0,2,1,3,4,5};
void R_DrawSkyBox (void)
{
    int		i, j, k;
    vec3_t	v;
    float	s, t;

    if (skyshadernum >= 0) {
	//	glColor3f(1,1,1);
	if (!cl.worldmodel->mapshaders[skyshadernum].texturechain) return;
	//	R_DrawSkyChain (cl.worldmodel->textures[skytexturenum]->texturechain);
	cl.worldmodel->mapshaders[skyshadernum].texturechain = NULL;
    }

    glDepthMask(0);

    if (gl_fog.value && !gl_wireframe.value) 
	glDisable(GL_FOG);

    for (i=0 ; i<6 ; i++)
    {
	/*
	  if (skymins[0][i] >= skymaxs[0][i]
	  || skymins[1][i] >= skymaxs[1][i])
	  continue;
	*/
	GL_Bind (SKY_TEX+skytexorder[i]);

	skymins[0][i] = -1;
	skymins[1][i] = -1;
	skymaxs[0][i] = 1;
	skymaxs[1][i] = 1;

	glBegin (GL_QUADS);
	MakeSkyVec (skymins[0][i], skymins[1][i], i);
	MakeSkyVec (skymins[0][i], skymaxs[1][i], i);
	MakeSkyVec (skymaxs[0][i], skymaxs[1][i], i);
	MakeSkyVec (skymaxs[0][i], skymins[1][i], i);
	glEnd ();
    }

    if (gl_fog.value && !gl_wireframe.value) 
	glEnable(GL_FOG);

    if (!skybox_hasclouds) {
	glDepthMask(1);		
	return;
    }

    GL_Bind (SKY_TEX+6);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(cl.time*skybox_cloudspeed,0,0);
    glColor4ub(255,255,255,255);
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0,0);
    glVertex3f(-800.0 + r_origin[0], -800.0 + r_origin[1], 20.0 + r_origin[2]);
		
    glTexCoord2f(0,40);
    glVertex3f(800.0 + r_origin[0], -800.0 + r_origin[1], 20.0 + r_origin[2]);

    glTexCoord2f(40,40);
    glVertex3f(800.0 + r_origin[0], 800.0 + r_origin[1], 20.0 + r_origin[2]);

    glTexCoord2f(40,0);
    glVertex3f(-800.0 + r_origin[0], 800.0 + r_origin[1], 20.0 + r_origin[2]);

    glEnd();
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (i=0 ; i<6 ; i++)
    {
	/*
	  if (skymins[0][i] >= skymaxs[0][i]
	  || skymins[1][i] >= skymaxs[1][i])
	  continue;
	*/
	GL_Bind (SKY_TEX+skytexorder[i]);


	skymins[0][i] = -1;
	skymins[1][i] = -1;
	skymaxs[0][i] = 1;
	skymaxs[1][i] = 1;

	glBegin (GL_QUADS);
	MakeSkyVec (skymins[0][i], skymins[1][i], i);
	MakeSkyVec (skymins[0][i], skymaxs[1][i], i);
	MakeSkyVec (skymaxs[0][i], skymaxs[1][i], i);
	MakeSkyVec (skymaxs[0][i], skymins[1][i], i);
	glEnd ();
    }

    glDisable(GL_BLEND);
    glDepthMask(1);
}
