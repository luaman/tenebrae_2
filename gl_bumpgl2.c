/*
Copyright (C) 2001-2002 Charles Hollemeersch
GLSlang version (C) 2003 Jarno Paananen

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

PENTA: the whole file is freakin penta...

Same as gl_bumpmap.c but using the GL Shading Language
These routines require 6 texture units and GL Shading Language extensions

All lights require 1 pass:
1 diffuse + specular with optional light filter

*/

#include "quakedef.h"

#ifndef GL_ATI_pn_triangles
// PN_triangles_ATI
#define GL_PN_TRIANGLES_ATI                       0x87F0
#define GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI 0x87F1
#define GL_PN_TRIANGLES_POINT_MODE_ATI            0x87F2
#define GL_PN_TRIANGLES_NORMAL_MODE_ATI           0x87F3
#define GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI     0x87F4
#define GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI     0x87F5
#define GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI      0x87F6
#define GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI    0x87F7
#define GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI 0x87F8

typedef void (APIENTRY *PFNGLPNTRIANGLESIATIPROC)(GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLPNTRIANGLESFATIPROC)(GLenum pname, GLfloat param);
#endif

// actually in gl_bumpradeon (duh...)
extern PFNGLPNTRIANGLESIATIPROC qglPNTrianglesiATI;
extern PFNGLPNTRIANGLESFATIPROC qglPNTrianglesfATI;

extern PFNGLSTENCILOPSEPARATEATIPROC qglStencilOpSeparateATI;
extern PFNGLSTENCILFUNCSEPARATEATIPROC qglStencilFuncSeparateATI;

#if  !defined(GL_ARB_vertex_shader) && !defined(GL_ARB_fragment_shader) && !defined(GL_ARB_shader_objects)

typedef int GLhandleARB;
typedef char GLcharARB;

#define GL_PROGRAM_OBJECT_ARB				0x8B40
#define GL_OBJECT_TYPE_ARB                  0x8B4E
#define GL_OBJECT_SUBTYPE_ARB               0x8B4F

#define GL_SHADER_OBJECT_ARB				0x8B48
#define GL_FLOAT_VEC2_ARB                   0x8B50
#define GL_FLOAT_VEC3_ARB                   0x8B51   
#define GL_FLOAT_VEC4_ARB                   0x8B52   
#define GL_INT_VEC2_ARB                     0x8B53   
#define GL_INT_VEC3_ARB                     0x8B54   
#define GL_INT_VEC4_ARB                     0x8B55   
#define GL_BOOL_ARB                         0x8B56   
#define GL_BOOL_VEC2_ARB                    0x8B57   
#define GL_BOOL_VEC3_ARB                    0x8B58   
#define GL_BOOL_VEC4_ARB                    0x8B59   
#define GL_FLOAT_MAT2_ARB                   0x8B5A   
#define GL_FLOAT_MAT3_ARB                   0x8B5B   
#define GL_FLOAT_MAT4_ARB                   0x8B5C 

#define GL_VERTEX_SHADER_ARB		        	 0x8B31  
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB     0x8B4A
#define GL_MAX_VERTEX_ATTRIBS_ARB                0x8869
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB           0x8872
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB    0x8B4C 
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB  0x8B4D
#define GL_MAX_TEXTURE_COORDS_ARB                0x8871

#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB       0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB          0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB        0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB          0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB    0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB             0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB       0x8645

#define GL_FRAGMENT_SHADER_ARB                   0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB   0x8B49

#define GL_MAX_VARYING_FLOATS_ARB			     0x8B4B

#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB         0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB           0x8643

#define GL_OBJECT_DELETE_STATUS_ARB              0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB             0x8B81
#define GL_OBJECT_LINK_STATUS_ARB                0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB            0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB            0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB           0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB            0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB  0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB       0x8B88
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB    0x8B8A
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB              0x8B89




//  *************************************************
//     Rest of these support the old GL2 or VERTEX_ARRAY code
//  ************************************************

// Keep for Vertex Array code
#define GL_VERTEX_ARRAY_OBJECT_GL2			0x40004
#define GL_VERTEX_ARRAY_FORMAT_OBJECT_GL2	0x40008
#define GL_PAD_ARRAY_GL2				    0x80000
#define GL_ALL_INDEX_ARRAY_GL2				0x80001
#define GL_TEXTURE_COORD0_ARRAY_GL2			0x80002
#define GL_TEXTURE_COORD1_ARRAY_GL2			0x80003
#define GL_TEXTURE_COORD2_ARRAY_GL2			0x80004
#define GL_TEXTURE_COORD3_ARRAY_GL2			0x80005
#define GL_TEXTURE_COORD4_ARRAY_GL2			0x80006
#define GL_TEXTURE_COORD5_ARRAY_GL2			0x80007
#define GL_TEXTURE_COORD6_ARRAY_GL2			0x80008
#define GL_TEXTURE_COORD7_ARRAY_GL2			0x80009
#define GL_USER_ATTRIBUTE_ARRAY0_GL2		0x8000A
#define GL_USER_ATTRIBUTE_ARRAY1_GL2		0x8000B
#define GL_USER_ATTRIBUTE_ARRAY2_GL2		0x8000C
#define GL_USER_ATTRIBUTE_ARRAY3_GL2		0x8000D
#define GL_USER_ATTRIBUTE_ARRAY4_GL2		0x8000E
#define GL_USER_ATTRIBUTE_ARRAY5_GL2		0x8000F
#define GL_USER_ATTRIBUTE_ARRAY6_GL2		0x80010
#define GL_USER_ATTRIBUTE_ARRAY7_GL2		0x80011
#define GL_USER_ATTRIBUTE_ARRAY8_GL2		0x80012
#define GL_USER_ATTRIBUTE_ARRAY9_GL2		0x80013
#define GL_USER_ATTRIBUTE_ARRAY10_GL2		0x80014
#define GL_USER_ATTRIBUTE_ARRAY11_GL2		0x80015
#define GL_USER_ATTRIBUTE_ARRAY12_GL2		0x80016
#define GL_USER_ATTRIBUTE_ARRAY13_GL2		0x80017
#define GL_USER_ATTRIBUTE_ARRAY14_GL2		0x80018
#define GL_USER_ATTRIBUTE_ARRAY15_GL2		0x80019


#endif // If shader_object, fragment_shader, and vertex_shader is not defined

#ifndef GL_ARB_shader_objects
#define GL_ARB_shader_objects 1

#ifndef GL_ARB_fragment_shader
#define GL_ARB_fragment_shader 1

#ifndef GL_ARB_vertex_shader
#define GL_ARB_vertex_shader 1

// Taken from ARB_vertex_program
#ifndef GL_ARB_vertex_program
#define GL_ARB_vertex_program 1
typedef void (APIENTRY * PFNGLVERTEXATTRIB4FVARBPROC)	(GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3FVARBPROC)	(GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2FVARBPROC)	(GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1FVARBPROC)	(GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4FARBPROC)		(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3FARBPROC)		(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2FARBPROC)		(GLuint index, GLfloat v0, GLfloat v1);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1FARBPROC)		(GLuint index, GLfloat v0);

