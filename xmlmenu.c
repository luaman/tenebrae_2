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


#define MAX_MLABEL     128
#define MAX_MACCESSKEY 26

// -------------------------------------------------------------------------------------

cvar_t  m_debug = {"m_debug","0"};
int m_mousex, m_mousey;

qmtable_t box_mtable =
{
	M_LoadXmlBox,      // Load
	NULL,              // Draw
	NULL,              // Focus
	M_XmlBoxKey,       // HandleKey
	NULL			   // Drag
};
qmtable_t label_mtable =
{
	M_LoadXmlLabel,    // Load
	M_DrawXmlLabel,    // Draw
	NULL,              // Focus
	NULL,               // HandleKey
	NULL			   // Drag
};
qmtable_t image_mtable =
{
	M_LoadXmlImage, // Load
	M_DrawXmlImage, // Draw
	NULL,         // Focus
	NULL,         // HandleKey
	NULL			   // Drag
};
qmtable_t button_mtable =
{
	M_LoadXmlButton, // Load
	M_DrawXmlButton, // Draw
	NULL,         // Focus
	M_XmlButtonKey,  // HandleKey
	NULL			   // Drag
};
qmtable_t checkbox_mtable =
{
	M_LoadXmlCheckBox, // Load
	M_DrawXmlCheckBox, // Draw
	NULL,         // Focus
	M_XmlCheckBoxKey, // HandleKey
	NULL			   // Drag
};
qmtable_t radio_mtable =
{
	M_LoadXmlRadio, // Load
	M_DrawXmlRadio,  // Draw
	NULL,         // Focus
	M_XmlRadioKey,   // HandleKey
	NULL			   // Drag
};
qmtable_t rgroup_mtable =
{
	M_LoadXmlRadioGroup, // Load
	M_DrawXmlRadioGroup, // Draw
	NULL,         // Focus
	M_XmlRadioGroupKey, // HandleKey
	NULL			   // Drag
};
qmtable_t slider_mtable =
{
	M_LoadXmlSlider, // Load
	M_DrawXmlSlider, // Draw
	NULL,         // Focus
	M_XmlSliderKey, // HandleKey
	M_XmlSliderDrag   // Drag
};
qmtable_t edit_mtable =
{
	M_LoadXmlEdit, // Load
	M_DrawXmlEdit, // Draw
	NULL,          // Focus
	M_XmlEditKey,   // HandleKey
	NULL			   // Drag
};
qmtable_t window_mtable =
{
	M_LoadXmlWindow, // Load
	NULL,         // Draw
	NULL,         // Focus
	M_XmlWindowKey,  // HandleKey
	NULL			   // Drag
};

/*
qmtable_t key_mtable =
{
	M_LoadXmlKey,
	NULL,
	NULL,
	NULL
};

qmtable_t broadcaster_mtable = 
{
	M_LoadXmlBroadcaster,
	NULL,
	NULL,
	NULL
};
*/

// -------------------------------------------------------------------------------------

typedef struct xmlimagedata_s
{
	char           src[MAX_OSPATH];
	shader_t      *pic;
	int            width;
	int            height;
} xmlimagedata_t;


typedef struct xmlbuttondata_s
{
	int       isdown:1;
	char      label[MAX_MLABEL];
	char      command[255];
	shader_t	*pic;
} xmlbuttondata_t;

typedef struct xmlcheckboxdata_s
{
	int       checked:1;
	char      label[MAX_MLABEL];
} xmlcheckboxdata_t;


typedef struct xmlradiodata_s
{
	int       selected:1;
	char      label[MAX_MLABEL];
} xmlradiodata_t;

typedef struct xmllabeldata_s
{
	qwidget_t *target;
	char      text[MAX_MLABEL];
} xmllabeldata_t;

typedef struct xmlsliderdata_s
{
	float     range;
	float     cursor;
	char      label[MAX_MLABEL];
	shader_t	*pic;
} xmlsliderdata_t;

typedef struct xmleditdata_s
{
	int		  cursor;
	int		  length;
	char      text[MAX_MLABEL];
} xmleditdata_t;

typedef struct xmlradiogroupdata_s
{
	char      label[MAX_MLABEL];
} xmlradiogroupdata_t;


typedef struct xmlmenudata_s
{
	int       dummy;
} xmlmenudata_t;

/*
    xml window data
 */

typedef struct refstring_s
{
	int refcount;
	char *str;
	struct refstring_s *next;
} refstring_t;

typedef struct qaccesskey_s 
{
	int key;
	qwidget_t *widget;
} qaccesskey_t;

typedef struct qwindow_s {
	struct qwindow_s *next;   // available window list
	//char       file[MAX_OSPATH];
	qwidget_t *widget;
	qwidget_t *focused;
	qwidget_t *mouse_capture; // if nonnull this widget has the mouse captured
	refstring_t string_table;
	int keycount;
	qaccesskey_t keytable[MAX_MACCESSKEY];
	struct qwindow_s *stack;  // visible window stack
} qwindow_t;


char box_tag[]      = "box";
char hbox_tag[]     = "hbox";
char vbox_tag[]     = "vbox";
char label_tag[]    = "label";
char image_tag[]    = "image";
char button_tag[]   = "button";
char menuitem_tag[] = "menuitem";
char checkbox_tag[] = "checkbox";
char radio_tag[]    = "radio";
char rgroup_tag[]   = "radiogroup";
char slider_tag[]   = "slider";
char edit_tag[]     = "edit";
char window_tag[]   = "window";
char package_tag[]  = "package";

/*
  xml primitives template
*/

#define DEF_STR_VALUE NULL

qwidget_t box_template =
		{
				&box_mtable,   // mtable
				DEF_STR_VALUE,           // name
				DEF_STR_VALUE,           // id
				box_tag,         // tag
				NULL,			//font
				NULL,			//focusshader
				0,				//focus type
				0,            // num_children
				NULL,         // parent
				NULL,         // previous
				NULL,         // next
				NULL,         // children
				NULL,         // rchildren
			0,            // debug
			1,            // enabled
			0,            // focusable
			0,            // orient = horizontal
			a_stretch,      // align
			p_start,        // pack
			0,            // xpos
			0,            // ypos
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // accesskey
			NULL,         // onCommand
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		};

qwidget_t vbox_template =
		{
				&box_mtable,   // mtable
				DEF_STR_VALUE,           // name
				DEF_STR_VALUE,           // id
				vbox_tag,     // tag
				NULL,			//font
				NULL,			//focusshader
				0,				//focus type
				0,            // num_children
				NULL,         // parent
				NULL,         // previous
				NULL,         // next
				NULL,         // children
				NULL,         // rchildren
			0,            // debug
			1,            // enabled
			0,            // focusable
			1,            // orient = vertical
			a_stretch,      // align
			p_start,        // pack
			0,            // xpos
			0,            // ypos
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // accesskey
			NULL,         // onCommand
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		};

qwidget_t label_template =
		{
				&label_mtable, // mtable
				DEF_STR_VALUE,           // name
				DEF_STR_VALUE,           // id
				label_tag,    // tag
				NULL,			//font
				NULL,			//focusshader
				0,				//focus type
				0,            // num_children
				NULL,         // parent
				NULL,         // previous
				NULL,         // next
				NULL,         // children
				NULL,         // rchildren
			0,            // debug
			1,            // enabled
			0,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			0,            // xpos
			0,            // ypos
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // accesskey
			NULL,         // onCommand
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		};

