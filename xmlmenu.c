/*
Copyright (C) 2003 Tenebrae Team

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


/* 
   -DC-
   a pretty inefficient way of implementing XUL elements ;)
   most of the attributes aren't supported

*/ 

#include "quakedef.h"

#ifdef _WIN32
#include "winquake.h"
#endif


#define MAX_MLABEL 128


// -------------------------------------------------------------------------------------

// --X--X--X-- WARNING WARNING WARNING --X--X--X--

// FIXME :
// copy & pasted from gl_draw.c
// update me accordingly or better, move me in glquake.h 
// while I'm still supported

typedef struct
{
	int		texnum;
	float	sl, tl, sh, th;
} glpic_t;

// --X--X--X-- WARNING WARNING WARNING --X--X--X--

void GL_LoadPic (char *src, glpic_t *target)
{
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
	qpic_t *pic;

	int len = strlen(src);

	if (strncmp(src+len-4,".lmp",4)){

		// load file in memory
		LoadTexture (src, 4);
		// assign it to a texture object
		target->texnum = GL_LoadTexture (src, targa_header.width, targa_header.height, targa_rgba, false, true, false);
		free (targa_rgba);

	}
	else {  
		// lump file
		// FIXME : drop lump file support in the future 
		pic = (qpic_t *) COM_LoadTempFile (src);		
		if (!pic)
			Sys_Error ("GL_LoadPic: failed to load %s", src);
		SwapPic (pic);
		target->texnum = GL_LoadTexture (src, pic->width, pic->height, pic->data, false, true, false);
	}
	target->sl = 0;
	target->sh = 1;
	target->tl = 0;
	target->th = 1;
}

// -------------------------------------------------------------------------------------

cvar_t  m_debug = {"m_debug","0"};


typedef struct xmlimagedata_s
{
	char       src[MAX_OSPATH];
	glpic_t    pic;
	int        width;
	int        height;
} xmlimagedata_t;


typedef struct xmlbuttondata_s
{
	int       disabled:1;
	int       isdown:1;
	char      label[MAX_MLABEL];
	char      command[255];
} xmlbuttondata_t;


typedef struct xmlcheckboxdata_s
{
	int       disabled:1;
	int       checked:1;
	char      label[MAX_MLABEL];
} xmlcheckboxdata_t;


typedef struct xmllabeldata_s
{
	char      text[MAX_MLABEL];
} xmllabeldata_t;

typedef struct xmlsliderdata_s
{
	int       range;
	int       cursor;
	char      label[MAX_MLABEL];
} xmlsliderdata_t;

typedef struct xmlradiodata_s
{
	int       disabled:1;
	int       selected:1;
	char      label[MAX_MLABEL];
} xmlradiodata_t;

typedef struct xmlradiogroupdata_s
{
	int       disabled:1;
	char      label[MAX_MLABEL];
} xmlradiogroupdata_t;


typedef struct xmlmenudata_s
{
	int       dummy;
} xmlmenudata_t;

/*
    xml window data
 */

typedef struct qwindow_s {
	char       file[MAX_OSPATH];
	qwidget_t *widget;
	qwidget_t *focused;	
	struct qwindow_s *stack;  // visible window stack
	struct qwindow_s *next;   // available window list
} qwindow_t;