typedef void (APIENTRY * PFNGLVERTEXATTRIB4DVARBPROC)	(GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3DVARBPROC)	(GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2DVARBPROC)	(GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1DVARBPROC)	(GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4DARBPROC)		(GLuint index, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3DARBPROC)		(GLuint index, GLdouble v0, GLdouble v1, GLdouble v2);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2DARBPROC)		(GLuint index, GLdouble v0, GLdouble v1);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1DARBPROC)		(GLuint index, GLdouble v0);

typedef void (APIENTRY * PFNGLVERTEXATTRIB4SVARBPROC)	(GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3SVARBPROC)	(GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2SVARBPROC)	(GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1SVARBPROC)	(GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4SARBPROC)		(GLuint index, GLshort v0, GLshort v1, GLshort v2, GLshort v3);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3SARBPROC)		(GLuint index, GLshort v0, GLshort v1, GLshort v2);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2SARBPROC)		(GLuint index, GLshort v0, GLshort v1);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1SARBPROC)		(GLuint index, GLshort v0);

typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUBARBPROC)	(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4UBVARBPROC)	(GLuint index, const GLubyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4USVARBPROC)	(GLuint index, const GLushort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4UIVARBPROC)	(GLuint index, const GLuint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4IVARBPROC)	(GLuint index, const GLint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4BVARBPROC)	(GLuint index, const GLbyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NBVARBPROC)	(GLuint index, const GLbyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NSVARBPROC)	(GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NIVARBPROC)	(GLuint index, const GLint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUBVARBPROC)	(GLuint index, const GLubyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUSVARBPROC)	(GLuint index, const GLushort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUIVARBPROC)	(GLuint index, const GLuint *v);

typedef void (APIENTRY * PFNGLVERTEXATTRIBPOINTERARBPROC)	(GLuint index, GLint size, GLenum type, GLboolean normalize, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLENABLEVERTEXATTRIBARRAYARBPROC)	(GLuint index);
typedef void (APIENTRY * PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)	(GLuint index);

typedef void (APIENTRY * PFNGLGETVERTEXATTRIBPOINTERVARBPROC) (GLuint index, GLenum pname, void **pointer);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBDVARBPROC) (GLuint index, GLenum pname, GLdouble *params);

typedef void (APIENTRY * PFNGLGETVERTEXATTRIBFVARBPROC) (GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBIVARBPROC) (GLuint index, GLenum pname, GLint *params);
#endif
typedef void (APIENTRY * PFNGLBINDATTRIBLOCATIONARBPROC) (GLhandleARB programObj, GLuint index, const GLcharARB *name);
typedef void (APIENTRY * PFNGLBINDARRAYGL2PROC) (GLhandleARB shaderObject, GLenum array, const GLcharARB *name, GLint length);
typedef GLhandleARB (APIENTRY * PFNGLCREATESHADEROBJECTARBPROC) (GLenum shaderType);
typedef GLhandleARB (APIENTRY * PFNGLCREATEPROGRAMOBJECTARBPROC) ();
typedef void (APIENTRY * PFNGLDELETEOBJECTARBPROC) (GLhandleARB obj);

typedef void (APIENTRY * PFNGLSHADERSOURCEARBPROC) (GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length);
typedef void (APIENTRY * PFNGLGETSHADERSOURCEARBPROC) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);