qwidget_t image_template =
		{
				&image_mtable, // mtable
				DEF_STR_VALUE,           // name
				DEF_STR_VALUE,           // id
				image_tag,         // tag
				NULL,			//font
				NULL,			//focusshader
				0,				//focus type
				0,            // num_children
				NULL,         // parent
				NULL,         // previous
				NULL,         // next
				NULL,         // children
				NULL,         // rchildren
			0,            // debug
			1,            // enabled
			0,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			0,            // xpos
			0,            // ypos
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // accesskey
			NULL,         // onCommand
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		};

qwidget_t button_template =
		{
				&button_mtable,// mtable
				DEF_STR_VALUE,           // name
				DEF_STR_VALUE,           // id
				button_tag,         // tag
				NULL,			//font
				NULL,			//focusshader
				0,				//focus type
				0,            // num_children
				NULL,         // parent
				NULL,         // previous
				NULL,         // next
				NULL,         // children
				NULL,         // rchildren
			0,            // debug
			1,            // enabled
			1,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			0,            // xpos
			0,            // ypos
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // accesskey
			NULL,         // onCommand
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		};

qwidget_t checkbox_template =
		{
				&checkbox_mtable, // mtable
				DEF_STR_VALUE,           // name
				DEF_STR_VALUE,           // id
				checkbox_tag,         // tag
				NULL,			//font
				NULL,			//focusshader
				0,				//focus type
				0,            // num_children
				NULL,         // parent
				NULL,         // previous
				NULL,         // next
				NULL,         // children
				NULL,         // rchildren
			0,            // debug
			1,            // enabled
			1,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			0,            // xpos
			0,            // ypos
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // accesskey
			NULL,         // onCommand
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		};

qwidget_t radio_template =
		{
				&radio_mtable, // mtable
				DEF_STR_VALUE,           // name
				DEF_STR_VALUE,           // id
				radio_tag,         // tag
				NULL,			//font
				NULL,			//focusshader
				0,				//focus type
				0,            // num_children
				NULL,         // parent
				NULL,         // previous
				NULL,         // next
				NULL,         // children
				NULL,         // rchildren
			0,            // debug
			1,            // enabled
			1,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			0,            // xpos
			0,            // ypos
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // accesskey
			NULL,         // onCommand
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		};

qwidget_t rgroup_template =
		{
				&rgroup_mtable, // mtable
				DEF_STR_VALUE,           // name
				DEF_STR_VALUE,           // id
				rgroup_tag,         // tag
				NULL,			//font
				NULL,			//focusshader
				0,				//focus type
				0,            // num_children
				NULL,         // parent
				NULL,         // previous
				NULL,         // next
				NULL,         // children
				NULL,         // rchildren
			0,            // debug
			1,            // enabled
			1,            // focusable
			1,            // orient = vertical
			a_stretch,      // align
			p_start,        // pack
			0,            // xpos
			0,            // ypos
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // accesskey
			NULL,         // onCommand
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		};

qwidget_t slider_template =
		{
				&slider_mtable, // mtable
				DEF_STR_VALUE,           // name
				DEF_STR_VALUE,           // id
				slider_tag,         // tag
				NULL,			//font
				NULL,			//focusshader
				0,				//focus type
				0,            // num_children
				NULL,         // parent
				NULL,         // previous
				NULL,         // next
				NULL,         // children
				NULL,         // rchildren
			0,            // debug
			1,            // enabled
			1,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			0,            // xpos
			0,            // ypos
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // accesskey
			NULL,         // onCommand
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		};

qwidget_t edit_template =
		{
			&edit_mtable, // mtable
			DEF_STR_VALUE,           // name
			DEF_STR_VALUE,           // id
			edit_tag,         // tag
			NULL,			//font
				NULL,			//focusshader
				0,				//focus type
			0,            // num_children
			NULL,         // parent
			NULL,         // previous
			NULL,         // next
			NULL,         // children
			NULL,         // rchildren
			0,            // debug
			1,            // enabled
			1,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			0,            // xpos
			0,            // ypos
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // accesskey
			NULL,         // onCommand
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		};

qwidget_t window_template =
		{
				&window_mtable,  // mtable
				DEF_STR_VALUE,           // name
				DEF_STR_VALUE,           // id
				window_tag,         // tag
				NULL,			//font
				NULL,			//focusshader
				0,				//focus type
				0,            // num_children
				NULL,         // parent
				NULL,         // previous
				NULL,         // next
				NULL,         // children
				NULL,         // rchildren
			0,            // debug
			1,            // enabled
			0,            // focusable
			0,            // orient
			a_stretch,      // align
			p_start,        // pack
			0,            // xpos
			0,            // ypos
			0,            // xpos
			0,            // ypos
			{0,0},        // width
			{0,0},        // height
			0,            // accesskey
			NULL,         // onCommand
			NULL,         // onMouseOver
			NULL,         // onMouseDown
			NULL,         // onMouseUp
			NULL          // data
		};

typedef struct xmlhandlers_s {
        char          *tag;
	void          *template;
	int            templatesize;
	int            datasize;
} xmlhandler_t;

