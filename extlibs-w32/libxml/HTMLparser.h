/*
 * HTMLparser.h : interface for an HTML 4.0 non-verifying parser
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __HTML_PARSER_H__
#define __HTML_PARSER_H__
#include <libxml/parser.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Most of the back-end structures from XML and HTML are shared.
 */
typedef xmlParserCtxt htmlParserCtxt;
typedef xmlParserCtxtPtr htmlParserCtxtPtr;
typedef xmlParserNodeInfo htmlParserNodeInfo;
typedef xmlSAXHandler htmlSAXHandler;
typedef xmlSAXHandlerPtr htmlSAXHandlerPtr;
typedef xmlParserInput htmlParserInput;
typedef xmlParserInputPtr htmlParserInputPtr;
typedef xmlDocPtr htmlDocPtr;
typedef xmlNodePtr htmlNodePtr;

/*
 * Internal description of an HTML element, representing HTML 4.01
 * and XHTML 1.0 (which share the same structure).
 */
typedef struct _htmlElemDesc htmlElemDesc;
typedef htmlElemDesc *htmlElemDescPtr;
struct _htmlElemDesc {
    const char *name;	/* The tag name */
    char startTag;      /* Whether the start tag can be implied */
    char endTag;        /* Whether the end tag can be implied */
    char saveEndTag;    /* Whether the end tag should be saved */
    char empty;         /* Is this an empty element ? */
    char depr;          /* Is this a deprecated element ? */
    char dtd;           /* 1: only in Loose DTD, 2: only Frameset one */
    char isinline;      /* is this a block 0 or inline 1 element */
    const char *desc;   /* the description */

/* NRK Jan.2003
 * New fields encapsulating HTML structure
 *
 * Bugs:
 *	This is a very limited representation.  It fails to tell us when
 *	an element *requires* subelements (we only have whether they're
 *	allowed or not), and it doesn't tell us where CDATA and PCDATA
 *	are allowed.  Some element relationships are not fully represented:
 *	these are flagged with the word MODIFIER
 */
    const char** subelts;		/* allowed sub-elements of this element */
    const char* defaultsubelt;	/* subelement for suggested auto-repair
					   if necessary or NULL */
    const char** attrs_opt;		/* Optional Attributes */
    const char** attrs_depr;		/* Additional deprecated attributes */
    const char** attrs_req;		/* Required attributes */
};

/*
 * Internal description of an HTML entity.
 */
typedef struct _htmlEntityDesc htmlEntityDesc;
typedef htmlEntityDesc *htmlEntityDescPtr;
struct _htmlEntityDesc {
    unsigned int value;	/* the UNICODE value for the character */
    const char *name;	/* The entity name */
    const char *desc;   /* the description */
};

/*
 * There is only few public functions.
 */
const htmlElemDesc * 	htmlTagLookup	(const xmlChar *tag);
const htmlEntityDesc * 	htmlEntityLookup(const xmlChar *name);
const htmlEntityDesc * 	htmlEntityValueLookup(unsigned int value);

int			htmlIsAutoClosed(htmlDocPtr doc,
					 htmlNodePtr elem);
int			htmlAutoCloseTag(htmlDocPtr doc,
					 const xmlChar *name,
					 htmlNodePtr elem);
const htmlEntityDesc * 	htmlParseEntityRef(htmlParserCtxtPtr ctxt,
					 const xmlChar **str);
int			htmlParseCharRef(htmlParserCtxtPtr ctxt);
void			htmlParseElement(htmlParserCtxtPtr ctxt);

htmlParserCtxtPtr	htmlCreateMemoryParserCtxt(const char *buffer,
						   int size);

int			htmlParseDocument(htmlParserCtxtPtr ctxt);
htmlDocPtr		htmlSAXParseDoc	(xmlChar *cur,
					 const char *encoding,
					 htmlSAXHandlerPtr sax,
					 void *userData);
htmlDocPtr		htmlParseDoc	(xmlChar *cur,
					 const char *encoding);
htmlDocPtr		htmlSAXParseFile(const char *filename,
					 const char *encoding,
					 htmlSAXHandlerPtr sax,
					 void *userData);
htmlDocPtr		htmlParseFile	(const char *filename,
					 const char *encoding);
int			UTF8ToHtml	(unsigned char *out,
					 int *outlen,
					 const unsigned char *in,
					 int *inlen);
int			htmlEncodeEntities(unsigned char *out,
					 int *outlen,
					 const unsigned char *in,
					 int *inlen, int quoteChar);
int			htmlIsScriptAttribute(const xmlChar *name);
int			htmlHandleOmittedElem(int val);

/**
 * Interfaces for the Push mode.
 */
void			htmlFreeParserCtxt	(htmlParserCtxtPtr ctxt);
htmlParserCtxtPtr	htmlCreatePushParserCtxt(htmlSAXHandlerPtr sax,
						 void *user_data,
						 const char *chunk,
						 int size,
						 const char *filename,
						 xmlCharEncoding enc);
int			htmlParseChunk		(htmlParserCtxtPtr ctxt,
						 const char *chunk,
						 int size,
						 int terminate);

/* NRK/Jan2003: further knowledge of HTML structure
 */
typedef enum {
  HTML_NA = 0 ,		/* something we don't check at all */
  HTML_INVALID = 0x1 ,
  HTML_DEPRECATED = 0x2 ,
  HTML_VALID = 0x4 ,
  HTML_REQUIRED = 0xc /* VALID bit set so ( & HTML_VALID ) is TRUE */
} htmlStatus ;

/* Using htmlElemDesc rather than name here, to emphasise the fact
   that otherwise there's a lookup overhead
*/
htmlStatus htmlAttrAllowed(const htmlElemDesc*, const xmlChar*, int) ;
int htmlElementAllowedHere(const htmlElemDesc*, const xmlChar*) ;
htmlStatus htmlElementStatusHere(const htmlElemDesc*, const htmlElemDesc*) ;
htmlStatus htmlNodeStatus(const htmlNodePtr, int) ;
/**
 * htmlDefaultSubelement:
 * @elt: HTML element
 *
 * Returns the default subelement for this element
 */
#define htmlDefaultSubelement(elt) elt->defaultsubelt
/**
 * htmlElementAllowedHereDesc:
 * @parent: HTML parent element
 * @elt: HTML element
 *
 * Checks whether an HTML element description may be a
 * direct child of the specified element.
 *
 * Returns 1 if allowed; 0 otherwise.
 */
#define htmlElementAllowedHereDesc(parent,elt) \
	htmlElementAllowedHere((parent), (elt)->name)
/**
 * htmlRequiredAttrs:
 * @elt: HTML element
 *
 * Returns the attributes required for the specified element.
 */
#define htmlRequiredAttrs(elt) (elt)->attrs_req


#ifdef __cplusplus
}
#endif

#endif /* __HTML_PARSER_H__ */