typedef void (APIENTRY * PFNGLCOMPILESHADERARBPROC) (GLhandleARB shaderObj);
typedef void (APIENTRY * PFNGLDETACHOBJECTARBPROC) (GLhandleARB containerObj, GLhandleARB attachedObj);
typedef void (APIENTRY * PFNGLATTACHOBJECTARBPROC) (GLhandleARB containerObject, GLhandleARB obj);
typedef void (APIENTRY * PFNGLUSEPROGRAMOBJECTARBPROC) (GLhandleARB programObj);
typedef void (APIENTRY * PFNGLGETINFOLOGARBPROC) (GLhandleARB obj,GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
typedef void (APIENTRY * PFNGLGETATTACHEDOBJECTSARBPROC) (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count,
                          GLhandleARB *obj);
typedef void (APIENTRY * PFNGLLINKPROGRAMARBPROC) (GLhandleARB programObj);

typedef void (APIENTRY * PFNGLUNIFORM1FARBPROC) (GLint location, GLfloat v0);
typedef void (APIENTRY * PFNGLUNIFORM2FARBPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY * PFNGLUNIFORM3FARBPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY * PFNGLUNIFORM4FARBPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

typedef void (APIENTRY * PFNGLUNIFORM1IARBPROC) (GLint location, GLint v0);
typedef void (APIENTRY * PFNGLUNIFORM2IARBPROC) (GLint location, GLint v0, GLint v1);
typedef void (APIENTRY * PFNGLUNIFORM3IARBPROC) (GLint location, GLint v0, GLint v1, GLint v2);
typedef void (APIENTRY * PFNGLUNIFORM4IARBPROC) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

typedef void (APIENTRY * PFNGLUNIFORM1FVARBPROC) (GLint location, GLsizei count, GLfloat *value);
typedef void (APIENTRY * PFNGLUNIFORM2FVARBPROC) (GLint location, GLsizei count, GLfloat *value);
typedef void (APIENTRY * PFNGLUNIFORM3FVARBPROC) (GLint location, GLsizei count, GLfloat *value);
typedef void (APIENTRY * PFNGLUNIFORM4FVARBPROC) (GLint location, GLsizei count, GLfloat *value);

typedef void (APIENTRY * PFNGLUNIFORM1IVARBPROC) (GLint location, GLsizei count, GLint *value);
typedef void (APIENTRY * PFNGLUNIFORM2IVARBPROC) (GLint location, GLsizei count, GLint *value);
typedef void (APIENTRY * PFNGLUNIFORM3IVARBPROC) (GLint location, GLsizei count, GLint *value);
typedef void (APIENTRY * PFNGLUNIFORM4IVARBPROC) (GLint location, GLsizei count, GLint *value);

typedef void (APIENTRY * PFNGLUNIFORMMATRIX2FVARBPROC) (GLint location, GLuint count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * PFNGLUNIFORMMATRIX3FVARBPROC) (GLint location, GLuint count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * PFNGLUNIFORMMATRIX4FVARBPROC) (GLint location, GLuint count, GLboolean transpose, const GLfloat *value);
typedef GLint (APIENTRY * PFNGLGETUNIFORMLOCATIONARBPROC) (GLhandleARB programObj, const GLcharARB *name);
typedef void (APIENTRY * PFNGLGETACTIVEATTRIBARBPROC) (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, 
													   GLenum *type, GLcharARB *name);
typedef void (APIENTRY * PFNGLGETACTIVEUNIFORMARBPROC) (GLhandleARB programObj, GLuint index, GLsizei maxLength,
														GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
typedef void (APIENTRY * PFNGLGETUNIFORMFVARBPROC) (GLhandleARB programObj, GLint location, GLfloat *params);
typedef void (APIENTRY * PFNGLGETUNIFORMIVARBPROC) (GLhandleARB programObj, GLint location, GLint *params);
typedef void (APIENTRY * PFNGLGETATTRIBLOCATIONARBPROC) (GLhandleARB programObj, const GLcharARB *name);
typedef void (APIENTRY * PFNGLVALIDATEPROGRAMARBPROC) (GLhandleARB programObj);
typedef void (APIENTRY * PFNGLGETOBJECTPARAMETERFVARBPROC)(GLhandleARB obj, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETOBJECTPARAMETERIVARBPROC)(GLhandleARB obj, GLenum pname, GLint *params);

typedef GLhandleARB (APIENTRY * PFNGLGETHANDLEARBPROC) (GLenum pname);
typedef GLhandleARB (APIENTRY * PFNGLCREATEVERTEXARRAYOBJECTGL2PROC) (GLhandleARB formatObject, GLsizei count);
typedef void (APIENTRY * PFNGLLOADVERTEXARRAYDATAGL2PROC) (GLhandleARB object, GLuint start, GLsizei count, GLvoid *data, GLenum preserve);
typedef GLhandleARB (APIENTRY * PFNGLSTARTVERTEXARRAYFORMATGL2PROC) ();
typedef void (APIENTRY * PFNGLADDELEMENTGL2PROC) (GLhandleARB formatObject, GLenum array, GLsizei size, GLenum type);
typedef void (APIENTRY * PFNGLENABLEVERTEXARRAYOBJECTGL2PROC) (GLhandleARB object);
typedef void (APIENTRY * PFNGLDISABLEVERTEXARRAYOBJECTGL2PROC) (GLhandleARB object);
typedef void (APIENTRY * PFNGLDRAWINDEXEDARRAYSGL2PROC) (GLenum mode, GLuint first, GLsizei count);

#endif  // End for #ifndef GL_ARB_vertex_shader
#endif  // End for #ifndef GL_ARB_fragment_shader
#endif  // End for #ifndef GL_ARB_shader_objects

PFNGLCREATEPROGRAMOBJECTARBPROC qglCreateProgramObjectARB;
PFNGLCREATESHADEROBJECTARBPROC qglCreateShaderObjectARB;
PFNGLDELETEOBJECTARBPROC qglDeleteObjectARB;
PFNGLDETACHOBJECTARBPROC qglDetachObjectARB;
PFNGLATTACHOBJECTARBPROC qglAttachObjectARB;

PFNGLSHADERSOURCEARBPROC qglShaderSourceARB;
PFNGLCOMPILESHADERARBPROC qglCompileShaderARB;
PFNGLLINKPROGRAMARBPROC qglLinkProgramARB;
PFNGLGETINFOLOGARBPROC qglGetInfoLogARB;
PFNGLUSEPROGRAMOBJECTARBPROC qglUseProgramObjectARB;

PFNGLGETOBJECTPARAMETERIVARBPROC qglGetObjectParameterivARB;
PFNGLGETOBJECTPARAMETERFVARBPROC qglGetObjectParameterfvARB;
PFNGLGETUNIFORMLOCATIONARBPROC qglGetUniformLocationARB;

PFNGLUNIFORM1FARBPROC qglUniform1fARB;
PFNGLUNIFORM2FARBPROC qglUniform2fARB;
PFNGLUNIFORM3FARBPROC qglUniform3fARB;
PFNGLUNIFORM4FARBPROC qglUniform4fARB;

PFNGLUNIFORM1IARBPROC qglUniform1iARB;
PFNGLUNIFORM2IARBPROC qglUniform2iARB;
PFNGLUNIFORM3IARBPROC qglUniform3iARB;
PFNGLUNIFORM4IARBPROC qglUniform4iARB;

PFNGLUNIFORM1FVARBPROC qglUniform1fvARB;
PFNGLUNIFORM2FVARBPROC qglUniform2fvARB;
PFNGLUNIFORM3FVARBPROC qglUniform3fvARB;
PFNGLUNIFORM4FVARBPROC qglUniform4fvARB;

PFNGLUNIFORM1IVARBPROC qglUniform1ivARB;
PFNGLUNIFORM2IVARBPROC qglUniform2ivARB;
PFNGLUNIFORM3IVARBPROC qglUniform3ivARB;
PFNGLUNIFORM4IVARBPROC qglUniform4ivARB;


typedef enum
{
    BUMP_PROGRAM = 0,
    BUMP_PROGRAM_COLOR,
    BUMP_PROGRAM2,
    BUMP_PROGRAM2_COLOR,
    BUMP_PROGRAM3,
    BUMP_PROGRAM3_COLOR,
    DELUX_PROGRAM,
    DELUX_PROGRAM_COLOR,
    MAX_PROGRAM
} e_programs;

static GLcharARB* vertex_programs[MAX_PROGRAM] =
{
    "hardware/bump.vert",
    "hardware/bump.vert",
    "hardware/bump2.vert",
    "hardware/bump2.vert",
    "hardware/bump2.vert",
    "hardware/bump2.vert",
    "hardware/delux.vert",
    "hardware/delux.vert"
};

static GLcharARB* fragment_programs[MAX_PROGRAM] =
{
    "hardware/bump.frag",
    "hardware/bump_c.frag",
    "hardware/bump2.frag",
    "hardware/bump2_c.frag",
    "hardware/bump3.frag",
    "hardware/bump3_c.frag",
    "hardware/delux.frag",
    "hardware/delux_c.frag"
};


static GLhandleARB vertex_shaders[MAX_PROGRAM];
static GLhandleARB fragment_shaders[MAX_PROGRAM];
static GLhandleARB programs[MAX_PROGRAM];

#define GL2DEBUG

#if defined(GL2DEBUG) && defined(_WIN32)
static void GL2_checkerror()
{
    GLuint error = glGetError();
    if ( error != GL_NO_ERROR )
    {
        int line;
        const char* err;

        err = gluErrorString(error);
        Con_Printf("GL2: %s\n", err);
//        _asm { int 3 };
    }
}
#else

#define GL2_checkerror() do { } while(0)

#endif

void printlog(GLhandleARB obj)
{
    int blen = 0;	/* length of buffer to allocate */
    int slen = 0;	/* strlen actually written to buffer */
    GLcharARB *infoLog;

    qglGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB , &blen);
    if (blen > 1) {
	if ((infoLog = (GLcharARB*)malloc(blen)) == NULL) {
	    printf("ERROR: Could not allocate InfoLog buffer\n");
	    exit(1);
	}
	qglGetInfoLogARB(obj, blen, &slen, infoLog);
	Con_Printf("GL2: %s\n", infoLog);
	free(infoLog);
    }
}


void GL2_CreateShaders()
{
    int i, len;

#if !defined(__APPLE__) && !defined (MACOSX)
    SAFE_GET_PROC( qglCreateProgramObjectARB, PFNGLCREATEPROGRAMOBJECTARBPROC, "glCreateProgramObjectARB");
    SAFE_GET_PROC( qglCreateShaderObjectARB, PFNGLCREATESHADEROBJECTARBPROC, "glCreateShaderObjectARB");
    SAFE_GET_PROC( qglDeleteObjectARB, PFNGLDELETEOBJECTARBPROC, "glDeleteObjectARB");
    SAFE_GET_PROC( qglDetachObjectARB, PFNGLDETACHOBJECTARBPROC, "glDetachObjectARB");
    SAFE_GET_PROC( qglAttachObjectARB, PFNGLATTACHOBJECTARBPROC, "glAttachObjectARB");

    SAFE_GET_PROC( qglShaderSourceARB, PFNGLSHADERSOURCEARBPROC, "glShaderSourceARB");
    SAFE_GET_PROC( qglCompileShaderARB, PFNGLCOMPILESHADERARBPROC, "glCompileShaderARB");
    SAFE_GET_PROC( qglLinkProgramARB, PFNGLLINKPROGRAMARBPROC, "glLinkProgramARB");
    SAFE_GET_PROC( qglGetInfoLogARB, PFNGLGETINFOLOGARBPROC, "glGetInfoLogARB");
    SAFE_GET_PROC( qglUseProgramObjectARB, PFNGLUSEPROGRAMOBJECTARBPROC, "glUseProgramObjectARB");

    SAFE_GET_PROC( qglGetObjectParameterivARB, PFNGLGETOBJECTPARAMETERIVARBPROC, "glGetObjectParameterivARB");
    SAFE_GET_PROC( qglGetObjectParameterfvARB, PFNGLGETOBJECTPARAMETERFVARBPROC, "glGetObjectParameterfvARB");
    SAFE_GET_PROC( qglGetUniformLocationARB, PFNGLGETUNIFORMLOCATIONARBPROC, "glGetUniformLocationARB");

    SAFE_GET_PROC( qglUniform1fARB, PFNGLUNIFORM1FARBPROC, "glUniform1fARB");
    SAFE_GET_PROC( qglUniform2fARB, PFNGLUNIFORM2FARBPROC, "glUniform2fARB");
    SAFE_GET_PROC( qglUniform3fARB, PFNGLUNIFORM3FARBPROC, "glUniform3fARB");
    SAFE_GET_PROC( qglUniform4fARB, PFNGLUNIFORM4FARBPROC, "glUniform4fARB");

    SAFE_GET_PROC( qglUniform1iARB, PFNGLUNIFORM1IARBPROC, "glUniform1iARB");
    SAFE_GET_PROC( qglUniform2iARB, PFNGLUNIFORM2IARBPROC, "glUniform2iARB");
    SAFE_GET_PROC( qglUniform3iARB, PFNGLUNIFORM3IARBPROC, "glUniform3iARB");
    SAFE_GET_PROC( qglUniform4iARB, PFNGLUNIFORM4IARBPROC, "glUniform4iARB");

    SAFE_GET_PROC( qglUniform1fvARB, PFNGLUNIFORM1FVARBPROC, "glUniform1fvARB");
    SAFE_GET_PROC( qglUniform2fvARB, PFNGLUNIFORM2FVARBPROC, "glUniform2fvARB");
    SAFE_GET_PROC( qglUniform3fvARB, PFNGLUNIFORM3FVARBPROC, "glUniform3fvARB");
    SAFE_GET_PROC( qglUniform4fvARB, PFNGLUNIFORM4FVARBPROC, "glUniform4fvARB");

    SAFE_GET_PROC( qglUniform1ivARB, PFNGLUNIFORM1IVARBPROC, "glUniform1ivARB");
    SAFE_GET_PROC( qglUniform2ivARB, PFNGLUNIFORM2IVARBPROC, "glUniform2ivARB");
    SAFE_GET_PROC( qglUniform3ivARB, PFNGLUNIFORM3IVARBPROC, "glUniform3ivARB");
    SAFE_GET_PROC( qglUniform4ivARB, PFNGLUNIFORM4IVARBPROC, "glUniform4ivARB");

    if ( strstr(gl_extensions, "GL_ATI_pn_triangles") )
    {
        SAFE_GET_PROC( qglPNTrianglesiATI, PFNGLPNTRIANGLESIATIPROC, "glPNTrianglesiATI");
        SAFE_GET_PROC( qglPNTrianglesfATI, PFNGLPNTRIANGLESFATIPROC, "glPNTrianglesfATI");
    }

    if ( strstr(gl_extensions, "GL_ATI_separate_stencil") )
    {
        SAFE_GET_PROC( qglStencilOpSeparateATI, PFNGLSTENCILOPSEPARATEATIPROC, "glStencilOpSeparateATI");
        SAFE_GET_PROC( qglStencilFuncSeparateATI, PFNGLSTENCILFUNCSEPARATEATIPROC, "glStencilFuncSeparateATI");
    }
#endif /* !__APPLE__ && !MACOSX */

    for ( i = 0; i < MAX_PROGRAM; i++ )
    {
        char* shader;
        int status;
        GLint loc, value;

	programs[i] = qglCreateProgramObjectARB();
	GL2_checkerror();
	vertex_shaders[i] = qglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	GL2_checkerror();
	fragment_shaders[i] = qglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	GL2_checkerror();

        shader = COM_LoadTempFile(vertex_programs[i]);
     	if (!shader)
        {
		//this is serious we need shader to render stuff
		Sys_Error("GL2: %s not found\n", vertex_programs[i]);
	}

	len = strlen(shader);
	qglShaderSourceARB(vertex_shaders[i], 1, (const GLcharARB **)&shader, &len);
	GL2_checkerror();

        shader = COM_LoadTempFile(fragment_programs[i]);
     	if (!shader)
        {
		//this is serious we need shader to render stuff
		Sys_Error("GL2: %s not found\n", fragment_programs[i]);
	}
	len = strlen(shader);
	qglShaderSourceARB(fragment_shaders[i], 1, (const GLcharARB **)&shader, &len);
	GL2_checkerror();
    
	qglCompileShaderARB(vertex_shaders[i]);
	GL2_checkerror();
	qglGetObjectParameterivARB(vertex_shaders[i], GL_OBJECT_COMPILE_STATUS_ARB, &status);
        GL2_checkerror();
        printlog(vertex_shaders[i]);

	qglCompileShaderARB(fragment_shaders[i]);
	GL2_checkerror();
	qglGetObjectParameterivARB(fragment_shaders[i], GL_OBJECT_COMPILE_STATUS_ARB, &status);
        GL2_checkerror();
        printlog(fragment_shaders[i]);

	qglAttachObjectARB(programs[i], vertex_shaders[i]);
	GL2_checkerror();
	qglAttachObjectARB(programs[i], fragment_shaders[i]);
	GL2_checkerror();

	qglDeleteObjectARB(vertex_shaders[i]);
	GL2_checkerror();
	qglDeleteObjectARB(fragment_shaders[i]);
	GL2_checkerror();

	qglLinkProgramARB(programs[i]);
	GL2_checkerror();

	qglUseProgramObjectARB(programs[i]);
        
        // link textures to units...
        switch(i)
        {
            case BUMP_PROGRAM:
            case BUMP_PROGRAM_COLOR:
            case BUMP_PROGRAM2:
            case BUMP_PROGRAM2_COLOR:
            case BUMP_PROGRAM3:
            case BUMP_PROGRAM3_COLOR:
                qglUniform1iARB(qglGetUniformLocationARB(programs[i], "normalmap"), 0);
        	GL2_checkerror();
                qglUniform1iARB(qglGetUniformLocationARB(programs[i], "colormap"), 1);
        	GL2_checkerror();
                qglUniform1iARB(qglGetUniformLocationARB(programs[i], "attenuation"), 2);
        	GL2_checkerror();
                break;
            case DELUX_PROGRAM:
            case DELUX_PROGRAM_COLOR:
                qglUniform1iARB(qglGetUniformLocationARB(programs[i], "lightMap"), 0);
        	GL2_checkerror();
                qglUniform1iARB(qglGetUniformLocationARB(programs[i], "deluxMap"), 1);
        	GL2_checkerror();
                qglUniform1iARB(qglGetUniformLocationARB(programs[i], "normalMap"), 2);
        	GL2_checkerror();
                qglUniform1iARB(qglGetUniformLocationARB(programs[i], "baseMap"), 3);
        	GL2_checkerror();
                break;
        }

        switch(i)
        {
            case BUMP_PROGRAM2:
            case BUMP_PROGRAM2_COLOR:
            case BUMP_PROGRAM3:
            case BUMP_PROGRAM3_COLOR:
                qglUniform1iARB(qglGetUniformLocationARB(programs[i], "filter"), 3);
	        GL2_checkerror();
                break;
        }

        switch(i)
        {
            case BUMP_PROGRAM_COLOR:
                qglUniform1iARB(qglGetUniformLocationARB(programs[i], "glossmap"), 3);
	        GL2_checkerror();
                break;
            case BUMP_PROGRAM2_COLOR:
            case BUMP_PROGRAM3_COLOR:
                qglUniform1iARB(qglGetUniformLocationARB(programs[i], "glossmap"), 4);
	        GL2_checkerror();
                break;
            case DELUX_PROGRAM_COLOR:
                qglUniform1iARB(qglGetUniformLocationARB(programs[i], "glossmap"), 4);
	        GL2_checkerror();
                break;
        }
    }
    qglUseProgramObjectARB(0);
}

void GL2_DisableBumpShader(shader_t* shader)
{
    //tex 0 = normal map
    //tex 1 = color map
    //tex 2 = attenuation
    //tex 3 = (optional light filter, depends on light settings)
    //tex 3/4 = colored gloss map if used

    qglUseProgramObjectARB(0);

    GL_SelectTexture(GL_TEXTURE2_ARB);
    glPopMatrix();

    if (currentshadowlight->shader->numstages)
    {        
	GL_SelectTexture(GL_TEXTURE3_ARB);
	glPopMatrix();
    }
    glMatrixMode(GL_MODELVIEW);

    GL_SelectTexture(GL_TEXTURE0_ARB);
}


void GL2_SetupTcMods(stage_t *s);

void GL2_EnableBumpShader(const transform_t *tr, const lightobject_t *lo,
                          qboolean alias, shader_t* shader)
{
    int prog;

    //tex 0 = normal map
    //tex 1 = color map
    //tex 2 = attenuation
    //tex 3 = (optional light filter, depends on light settings)
    //tex 3/4 = colored gloss map if used
    GL_SelectTexture(GL_TEXTURE2_ARB);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();

    glBindTexture(GL_TEXTURE_3D, atten3d_texture_object);

    glTranslatef(0.5,0.5,0.5);
    glScalef(0.5,0.5,0.5);
    glScalef(1.0f/(currentshadowlight->radiusv[0]),
             1.0f/(currentshadowlight->radiusv[1]),
			1.0f/(currentshadowlight->radiusv[2]));
    GL_SetupAttenMatrix(tr);

    glGetError();

    if (currentshadowlight->shader->numstages)
    {
        GL_SelectTexture(GL_TEXTURE3_ARB);
        glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

        if (currentshadowlight->shader->stages[0].texture[0]->gltype == GL_TEXTURE_CUBE_MAP_ARB)
        {
            SH_SetupTcMods(&currentshadowlight->shader->stages[0]);
            GL_SetupCubeMapMatrix(tr);

  	    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, currentshadowlight->shader->stages[0].texture[0]->texnum);
   
            if ( shader->glossstages[0].type == STAGE_GLOSS )
            {
                qglUseProgramObjectARB( programs[BUMP_PROGRAM2_COLOR] );
                prog = BUMP_PROGRAM2_COLOR;
                GL_SelectTexture(GL_TEXTURE4_ARB);
                GL_BindAdvanced(shader->glossstages[0].texture[0]);
            }
            else
            {
                qglUseProgramObjectARB( programs[BUMP_PROGRAM2] );
                prog = BUMP_PROGRAM2;
            }
	    GL2_checkerror();
        }
        else
        {
            // 2D filter
            GL_BindAdvanced(currentshadowlight->shader->stages[0].texture[0]);
	    //Default = repeat the texture one time in the light's sphere
	    //Can be modified with the tcMod shader commands
	    glTranslatef(0.5,0.5,0.5);
	    glScalef(0.5,0.5,0.5);
	    glScalef(1.0f/(currentshadowlight->radiusv[0]), 
	  		   1.0f/(currentshadowlight->radiusv[1]),
			   1.0f/(currentshadowlight->radiusv[2]));
            SH_SetupTcMods(&currentshadowlight->shader->stages[0]);
            GL_SetupCubeMapMatrix(tr);

            if ( shader->glossstages[0].type == STAGE_GLOSS )
            {
                qglUseProgramObjectARB( programs[BUMP_PROGRAM3_COLOR] );
                prog = BUMP_PROGRAM3_COLOR;
                GL_SelectTexture(GL_TEXTURE4_ARB);
                GL_BindAdvanced(shader->glossstages[0].texture[0]);
            }
            else
            {
                qglUseProgramObjectARB( programs[BUMP_PROGRAM3] );
                prog = BUMP_PROGRAM3;
            }
	    GL2_checkerror();
        }
    }
    else
    {
        if ( shader->glossstages[0].type == STAGE_GLOSS )
        {
            qglUseProgramObjectARB( programs[BUMP_PROGRAM_COLOR] );
            prog = BUMP_PROGRAM_COLOR;
            GL_SelectTexture(GL_TEXTURE3_ARB);
            GL_BindAdvanced(shader->glossstages[0].texture[0]);
        }
        else
        {
            qglUseProgramObjectARB( programs[BUMP_PROGRAM] );
            prog = BUMP_PROGRAM;
        }
	GL2_checkerror();
    }

    GL_SelectTexture(GL_TEXTURE0_ARB);

    qglUniform3fARB(qglGetUniformLocationARB(programs[prog], "lightPos"), lo->objectorigin[0],
 		    lo->objectorigin[1],  lo->objectorigin[2]);
    GL2_checkerror();

    qglUniform3fARB(qglGetUniformLocationARB(programs[prog], "eyePos"), lo->objectvieworg[0],
                    lo->objectvieworg[1],  lo->objectvieworg[2]);
    GL2_checkerror();
}

void GL2_EnableDeluxShader(shader_t* shader)
{
    int prog;

    if ( shader->glossstages[0].type == STAGE_GLOSS )
    {
        qglUseProgramObjectARB( programs[DELUX_PROGRAM_COLOR] );
        prog = DELUX_PROGRAM_COLOR;
        GL_SelectTexture(GL_TEXTURE4_ARB);
        GL_BindAdvanced(shader->glossstages[0].texture[0]);
    }
    else
    {
        qglUseProgramObjectARB( programs[DELUX_PROGRAM] );
        prog = DELUX_PROGRAM;
    }
    GL2_checkerror();
    qglUniform3fARB(qglGetUniformLocationARB(programs[prog], "eyePos"),
		    r_refdef.vieworg[0], r_refdef.vieworg[1], r_refdef.vieworg[2]);
    GL2_checkerror();
}

void GL2_DisableDeluxShader(shader_t* shader)
{
    qglUseProgramObjectARB(0);
}


/************************

Generic triangle list routines

*************************/

void FormatError(); // In gl_bumpgf.c

void GL2_drawTriangleListBump (const vertexdef_t *verts, int *indecies,
			       int numIndecies, shader_t *shader,
			       const transform_t *tr, const lightobject_t *lo)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    GL_VertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);

    GL_AddColor();
    glColor3fv(&currentshadowlight->color[0]);

    //Check the input vertices
    if (IsNullDriver(verts->texcoords)) FormatError();
    if (IsNullDriver(verts->binormals)) FormatError();
    if (IsNullDriver(verts->tangents)) FormatError();
    if (IsNullDriver(verts->normals)) FormatError();

    GL2_EnableBumpShader(tr, lo, true, shader);

    //bind the correct textures
    GL_SelectTexture(GL_TEXTURE0_ARB);
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    if (shader->numbumpstages > 0)
	GL_BindAdvanced(shader->bumpstages[0].texture[0]);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GL_TexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);

    GL_SelectTexture(GL_TEXTURE1_ARB);
    qglClientActiveTextureARB(GL_TEXTURE1_ARB);
    if (shader->numcolorstages > 0)
	GL_BindAdvanced(shader->colorstages[0].texture[0]);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GL_TexCoordPointer(3, GL_FLOAT, verts->tangentstride, verts->tangents);

    GL_SelectTexture(GL_TEXTURE2_ARB);
    qglClientActiveTextureARB(GL_TEXTURE2_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GL_TexCoordPointer(3, GL_FLOAT, verts->binormalstride, verts->binormals);

    GL_SelectTexture(GL_TEXTURE3_ARB);
    qglClientActiveTextureARB(GL_TEXTURE3_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GL_TexCoordPointer(3, GL_FLOAT, verts->normalstride, verts->normals);

    glDrawElements(GL_TRIANGLES, numIndecies, GL_UNSIGNED_INT, indecies);

    qglClientActiveTextureARB(GL_TEXTURE3_ARB);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE2_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE1_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    GL2_DisableBumpShader(shader);
}

void GL2_drawTriangleListBase (vertexdef_t *verts, int *indecies,
			       int numIndecies, shader_t *shader,
                               int lightmapIndex)
{
    int i;

    glGetError();
    GL_VertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    GL_SelectTexture(GL_TEXTURE0_ARB);
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    GL_TexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    if (!shader->cull)
    { 
	glDisable(GL_CULL_FACE); 
    } 

    glColor3ub(255,255,255);

    for ( i = 0; i < shader->numstages; i++)
    {
	SH_SetupSimpleStage(&shader->stages[i]);
	glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
	glPopMatrix();
    }
    glMatrixMode(GL_MODELVIEW);

    if (!IsNullDriver(verts->lightmapcoords) && (lightmapIndex >= 0) && (shader->flags & SURF_PPLIGHT)) 
    {
	//Delux lightmapping
	qboolean usedelux = (sh_delux.value != 0);
	if (shader->numcolorstages)
	{
	    if (shader->colorstages[0].alphatresh > 0)
		usedelux = false;
	}
        usedelux = false; // drops to software emulation now...

	if (usedelux) 
	{
            // Textures:
            // 0 light map
            // 1 delux map
            // 2 normal map
            // 3 base map
            // 4 colored gloss if used
            // Tex coords:
            // 0 base coord
            // 1 lightmap coord
            // 2 tangent
            // 3 binormal
            // 4 normal
            // 5 (position for fragment shader, generated in vertex shader)

	    // Setup blending
	    if (shader->numstages && shader->numcolorstages)
	    {
		if (shader->colorstages[0].src_blend >= 0)
		{
		    glEnable(GL_BLEND);
		    glBlendFunc(shader->colorstages[0].src_blend,
				shader->colorstages[0].dst_blend);
		}
		else
		{
		    glEnable(GL_BLEND);
		    glBlendFunc(GL_ONE, GL_ONE);
		}
	    }
	    else 
            {
		glDisable(GL_BLEND);
	    }

	    // Light map
            GL_SelectTexture(GL_TEXTURE0_ARB);
            qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	    GL_TexCoordPointer(2, GL_FLOAT, verts->texcoordstride,
			       verts->texcoords);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	    GL_Bind(lightmap_textures+lightmapIndex);

	    // Delux map
            GL_SelectTexture(GL_TEXTURE1_ARB);
            qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	    GL_TexCoordPointer(2, GL_FLOAT, verts->lightmapstride,
			       verts->lightmapcoords);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	    GL_Bind(lightmap_textures+lightmapIndex+1);

	    // Setup normal map
            GL_SelectTexture(GL_TEXTURE2_ARB);
	    if (shader->numbumpstages)
	    {
		if (shader->bumpstages[0].numtextures)
		    GL_BindAdvanced(shader->bumpstages[0].texture[0]);
	    }
            qglClientActiveTextureARB(GL_TEXTURE2_ARB);
	    GL_TexCoordPointer(3, GL_FLOAT, verts->tangentstride,
			       verts->tangents);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

            // Setup base texture
            GL_SelectTexture(GL_TEXTURE3_ARB);
            qglClientActiveTextureARB(GL_TEXTURE3_ARB);
	    GL_TexCoordPointer(3, GL_FLOAT, verts->binormalstride,
			       verts->binormals);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	    if (shader->numcolorstages)
	    {
		if (shader->colorstages[0].numtextures)
		    GL_BindAdvanced(shader->colorstages[0].texture[0]);

		if (shader->colorstages[0].alphatresh > 0)
		{
		    glEnable(GL_ALPHA_TEST);
		    glAlphaFunc(GL_GEQUAL, shader->colorstages[0].alphatresh);
		}	
	    }

	    // Normals
            qglClientActiveTextureARB(GL_TEXTURE4_ARB);
	    GL_TexCoordPointer(3, GL_FLOAT, verts->normalstride,
			       verts->normals);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	    GL2_EnableDeluxShader(shader);
	    glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
	    GL2_DisableDeluxShader(shader);

            qglClientActiveTextureARB(GL_TEXTURE4_ARB);
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            qglClientActiveTextureARB(GL_TEXTURE3_ARB);
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            qglClientActiveTextureARB(GL_TEXTURE2_ARB);
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            GL_SelectTexture(GL_TEXTURE1_ARB);
            qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	    GL_DisableMultitexture();
            GL_SelectTexture(GL_TEXTURE0_ARB);
	}
	else
	{
	    if (shader->numstages && shader->numcolorstages)
	    {
		if (shader->colorstages[0].src_blend >= 0)
		{
		    glEnable(GL_BLEND);
		    glBlendFunc(shader->colorstages[0].src_blend, shader->colorstages[0].dst_blend);
		}
		else
		{
		    glEnable(GL_BLEND);
		    glBlendFunc(GL_ONE, GL_ONE);
		}
	    }
	    else
	    {
		glDisable(GL_BLEND);
	    }

	    if (shader->numcolorstages)
	    {
		if (shader->colorstages[0].numtextures)
		    GL_BindAdvanced(shader->colorstages[0].texture[0]);
		
		if (shader->colorstages[0].alphatresh > 0)
		{
		    glEnable(GL_ALPHA_TEST);
		    glAlphaFunc(GL_GEQUAL, shader->colorstages[0].alphatresh);
		}	
	    }

	    GL_SelectTexture(GL_TEXTURE0_ARB);
            qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	    GL_TexCoordPointer(2, GL_FLOAT, verts->texcoordstride,
			       verts->texcoords);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	    GL_SelectTexture(GL_TEXTURE1_ARB);
	    glEnable(GL_TEXTURE_2D);
	    GL_Bind(lightmap_textures+lightmapIndex);
            qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	    GL_TexCoordPointer(2, GL_FLOAT, verts->lightmapstride,
			       verts->lightmapcoords);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	    glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
	    
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	    glDisable(GL_TEXTURE_2D);
	    GL_SelectTexture(GL_TEXTURE0_ARB);
	}
    }
    else
    {
	if (!IsNullDriver(verts->colors) && (shader->flags & SURF_PPLIGHT))
	{
	    GL_ColorPointer(3, GL_UNSIGNED_BYTE, verts->colorstride, verts->colors);
	    glEnableClientState(GL_COLOR_ARRAY);
	    glShadeModel(GL_SMOOTH);

	    if (shader->numstages && shader->numcolorstages)
	    {
		if (shader->colorstages[0].src_blend >= 0)
		{
		    glEnable(GL_BLEND);
		    glBlendFunc(shader->colorstages[0].src_blend, shader->colorstages[0].dst_blend);
		}
		else
		{
		    glEnable(GL_BLEND);
		    glBlendFunc(GL_ONE, GL_ONE);
		}
	    }
	    else 
	    {
		glDisable(GL_BLEND);
	    }

	    if (shader->numcolorstages)
	    {
		if (shader->colorstages[0].numtextures)
		    GL_BindAdvanced(shader->colorstages[0].texture[0]);

		if (shader->colorstages[0].alphatresh > 0)
		{
		    glEnable(GL_ALPHA_TEST);
		    glAlphaFunc(GL_GEQUAL, shader->colorstages[0].alphatresh);
		}	
	    }

	    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	    glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);

	    glDisableClientState(GL_COLOR_ARRAY);
	}
	else
	{
	    if (shader->flags & SURF_PPLIGHT)
	    {

		if (shader->colorstages[0].src_blend >= 0) {
		    glBlendFunc(shader->colorstages[0].src_blend, shader->colorstages[0].dst_blend);
		    glEnable(GL_BLEND);
		} else {
		    glDisable(GL_BLEND);
		}

		glColor3f(0,0,0);
		glDisable(GL_TEXTURE_2D);
		glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
		glEnable(GL_TEXTURE_2D);
	    }
	}
    }

    if (!shader->cull)
    {
	glEnable(GL_CULL_FACE);
	//Con_Printf("Cullstuff %s\n",shader->name);
    }

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
}