xmlhandler_t widgethandlers[] =
{
	{
		box_tag,
		&box_template,
		sizeof(qwidget_t),
		0
	},
	{
		hbox_tag,
		&box_template,
		sizeof(qwidget_t),
		0
	},
	{
		vbox_tag,
		&vbox_template,
		sizeof(qwidget_t),
		0
	},
	{
		label_tag,
		&label_template,
		sizeof(qwidget_t),
		sizeof(xmllabeldata_t)
	},
	{
		image_tag,
		&image_template,
		sizeof(qwidget_t),
		sizeof(xmlimagedata_t)
	},
	{
		button_tag,
		&button_template,
		sizeof(qwidget_t),
		sizeof(xmlbuttondata_t)
	},
	{
		menuitem_tag,
		&button_template,
		sizeof(qwidget_t),
		sizeof(xmlbuttondata_t)
	},
	{
		checkbox_tag,
		&checkbox_template,
		sizeof(qwidget_t),
		sizeof(xmlcheckboxdata_t)
	},
	{
		radio_tag,
		&radio_template,
		sizeof(qwidget_t),
		sizeof(xmlradiodata_t)
	},
	{
		rgroup_tag,
		&rgroup_template,
		sizeof(qwidget_t),
		sizeof(xmlradiogroupdata_t)
	},
	{
		slider_tag,
		&slider_template,
		sizeof(qwidget_t),
		sizeof(xmlsliderdata_t)
	},
	{
		edit_tag,
		&edit_template,
		sizeof(qwidget_t),
		sizeof(xmleditdata_t)
	},
	{
		window_tag,
		&window_template,
		sizeof(qwidget_t),
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

void M_DrawXmlNestedWidget (qwidget_t *self);


// -------------------------------------------------------------------------------------

#define FOCUS_NOTSPECIFIED	0	//use parent's focus shader
#define FOCUS_BORDER		1
#define FOCUS_UNDER			2
#define FOCUS_OVER			3

typedef struct {
	int focusType;			//Type of focus to use
	shader_t *focusShader;	//Shader to use for focus
	drawfont_t *currentFont;//Font to use for text
} menudrawstate_t;

qwindow_t *windows_stack = NULL; //  window_stack : holds all instances of qwindow_t
static menudrawstate_t currentState;

void M_AddWindowInList (qwindow_t *window)
{
	window->next = windows_stack;
	windows_stack = window;	
}

qwindow_t * M_RemoveWindowInList (char *name)
{
	qwindow_t **w = &windows_stack;
	qwindow_t *ret;

	while (*w!=NULL && !strcmp (name,(*w)->widget->name))
		w = &(*w)->next;
	if (!*w)
		return NULL;
	ret = *w;
	*w = (*w)->next;
	return ret;
}

qwindow_t *loading;

qwindow_t *visible_window;

shader_t *focus_shader;

// -------------------------------------------------------------------------------------

/* object name table */

char *M_CreateRefString (char *str, int len)
{
	refstring_t *ptr = &(loading->string_table);

	 while (ptr->next) {
		if (!Q_strcmp (ptr->str,str)) {
			ptr->refcount++;
			return ptr->str;
		}
		if (ptr->next == NULL)
			break;
		ptr = ptr->next;
	} 

	ptr->next = Z_Malloc (sizeof (refstring_t));
	ptr = ptr->next;
	ptr->next = NULL;
	ptr->refcount = 1;
	ptr->str = Z_Malloc (len+1);
	strncpy (ptr->str, str, len);
	ptr->str[len] = '\0';
	return ptr->str;
}

void M_DeleteRefString (char *str, int len)
{
	refstring_t *ptr = &(loading->string_table);
	while (ptr->next != NULL) {
		if (!Q_strcmp (ptr->next->str,str)) {
			refstring_t *next = ptr->next;
			next->refcount--;
			if (next->refcount == 0) {
				ptr->next = next->next;
				Z_Free (ptr->str);
				Z_Free (ptr);
			}
			return;
		}
		else		 
			ptr = ptr->next;
	}	
}

// -------------------------------------------------------------------------------------

/*  shortcuts */

void M_AddAccessKey (qwidget_t *self, qwindow_t *window)
{
	//qwidget_t *self=ptr;
	if (!self->accesskey)
		return;
	if (window->keycount >= MAX_MACCESSKEY) {
		Con_Printf("warning : accesskey table maximum reached for %s",window->widget->id);
		return;
	}
	window->keytable[window->keycount].key = self->accesskey;
	window->keytable[window->keycount++].widget = self;
}

void M_ReadAccessKeys (qwidget_t *self, qwindow_t *root)
{
	qwidget_t *w;

	M_AddAccessKey (self, root);
	for (w = self->children; w != NULL; w = w->next)
		M_ReadAccessKeys (w, root);
	
}

// -------------------------------------------------------------------------------------

/*
struct qmattrptr_s 
{
	unsigned int attr_offset;
	unsigned int attr_size;
	unsigned int mask;
};

struct qmobserver_s 
{
	qwidget *owner;
	char *broadcaster;
	char *attribute;
};

struct qmbroadcaster_s
{
	qwidget_t widget;
	
}
*/

// -------------------------------------------------------------------------------------


/*
typedef struct m_iterator_s 
{
	qmelement_t *w;
	void (*next_f)(struct eliterator_s *self);
} m_iterator_t;

void iterator_next (m_iterator_t *self)
{
	qmelement_t *w = self->w;
	if (w->next) 
		self->w = w->next;
	else
		self->w = NULL;
}

void iterator_prev (m_iterator_t *self)
{
	qmelement_t *w = self->w;
	if (w->previous)
		self->w = w->previous;
	else 
		self->w = NULL;
}

void treeiterator_next (m_iterator_t *self)
{
	qmelement_t *w = self->w;
	if (w->children)
		self->w = w->children;
	else if (!w->next)
		while (w->parent && !w->next)
			w = w->parent;
	self->w = w->next;
}

void treeiterator_prev (m_iterator_t *self)
{
	qmelement_t *w = self->w;
	if (w->rchildren)
		self->w = w->rchildren;
	else if (!w->previous)
		while (w->parent && !w->previous)
			w = w->parent;
	self->w = w->previous;
}
*/

// -------------------------------------------------------------------------------------

#define	SLIDER_RANGE      10
#define SLIDER_START_CHAR 128
#define SLIDER_CHAR       129
#define SLIDER_END_CHAR   131

qboolean        m_entersound;           // play after drawing a frame, so caching
                                        // won't disrupt the sound
qboolean        m_recursiveDraw;

int             m_return_state;
qboolean        m_return_onerror;
char            m_return_reason [32];

enum {m_none, m_menu } m_state;

void M_SetFocus(qwindow_t *w, qwidget_t *newFocus) {
	
	//notify widget of lost focus
	if (w->focused->mtable->Focus) {
		w->focused->mtable->Focus(w->focused);
	}

	//make new widged focused
	w->focused = newFocus;
}

// -------------------------------------------------------------------------------------
//
//	Menu action script commands, these are all normal console commands but are specially
//  aimed to be used in menu actions.
//
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
	
	//release focus for this window
	M_SetFocus(visible_window, NULL);

	if (visible_window != NULL){
		visible_window = visible_window->stack;
		temp->stack = NULL;
	}
	if (visible_window == NULL)
		key_dest = key_game;
}

void M_OpenWindow_f (void)
{
	qwindow_t *w = windows_stack;
	qwindow_t *temp = visible_window;
	char *name;
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("openwindow <name> : bring up the window called 'name'\n");
		return;
	}
	name = Cmd_Argv(1);
	while (w){
		if (!strncmp (w->widget->id, name, strlen(w->widget->id)))
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
	qwindow_t *w = windows_stack;
	while (w){
		Con_Printf ("%s\n", w->widget->id);		
		w = w->next;
	}
	
}

// -------------------------------------------------------------------------------------
//
//	Interface of the menus towards the rest of the game engine
//
// -------------------------------------------------------------------------------------

void M_Init (void)
{

	Cmd_AddCommand ("startgame", M_StartGame_f);
	Cmd_AddCommand ("openwindow", M_OpenWindow_f);
	Cmd_AddCommand ("closewindow", M_CloseWindow_f);
	Cmd_AddCommand ("windowlist", M_WindowList_f);
	Cvar_RegisterVariable (&m_debug);
	// let's load the whole stuff now
	Con_Printf("=================================\n");
	Con_Printf("M_Init: Initializing xml menu system\n");
	COM_FindAllExt ("menu", "xul", M_LoadXmlWindowFile);
	Con_Printf("=================================\n");

	currentState.currentFont = Draw_DefaultFont();
	currentState.focusShader = GL_ShaderForName ("menu/focus");
	currentState.focusType = FOCUS_BORDER;
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

/*
========
M_Keydown

recursive function set : this one is at top level, and calls the focused widget key handler
========
*/
void M_Keydown (int key)
{
	qwidget_t *focused = visible_window->focused;
	if (focused && focused->mtable->HandleKey)
		M_XmlElementKey (focused,key);
	else 	// current window get the key
		M_XmlElementKey (visible_window->widget, key);
}

void M_Keyup (int key)
{
	Con_Printf("Mkeyup %i\n",key);
	if (visible_window->mouse_capture && key == K_MOUSE1) {
		visible_window->mouse_capture = NULL;
	}
}

void M_XmlMouseMove ( int x, int y );
void M_MouseMove ( int x, int y ) {
	m_mousex = x;
	m_mousey = y;
	M_XmlMouseMove (x, y);
}

// -------------------------------------------------------------------------------------
// STATUS BAR CODE STUB
// -------------------------------------------------------------------------------------

int sb_lines;
qwindow_t *statusWindow = NULL;
qwindow_t *bigStatusWindow = NULL;

/*
========
Sbar_Changed
 
marks status bar to be updated next frame 
========
*/
void Sbar_Changed (void)
{
	
}
/*
========
Sbar_Draw
 
render status bar 
========
*/
void Sbar_Draw (void)
{
	qwindow_t *oldviz = visible_window;
	qwindow_t *wnd;
	int w, h;

	if (sb_lines <= 0) return;

	if (sb_lines <= 24) {
		if (!statusWindow) return;
		wnd = statusWindow;
	} else {
		if (!bigStatusWindow) return;
		wnd = bigStatusWindow;
	}

	visible_window = wnd;

	w = vid.width - wnd->widget->width.absolute;
	h = sb_lines - wnd->widget->height.absolute;
	M_DrawXmlNestedWidget (wnd->widget);

	visible_window = oldviz;
}
/*
========
Sbar_FinaleOverlay
 
end of game panel 
========
*/
void Sbar_FinaleOverlay (void)
{
}
/*
========
Sbar_Init
 
setup data, ccommands and cvars related to status bar 
========
*/
void Sbar_Init (void)
{
	qwindow_t *w = windows_stack;
	while (w){
		if (!strncmp (w->widget->id, "statusbar", strlen(w->widget->id)))
			break;
		w = w->next;
	}
	if (!w) {
		Con_Printf ("Sbar_Init :no statusbar defined\n");
	} else {
		statusWindow = w;
	}

	w = windows_stack;
	while (w){
		if (!strncmp (w->widget->id, "bigstatusbar", strlen(w->widget->id)))
			break;
		w = w->next;
	}
	if (!w) {
		Con_Printf ("Sbar_Init :no bigstatusbar defined\n");
	} else {
		bigStatusWindow = w;
	}
}
/*
========
Sbar_IntermissionOverlay
 
end of level statistics box  
========
*/
void Sbar_IntermissionOverlay (void)
{
}

// -------------------------------------------------------------------------------------

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
//
//	Menu event handling, code that handles key presses and mouse moves for the different
//	widgets.
//
// -------------------------------------------------------------------------------------

void M_GetSaveDescription(const char *filename, char *buff, int len) {

	char	name[MAX_OSPATH];
	int		version;
	FILE	*f;

	strncpy (buff, "--- UNUSED SLOT ---", len-1);
	
	sprintf (name, "%s%s.sav", com_gamedir, filename);
	f = fopen (name, "r");
	if (!f)
		return;
	fscanf (f, "%i\n", &version);
	fscanf (f, "%79s\n", name);
	strncpy (buff, name, len-1);

	fclose(f);
}

qboolean M_IsSaveLoadable(const char *filename) {

	char	name[MAX_OSPATH];
	FILE	*f;

	sprintf (name, "%s%s.sav", com_gamedir, filename);
	f = fopen (name, "r");
	if (!f)
		return false;
	fclose(f);
	return true;
}

void M_XmlButtonCommand (qwidget_t *self)
{
	// qwidget_t *self = ptr;
	xmlbuttondata_t *data = self->data;
	Cbuf_AddText (data->command);
	Cbuf_AddText ("\n");
}

qmelement_t *M_NextQMElement (qmelement_t *w)
{
        if (w->children)
		return w->children;
	if (w->next)
		return w->next;
	while (w->parent && !w->next)
		w = w->parent;
	if (!w->next)
		return NULL;
	return w->next;
}

qmelement_t *M_PreviousQMElement (qmelement_t *w)
{
	if (w->rchildren)
		return w->rchildren;
	if (w->previous)
		return w->previous;
	while (w->parent && !w->previous)
		w = w->parent;
	if (!w->next)
		return NULL;
	return w->previous;
}

qboolean M_IsFocusable (qmelement_t *e)
{
	return e->focusable && e->enabled;
}

void M_CycleFocus (qmelement_t *(*next_f)(qmelement_t *))
{
	qmelement_t *w = (qmelement_t *)visible_window->focused;
	qmelement_t *start = w;	
	do {
		w = next_f (w);
		if (w == NULL)
			w = (qmelement_t *)visible_window->widget;
	} while ((w != start) && !M_IsFocusable (w));
	M_SetFocus(visible_window, (qwidget_t *)w);
}

// -------------------------------------------------------------------------------------

qboolean M_XmlBoxKey (qwidget_t *self, int k)
{
	// qwidget_t *self = ptr;

	if (!self->children)
		return false;

	switch (k)
	{
	case K_UPARROW:
		//S_LocalSound ("misc/menu1.wav");		
		M_CycleFocusPrevious ();		
		break;

	case K_DOWNARROW:
	case K_TAB:
		//S_LocalSound ("misc/menu1.wav");
		M_CycleFocusNext ();		
		break;
	case K_ESCAPE:
		// pop up previous menu
		//S_LocalSound ("misc/menu2.wav");
		M_CloseWindow_f ();
		break;
	default:
		return false;
	}
	return true;
}

qboolean M_XmlCheckBoxKey (qwidget_t *self, int k)
{
	// qwidget_t *self = ptr;
	xmlcheckboxdata_t *data = self->data;
	switch (k)
	{
	case K_ENTER:
	case K_MOUSE1:
		// (un)check the checkbox
		//S_LocalSound ("misc/menu2.wav");
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
	// qwidget_t *self = ptr;
	switch (k)
	{
	case K_ENTER:
	case K_MOUSE1:
		//S_LocalSound ("misc/menu2.wav");
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
	case K_MOUSE1:
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
	// qwidget_t *self = ptr;
	xmlsliderdata_t *data = self->data;
	switch (k)
	{
	case K_LEFTARROW:
		// reduce cvar value
		//S_LocalSound ("misc/menu3.wav");
		data->cursor -= 1/SLIDER_RANGE;
		break;
	case K_RIGHTARROW:
		// augment cvar value
		//S_LocalSound ("misc/menu3.wav");
		data->cursor += 1/SLIDER_RANGE;
		break;
	case K_MOUSE1:
		//Get the capture
		visible_window->mouse_capture = self;
		break;
	default:
		return false;
	}
	return true;
}

void M_XmlSliderDrag (qwidget_t *self, int x, int y)
{
	xmlsliderdata_t *data = self->data;
	float zeroone;

	if (x < self->parent->xpos)
		x = self->parent->xpos;
	if (x+self->width.absolute >= self->parent->xpos+self->parent->width.absolute)
		x = self->parent->xpos+self->parent->width.absolute-self->width.absolute;

	//Set the value
	zeroone = (x-self->parent->xpos)/(float)(self->parent->width.absolute-self->width.absolute);
	data->cursor = zeroone*data->range;
	Cvar_SetValue(self->id, zeroone*data->range);
}

qboolean M_XmlEditKey (qwidget_t *self, int k)
{
	// qwidget_t *self = ptr;
	xmleditdata_t *data = self->data;
	int textlen = strlen(data->text);
	switch (k)
	{
	case K_LEFTARROW:
		// reduce cvar value
		//S_LocalSound ("misc/menu3.wav");
		if (data->cursor > 0)
			data->cursor -= 1;
		break;
	case K_RIGHTARROW:
		// augment cvar value
		//S_LocalSound ("misc/menu3.wav");
		if (data->cursor < textlen)
		data->cursor += 1;
		break;
	case K_BACKSPACE:
		if (data->cursor > 0) {
			data->text[data->cursor-1] = 0;
			data->cursor -= 1;
		}
		Cvar_Set (self->id, data->text);
		break;
	default:
		if ((k >= 32) && (k <= 126)) {
			data->text[data->cursor] = k;
			if (data->cursor >= textlen) {
				data->text[data->cursor+1] = 0; //null terminator was overwritten
			}
			data->cursor += 1;
			Cvar_Set (self->id, data->text);
			return true;
		}
		return false;
	}
	return true;
}

qboolean M_XmlWindowKey (qwidget_t *self, int k)
{
	// qwidget_t *self = ptr;
	qwindow_t *data = self->data;
	int i;
	if (M_XmlBoxKey (self,k))
		return true;
	for (i=0; i<data->keycount; ++i) 
		if (data->keytable[i].key==k) {
			M_SetFocus(data, data->keytable[i].widget);
			return true;
		}
	return false;
}

qboolean M_XmlMenuKey (qwidget_t *self, int k)
{
	return M_XmlBoxKey (self,k);
}

qboolean M_XmlElementKey (qwidget_t *self, int k)
{
	// start from widget 
	// and propagate the key up the parents tree 
	// until it has been consumed or it reaches the
	// tree root
	//qmelement_t *widget = ptr;
	while (self){ 
		if (self->mtable->HandleKey)
			if (self->mtable->HandleKey (self, k))
					 return true;
		self = self->parent;
	}
	return false;
}

qboolean M_XmlMouseMove_R (qwidget_t *self, int mouse_x, int mouse_y )
{
	qwidget_t *w;
	int xpos = self->xpos;
	int ypos = self->ypos;

	//mouse is not in this screen
	if ((mouse_x < xpos) || (mouse_x > xpos+self->width.absolute) ||
		(mouse_y < ypos) || (mouse_y > ypos+self->height.absolute)) {
		return false;
	}

	//No cildren and cursor is in bounds make this the focused element
	if (self->children == NULL) {
		if (M_IsFocusable ((qmelement_t *)self)) {
			M_SetFocus(visible_window, self);
			return true;
		} else
			return false;
	}
		
	for (w = self->children; w != NULL; w = w->next){
		if (M_XmlMouseMove_R (w, mouse_x, mouse_y)) return true;
	}

	//None of the children could be selected so try ourselve
	if (M_IsFocusable ((qmelement_t *)self)) {
		M_SetFocus(visible_window, self);
		return true;
	} else
		return false;
}

void M_XmlMouseMove ( int x, int y ) {
	if (visible_window->mouse_capture) {
		//Hand over to the component
		if (visible_window->focused->mtable->Drag)
			(*visible_window->focused->mtable->Drag)(visible_window->focused,x,y);
	} else {
		M_XmlMouseMove_R (visible_window->widget, x, y);
	}
}

// -------------------------------------------------------------------------------------
//
//	Menu drawing code
//
// -------------------------------------------------------------------------------------
void M_DrawPic (int x1, int y1, int x2, int y2, shader_t *sh)
{
	Draw_Pic (x1, y1, x2, y2, sh);
}

void M_DrawCharacter (int cx, int cy, char c)
{
	Draw_Character(cx, cy, c);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (*str));
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, *str);
		str++;
		cx += 8;
	}
}


void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	//Draw_TransPic (x + ((vid.width - 320)>>1), y, pic);
}

