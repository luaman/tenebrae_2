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


/*  -DC- 
   right now this thing only contains a simple pointer to a copy of the string found in the xml file. 
   The script language is very simple : no flow control, only function call and assignement.
   The engine could perhaps in the future crudely compile the scripts in the bytecode form, hence reducing the performance hit of script interpretation (like just-in-time compilation).
*/

typedef struct script_s
{
	char *str;
} script_t;


#define MAX_WIDGETNAME    32

typedef struct qwidget_s qwidget_t;

typedef struct qmtable_s
{
	void (*Load) (qwidget_t *self, xmlNodePtr node);
	void (*Draw) (qwidget_t *self, int x, int y);
	void (*Focus) (qwidget_t *self);
	qboolean (*HandleKey) (qwidget_t *self, int key);	
	//int  type;
} qmtable_t;


/*
  Tenebrae Quake Menu object ( loosely xml element like )
  
typedef struct qmelement_s
{
	qmtable_t      *mtable;
	char              name[MAX_WIDGETNAME];
	char              id[MAX_WIDGETNAME];
	char             *tag;
	int               num_children;	
	struct qmelement_s *parent;     // bounding element
	struct qmelement_s *previous;   // previous sibling
	struct qmelement_s *next;       // next sibling
	struct qmelement_s *children;   // bounded element list
	struct qmelement_s *rchildren;  // idem (reverse order)
	int               debug:1;
	int               enabled:1;
	int               focusable:1; // should always be 0
	int               orient:1;
} qmelement_t;

*/

/*
  Tenebrae Quake Widget -> visible menu element
*/


typedef qwidget_t qmelement_t;
struct qwidget_s
{
	qmtable_t        *mtable;
	char             *name;       // stored in a string table
	char             *id;         // idem
	char             *tag;
	int               num_children;	
	qmelement_t      *parent;     // bounding element
	qmelement_t      *previous;   // previous sibling
	qmelement_t      *next;       // next sibling
	qmelement_t      *children;   // bounded element list
	qmelement_t      *rchildren;  // idem (reverse order)
	int               debug:1;
	int               enabled:1;
	int               focusable:1;
	int               orient:1;
	xmlalign_t        align;
	xmlpack_t         pack;
	int               xoffset;
	int               yoffset;
	xmldim_t          width;
	xmldim_t          height;
	int               accesskey;
	script_t         *onCommand;
	script_t         *onMouseOver;
	script_t         *onMouseDown;
	script_t         *onMouseUp;
	void             *data;
};

/*
  Tenebrae Quake template widget   
*/

typedef struct qwtemplate_s
{
	qwidget_t  *template;
	qwidget_t **children;
} qwtemplate_t;

void M_OpenWindow_f (void);
void M_CloseWindow_f (void);
void M_DrawVisibleWindow (void);
void M_DrawNamedWindow (const char *name);

void M_LoadXmlBox (qwidget_t *ptr, xmlNodePtr node);
void M_LoadXmlVBox (qwidget_t *ptr, xmlNodePtr node);
void M_LoadXmlLabel (qwidget_t *ptr, xmlNodePtr node);
void M_LoadXmlImage (qwidget_t *ptr, xmlNodePtr node);
void M_LoadXmlButton (qwidget_t *ptr, xmlNodePtr node);
void M_LoadXmlCheckBox (qwidget_t *ptr, xmlNodePtr node);
void M_LoadXmlMenu (qwidget_t *ptr, xmlNodePtr node);
void M_LoadXmlSlider (qwidget_t *ptr, xmlNodePtr node);
void M_LoadXmlEdit (qwidget_t *ptr, xmlNodePtr node);
void M_LoadXmlRadio (qwidget_t *ptr, xmlNodePtr node);
void M_LoadXmlRadioGroup (qwidget_t *ptr, xmlNodePtr node);
void M_LoadXmlWindow (qwidget_t *ptr, xmlNodePtr node);

void M_LoadXmlWindowFile (const char *filename);

void M_XmlButtonCommand (qwidget_t *ptr);


qmelement_t *M_NextQMElement (qmelement_t *w);
qmelement_t *M_PreviousQMElement (qmelement_t *w);

void M_CycleFocus (qmelement_t *(*next_f)(qmelement_t *));

#define M_CycleFocusNext() M_CycleFocus (M_NextQMElement)
#define M_CycleFocusPrevious() M_CycleFocus (M_PreviousQMElement)


qboolean M_XmlBoxKey (qwidget_t *ptr, int k);
qboolean M_XmlCheckBoxKey (qwidget_t *ptr, int k);
qboolean M_XmlButtonKey (qwidget_t *ptr, int k);
qboolean M_XmlRadioKey (qwidget_t *ptr, int k);
qboolean M_XmlRadioGroupKey (qwidget_t *ptr, int k);
qboolean M_XmlSliderKey (qwidget_t *ptr, int k);
qboolean M_XmlEditKey (qwidget_t *ptr, int k);
qboolean M_XmlWindowKey (qwidget_t *ptr, int k);
qboolean M_XmlMenuKey (qwidget_t *ptr, int k);
qboolean M_XmlElementKey (qwidget_t *ptr, int k);

void M_DrawXmlButton (qwidget_t *ptr, int x, int y);
void M_DrawXmlImage (qwidget_t *ptr, int x, int y);
void M_DrawXmlLabel (qwidget_t *ptr, int x, int y);
void M_DrawXmlBox (qwidget_t *ptr, int x, int y);
void M_DrawXmlSlider (qwidget_t *ptr, int x, int y);
void M_DrawXmlEdit (qwidget_t *ptr, int x, int y);
void M_DrawXmlCheckBox (qwidget_t *ptr,int x, int y);
void M_DrawXmlRadio (qwidget_t *ptr,int x, int y);
void M_DrawXmlRadioGroup (qwidget_t *ptr,int x, int y);
void M_DrawXmlWindow (qwidget_t *ptr,int x, int y);


#endif
