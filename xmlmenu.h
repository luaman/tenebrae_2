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

#ifndef _XMLMENU_H
#define _XMLMENU_H

#include "libxml/parser.h"


typedef struct xmldim_s 
{
	float            ratio;
	int              absolute;
} xmldim_t;

typedef enum {a_start=0,a_center=1,a_end=2,a_baseline=3,a_stretch=4} xmlalign_t;
typedef enum {p_start=0,p_center=1,p_end=2} xmlpack_t;

typedef struct qwidget_s 
{
	int               debug:1;
	int               focusable:1;
	int               orient:1;
	xmlalign_t        align;
	xmlpack_t         pack;
	char              name[32];
	char              id[32];
	int               xoffset;
	int               yoffset;
	xmldim_t          width;
	xmldim_t          height;
	int               num_children;	
	char             *tag;
	struct qwidget_s *parent;     // bounding widget
	//struct qwidget_s *root;       // root widget
	struct qwidget_s *previous;   // next sibling
	struct qwidget_s *next;       // next sibling
	struct qwidget_s *children;   // bounded widget list
	struct qwidget_s *rchildren;  // bounded widget list (reverse order)
	// functions 
	void (*Load) (struct qwidget_s *self,xmlNodePtr node);
	void (*Draw) (struct qwidget_s *self, int x, int y);
	void (*Focus) (struct qwidget_s *self);
	qboolean (*HandleKey) (struct qwidget_s *self, int key);
	void (*onMouseOver) (struct qwidget_s *self);
	void (*onMouseDown) (struct qwidget_s *self);
	void (*onMouseUp) (struct qwidget_s *self);
	void *data;
} qwidget_t;


void M_OpenWindow_f (void);
void M_CloseWindow_f (void);
void M_DrawVisibleWindow (void);

void M_LoadXmlBox (qwidget_t *widget, xmlNodePtr node);
void M_LoadXmlVBox (qwidget_t *widget, xmlNodePtr node);
void M_LoadXmlLabel (qwidget_t *widget, xmlNodePtr node);
void M_LoadXmlImage (qwidget_t *widget, xmlNodePtr node);
void M_LoadXmlButton (qwidget_t *widget, xmlNodePtr node);
void M_LoadXmlCheckBox (qwidget_t *widget, xmlNodePtr node);
void M_LoadXmlMenu (qwidget_t *widget, xmlNodePtr node);
void M_LoadXmlSlider (qwidget_t *widget, xmlNodePtr node);
void M_LoadXmlRadio (qwidget_t *widget, xmlNodePtr node);
void M_LoadXmlRadioGroup (qwidget_t *widget, xmlNodePtr node);
void M_LoadXmlWindow (qwidget_t *widget, xmlNodePtr node);

void M_LoadXmlWindowFile (const char *filename);

void M_XmlButtonCommand (qwidget_t *self);


qwidget_t *M_NextXmlElement (qwidget_t *w);
qwidget_t *M_PreviousXmlElement (qwidget_t *w);

void M_CycleFocus (qwidget_t *(*next_f)(qwidget_t *));

#define M_CycleFocusNext() M_CycleFocus (M_NextXmlElement)
#define M_CycleFocusPrevious() M_CycleFocus (M_PreviousXmlElement)


qboolean M_XmlBoxKey (qwidget_t *self, int k);
qboolean M_XmlCheckBoxKey (qwidget_t *self, int k);
qboolean M_XmlButtonKey (qwidget_t *self, int k);
qboolean M_XmlRadioKey (qwidget_t *self, int k);
qboolean M_XmlRadioGroupKey (qwidget_t *self, int k);
qboolean M_XmlSliderKey (qwidget_t *self, int k);
qboolean M_XmlWindowKey (qwidget_t *self, int k);
qboolean M_XmlMenuKey (qwidget_t *self, int k);
qboolean M_XmlElementKey (qwidget_t *self, int k);

void M_DrawXmlButton (qwidget_t *self, int x, int y);
void M_DrawXmlImage (qwidget_t *self, int x, int y);
void M_DrawXmlLabel (qwidget_t *self, int x, int y);
void M_DrawXmlBox (qwidget_t *self, int x, int y);
void M_DrawXmlSlider (qwidget_t *self, int x, int y);
void M_DrawXmlCheckBox (qwidget_t *self,int x, int y);
void M_DrawXmlRadio (qwidget_t *self,int x, int y);
void M_DrawXmlRadioGroup (qwidget_t *self,int x, int y);
void M_DrawXmlWindow (qwidget_t *self,int x, int y);

typedef struct xmlhandlers_s {
	const xmlChar *tag;
	qwidget_t template;
	int datasize;
} xmlhandler_t;

#endif