/*
================
M_DrawString

draws the str string of length len in the (x1,y1,x2,y2) rectangle 
================
*/
int M_DrawString (int x1, int y1, int x2, int y2, char *str, int len)
{
	// calculate string width and height, center it
	int w = Draw_StringWidth(str, currentState.currentFont);
	int h = Draw_StringHeight(str, currentState.currentFont);
	int x = x1 + (x2 - x1 - w)/2;
	int y = y1 + (y2 - y1 - h)/2;

	Draw_StringFont(x, y, str, currentState.currentFont);
	return x + w;
}

/*
================
M_Draw

called each frame if there's a menu to render
================
*/

void M_Draw (void)
{
	usercmd_t dummy;

	if (m_state == m_none || key_dest != key_menu)
		return;

	if (!m_recursiveDraw)
	{
		scr_copyeverything = 1;

		if (scr_con_current)
		{
			Draw_ConsoleBackground (vid.height);
			S_ExtraUpdate ();
		}
		//PENTA: Fading should be done with a shader now...
		//so the artist can specify how we fade
		//else
		//	Draw_FadeScreen ();

		scr_fullupdate = 0;
	}
	else
	{
		m_recursiveDraw = false;
	}

	// draw current window
	M_DrawVisibleWindow ();

	//Draw a mouse pointer
	M_DrawCharacter(m_mousex-4, m_mousey-8, '+');

	if (m_entersound)
	{
		//S_LocalSound ("misc/menu2.wav");
		m_entersound = false;
	}

	S_ExtraUpdate ();
}

