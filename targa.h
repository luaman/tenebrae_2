#ifndef _TARGA_H
#define _TARGA_H

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


typedef struct _TargaHeader
{
    unsigned char 	id_length, colormap_type, image_type;
    unsigned short	colormap_index, colormap_length;
    unsigned char	colormap_size;
    unsigned short	x_origin, y_origin, width, height;
    unsigned char	pixel_size, attributes;
} TargaHeader;


extern TargaHeader		targa_header;
extern byte			*targa_rgba;


#endif