/*************************

Generic world surfaces routines

**************************/

void GL2_sendSurfacesBase(msurface_t **surfs, int numSurfaces,
			  qboolean bindLightmap)
{
    int i;
    glpoly_t *p;
    msurface_t *surf;

    for ( i = 0; i < numSurfaces; i++)
            {
	surf = surfs[i];
	if (surf->visframe != r_framecount)
	    continue;
	p = surf->polys;
	if (bindLightmap)
	{
	    if (surf->lightmaptexturenum < 0)
		continue;
	    GL_Bind(lightmap_textures+surf->lightmaptexturenum);
	}
	glDrawElements(GL_TRIANGLES, p->numindecies, GL_UNSIGNED_INT,
		       &p->indecies[0]);
    }
}

void GL2_sendSurfacesDeLux(msurface_t **surfs, int numSurfaces,
			   qboolean bindLightmap)
{
    int i;
    glpoly_t *p;
    msurface_t *surf;

    for (i=0; i<numSurfaces; i++)
    {
	surf = surfs[i];
	if (surf->visframe != r_framecount)
	    continue;
	p = surf->polys;

	if (bindLightmap)
	{
	    if (surf->lightmaptexturenum < 0)
		continue;
            // Bind light map
            GL_SelectTexture(GL_TEXTURE0_ARB);
	    GL_Bind(lightmap_textures+surf->lightmaptexturenum);
            // Bind delux map
            GL_SelectTexture(GL_TEXTURE1_ARB);
	    GL_Bind(lightmap_textures+surf->lightmaptexturenum+1);
	}

	qglMultiTexCoord3fvARB(GL_TEXTURE2_ARB, &surf->tangent[0]);
	qglMultiTexCoord3fvARB(GL_TEXTURE3_ARB, &surf->binormal[0]);
        qglMultiTexCoord3fvARB(GL_TEXTURE4_ARB, &surf->plane->normal[0]);

	glDrawElements(GL_TRIANGLES, p->numindecies, GL_UNSIGNED_INT,
		       &p->indecies[0]);
    }
}