/*
================
M_DrawXml*

handlers taking care of the rendering of each widget available
These functions are called for _each_ frame (as some widget may 
change from frame to frame, depending of what they contain : shaders, 
models etc...)
So the idea here is to avoid recomputing things if possible. 
for ex. : width/height are calculated at loading time 
(FIXME : or only when a widget size changes) 
================
*/


void M_DrawXmlButton (qwidget_t *self, int x, int y)
{
	xmlbuttondata_t *data = self->data;
	if (data->pic) {
		M_DrawPic (x, y, x+self->width.absolute, y+self->height.absolute, data->pic);	
	}
	M_DrawString (x, y, x+self->width.absolute, y+self->height.absolute, data->label, /*sizeof(data->label)*/ strlen(data->label) );
}

void M_DrawXmlImage (qwidget_t *self, int x, int y)
{
	xmlimagedata_t *data = self->data;
	if (!data->pic)
		data->pic = GL_ShaderForName (data->src);
	if (data->pic)
		M_DrawPic (x, y, x+self->width.absolute, y+self->height.absolute, data->pic);
}

void M_DrawXmlLabel (qwidget_t *self, int x, int y)
{
	xmllabeldata_t *data = self->data;
	M_DrawString (x, y, x+self->width.absolute, y+self->height.absolute, data->text, strlen(data->text));
}

void M_DrawXmlBox (qwidget_t *self, int x, int y)
{
	// nothing to do

}

