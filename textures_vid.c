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

Video's as textures
These use the roq format, you can make them by using the quake3 roq tools.
They are dynamically streamed from disc so size doesn't really matter (but small ones tend
to fit in the disc cache)
*/

#include "quakedef.h"
#include "roq/roq.h"

extern unsigned	trans[1024*1024]; //just a static scratch buffer (declared in textures.c)

void Roq_UpdateTexture(gltexture_t *tex);
void Roq_ConvertColors(roq_info *ri);

/**
* Console command that prints some information about a video
*/
void Roq_Info_f(void) {
	roq_info *ri;

	if(Cmd_Argc() < 2) {
		Con_Printf("roq_info <input roq>\n");
		return;
	}

	if((ri = roq_open(Cmd_Argv(1))) == NULL) {
		Con_Printf("File not found %s\n",Cmd_Argv(0));
		return;
	}

	Con_Printf("RoQ: length %ld:%02ld  video: %dx%d %ld frames  audio: %d channels.\n",
		ri->stream_length/60000, ((ri->stream_length + 500)/1000) % 60,
		ri->width, ri->height, ri->num_frames, ri->audio_channels);

	roq_close(ri);
	return;
}

/**
* Setup a video texture (load and parse the file)
*/
void Roq_SetupTexture(gltexture_t *tex,char *filename) {
	roq_info *ri;
	int i;

	if((ri = roq_open(filename)) == NULL) {
		Con_Printf("Error loading video %s\n",filename);
		return;
	}
	Con_Printf("Video loaded\n");
	tex->dynamic = ri;
	tex->width = ri->width;
	tex->height = ri->height;

	//Load one frame as start frame
/*	roq_read_frame(ri);
	Roq_ConvertColors(ri);
	GL_Bind(tex->texnum);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, ri->width, ri->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/
}

/**
* Free a video texture (closes the file)
*/
void Roq_FreeTexture(gltexture_t *tex) {
	roq_info *ri;

	if(tex->dynamic == NULL) {
		return;
	}

	ri = tex->dynamic;
	roq_close(ri);
}

/**
* Converts the yuv to usable rgb data.
*/
void Roq_ConvertColors(roq_info *ri) {

	int i, j, y1, y2, u, v;
	byte *pa, *pb, *pc, *c;

	pa = ri->y[0];
	pb = ri->u[0];
	pc = ri->v[0];
	c = (byte *)&trans[0];


#define limit(x) ((((x) > 0xffffff) ? 0xff0000 : (((x) <= 0xffff) ? 0 : (x) & 0xff0000)) >> 16)
	for(j = 0; j < ri->height; j++) {
		for(i = 0; i < ri->width/2; i++) {
			int r, g, b;
			y1 = *(pa++); y2 = *(pa++);
			u = pb[i] - 128;
			v = pc[i] - 128;
			y1 *= 65536; y2 *= 65536;
			r = 91881 * v;
			g = -22554 * u + -46802 * v;
			b = 116130 * u;

			(*c++) = limit(r + y1);
			(*c++) = limit(g + y1);
			(*c++) = limit(b + y1);
			(*c++) = 255; //alpha
			(*c++) = limit(r + y2);
			(*c++) = limit(g + y2);
			(*c++) = limit(b + y2);
			(*c++) = 255; //alpha
		}
		if(j & 0x01) { pb += ri->width/2; pc += ri->width/2; }
	}
}

/**
* This updates the pixels of the video (uploads them to opengl)
* this should be called regulary when the texture is to be drawn.
*/
void Roq_UpdateTexture(gltexture_t *tex) {
	roq_info *ri;
	int newread, i, j, pos;
	byte *c, *r, *b, *g;

	if(tex->dynamic == NULL) {
		return;
	}

	ri = tex->dynamic;
	//Con_Printf("Video tex %i\n",ri->frame_num);

	newread = 0;
	//first frame?
	if (ri->lastframe_time < 0) {
		ri->lastframe_time = host_time;
		roq_read_frame(ri);
		newread++;
	//loop video
	} else if (ri->frame_num >= ri->num_frames) {
		roq_reset_stream (ri);
	//read frames untill we have catched up with the curent time
	}  else {
		while ((host_time - ri->lastframe_time) > (1.0/30.0)) {
			ri->lastframe_time = host_time;
			roq_read_frame(ri);
			newread++;
		}
	}

	if (newread){
		Roq_ConvertColors(ri);

		//never mip or compress vids, it's too slow!
		GL_Bind(tex->texnum);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, ri->width, ri->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
}