void GL2_drawSurfaceListBase (vertexdef_t* verts, msurface_t** surfs,
			      int numSurfaces, shader_t* shader)
{
    int i;
    int usedelux;

    checkerror();
    GL_VertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    GL_SelectTexture(GL_TEXTURE0_ARB);
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GL_TexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);

    glColor3ub(255,255,255);

    if (!shader->cull)
    {
	glDisable(GL_CULL_FACE);
	//Con_Printf("Cullstuff %s\n",shader->name);
    }

    for (i = 0; i < shader->numstages; i++)
    {
	SH_SetupSimpleStage(&shader->stages[i]);
	GL2_sendSurfacesBase(surfs, numSurfaces, false);
	glPopMatrix();
    }
    if (!IsNullDriver(verts->lightmapcoords) && (shader->flags & SURF_PPLIGHT))
    {
	GL_SelectTexture(GL_TEXTURE1_ARB);
        qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	GL_TexCoordPointer(2, GL_FLOAT, verts->lightmapstride,
			   verts->lightmapcoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	//Delux lightmapping
	usedelux = (sh_delux.value != 0);
        if (shader->colorstages[0].alphatresh > 0)
            usedelux = false;

        usedelux = false; // drops to software emulation now...
	if (usedelux)
	{
            // Textures:
            // 0 light map
            // 1 delux map
            // 2 normal map
            // 3 base map
            // 4 colored gloss if used
            // Tex coords:
            // 0 base coord
            // 1 lightmap coord
            // 2 tangent
            // 3 binormal
            // 4 normal
            // 5 (position for fragment shader, generated in vertex shader)

	    // Setup blending
	    if (shader->numstages && shader->numcolorstages)
	    {
		if (shader->colorstages[0].src_blend >= 0)
		{
		    glEnable(GL_BLEND);
		    glBlendFunc(shader->colorstages[0].src_blend,
				shader->colorstages[0].dst_blend);
		}
		else
		{
		    glEnable(GL_BLEND);
		    glBlendFunc(GL_ONE, GL_ONE);
		}
	    }
	    else 
            {
		glDisable(GL_BLEND);
	    }

	    // Setup normal map
	    if (shader->numbumpstages && shader->bumpstages[0].numtextures)
	    {
                GL_SelectTexture(GL_TEXTURE2_ARB);
		GL_BindAdvanced(shader->bumpstages[0].texture[0]);
	    }

            // Setup base texture
	    if (shader->numcolorstages)
	    {
		if (shader->colorstages[0].numtextures)
                {
                    GL_SelectTexture(GL_TEXTURE3_ARB);
		    GL_BindAdvanced(shader->colorstages[0].texture[0]);
                }
		if (shader->colorstages[0].alphatresh > 0)
		{
		    glEnable(GL_ALPHA_TEST);
		    glAlphaFunc(GL_GEQUAL, shader->colorstages[0].alphatresh);
		}	
	    }

	    GL2_EnableDeluxShader(shader);
	    GL2_sendSurfacesDeLux(surfs, numSurfaces, true);
	    GL2_DisableDeluxShader(shader);

            qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	    GL_DisableMultitexture();
            GL_SelectTexture(GL_TEXTURE0_ARB);
	}
        else
        {
	    // Regular lightmapping
	    if (shader->numstages && shader->numcolorstages)
	    {
		if (shader->colorstages[0].src_blend >= 0)
		{
		    glEnable(GL_BLEND);
		    glBlendFunc(shader->colorstages[0].src_blend,
				shader->colorstages[0].dst_blend);
		}
		else
		{
		    glEnable(GL_BLEND);
		    glBlendFunc(GL_ONE, GL_ONE);
		}
	    }
	    else 
	    {
		glDisable(GL_BLEND);
	    }

            GL_SelectTexture(GL_TEXTURE0_ARB);
	    if (shader->numcolorstages)
	    {
		if (shader->colorstages[0].numtextures)
		    GL_BindAdvanced(shader->colorstages[0].texture[0]);

		if (shader->colorstages[0].alphatresh > 0)
		{
		    glEnable(GL_ALPHA_TEST);
		    glAlphaFunc(GL_GEQUAL, shader->colorstages[0].alphatresh);
		}	
	    }

	    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	    GL_EnableMultitexture();
	    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	    glColor3f(sh_lightmapbright.value, sh_lightmapbright.value,
		      sh_lightmapbright.value);

	    GL2_sendSurfacesBase(surfs, numSurfaces, true);

	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	    GL_DisableMultitexture();
            GL_SelectTexture(GL_TEXTURE0_ARB);
        }
    }
    if (!shader->cull)
    {
	glEnable(GL_CULL_FACE);
    }

    glDisable(GL_ALPHA_TEST);
    glMatrixMode(GL_MODELVIEW);
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}