void M_DrawXmlRadio (qwidget_t *self, int x, int y)
{
	xmlradiodata_t *data = self->data;
	qboolean on = data->selected;
	M_DrawString (x, y, x+self->width.absolute, y+self->height.absolute, data->label, strlen(data->label));
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
	xmlsliderdata_t *data = self->data;
	float zeroone = data->cursor / data->range;
	float newx = self->parent->xpos + (self->parent->width.absolute-self->width.absolute)*zeroone;

	//HACK: Initialize the cursor if it was not initialized yet.
	//We have to do this as cvars may not be registred during load time
	if (data->cursor == -1.0f) {
		data->cursor = Cvar_VariableValue(self->id);
	}

	if (data->pic) {
		M_DrawPic (newx, y, newx+self->width.absolute, y+self->height.absolute, data->pic);	
	}
	M_DrawString (newx, y, newx+self->width.absolute, y+self->height.absolute, data->label, /*sizeof(data->label)*/ strlen(data->label) );
	self->xpos = (int)newx;
}

void M_DrawXmlEdit (qwidget_t *self, int x, int y)
{
	int              i;
	xmleditdata_t *data = self->data;
	int              cursor = data->cursor;
	char		label[MAX_MLABEL];

	if ((int)(realtime*4)&1 && self == visible_window->focused) {
		strcpy(label, data->text);
		if (cursor >= strlen(label)) {
			label[cursor+1] = 0;
		}
		label[cursor] = '#';
		M_DrawString (x, y, x+self->width.absolute, y+self->height.absolute, label, data->length );
	} else {
		M_DrawString (x, y, x+self->width.absolute, y+self->height.absolute, data->text, data->length );
	}
}

// almost pasted from menu.c  

void M_DrawXmlCheckBox (qwidget_t *self,int x, int y)
{
	xmlcheckboxdata_t *data = self->data;
	qboolean on;

	//HACK: Initialize the cursor if it was not initialized yet.
	//We have to do this as cvars may not be registred during load time
	if (data->checked == -1) {
		data->checked = Cvar_VariableValue(self->id);
	}
	on = data->checked;

	x = M_DrawString (x, y, x+self->width.absolute, y+self->height.absolute, data->label, strlen(data->label));

	if (on)
		M_Print (x, y, "[x]");
	else
		M_Print (x, y, "[ ]");
}

void M_DrawXmlWindow (qwidget_t *self,int x, int y)
{
	// draw a border ?
}


void M_DrawFocus (qwidget_t *self, int x, int y)
{
	int xs = x+self->width.absolute;
	int ys = y+self->height.absolute;


	//if (focus_shader == NULL)
	//	focus_shader = GL_ShaderForName ("menu/focus");
	if (currentState.focusType == FOCUS_BORDER) {
		Draw_Border (x, y, xs, ys, 5, currentState.focusShader);
	} else {
		M_DrawPic (x, y, x+self->width.absolute, y+self->height.absolute, currentState.focusShader);
	}
}

/* -DC-
   FIXME :  doesn't work at all
   So this function is a little hack to display widget outline
   just for debugging purpose
   -> There is certainly a (better) way to do this
      cause I don't know much about OpenGL (shame on me ^^;)
*/

void M_DrawOutlines (qwidget_t *self, int x, int y)
{
	// qwidget_t *self = ptr;
	int xs = x+self->width.absolute;
	int ys = x+self->height.absolute;
	GLfloat color[4];

	glGetFloatv(GL_CURRENT_COLOR, color);
	glColor4f(1,1,1,1);
	glDisable(GL_TEXTURE_2D);
	glBegin (GL_LINE_STRIP);
		glVertex2f (x, y);
		glVertex2f (xs, y);
		glVertex2f (xs, ys);
		glVertex2f (x, ys);
		glVertex2f (x, y);
	glEnd ();
	glColor4fv(color);
	glEnable(GL_TEXTURE_2D);
}

/* -DC-
   instead of having a function handling recursive drawing of widget
   we could perhaps put this code in widget rendering code directly 
   -> more flexible if some widget need to do stuff in between children
      rendering
   -> but code duplication
 */

void M_DrawXmlNestedWidget (qwidget_t *self)
{
	// all objects within the window tree have to be a widget
	//qwidget_t *widget=ptr;
	qwidget_t *w;
	menudrawstate_t stateStack;
	int	xpos = self->xpos;
	int ypos = self->ypos;

	//save the current drawstate on the stack
	//and set the widget's font as current font
	stateStack = currentState;

	if (self->font)
		currentState.currentFont = self->font;
	if (self->focusType)
		currentState.focusType = self->focusType;
	if (self->focusShader)
		currentState.focusShader = self->focusShader;

	if (self->mtable->Draw){
		self->mtable->Draw (self, xpos, ypos);
	}

	if (self == visible_window->focused){
		M_DrawFocus (self, xpos, ypos);
	}

	if (m_debug.value) { 
		// draw outlines
		M_DrawOutlines (self, xpos, ypos);
	}

	if (self->children == NULL) {
		currentState = stateStack; //restore drawstate
		return;
	}

	for (w = self->children; w != NULL; w = w->next){
		M_DrawXmlNestedWidget (w);
	}

	currentState = stateStack;
}

void M_DrawWindow (qwindow_t *wnd)
{
	int w = vid.width - wnd->widget->width.absolute;
	int h = vid.height - wnd->widget->height.absolute;
	M_DrawXmlNestedWidget (wnd->widget);
}

void M_DrawVisibleWindow ()
{
	M_DrawWindow(visible_window);
}

void M_DrawNamedWindow (const char *name)
{
	qwindow_t *w = windows_stack;
	qwindow_t *oldviz = visible_window;
	while (w){
		if (!strncmp (w->widget->id, name, strlen(w->widget->id)))
			break;
		w = w->next;
	}
	if (!w) {
		Con_Printf ("M_DrawNamedWindow : no such window '%s'\n", name);
	} else {
		visible_window = w;
		M_DrawWindow(w);
	}
	visible_window = oldviz;
}

// -------------------------------------------------------------------------------------
//
//	Menu script file parsing and initialization
//
// -------------------------------------------------------------------------------------


char empty_str[]    = "";
#define EMPTY_STR empty_str

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


int M_ReadXmlPropAsChar (xmlNodePtr node, char *name, int defvalue)
{
	int ret;
	int p;
	xmlChar *value; 
	value = xmlGetProp (node, name);
	if (value){
		ret = (int)value[0];
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
		ret[size-1] = 0;
		xmlFree (value);
	} else
		ret=EMPTY_STR;

	return ret;
}

char *M_ReadXmlPropAsRefString (xmlNodePtr node, char *name)
{
	xmlChar *value;
	char *ret;
	value = xmlGetProp (node, name);
	if (value){
		ret = M_CreateRefString (value, strlen(value));
		xmlFree (value);
	} else
		ret=EMPTY_STR;

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
	for (w = root->children; w != NULL; w = w->next) {
		M_CalculateWidgetDim (w);
	}
}