xmlhandler_t widgethandlers[] =
{
	{
		(const xmlChar *)"box",
		{
			0,            // debug
			0,            // focusable
			0,            // orient = horizontal
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlBox, // Load
			NULL,         // Draw
			NULL,         // Focus
			M_XmlBoxKey,  // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
		0
	},
	{(const xmlChar *)"hbox",
		{
			0,            // debug
			0,            // focusable
			0,            // orient = horizontal
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlBox, // Load
			NULL,         // Draw
			NULL,         // Focus
			M_XmlBoxKey,  // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
	 0
	},
	{
		(const xmlChar *)"vbox",
		{
			0,            // debug
			0,            // focusable
			1,            // orient = vertical
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlBox, // Load
			NULL,         // Draw
			NULL,         // Focus
			M_XmlBoxKey,  // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
		0
	},
	{
		(const xmlChar *)"label",
		{
			0,            // debug
			0,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlLabel, // Load
			M_DrawXmlLabel, // Draw
			NULL,         // Focus
			NULL,         // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
		sizeof(xmllabeldata_t)
	},
	{
		(const xmlChar *)"image",
		{
			0,            // debug
			0,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlImage, // Load
			M_DrawXmlImage, // Draw
			NULL,         // Focus
			NULL,         // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
		sizeof(xmlimagedata_t)
	},
	{
		(const xmlChar *)"button",
		{
			0,            // debug
			1,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlButton, // Load
			M_DrawXmlButton, // Draw
			NULL,         // Focus
			M_XmlButtonKey,  // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
		sizeof(xmlbuttondata_t)
	},
	{
		(const xmlChar *)"menuitem",
		{
			0,            // debug
			1,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlButton, // Load
			M_DrawXmlButton, // Draw
			NULL,         // Focus
			M_XmlButtonKey,  // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
		sizeof(xmlbuttondata_t)
	},
	{
		(const xmlChar *)"checkbox",
		{
			0,            // debug
			1,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlCheckBox, // Load
			M_DrawXmlCheckBox, // Draw
			NULL,         // Focus
			M_XmlCheckBoxKey, // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
		sizeof(xmlcheckboxdata_t)
	},
	{
		(const xmlChar *)"radio",
		{
			0,            // debug
			1,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlRadio, // Load
			M_DrawXmlRadio,  // Draw
			NULL,         // Focus
			M_XmlRadioKey,   // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
		sizeof(xmlradiodata_t)
	},
	{
		(const xmlChar *)"radiogroup",
		{
			0,            // debug
			1,            // focusable
			1,            // orient = vertical
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlRadioGroup, // Load
			M_DrawXmlRadioGroup, // Draw
			NULL,         // Focus
			M_XmlRadioGroupKey, // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
		sizeof(xmlradiogroupdata_t)
	},
	{
		(const xmlChar *)"slider",
		{
			0,            // debug
			1,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlSlider, // Load
			M_DrawXmlSlider, // Draw
			NULL,         // Focus
			M_XmlSliderKey, // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
		sizeof(xmlsliderdata_t)
	},
	{
		(const xmlChar *)"window",
		{
			0,            // debug
			0,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			"",           // name
			"",           // id
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // num_children
			NULL,         // tag
			NULL,         // parent
//			NULL,         // root
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			// functions 
			M_LoadXmlWindow, // Load
			NULL,         // Draw
			NULL,         // Focus
			M_XmlWindowKey,  // HandleKey
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		},
		sizeof(qwindow_t)
	},
	NULL
};

xmlChar *xmlalign[] = {
	"start",
	"center",
	"end",
	"baseline",
	"stretch"
};

xmlChar *xmlpack[] = {
	"start",
	"center",
	"end"
};

xmlChar *xmlorient[] = {
	"horizontal",
	"vertical"
};

xmlChar *xmlbool[] = {
	"false",
	"true"
};

/*  -DC-
 *  perhaps we could use xml menus to define hud ?
 *  qwidget_t *hud_window;
 */

void M_DrawXmlNestedWidget (qwidget_t *widget, int xpos, int ypos);

qwindow_t *windows = NULL;


qwindow_t *visible_window;


glpic_t focus_pic;

// -------------------------------------------------------------------------------------

/* -DC- useless for now


typedef struct qwiterator_s {
	qwidget_t *w;
	void *(next_f)(struct qwiterator_s *self);
} qwiterator_t;

void iterator_next (qwiterator_t *self)
{
	qwidget_t *w = self->w;
	if (w->next) {
		self->w = w->next;
	else
		self->w = NULL;
}

void iterator_prev (qwiterator_t *self)
{
	qwidget_t *w = self->w;
	if (w->previous)
		self->w = w->previous;
	else 
		self->w = NULL;
}

void treeiterator_next (qwiterator_t *self)
{
	qwidget_t *w = self->w;
	if (w->children)
		self->w = w->children;
	else if (!w->next)
		while (w->parent && !w->next)
			w = w->parent;
	self->w = w->next;
}

void treeiterator_prev (qwiterator_t *self)
{
	qwidget_t *w = self->w;
	if (w->rchildren)
		self->w = w->rchildren;
	else if (!w->previous)
		while (w->parent && !w->previous)
			w = w->parent;
	self->w = w->previous;
}

*/

// -------------------------------------------------------------------------------------

#define	SLIDER_RANGE	10

qboolean        m_entersound;           // play after drawing a frame, so caching
                                        // won't disrupt the sound
qboolean        m_recursiveDraw;

int             m_return_state;
qboolean        m_return_onerror;
char            m_return_reason [32];


extern cvar_t con_spiral;

enum {m_none, m_menu } m_state;



// -------------------------------------------------------------------------------------

void M_StartGame_f (void)
{
	if (sv.active) {
		if (!SCR_ModalMessage("Are you sure you want to\nstart a new game?\n"))
			return;
		Cbuf_AddText ("disconnect\n");
	}
	Cbuf_AddText ("maxplayers 1\n");
	Cbuf_AddText ("map start\n");
}



/* -DC-  
   FIXME : 
   change Host_Quit_f in host_cmd to M_Quit_f code
*/
void M_Quit_f (void)
{
	CL_Disconnect ();
	Host_ShutdownServer(false);
	Sys_Quit ();
}


void M_CloseWindow_f (void)
{
	qwindow_t *temp = visible_window;
	if (visible_window != NULL){
		visible_window = visible_window->stack;
		temp->stack = NULL;
	}
	if (visible_window == NULL)
		key_dest = key_game;
}


void M_OpenWindow_f (void)
{
	qwindow_t *w = windows;
	qwindow_t *temp = visible_window;
	char *name;
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("openwindow <name> : bring up the window called 'name'\n");
		return;
	}
	name = Cmd_Argv(1);
	while (w){
		if (!strncmp (w->widget->id, name, sizeof(w->widget->id)))
			break;
		w = w->next;
	}
	if (!w) {
		Con_Printf ("openwindow : no such window '%s'\n", name);
		return;
	}

	while (temp && (temp!=w))
		temp = temp->stack;
		
	if (temp == w){
		// which one ?
		// - close all opened windows back to w
		// - close all opened windows from w and stack up w
		// * close all opened windows then stack up w
		// - change the whole damn window code
		while (visible_window)
			M_CloseWindow_f ();
	}
	w->stack = visible_window;
	visible_window = w;
	if (w->focused == NULL) {
		w->focused = w->widget;
		M_CycleFocusNext ();
	}
	key_dest = key_menu;
	m_state = m_menu;
}


void M_WindowList_f (void)
{
	qwindow_t *w = windows;
	while (w){
		Con_Printf ("%s\n", w->widget->id);		
		w = w->next;
	}
	
}

void M_Init (void)
{

	Cmd_AddCommand ("startgame", M_StartGame_f);
	Cmd_AddCommand ("openwindow", M_OpenWindow_f);
	Cmd_AddCommand ("closewindow", M_CloseWindow_f);
	Cmd_AddCommand ("windowlist", M_WindowList_f);
	Cvar_RegisterVariable (&m_debug);
	// let's load the whole stuff now
	COM_FindAllExt ("menu", "xul", M_LoadXmlWindowFile);

	GL_LoadPic ("menu/focus.png",&focus_pic);
}

/*
========
M_Menu_Main_f
M_Menu_Options_f
M_Menu_Quit_f

these functions are used in another part of the code (console.c, vid_glnt.c, vid_glsdl.c) so I implemented them as stub to
the console commands they are bound to originally to concentrate the menus change in one file.
So basically here we're just expecting that some other part of the game will have defined these commands/aliases/whatever
(the menus should create the corresponding windows).
========
*/

void M_Menu_Main_f (void)
{
	Cbuf_AddText ("openwindow main\n");
}

void M_Menu_Quit_f (void)
{
	M_Quit_f ();
}

void M_Menu_Options_f (void)
{
	Cbuf_AddText ("openwindow help\n");
}

void (*vid_menudrawfn)(void);
void (*vid_menukeyfn)(int key);

/*
========
M_Keydown

recursive function set : this one is at top level, and calls the focused widget key handler
========
*/
void M_Keydown (int key)
{
	qwidget_t *focused = visible_window->focused;
	if (focused && focused->HandleKey)
		M_XmlElementKey (focused,key);
	else 	// current window get the key
		M_XmlElementKey (visible_window->widget, key);
}

// -------------------------------------------------------------------------------------

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		Draw_Character (cx, cy, (*str)+128);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width - 320)>>1), y, pic);
}

void M_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x + ((vid.width - 320)>>1), y, pic);
}



byte identityTable[256];
byte translationTable[256];

void M_BuildTranslationTable(int top, int bottom)
{
	int		j;
	byte	*dest, *source;

	for (j = 0; j < 256; j++)
		identityTable[j] = j;
	dest = translationTable;
	source = identityTable;
	memcpy (dest, source, 256);

	if (top < 128)	// the artists made some backwards ranges.  sigh.
		memcpy (dest + TOP_RANGE, source + top, 16);
	else
		for (j=0 ; j<16 ; j++)
			dest[TOP_RANGE+j] = source[top+15-j];

	if (bottom < 128)
		memcpy (dest + BOTTOM_RANGE, source + bottom, 16);
	else
		for (j=0 ; j<16 ; j++)
			dest[BOTTOM_RANGE+j] = source[bottom+15-j];
}

void M_DrawTransPicTranslate (int x, int y, qpic_t *pic)
{
	Draw_TransPicTranslate (x + ((vid.width - 320)>>1), y, pic, translationTable);
}


void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 16;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+16, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic ("gfx/box_tm.lmp");
		M_DrawTransPic (cx, cy, p);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 16;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		p = Draw_CachePic ("gfx/box_bm.lmp");
		M_DrawTransPic (cx, cy+16, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}

void M_ToggleMenu_f (void)
{
	m_entersound = true;

	if (key_dest == key_menu)
	{
		// if not main menu  
		{
			M_Menu_Main_f ();
			return;
		}
		key_dest = key_game;
		m_state = m_none;
		return;
	}
	if (key_dest == key_console)
	{
		Con_ToggleConsole_f ();
	}
	else
	{
		M_Menu_Main_f ();
	}
}


// -------------------------------------------------------------------------------------

void M_XmlButtonCommand (qwidget_t *self)
{
	xmlbuttondata_t *data = self->data;
	Cbuf_AddText (data->command);
	Cbuf_AddText ("\n");
}

// -------------------------------------------------------------------------------------


qwidget_t *M_NextXmlElement (qwidget_t *w)
{
        if (w->children)
		return w->children;
	if (w->next)
		return w->next;
	while (w->parent && !w->next)
		w = w->parent;
	if (!w->next)
		return visible_window->widget;
	return w->next;
}

qwidget_t *M_PreviousXmlElement (qwidget_t *w)
{
	if (w->rchildren)
		return w->rchildren;
	if (w->previous)
		return w->previous;
	while (w->parent && !w->previous)
		w = w->parent;
	if (!w->next)
		return visible_window->widget;
	return w->previous;
}

void M_CycleFocus (qwidget_t *(*next_f)(qwidget_t *))
{
	qwidget_t *w = visible_window->focused;
	do {
		w = next_f (w);
	} while ((w != visible_window->focused) && !(w->focusable));
	visible_window->focused = w;
}

// -------------------------------------------------------------------------------------

qboolean M_XmlBoxKey (qwidget_t *self, int k)
{
	if (!self->children)
		return false;

	switch (k)
	{
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");		
		M_CycleFocusPrevious ();		
		break;

	case K_DOWNARROW:
	case K_TAB:
		S_LocalSound ("misc/menu1.wav");
		M_CycleFocusNext ();		
		break;
	case K_ESCAPE:
		// pop up previous menu
		S_LocalSound ("misc/menu2.wav");
		M_CloseWindow_f ();
		break;
	default:
		return false;
	}
	return true;
}

qboolean M_XmlCheckBoxKey (qwidget_t *self, int k)
{
	xmlcheckboxdata_t *data = self->data;
	switch (k)
	{
	case K_ENTER:
		// (un)check the checkbox
		S_LocalSound ("misc/menu2.wav");
		data->checked = !data->checked;
		Cvar_SetValue (self->id, data->checked);
		break;
	default:
		return false;
	}
	return true;
}


qboolean M_XmlButtonKey (qwidget_t *self, int k)
{
	switch (k)
	{
	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		M_XmlButtonCommand (self);
		break;
	default:
		return false;
	}
	return true;
}

qboolean M_XmlRadioKey (qwidget_t *self, int k)
{
	switch (k)
	{
	case K_ENTER:
		// select the radio button
		break;
	default:
		return false;
	}
	return true;
}

qboolean M_XmlRadioGroupKey (qwidget_t *self, int k)
{
	return M_XmlBoxKey (self,k);
}


qboolean M_XmlSliderKey (qwidget_t *self, int k)
{
	xmlsliderdata_t *data = self->data;
	switch (k)
	{
	case K_LEFTARROW:
		// reduce cvar value
		S_LocalSound ("misc/menu3.wav");
		data->range -= 1/SLIDER_RANGE;
		break;
	case K_RIGHTARROW:
		// augment cvar value
		S_LocalSound ("misc/menu3.wav");
		data->range += 1/SLIDER_RANGE;
		break;
	default:
		return false;
	}
	return true;
}

qboolean M_XmlWindowKey (qwidget_t *self, int k)
{
	return M_XmlBoxKey (self,k);
}

qboolean M_XmlMenuKey (qwidget_t *self, int k)
{
	return M_XmlBoxKey (self,k);
}

qboolean M_XmlElementKey (qwidget_t *widget, int k)
{
	// start from widget 
	// and propagate the key up the parents tree 
	// until it has been consumed or it reaches the
	// tree root
	while (widget){ 
		if (widget->HandleKey)
			if (widget->HandleKey (widget, k))
					 return true;
		widget = widget->parent;
	}
	return false;
}

// -------------------------------------------------------------------------------------

/*
================
Draw_StringN

draws the str string of length len in the (x1,y1,x2,y2) frame 
================
*/
int Draw_StringN (int x1, int y1, int x2, int y2, char *str, int len)
{
	// calculate string width and height, center it
	int w = len * 8;
	int h = 16;
	int x = x1 + (x2 - x1 - w)/2;
	int y = y1 + (y2 - y1 - h)/2;
	while (*str && len)
	{
		Draw_Character (x, y, (*str)+128);
		str++;
		x += 8;
		len--;
	}
	return x;
}

void M_Draw (void)
{
	
	if (m_state == m_none || key_dest != key_menu)
		return;

	if (!m_recursiveDraw)
	{
		scr_copyeverything = 1;

		if (scr_con_current)
		{
			if (con_spiral.value) //Console Spiral - Eradicator
				Draw_SpiralConsoleBackground (vid.height);
			else
				Draw_ConsoleBackground (vid.height);
			VID_UnlockBuffer ();
			S_ExtraUpdate ();
			VID_LockBuffer ();
		}
		else
			Draw_FadeScreen ();

		scr_fullupdate = 0;
	}
	else
	{
		m_recursiveDraw = false;
	}

	// draw current window
	M_DrawVisibleWindow ();

	if (m_entersound)
	{
		S_LocalSound ("misc/menu2.wav");
		m_entersound = false;
	}

	VID_UnlockBuffer ();
	S_ExtraUpdate ();
	VID_LockBuffer ();
}


void M_DrawXmlButton (qwidget_t *self, int x, int y)
{
	xmlbuttondata_t *data = self->data;
	Draw_StringN (x, y, x+self->width.absolute, y+self->height.absolute, data->label, sizeof(data->label));
}

void M_DrawXmlImage (qwidget_t *self, int x, int y)
{
	xmlimagedata_t *data = self->data;
	if (data->pic.texnum == 0)
		GL_LoadPic (data->src, &(data->pic));
        glColor4f (1,1,1,1);
	Draw_GLPic (x, y, x+self->width.absolute, y+self->height.absolute, &(data->pic));
}

void M_DrawXmlLabel (qwidget_t *self, int x, int y)
{
	xmllabeldata_t *data = self->data;
	Draw_StringN (x, y, x+self->width.absolute, y+self->height.absolute, data->text, sizeof(data->text));
}

void M_DrawXmlBox (qwidget_t *self, int x, int y)
{
	// nothing to do

}

void M_DrawXmlRadio (qwidget_t *self, int x, int y)
{
	xmlradiodata_t *data = self->data;
	qboolean on = data->selected;
	Draw_StringN (x, y, x+self->width.absolute, y+self->height.absolute, data->label, sizeof(data->label));
	if (on)
		M_Print (x, y, "(o)");
	else
		M_Print (x, y, "( )");
}

void M_DrawXmlRadioGroup (qwidget_t *self, int x, int y)
{
	// nothing to do
}

// almost pasted from menu.c  

void M_DrawXmlSlider (qwidget_t *self, int x, int y)
{
	int	i;
	xmlsliderdata_t *data = self->data;
	int range = data->range;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;

	Draw_Character (x, y, 128);
	x += 8;
	for (i=0 ; i<SLIDER_RANGE ; i++)
		Draw_Character (x + i*8, y, 129);
	Draw_Character (x+i*8, y, 130);
	Draw_Character (x + (SLIDER_RANGE-1)*8 * range, y, 131);
}

// almost pasted from menu.c  

void M_DrawXmlCheckBox (qwidget_t *self,int x, int y)
{
	xmlcheckboxdata_t *data = self->data;
	qboolean on = data->checked;

	x = Draw_StringN (x, y, x+self->width.absolute, y+self->height.absolute, data->label, sizeof(data->label));

	if (on)
		M_Print (x, y, "[x]");
	else
		M_Print (x, y, "[ ]");
}

void M_DrawXmlWindow (qwidget_t *self,int x, int y)
{
	qwindow_t *data = self->data;
	// draw a border ?
}


void M_DrawFocus (qwidget_t *self, int x, int y){

	int xs = x+self->width.absolute;
	int ys = y+self->height.absolute;

	if (focus_pic.texnum == 0)
		GL_LoadPic ("menu/focus.png",&focus_pic);

	glDisable (GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f (0.5,0.5,0.5,0.5);
	Draw_GLPic (x, y, xs, ys, &focus_pic);
	glDisable (GL_BLEND);
	glEnable (GL_ALPHA_TEST);
}

/* -DC-
   FIXME :  doesn't work at all
   So this function is a little hack to display widget outline
   just for debugging purpose
   -> There is certainly a (better) way to do this
      cause I don't know much about OpenGL (shame on me ^^;)
*/

void M_DrawOutlines (qwidget_t *self, int x, int y){

	int xs = x + self->width.absolute;
	int ys = x + self->height.absolute;
	GLfloat color[4];
	GLint shademodel;
	glGetFloatv(GL_CURRENT_COLOR, color);
	glColor4f(1,1,1,1);
	//glGetIntegerv(GL_SHADE_MODEL, &shademodel);
	//glShadeModel (GL_FLAT);
	glBegin (GL_LINE_STRIP);
		glVertex2f (x, y);
		glVertex2f (xs, y);
		glVertex2f (xs, ys);
		glVertex2f (x, ys);
	glEnd ();
	//glShadeModel (shademodel);
	glColor4fv(color);
	GL_EnableMultitexture ();
}

/* -DC-
   instead of having a function handling recursive drawing of widget
   we could perhaps put this code in widget rendering code directly 
   -> more flexible if some widget need to do stuff in between children
      rendering
   -> but code duplication

 */

void M_DrawXmlNestedWidget (qwidget_t *widget, int xpos, int ypos)
{
	qwidget_t *w;


	if (widget->Draw){
		widget->Draw (widget, xpos, ypos);
	}

	if (widget == visible_window->focused){
		M_DrawFocus (widget, xpos, ypos);
	}

	if (m_debug.value) { 
		// draw outlines
		M_DrawOutlines (widget, xpos, ypos);
	}
	if (widget->children == NULL)
		return;
	// vertical orientation
	if (widget->orient) {
		int wwidth;
		//ypos += widget->yoffset;
		switch (widget->pack){
		case p_start:
			for (w = widget->children; w != NULL; w = w->next){
				M_DrawXmlNestedWidget (w, xpos, ypos);
				ypos += w->height.absolute;
			}
			break;
		case p_center:
			for (w = widget->children; w != NULL; w = w->next){
				wwidth = widget->width.absolute - w->width.absolute;
				M_DrawXmlNestedWidget (w, xpos+wwidth/2, ypos);
				ypos += w->height.absolute;
			}
			break;
		case p_end:
			for (w = widget->children; w != NULL; w = w->next){
				wwidth = widget->width.absolute - w->width.absolute;
				M_DrawXmlNestedWidget (w, xpos+wwidth, ypos);
				ypos += w->height.absolute;
			}
			break;
		}
	}
	else {
		int wheight;
		//xpos += widget->xoffset;
		switch (widget->pack){
		case p_start:
			for (w = widget->children; w != NULL; w = w->next){
				M_DrawXmlNestedWidget (w, xpos, ypos);
				xpos += w->width.absolute;
			}
			break;
		case p_center:
			for (w = widget->children; w != NULL; w = w->next){
				wheight = - w->height.absolute + widget->height.absolute;
				M_DrawXmlNestedWidget (w, xpos, ypos+wheight/2);
				xpos += w->width.absolute;
			}
			break;
		case p_end:
			for (w = widget->children; w != NULL; w = w->next){
				wheight = - w->height.absolute + widget->height.absolute;
				M_DrawXmlNestedWidget (w, xpos, ypos+wheight);
				xpos += w->width.absolute;
			}
			break;
		}
	}
}


void M_DrawVisibleWindow ()
{
	int w = vid.width - visible_window->widget->width.absolute;
	int h = vid.height - visible_window->widget->height.absolute;
	M_DrawXmlNestedWidget (visible_window->widget, w/2, h/2);
}

// -------------------------------------------------------------------------------------

int M_ReadXmlPropAsInt (xmlNodePtr node, char *name, int defvalue)
{
	int ret;
	int p;
	xmlChar *value; 
	value = xmlGetProp (node, name);
	if (value){
		ret = atoi (value);
		xmlFree (value);
	} else 
		ret = defvalue;
	return ret;
}

int M_ReadXmlPropAsFloat (xmlNodePtr node, char *name, float defvalue)
{
	float ret;
	xmlChar *value; 
	value = xmlGetProp (node, name);
	if (value) {
		ret = atof (value);
		xmlFree (value);
	} else
		ret = defvalue;
	return ret;
}

char *M_ReadXmlPropAsString (xmlNodePtr node, char *name, char *ret, int size)
{
	xmlChar *value; 
	value = xmlGetProp (node, name);
	if (value){
		strncpy (ret, (char *)value, size);
		xmlFree (value);
	} else
		*ret='\0';

	return ret;
}

int M_CompareXmlProp (xmlNodePtr node, char *name, xmlChar **str, int count)
{
	int ret;
	xmlChar *value; 
	value = xmlGetProp (node, name);
	if (!value)
		return -1;
	for (ret = 0; ret < count; ret++){
		if (strcmp ((char *)value, (char *)str[ret]) == 0){
			xmlFree (value);
			return ret;
		}
	}
	xmlFree (value);
	return -1;
}

void M_ReadXmlDim (xmlNodePtr node, char *name, xmldim_t *dim)
{
	char *p;
	xmlChar *value; 
	value = xmlGetProp (node, name);
	if (value) {
		p = strchr (value,'%');
		if (p){
			*p = '\0';
			dim->ratio = atof (value) / 100;
			*p = '%';
			dim->absolute = -1;			
		} 
		else {
			dim->absolute = atoi (value);
			dim->ratio = -1;
		}
	}
	else {
		dim->absolute = -1;
		dim->ratio = -1;
	}
}

// -------------------------------------------------------------------------------------

void *M_AllocateMem (int size, void *template)
{
	// FIXME : allocate it in Cache ? 
	void *ret = Hunk_AllocName (size,"xmlmenu");
	if (template)
		memcpy (ret, template, size);
	else
		memset (ret,0,size);
	return ret;
}


void M_RemoveXmlWidget (qwidget_t *owner, qwidget_t *child)
{
	if (child->parent == owner) {
		if (child->previous)
			child->previous->next = child->next;
		else
			owner->children = child->next;
		if (child->next)
			child->next->previous = child->previous;
		else 
			owner->rchildren = child->previous;
		owner->num_children--;
		child->next = child->previous = NULL;
	}
}

void M_InsertXmlWidget (qwidget_t *owner, qwidget_t *child)
{

	child->parent = owner;
	owner->num_children++;
	if (owner->rchildren) {
		qwidget_t *w = owner->rchildren;
		child->next = NULL;
		owner->rchildren = child;
		w->next = child;
		child->previous = w;
	}
	else {
		owner->children = child;
		owner->rchildren = child;
	}
}


// -------------------------------------------------------------------------------------

/*
================
M_BuildHardcodedMenu

Build a default menu set
FIXME : should make something here and add a call in case no menu have been loaded
        (perhaps hardcode the original quake menu) 
================
*/
  
void M_BuildHardcodedMenus ()
{
	// check legacy aliases (menu_main etc...)
}

// -------------------------------------------------------------------------------------


/*
================
M_CalculateWidgetDim

evaluate widgets size and offsets recursively
The goal is to avoid recalculating everything for each frame
================
*/

void M_CalculateWidgetDim (qwidget_t *root)
{	
	// we don't want unecessary data on the stack -> so let's make them static
	qwidget_t *w;
	static int num;     // undimensioned widget counter
	static int width;   // remaining width 
	static int height;  // and height

	if (root->children == NULL)
		return;

	num = 0;
	//Con_DPrintf("%s (%d,%d)\n",root->tag,root->width.absolute,root->height.absolute);
	width = root->width.absolute;
	height = root->height.absolute;

	if (root->orient){ // vertical orientation
		for (w = root->children; w != NULL; w = w->next){
			if (w->height.ratio != -1)
				w->height.absolute = (root->height.absolute * w->height.ratio);
			if (w->height.absolute != -1)
				height -= w->height.absolute;
			else num++;
			if (w->width.ratio != -1)
				w->width.absolute = (root->width.absolute * w->width.ratio);
			if (w->width.absolute == -1)			
				w->width.absolute = root->width.absolute;
		}
		if (num) {
			// widget without defined dimension take a part of the remaining space
			for (w = root->children; w != NULL; w = w->next)
				if (w->height.absolute == -1)
					w->height.absolute = height / num;
		} else {
			// check for remaining space and set x offset according to the align properties
			if (width)
				// if orient == vertical -> align changes x offset
				switch (root->align) {
				case a_start:
					root->xoffset = 0;
					break;
				case a_center:
					root->xoffset = width / 2;
					break;
				case a_end:
					root->xoffset = width;
					break;
				case a_baseline:
					// not supported for vertical boxes
					root->xoffset = 0;
					break;
				case a_stretch:
					// search for first flexible child and resize it
					// FIXME : support flex attribute
					root->xoffset = 0;
					break;
				}
		}
			
	} 
	else { // horizontal orientation
		for (w = root->children; w != NULL; w = w->next){
			if (w->width.ratio != -1)
				w->width.absolute = (root->width.absolute * w->width.ratio);
			if (w->width.absolute != -1)
				width -= w->width.absolute;
			else num++;
			if (w->height.ratio != -1)
				w->height.absolute = (height * w->height.ratio);
			if (w->height.absolute == -1)
				w->height.absolute = height;
		}
		if (num) {
			// widget without defined dimension take a part of the remaining space
			for (w = root->children; w != NULL; w = w->next)
				if (w->width.absolute == -1)
					w->width.absolute = width / num;
		} else {
			// check for remaining space and set y offset according to the align properties
			if (height)
				// if orient == horizontal -> align changes y offset
				switch (root->align) {
				case a_start:
					root->yoffset = 0;
					break;
				case a_center:
					root->yoffset = height / 2;
					break;
				case a_end:
					root->yoffset = height;
					break;
				case a_baseline:
					// FIXME: here we should look for label 
					//        in children and align them accordingly
					//        but it's getting complicated
					root->yoffset = 0;
					break;
				case a_stretch:
					// search for first flexible child and resize it
					// FIXME : support flex attribute
					root->yoffset = 0;
					break;
				}
		}
	}
	// let's do it for the children now
	for (w = root->children; w != NULL; w = w->next)
		M_CalculateWidgetDim(w);
}

// -------------------------------------------------------------------------------------

/*
================
M_LoadXml*

Load a Xml Element definition from the provided libxml tree pointer 
================
*/


void M_LoadXmlBox (qwidget_t *widget, xmlNodePtr node)
{

	/* 
	 * --- supported attributes :
	 * --- unsupported :
	 */


}

void M_LoadXmlLabel (qwidget_t *widget, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :
	 
	   control    =  string : xul element id 
	   disabled   =  bool   
	   value      =  string : label text

	 * --- unsupported :

	   accesskey  =  string 

	 */
	xmllabeldata_t *data = widget->data;	
	M_ReadXmlPropAsString (node, "value", data->text, sizeof (data->text));	
}

void M_LoadXmlImage (qwidget_t *widget, xmlNodePtr node)
{

	/* 
	 * --- supported attributes :
	 
	   src        =  URL 
	 
	 * --- unsupported :
	 
	   onerror    =  string
	   onload     =  string
	   validate   =  {always|never} : load image from cache

	 */
	xmlimagedata_t *data = widget->data;
	M_ReadXmlPropAsString (node, "src", data->src, sizeof (data->src));
	//GL_LoadPic (data->src,&(data->pic));
}


void M_LoadXmlButton (qwidget_t *widget, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   command    =  string
	   disabled   =  bool
	   label      =  string
	   
	 * --- unsupported :

	   image      =  URL	   
	   accesskey  =  string : shortcut key
	   autoCheck  =  bool ?
	   checked    =  bool ?
	   checkState =  bool ?
	   crop       =  {start|end|center|none} : how the text is cropped when too large 
	   dlgType    =  {accept|cancel|help|disclosure}
	   group      =  int
	   open       =  ?
	   tabindex   =  int : tab order 
	   type       =  {checkbox|menu|menu-button|radio}
	   value      =  string ? : user attribute

	*/
	xmlbuttondata_t *data = widget->data;
	int temp;

	M_ReadXmlPropAsString (node, "command", data->command, sizeof(data->command));
	M_ReadXmlPropAsString (node, "label", data->label, sizeof(data->label));
	temp = M_CompareXmlProp (node, "disabled", xmlbool, 2);
	if (temp != -1)
		data->disabled = temp;
	
}

void M_LoadXmlCheckBox (qwidget_t *widget, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   disabled   =  bool
	   label      =  string
	   
	 * --- unsupported :

	   accesskey  =  string : shortcut key
	   autoCheck  =  bool ?
	   checked    =  bool ?
	   checkState =  bool ?
	   crop       =  {start|end|center|none} : how the text is cropped when too large 
	   dlgType    =  {accept|cancel|help|disclosure}
	   group      =  int
	   open       =  ?
	   tabindex   =  int : tab order 
	   type       =  {checkbox|menu|menu-button|radio}
	   value      =  string ? : user attribute
	*/
	xmlcheckboxdata_t *data = widget->data;
	int temp;

	M_ReadXmlPropAsString (node, "label", data->label, sizeof (data->label));
	temp = M_CompareXmlProp (node, "disabled", xmlbool, 2);
	if (temp != -1)
		data->disabled = temp;
	data->checked = Cvar_VariableValue (widget->id);	
}

void M_LoadXmlRadio (qwidget_t *widget, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   disabled   =  bool
	   label      =  string
	   selected   =  bool

	 * --- unsupported :

	   accesskey  =  string : shortcut key
	   autoCheck  =  bool ?
	   checked    =  bool ?
	   checkState =  bool ?
	   crop       =  {start|end|center|none} : how the text is cropped when too large 
	   focused    =  bool
	   group      =  int
	   open       =  ?
	   tabindex   =  int : tab order 
	   type       =  {checkbox|menu|menu-button|radio}
	   value      =  string ? : user attribute
	*/
	xmlradiodata_t *data = widget->data;
	int temp;

	M_ReadXmlPropAsString (node, "label", data->label, sizeof (data->label));

	temp = M_CompareXmlProp (node, "disabled", xmlbool, 2);
	if (temp != -1)
		data->disabled = temp;

	temp = M_CompareXmlProp (node, "selected", xmlbool, 2);
	if (temp != -1)
		data->selected = temp;

}

void M_LoadXmlRadioGroup (qwidget_t *widget, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   disabled   =  bool

	 * --- unsupported :

	   value      =  string ? : user attribute
	*/
	xmlradiogroupdata_t *data = widget->data;
	int temp;
	temp = M_CompareXmlProp (node, "disabled", xmlbool, 2);
	if (temp != -1)
		data->disabled = temp;
}

void M_LoadXmlSlider (qwidget_t *widget, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   curpos     =  [0,maxpos] : cursor position
	   maxpos     =  int : maximum cursor value
	   pageincrement =  int 

	 * --- unsupported :
	 
   	   increment  =  int : 

	*/
	xmlsliderdata_t *data = widget->data;

	data->range = M_ReadXmlPropAsInt (node, "maxpos", 0);
	// FIXME : shoud property override cvar value ?
	data->cursor = M_ReadXmlPropAsInt (node, "curpos", Cvar_VariableValue (widget->name));
}


void M_LoadXmlMenu (qwidget_t *widget, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :
	 
	 * --- unsupported :
	   
	   screenX    =  int : window x position (persistent ?)
	   screenY    =  int : window y position (persistent ?)
	   sizemode   =  {maximized|minimized|normal} : window size state
	   title      =  string
	   windowtype =  string	   
	*/

}

void M_LoadXmlWindow (qwidget_t *widget, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	 * --- unsupported :
	   
	   screenX    =  int : window x position (persistent ?)
	   screenY    =  int : window y position (persistent ?)
	   sizemode   =  {maximized|minimized|normal} : window size state
	   title      =  string
	   windowtype =  string	   
	*/
	qwidget_t *w; 
	qwindow_t *data = widget->data;
	char command[255];
	data->widget = widget;

	// check window size
	if (widget->width.ratio != -1)
		widget->width.absolute = widget->width.ratio * vid.width;
	if (widget->height.ratio != -1)
		widget->height.absolute = widget->height.ratio * vid.height;
	
	if (widget->width.absolute == -1)
		widget->width.absolute = vid.width;		
	if (widget->height.absolute == -1)
		widget->height.absolute = vid.height;
	
	// add newly created window to the list
	data->next = windows;
	windows = data;
	
	// register alias for quake compat
	strcpy (command, "alias menu_\0"); 
	strcat (command, widget->id);           // no more that 32 char in widget->id
	strcat (command, " \"openwindow ");     //
	strcat (command, widget->id);           // no more that 32 char in widget->id
	strcat (command, "\"\n");               // so it should fit in command

	Cbuf_AddText (command);	

	// we have a fully loaded window here
	M_CalculateWidgetDim (widget);

	/*
	// set root on all sub-widget
	w = widget;
	while (w){
		w->root = widget;
		w = M_NextXmlElement (w);
	}
	*/

}


qwidget_t *M_LoadXmlElement (xmlNodePtr root)
{
	/*
	 * --- supported attributes :

	   align       =  {start|center|end|baseline|stretch}
	   pack        =  {start|center|end}
	   debug       =  bool  : draw a border around it and all children ?
	   height      =  int 
	   width       =  int 
	   id          =  string : the name/command of the object
	   orient      =  {horizontal|vertical}

	 * --- unsupported :


	   allowevents
	   allownegativeassertions
	   class
	   coalesceduplicatearcs
	   collapsed
	   container
	   containment
	   context
	   contextmenu
	   datasources
	   dir
	   empty
	   equalsize   =  {always|never} : This attribute can be used to make the children of the element equal in size.
	   flags       =  {dont-test-empty|dont-build-content}
	   flex
	   flexgroup
	   hidden    
	   insertafter
	   insertbefore
	   left
	   maxheight
	   maxwidth
	   menu
	   minheight
	   minwidth
	   observes
	   ordinal
	   pack       =  {start|center|end}
	   persist
	   popup
	   position
	   ref
	   removeelement
	   statustext
	   style
	   template
	   tooltip
	   tooltiptext
	   top
	   uri
	*/

	xmlNodePtr     node;
	qwidget_t *ret,*sub;
	xmlhandler_t *handler;
	int temp;
	static int sp;
	
	// we don't want anonymous tags
	if (!root->name)
		return NULL;
	// first read/initialize specifics attributes 
	handler = widgethandlers;

	while (handler->tag) {
		if (!xmlStrcmp (root->name, handler->tag)){
			break;
		}
		handler++;
	}
	
	if (!handler->tag)
		return NULL;
	
	ret = M_AllocateMem (sizeof(qwidget_t), &(handler->template));	

	if (handler->datasize)
		ret->data = M_AllocateMem (handler->datasize, NULL);
	ret->tag = (char *)handler->tag;
	
	// then read general attributes from the file 
	
	M_ReadXmlDim (root, "width", &ret->width);
	M_ReadXmlDim (root, "height", &ret->height);
	ret->debug = (M_ReadXmlPropAsInt (root, "debug", 0) != 0);
	M_ReadXmlPropAsString (root, "id", ret->id, sizeof(ret->id));
	M_ReadXmlPropAsString (root, "name", ret->name, sizeof(ret->name));
	temp = M_CompareXmlProp (root, "orient", xmlorient, 2);
	if (temp != -1)
		ret->orient = temp;
	
	temp = M_CompareXmlProp (root, "align", xmlalign, 5);
	if (temp != -1)
		ret->align = temp;
	temp = M_CompareXmlProp (root, "pack", xmlpack, 3);
	if (temp != -1)
		ret->pack = temp;

#if 0
	for (temp=0;temp<sp;temp++)
		Con_Printf(" ");
	Con_Printf("%s [%d:%d] (%d,%d)\n",ret->tag, ret->align, ret->pack, ret->width.absolute,ret->height.absolute);
	sp++;
#endif	
	/* load all the subnodes */
	node = root->xmlChildrenNode;
	while ( node != NULL ){
		// comment nodes have node->name == NULL  
		if (!xmlIsBlankNode (node) && (node->type != XML_COMMENT_NODE)){
			sub = M_LoadXmlElement (node);
			if (sub)
				M_InsertXmlWidget (ret, sub);
		}
		node = node->next;
	}
#if 0
	sp--;
#endif
	// finally init the private data
	ret->Load (ret, root);

	return ret;
}

/*
================
M_LoadXmlMenu

Load a Menu(Window) definition from a file 
================
*/

void M_LoadXmlWindowFile (const char *filename)
{
#if 0	
	// best method : lower resource need
	// unfinished ^^;
	int res, size;
	xmlParserCtxtPtr ctxt;
	FILE *f=NULL;

	COM_FOpenFile(filename, &f);
	// here set the elements callbacks

	if (f != NULL) {
                int res, size = 1024;
                char chars[1024];
                xmlParserCtxtPtr ctxt;
		
                res = fread (chars, 1, 4, f);
                if (res > 0) {
			ctxt = xmlCreatePushParserCtxt (NULL, NULL,
						        chars, res, filename);
			while ((res = fread (chars, 1, size, f)) > 0) {
				xmlParseChunk (ctxt, chars, res, 0);
			}
			xmlParseChunk (ctxt, chars, 0, 1);
			doc = ctxt->myDoc;
			xmlFreeParserCtxt (ctxt);
                }
	}
	   
#else
	// easier method : memory heavier

	char *buffer;
	xmlDocPtr      doc;
	xmlNodePtr     node;
	int h, len;

	Con_DPrintf ("XML : loading %s document started \n",filename);

	// yuck - dirty filesize check
	len = COM_OpenFile (filename, &h);
	if (h == -1) {
		Con_Printf ("Warning : %s document not found\n",filename);
		return;
	}
	COM_CloseFile (h);

	buffer = COM_LoadTempFile (filename);
	
	doc = xmlParseMemory (buffer, len);
	if (doc == NULL) {
		Con_Printf ("Warning : %s document not found\n",filename);
		return;
	}
	node = xmlDocGetRootElement (doc);
	if (node == NULL) {
		Con_Printf ("Warning : %s empty document\n",filename);
		xmlFreeDoc (doc);
		return;
	}
	
	if (!xmlIsBlankNode (node) && (node->name)){
		if (!xmlStrcmp (node->name, (const xmlChar *)"window")) {
			M_LoadXmlElement (node);
		}
		else {
			Con_Printf ("Warning : %s document root isn't a window\n",filename);
		}
	}

	xmlFreeDoc (doc);

#endif
	Con_DPrintf ("XML : loading %s document ended \n",filename);		
}

// -------------------------------------------------------------------------------------