void GL2_sendSurfacesTA(msurface_t** surfs, int numSurfaces, const transform_t *tr, const lightobject_t *lo)
{
    int i,j;
    glpoly_t *p;
    msurface_t *surf;
    shader_t *shader, *lastshader;
    float *v;
    qboolean cull;
    lastshader = NULL;

    cull = true;
    for ( i = 0; i < numSurfaces; i++)
    {
	surf = surfs[i];
	if (surf->visframe != r_framecount)
	    continue;

	if (!(surf->flags & SURF_PPLIGHT))
	    continue;

	p = surf->polys;
		
	shader = surfs[i]->shader->shader;

	//less state changes
	if (lastshader != shader)
	{
//            if ( lastshader && lastshader->glossstages[0].type != shader->glossstages[0].type )
            if ( lastshader )
            {
                // disable previous shader if switching between colored and mono gloss
                GL2_DisableBumpShader(lastshader);
                GL2_EnableBumpShader(tr, lo, true, shader);
            }
            else
            {
                if ( !lastshader )
                {
                    // Enable shader for the first surface
                    GL2_EnableBumpShader(tr, lo, true, shader);
                }
            }

	    if (!shader->cull)
	    {
		glDisable(GL_CULL_FACE);
		cull = false;
	    }
	    else
	    {
		if (!cull)
		    glEnable(GL_CULL_FACE);
		cull = true;
	    }
	    //bind the correct texture
	    GL_SelectTexture(GL_TEXTURE0_ARB);
	    if ( shader->numbumpstages > 0 )
		GL_BindAdvanced(shader->bumpstages[0].texture[0]);
	    GL_SelectTexture(GL_TEXTURE1_ARB);
	    if ( shader->numcolorstages > 0 )
		GL_BindAdvanced(shader->colorstages[0].texture[0]);
            if ( shader->glossstages[0].type == STAGE_GLOSS )
            {
                // Bind colored gloss
                if (currentshadowlight->shader->numstages)
                {
                    GL_SelectTexture(GL_TEXTURE4_ARB);
                }
                else
                {
                    GL_SelectTexture(GL_TEXTURE3_ARB);
                }
                GL_BindAdvanced(shader->glossstages[0].texture[0]);
            }
	    lastshader = shader;
	}

	//Note: texture coords out of begin-end are not a problem...
	qglMultiTexCoord3fvARB(GL_TEXTURE1_ARB, &surf->tangent[0]);
	qglMultiTexCoord3fvARB(GL_TEXTURE2_ARB, &surf->binormal[0]);
	qglMultiTexCoord3fvARB(GL_TEXTURE3_ARB, &surf->plane->normal[0]);
	glDrawElements(GL_TRIANGLES, p->numindecies, GL_UNSIGNED_INT, &p->indecies[0]);
    }

    if (!cull)
	glEnable(GL_CULL_FACE);

    if ( lastshader )
        GL2_DisableBumpShader(lastshader);
}



void GL2_drawSurfaceListBump (vertexdef_t *verts, msurface_t **surfs,
			      int numSurfaces,const transform_t *tr, const lightobject_t *lo)
{
    GL_VertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GL_TexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);

    GL_AddColor();
    glColor3fv(&currentshadowlight->color[0]);

    GL2_sendSurfacesTA(surfs,numSurfaces, tr, lo);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void GL2_freeDriver(void)
{
    //nothing here...
}


void BUMP_InitGL2(void)
{
    GLint errPos, errCode;
    const GLubyte *errString;

    if ( gl_cardtype != GL2 ) return;

    GL2_CreateShaders();


    //bind the correct stuff to the bump mapping driver
    gl_bumpdriver.drawSurfaceListBase = GL2_drawSurfaceListBase;
    gl_bumpdriver.drawSurfaceListBump = GL2_drawSurfaceListBump;
    gl_bumpdriver.drawTriangleListBase = GL2_drawTriangleListBase;
    gl_bumpdriver.drawTriangleListBump = GL2_drawTriangleListBump;
    gl_bumpdriver.freeDriver = GL2_freeDriver;
}