/*
================
M_CalculateWidgetPos

evaluate widgets x and y positions
The goal is to have all size calculations in one place
================
*/
void M_CalculateWidgetPos (qwidget_t *self, int xpos, int ypos)
{
	qwidget_t *w;

	//Store our x and y position on the screen
	self->xpos = xpos;
	self->ypos = ypos;

	if (self->children == NULL) {
		return;
	}

	// vertical orientation
	if (self->orient) {
		int wwidth;
		ypos += self->yoffset;
		switch (self->pack){
		case p_start:
			for (w = self->children; w != NULL; w = w->next){
				M_CalculateWidgetPos (w, xpos, ypos);
				ypos += w->height.absolute;
			}
			break;
		case p_center:
			for (w = self->children; w != NULL; w = w->next){
				wwidth = self->width.absolute - w->width.absolute;
				M_CalculateWidgetPos (w, xpos+wwidth/2, ypos);
				ypos += w->height.absolute;
			}
			break;
		case p_end:
			for (w = self->children; w != NULL; w = w->next){
				wwidth = self->width.absolute - w->width.absolute;
				M_CalculateWidgetPos (w, xpos+wwidth, ypos);
				ypos += w->height.absolute;
			}
			break;
		}
	}
	else {
		int wheight;
		xpos += self->xoffset;
		switch (self->pack){
		case p_start:
			for (w = self->children; w != NULL; w = w->next){
				M_CalculateWidgetPos (w, xpos, ypos);
				xpos += w->width.absolute;
			}
			break;
		case p_center:
			for (w = self->children; w != NULL; w = w->next){
				wheight = - w->height.absolute + self->height.absolute;
				M_CalculateWidgetPos (w, xpos, ypos+wheight/2);
				xpos += w->width.absolute;
			}
			break;
		case p_end:
			for (w = self->children; w != NULL; w = w->next){
				wheight = - w->height.absolute + self->height.absolute;
				M_CalculateWidgetPos (w, xpos, ypos+wheight);
				xpos += w->width.absolute;
			}
			break;
		}
	}
}

void M_PrepareQWindow (qwidget_t *widget)
{
	char command[255];
	int w,h;

	// register alias for quake compat
	strcpy (command, "alias menu_\0"); 
	strcat (command, widget->id);           // no more that 32 char in widget->id
	strcat (command, " \"openwindow ");     //
	strcat (command, widget->id);           // no more that 32 char in widget->id
	strcat (command, "\"\n");               // so it should fit in command

	Cbuf_AddText (command);	

	// we have a fully loaded window here : 
	// let's initialize children dependent data
	M_CalculateWidgetDim (widget);

	//positions
	w = vid.width - widget->width.absolute;
	h = vid.height - widget->height.absolute;
	M_CalculateWidgetPos (widget, w/2, h/2);


	/*
	// set root on all sub-widget
	w = widget;
	while (w){
		w->root = widget;
		w = M_NextQMElement (w);
	}
	*/
}


// -------------------------------------------------------------------------------------


/*
================
M_LoadXml*

Load a Xml Element definition from the provided libxml tree pointer 
================
*/


void M_LoadXmlBox (qwidget_t *self, xmlNodePtr node)
{

	/* 
	 * --- supported attributes :
	 * --- unsupported :
	 */


}

void M_LoadXmlLabel (qwidget_t *self, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   value      =  string : label text

	 * --- unsupported :

	   disabled   =  bool
	   control    =  string : xul element id 
	   accesskey  =  string 

	 */
	// qwidget_t *self = ptr;
	xmllabeldata_t *data = self->data;	
	M_ReadXmlPropAsString (node, "value", data->text, sizeof (data->text));	
}

void M_LoadXmlImage (qwidget_t *self, xmlNodePtr node)
{

	/* 
	 * --- supported attributes :
	 
	   src        =  URL 
	 
	 * --- unsupported :
	 
	   onerror    =  string
	   onload     =  string
	   validate   =  {always|never} : load image from cache

	 */
	// qwidget_t *self = ptr;
	xmlimagedata_t *data = self->data;
	M_ReadXmlPropAsString (node, "src", data->src, sizeof (data->src));
	//GL_LoadPic (data->src,&(data->pic));
}


void M_LoadXmlButton (qwidget_t *self, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   command    =  string
	   disabled   =  bool
	   label      =  string
	   accesskey  =  string : shortcut key
	   image      =  URL
	   
	 * --- unsupported :
	   
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
	// qwidget_t *self = ptr;
	char buffer[MAX_QPATH];
	xmlbuttondata_t *data = self->data;
	int temp;

	M_ReadXmlPropAsString (node, "command", data->command, sizeof(data->command));
	M_ReadXmlPropAsString (node, "label", data->label, sizeof(data->label));

	buffer[0] = 0;
	M_ReadXmlPropAsString (node, "image", buffer, sizeof(buffer));
	
	if (buffer[0])
		data->pic = GL_ShaderForName(buffer);
	else
		data->pic = NULL;

	self->accesskey = M_ReadXmlPropAsChar (node, "accesskey", 0);
	temp = M_CompareXmlProp (node, "disabled", xmlbool, 2);
	if (temp != -1)
		self->enabled = !temp;
	
}

void M_LoadXmlCheckBox (qwidget_t *self, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   disabled   =  bool
	   label      =  string
	   accesskey  =  string : shortcut key
	   
	 * --- unsupported :

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
	// qwidget_t *self = ptr;
	xmlcheckboxdata_t *data = self->data;
	int temp;

	M_ReadXmlPropAsString (node, "label", data->label, sizeof (data->label));
	self->accesskey = M_ReadXmlPropAsChar (node, "accesskey", 0);
	temp = M_CompareXmlProp (node, "disabled", xmlbool, 2);
	if (temp != -1)
		self->enabled = !temp;
	data->checked = -1;	
}

void M_LoadXmlRadio (qwidget_t *self, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   disabled   =  bool
	   label      =  string
	   selected   =  bool
	   accesskey  =  string : shortcut key

	 * --- unsupported :

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
	// qwidget_t *self = ptr;
	xmlradiodata_t *data = self->data;
	int temp;

	M_ReadXmlPropAsString (node, "label", data->label, sizeof (data->label));
	self->accesskey = M_ReadXmlPropAsChar (node, "accesskey", 0);

	temp = M_CompareXmlProp (node, "disabled", xmlbool, 2);
	if (temp != -1)
		self->enabled = !temp;

	temp = M_CompareXmlProp (node, "selected", xmlbool, 2);
	if (temp != -1)
		data->selected = temp;

}

void M_LoadXmlRadioGroup (qwidget_t *self, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   disabled   =  bool

	 * --- unsupported :

	   value      =  string ? : user attribute
	*/
	// qwidget_t *self = ptr;
	xmlradiogroupdata_t *data = self->data;
	int temp;
	temp = M_CompareXmlProp (node, "disabled", xmlbool, 2);
	if (temp != -1)
		self->enabled = !temp;
}

void M_LoadXmlSlider (qwidget_t *self, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   curpos     =  [0,maxpos] : cursor position
	   maxpos     =  int : maximum cursor value

	 * --- unsupported :
	   pageincrement =  int 	 
   	   increment  =  int : 

	*/
	// qwidget_t *self = ptr;
	char buffer[MAX_QPATH];
	xmlsliderdata_t *data = self->data;

	data->range = M_ReadXmlPropAsFloat (node, "maxpos", 1.0f);
	// FIXME : shoud property override cvar value ?
	data->cursor = M_ReadXmlPropAsFloat (node, "curpos", -1.0f);//Cvar_VariableValue doesn't work here as the menus are loaded before some cvars are registred

	M_ReadXmlPropAsString (node, "label", data->label, sizeof(data->label));

	buffer[0] = 0;
	M_ReadXmlPropAsString (node, "image", buffer, sizeof(buffer));
	
	if (buffer[0])
		data->pic = GL_ShaderForName(buffer);
	else
		data->pic = NULL;
}

void M_LoadXmlEdit (qwidget_t *self, xmlNodePtr node)
{
	/* 
	 * --- supported attributes :

	   text       =  inital text
	   length     =  length of the textfield

	*/
	// qwidget_t *self = ptr;
	xmleditdata_t *data = self->data;
	cvar_t *var;

	data->length = M_ReadXmlPropAsInt (node, "length", 20);
	
	var = Cvar_FindVar(self->id);
	if (!var) {
		M_ReadXmlPropAsString (node, "text", data->text, sizeof (data->text));	
	//	Con_Printf("var not found %s\n",self->id);
	} else {
		strcpy(data->text, var->string);
	}
	data->cursor = strlen(data->text);
}

void M_LoadXmlMenu (qwidget_t *self, xmlNodePtr node)
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

void M_LoadXmlWindow (qwidget_t *self, xmlNodePtr node)
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
	// qwidget_t *self = ptr;
	qwidget_t *w; 
	qwindow_t *data = self->data;
	data->widget = self;

	// check window size
	if (self->width.ratio != -1)
		self->width.absolute = self->width.ratio * vid.width;
	if (self->height.ratio != -1)
		self->height.absolute = self->height.ratio * vid.height;
	
	if (self->width.absolute == -1)
		self->width.absolute = vid.width;		
	if (self->height.absolute == -1)
		self->height.absolute = vid.height;		
	
	// init some data
	data->string_table.str = "";
	data->keycount = 0;
	data->focused = 0;
	data->stack  = NULL;

	// add newly created window to the list
	M_AddWindowInList (data);
}

void M_LoadXmlElement (qwidget_t *self, xmlNodePtr root)
{
	/*
		These are attributes shared by all widgets.

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

	int temp;
	char buffer[MAX_QPATH];
	
	// read general attributes from the file 
	
	M_ReadXmlDim (root, "width", &self->width);
	M_ReadXmlDim (root, "height", &self->height);
	self->debug = (M_ReadXmlPropAsInt (root, "debug", 0) != 0);

	self->id = M_ReadXmlPropAsRefString (root, "id");
	self->name = M_ReadXmlPropAsRefString (root, "name");

	//check for a font
	buffer[0] = 0;
	self->font = NULL;
	M_ReadXmlPropAsString (root, "font", buffer, sizeof(buffer));
	if (buffer[0])
		self->font = Draw_FontForName(buffer);

	//check for a focus shader
	buffer[0] = 0;
	self->focusShader = NULL;
	M_ReadXmlPropAsString (root, "focusshader", buffer, sizeof(buffer));
	if (buffer[0])
		self->focusShader = GL_ShaderForName(buffer);

	//check for a focus type
	buffer[0] = 0;
	self->focusType = FOCUS_NOTSPECIFIED;
	M_ReadXmlPropAsString (root, "focustype", buffer, sizeof(buffer));
	if (!strcmp(buffer, "border")) {
		self->focusType = FOCUS_BORDER;
	} else if (!strcmp(buffer,"rectangle")) {
		self->focusType = FOCUS_OVER;
	}
		
	temp = M_CompareXmlProp (root, "orient", xmlorient, 2);
	if (temp != -1)
		self->orient = temp;
	
	temp = M_CompareXmlProp (root, "align", xmlalign, 5);
	if (temp != -1)
		self->align = temp;
	temp = M_CompareXmlProp (root, "pack", xmlpack, 3);
	if (temp != -1)
		self->pack = temp;

	// then init the private data

	self->mtable->Load (self, root);
}

/*
	This actually traverses the xml parse tree and creates windows, widgets, ... from it
*/
qwidget_t *M_LoadXmlElementTree (xmlNodePtr root)
{
	xmlNodePtr     node;
	qwidget_t     *ret,*sub;
	xmlhandler_t *handler;

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

	ret = M_AllocateMem (handler->templatesize, handler->template);	

	if (handler->datasize)
		ret->data = M_AllocateMem (handler->datasize, NULL);
	ret->tag = handler->tag;

	// new window ?
	if (ret->tag == window_tag)
		loading = ret->data;

	M_LoadXmlElement (ret, root);

	/* load all the subnodes */
	node = root->xmlChildrenNode;
	while ( node != NULL ){
		// comment nodes have node->name == NULL  
		if (!xmlIsBlankNode (node) && (node->type != XML_COMMENT_NODE)){
			sub = M_LoadXmlElementTree (node);
			if (sub)
				M_InsertXmlWidget (ret, sub);
		}
		node = node->next;
	}
	return ret;
}



void M_XMLError (void *ctx, const char *fmt, ...) {
    va_list		argptr;
    char		msg[512];

	//Creepy!
    va_start (argptr,fmt);
#ifdef _MSC_VER

    vsprintf(msg, fmt, argptr);

#else

    vsnprintf (msg,512,fmt,argptr);
#endif

    va_end (argptr);

	Con_Printf(msg);
}

void M_XMLFatalError (void *ctx, const char *fmt, ...) {
    va_list		argptr;
    char		msg[512];

	//Creepy!
    va_start (argptr,fmt);
#ifdef _MSC_VER

    vsprintf(msg, fmt, argptr);

#else

    vsnprintf (msg,512,fmt,argptr);
#endif

    va_end (argptr);

	Con_Printf("fatal error: %s",msg);
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

	//Setup error funciton pointers
	xmlInitParser();
	xmlSetGenericErrorFunc(NULL, M_XMLError);

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
		Con_Printf ("Warning : %s xmlParseMemory returned NULL\n",filename);
		return;
	}
	node = xmlDocGetRootElement (doc);
	if (node == NULL) {
		Con_Printf ("Warning : %s empty document\n",filename);
		xmlFreeDoc (doc);
		return;
	}
	
	if (!xmlIsBlankNode (node) && (node->name)){
		if (!xmlStrcmp (node->name, (const xmlChar *)"package")) {
			//parse all the menus in this package
			node = node->xmlChildrenNode;
			while ( node != NULL ){
				// comment nodes have node->name == NULL  
				if (!xmlIsBlankNode (node) && (node->type != XML_COMMENT_NODE)){
					qwidget_t *qwidget = M_LoadXmlElementTree (node);
					if (qwidget)
						M_PrepareQWindow (qwidget);
				}
				node = node->next;
			}
		}
		else {
			Con_Printf ("Warning : %s document root isn't a package\n",filename);
		}
	}

	xmlFreeDoc (doc);

#endif
	Con_DPrintf ("XML : loading %s document ended \n",filename);		
}

// -------------------------------------------------------------------------------------
