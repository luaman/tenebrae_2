/*
Copyright (C) 2001-2002 Charles Hollemeersch
ARB_fragment_progam version (C) 2002-2003 Jarno Paananen

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

Same as gl_bumpmap.c but Radeon 9700 / NV30 optimized 
These routines require 6 texture units, vertex shader and pixel shader

All lights require 1 pass:
1 diffuse + specular with optional light filter

*/

#include "quakedef.h"

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

// actually in gl_bumpradeon (duh...)
extern PFNGLPNTRIANGLESIATIPROC qglPNTrianglesiATI;
extern PFNGLPNTRIANGLESFATIPROC qglPNTrianglesfATI;

// Separate_stencil_ATI
typedef void (APIENTRY *PFNGLSTENCILOPSEPARATEATIPROC)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
typedef void (APIENTRY *PFNGLSTENCILFUNCSEPARATEATIPROC)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);

extern PFNGLSTENCILOPSEPARATEATIPROC qglStencilOpSeparateATI;
extern PFNGLSTENCILFUNCSEPARATEATIPROC qglStencilFuncSeparateATI;

// ARB_vertex_program

typedef void (APIENTRY * glVertexAttrib1sARBPROC) (GLuint index, GLshort x);
typedef void (APIENTRY * glVertexAttrib1fARBPROC) (GLuint index, GLfloat x);
typedef void (APIENTRY * glVertexAttrib1dARBPROC) (GLuint index, GLdouble x);
typedef void (APIENTRY * glVertexAttrib2sARBPROC) (GLuint index, GLshort x, GLshort y);
typedef void (APIENTRY * glVertexAttrib2fARBPROC) (GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRY * glVertexAttrib2dARBPROC) (GLuint index, GLdouble x, GLdouble y);
typedef void (APIENTRY * glVertexAttrib3sARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY * glVertexAttrib3fARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * glVertexAttrib3dARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY * glVertexAttrib4sARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY * glVertexAttrib4fARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * glVertexAttrib4dARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * glVertexAttrib4NubARBPROC) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
typedef void (APIENTRY * glVertexAttrib1svARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * glVertexAttrib1fvARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * glVertexAttrib1dvARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * glVertexAttrib2svARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * glVertexAttrib2fvARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * glVertexAttrib2dvARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * glVertexAttrib3svARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * glVertexAttrib3fvARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * glVertexAttrib3dvARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * glVertexAttrib4bvARBPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * glVertexAttrib4svARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * glVertexAttrib4ivARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * glVertexAttrib4ubvARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * glVertexAttrib4usvARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRY * glVertexAttrib4uivARBPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * glVertexAttrib4fvARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * glVertexAttrib4dvARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * glVertexAttrib4NbvARBPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * glVertexAttrib4NsvARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * glVertexAttrib4NivARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * glVertexAttrib4NubvARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * glVertexAttrib4NusvARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRY * glVertexAttrib4NuivARBPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * glVertexAttribPointerARBPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * glEnableVertexAttribArrayARBPROC) (GLuint index);
typedef void (APIENTRY * glDisableVertexAttribArrayARBPROC) (GLuint index);
typedef void (APIENTRY * glProgramStringARBPROC) (GLenum target, GLenum format, GLsizei len, const GLvoid *string); 
typedef void (APIENTRY * glBindProgramARBPROC) (GLenum target, GLuint program);
typedef void (APIENTRY * glDeleteProgramsARBPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRY * glGenProgramsARBPROC) (GLsizei n, GLuint *programs);
typedef void (APIENTRY * glProgramEnvParameter4dARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * glProgramEnvParameter4dvARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * glProgramEnvParameter4fARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * glProgramEnvParameter4fvARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * glProgramLocalParameter4dARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * glProgramLocalParameter4dvARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * glProgramLocalParameter4fARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * glProgramLocalParameter4fvARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * glGetProgramEnvParameterdvARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * glGetProgramEnvParameterfvARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * glGetProgramLocalParameterdvARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * glGetProgramLocalParameterfvARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * glGetProgramivARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * glGetProgramStringARBPROC) (GLenum target, GLenum pname, GLvoid *string);
typedef void (APIENTRY * glGetVertexAttribdvARBPROC) (GLuint index, GLenum pname, GLdouble *params);
typedef void (APIENTRY * glGetVertexAttribfvARBPROC) (GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRY * glGetVertexAttribivARBPROC) (GLuint index, GLenum pname, GLint *params);
typedef void (APIENTRY * glGetVertexAttribPointervARBPROC) (GLuint index, GLenum pname, GLvoid **pointer);
typedef GLboolean (APIENTRY * glIsProgramARBPROC) (GLuint program);

static glVertexAttrib1sARBPROC qglVertexAttrib1sARB = NULL;
static glVertexAttrib1fARBPROC qglVertexAttrib1fARB = NULL;
static glVertexAttrib1dARBPROC qglVertexAttrib1dARB = NULL;
static glVertexAttrib2sARBPROC qglVertexAttrib2sARB = NULL;
static glVertexAttrib2fARBPROC qglVertexAttrib2fARB = NULL;
static glVertexAttrib2dARBPROC qglVertexAttrib2dARB = NULL;
static glVertexAttrib3sARBPROC qglVertexAttrib3sARB = NULL;
static glVertexAttrib3fARBPROC qglVertexAttrib3fARB = NULL;
static glVertexAttrib3dARBPROC qglVertexAttrib3dARB = NULL;
static glVertexAttrib4sARBPROC qglVertexAttrib4sARB = NULL;
static glVertexAttrib4fARBPROC qglVertexAttrib4fARB = NULL;
static glVertexAttrib4dARBPROC qglVertexAttrib4dARB = NULL;
static glVertexAttrib4NubARBPROC qglVertexAttrib4NubARB = NULL;
static glVertexAttrib1svARBPROC qglVertexAttrib1svARB = NULL;
static glVertexAttrib1fvARBPROC qglVertexAttrib1fvARB = NULL;
static glVertexAttrib1dvARBPROC qglVertexAttrib1dvARB = NULL;
static glVertexAttrib2svARBPROC qglVertexAttrib2svARB = NULL;
static glVertexAttrib2fvARBPROC qglVertexAttrib2fvARB = NULL;
static glVertexAttrib2dvARBPROC qglVertexAttrib2dvARB = NULL;
static glVertexAttrib3svARBPROC qglVertexAttrib3svARB = NULL;
static glVertexAttrib3fvARBPROC qglVertexAttrib3fvARB = NULL;
static glVertexAttrib3dvARBPROC qglVertexAttrib3dvARB = NULL;
static glVertexAttrib4bvARBPROC qglVertexAttrib4bvARB = NULL;
static glVertexAttrib4svARBPROC qglVertexAttrib4svARB = NULL;
static glVertexAttrib4ivARBPROC qglVertexAttrib4ivARB = NULL;
static glVertexAttrib4ubvARBPROC qglVertexAttrib4ubvARB = NULL;
static glVertexAttrib4usvARBPROC qglVertexAttrib4usvARB = NULL;
static glVertexAttrib4uivARBPROC qglVertexAttrib4uivARB = NULL;
static glVertexAttrib4fvARBPROC qglVertexAttrib4fvARB = NULL;
static glVertexAttrib4dvARBPROC qglVertexAttrib4dvARB = NULL;
static glVertexAttrib4NbvARBPROC qglVertexAttrib4NbvARB = NULL;
static glVertexAttrib4NsvARBPROC qglVertexAttrib4NsvARB = NULL;
static glVertexAttrib4NivARBPROC qglVertexAttrib4NivARB = NULL;
static glVertexAttrib4NubvARBPROC qglVertexAttrib4NubvARB = NULL;
static glVertexAttrib4NusvARBPROC qglVertexAttrib4NusvARB = NULL;
static glVertexAttrib4NuivARBPROC qglVertexAttrib4NuivARB = NULL;
static glVertexAttribPointerARBPROC qglVertexAttribPointerARB = NULL;
static glEnableVertexAttribArrayARBPROC qglEnableVertexAttribArrayARB = NULL;
static glDisableVertexAttribArrayARBPROC qglDisableVertexAttribArrayARB = NULL;
static glProgramStringARBPROC qglProgramStringARB = NULL;
static glBindProgramARBPROC qglBindProgramARB = NULL;
static glDeleteProgramsARBPROC qglDeleteProgramsARB = NULL;
static glGenProgramsARBPROC qglGenProgramsARB = NULL;
static glProgramEnvParameter4dARBPROC qglProgramEnvParameter4dARB = NULL;
static glProgramEnvParameter4dvARBPROC qglProgramEnvParameter4dvARB = NULL;
static glProgramEnvParameter4fARBPROC qglProgramEnvParameter4fARB = NULL;
static glProgramEnvParameter4fvARBPROC qglProgramEnvParameter4fvARB = NULL;
static glProgramLocalParameter4dARBPROC qglProgramLocalParameter4dARB = NULL;
static glProgramLocalParameter4dvARBPROC qglProgramLocalParameter4dvARB = NULL;
static glProgramLocalParameter4fARBPROC qglProgramLocalParameter4fARB = NULL;
static glProgramLocalParameter4fvARBPROC qglProgramLocalParameter4fvARB = NULL;
static glGetProgramEnvParameterdvARBPROC qglGetProgramEnvParameterdvARB = NULL;
static glGetProgramEnvParameterfvARBPROC qglGetProgramEnvParameterfvARB = NULL;
static glGetProgramLocalParameterdvARBPROC qglGetProgramLocalParameterdvARB = NULL;
static glGetProgramLocalParameterfvARBPROC qglGetProgramLocalParameterfvARB = NULL;
static glGetProgramivARBPROC qglGetProgramivARB = NULL;
static glGetProgramStringARBPROC qglGetProgramStringARB = NULL;
static glGetVertexAttribdvARBPROC qglGetVertexAttribdvARB = NULL;
static glGetVertexAttribfvARBPROC qglGetVertexAttribfvARB = NULL;
static glGetVertexAttribivARBPROC qglGetVertexAttribivARB = NULL;
static glGetVertexAttribPointervARBPROC qglGetVertexAttribPointervARB = NULL;
static glIsProgramARBPROC qglIsProgramARB = NULL;

#define GL_VERTEX_PROGRAM_ARB                                   0x8620
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB                        0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB                          0x8643
#define GL_COLOR_SUM_ARB                                        0x8458
#define GL_PROGRAM_FORMAT_ASCII_ARB                             0x8875
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB                      0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB                         0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB                       0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB                         0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB                   0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB                            0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB                      0x8645
#define GL_PROGRAM_LENGTH_ARB                                   0x8627
#define GL_PROGRAM_FORMAT_ARB                                   0x8876
#define GL_PROGRAM_BINDING_ARB                                  0x8677
#define GL_PROGRAM_INSTRUCTIONS_ARB                             0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB                         0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB                      0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB                  0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB                              0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB                          0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB                       0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB                   0x88A7
#define GL_PROGRAM_PARAMETERS_ARB                               0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB                           0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB                        0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB                    0x88AB
#define GL_PROGRAM_ATTRIBS_ARB                                  0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB                              0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB                           0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB                       0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB                        0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB                    0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB                 0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB             0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB                     0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB                       0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB                      0x88B6
#define GL_PROGRAM_STRING_ARB                                   0x8628
#define GL_PROGRAM_ERROR_POSITION_ARB                           0x864B
#define GL_CURRENT_MATRIX_ARB                                   0x8641
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB                         0x88B7
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB                       0x8640
#define GL_MAX_VERTEX_ATTRIBS_ARB                               0x8869
#define GL_MAX_PROGRAM_MATRICES_ARB                             0x862F
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB                   0x862E
#define GL_PROGRAM_ERROR_STRING_ARB                             0x8874
#define GL_MATRIX0_ARB                                          0x88C0
#define GL_MATRIX1_ARB                                          0x88C1
#define GL_MATRIX2_ARB                                          0x88C2
#define GL_MATRIX3_ARB                                          0x88C3
#define GL_MATRIX4_ARB                                          0x88C4
#define GL_MATRIX5_ARB                                          0x88C5
#define GL_MATRIX6_ARB                                          0x88C6
#define GL_MATRIX7_ARB                                          0x88C7
#define GL_MATRIX8_ARB                                          0x88C8
#define GL_MATRIX9_ARB                                          0x88C9
#define GL_MATRIX10_ARB                                         0x88CA
#define GL_MATRIX11_ARB                                         0x88CB
#define GL_MATRIX12_ARB                                         0x88CC
#define GL_MATRIX13_ARB                                         0x88CD
#define GL_MATRIX14_ARB                                         0x88CE
#define GL_MATRIX15_ARB                                         0x88CF
#define GL_MATRIX16_ARB                                         0x88D0
#define GL_MATRIX17_ARB                                         0x88D1
#define GL_MATRIX18_ARB                                         0x88D2
#define GL_MATRIX19_ARB                                         0x88D3
#define GL_MATRIX20_ARB                                         0x88D4
#define GL_MATRIX21_ARB                                         0x88D5
#define GL_MATRIX22_ARB                                         0x88D6
#define GL_MATRIX23_ARB                                         0x88D7
#define GL_MATRIX24_ARB                                         0x88D8
#define GL_MATRIX25_ARB                                         0x88D9
#define GL_MATRIX26_ARB                                         0x88DA
#define GL_MATRIX27_ARB                                         0x88DB
#define GL_MATRIX28_ARB                                         0x88DC
#define GL_MATRIX29_ARB                                         0x88DD
#define GL_MATRIX30_ARB                                         0x88DE
#define GL_MATRIX31_ARB                                         0x88DF

// ARB_fragment_program
#define GL_FRAGMENT_PROGRAM_ARB                                 0x8804

static char vertexprogram[] =
"!!ARBvp1.0\n"
"ATTRIB iPos         = vertex.position;\n"
"ATTRIB iNormal      = vertex.normal;\n"
"ATTRIB iColor       = vertex.color;\n"
"ATTRIB iTex0        = vertex.texcoord[0];\n"
"ATTRIB iTex1        = vertex.texcoord[1];\n"
"ATTRIB iTex2        = vertex.texcoord[2];\n"
"ATTRIB iTex3        = vertex.texcoord[3];\n"
"PARAM  mvp[4]       = { state.matrix.mvp };\n"
"PARAM  modelview[4] = { state.matrix.modelview[0] };\n"
"PARAM  texMatrix[4] = { state.matrix.texture[4] };\n"
"PARAM  fogparams    = state.fog.params;\n"
"PARAM  lightPos     = program.env[0];\n"
"PARAM  eyePos       = program.env[1];\n"
"TEMP   disttemp, lightVec, halfVec, temp;\n"
"OUTPUT oPos         = result.position;\n"
"OUTPUT oColor       = result.color;\n"
"OUTPUT oTex0        = result.texcoord[0];\n"
"OUTPUT oTex1        = result.texcoord[1];\n"
"OUTPUT oTex2        = result.texcoord[2];\n"
"OUTPUT oTex3        = result.texcoord[3];\n"
"OUTPUT oTex4        = result.texcoord[4];\n"
"OUTPUT oFog         = result.fogcoord;\n"
"DP4   oPos.x, mvp[0], iPos;\n"
"DP4   oPos.y, mvp[1], iPos;\n"
"DP4   oPos.z, mvp[2], iPos;\n"
"DP4   oPos.w, mvp[3], iPos;\n"
"DP4   oTex3.x, texMatrix[0], iPos;\n"
"DP4   oTex3.y, texMatrix[1], iPos;\n"
"DP4   oTex3.z, texMatrix[2], iPos;\n"
"DP4   oTex3.w, texMatrix[3], iPos;\n"
"MOV   oTex0, iTex0;\n"
"ADD   lightVec, lightPos, -iPos;\n"
"DP3   temp.x, lightVec, lightVec;\n"
"RSQ   temp.x, temp.x;\n"
"MUL   lightVec, lightVec, temp.x;\n"
"ADD   halfVec, eyePos, -iPos;\n"
"ADD   halfVec, halfVec, lightVec;\n"
"DP3   temp.x, halfVec, halfVec;\n"
"RSQ   temp.x, temp.x;\n"
"MUL   halfVec, halfVec, temp.x;\n"
"DP4   oTex1.x, lightVec, iTex1;\n"
"DP4   oTex1.y, lightVec, iTex2;\n"
"DP4   oTex1.z, lightVec, iTex3;\n"
"DP4   oTex2.x, halfVec, iTex1;\n"
"DP4   oTex2.y, halfVec, iTex2;\n"
"DP4   oTex2.z, halfVec, iTex3;\n"
"MOV   oColor, iColor;\n"
"DP4   disttemp.x, modelview[2], iPos;\n"
"SUB   disttemp.x, fogparams.z, disttemp.x;\n"
"MUL   oFog.x, disttemp.x, fogparams.w;\n"
"END";

static char vertexprogram2[] =
"!!ARBvp1.0\n"
"ATTRIB iPos         = vertex.position;\n"
"ATTRIB iNormal      = vertex.normal;\n"
"ATTRIB iColor       = vertex.color;\n"
"ATTRIB iTex0        = vertex.texcoord[0];\n"
"ATTRIB iTex1        = vertex.texcoord[1];\n"
"ATTRIB iTex2        = vertex.texcoord[2];\n"
"ATTRIB iTex3        = vertex.texcoord[3];\n"
"PARAM  mvp[4]       = { state.matrix.mvp };\n"
"PARAM  modelview[4] = { state.matrix.modelview[0] };\n"
"PARAM  texMatrix[4] = { state.matrix.texture[4] };\n"
"PARAM  texMatrix2[4]= { state.matrix.texture[5] };\n"
"PARAM  fogparams    = state.fog.params;\n"
"PARAM  lightPos     = program.env[0];\n"
"PARAM  eyePos       = program.env[1];\n"
"TEMP   disttemp, lightVec, halfVec, temp;\n"
"OUTPUT oPos         = result.position;\n"
"OUTPUT oColor       = result.color;\n"
"OUTPUT oTex0        = result.texcoord[0];\n"
"OUTPUT oTex1        = result.texcoord[1];\n"
"OUTPUT oTex2        = result.texcoord[2];\n"
"OUTPUT oTex3        = result.texcoord[3];\n"
"OUTPUT oTex4        = result.texcoord[4];\n"
"OUTPUT oFog         = result.fogcoord;\n"
"DP4   oPos.x, mvp[0], iPos;\n"
"DP4   oPos.y, mvp[1], iPos;\n"
"DP4   oPos.z, mvp[2], iPos;\n"
"DP4   oPos.w, mvp[3], iPos;\n"
"DP4   oTex3.x, texMatrix[0], iPos;\n"
"DP4   oTex3.y, texMatrix[1], iPos;\n"
"DP4   oTex3.z, texMatrix[2], iPos;\n"
"DP4   oTex3.w, texMatrix[3], iPos;\n"
"DP4   oTex4.x, texMatrix2[0], iPos;\n"
"DP4   oTex4.y, texMatrix2[1], iPos;\n"
"DP4   oTex4.z, texMatrix2[2], iPos;\n"
"DP4   oTex4.w, texMatrix2[3], iPos;\n"
"MOV   oTex0, iTex0;\n"
"ADD   lightVec, lightPos, -iPos;\n"
"DP3   temp.x, lightVec, lightVec;\n"
"RSQ   temp.x, temp.x;\n"
"MUL   lightVec, lightVec, temp.x;\n"
"ADD   halfVec, eyePos, -iPos;\n"
"ADD   halfVec, halfVec, lightVec;\n"
"DP3   temp.x, halfVec, halfVec;\n"
"RSQ   temp.x, temp.x;\n"
"MUL   halfVec, halfVec, temp.x;\n"
"DP4   oTex1.x, lightVec, iTex1;\n"
"DP4   oTex1.y, lightVec, iTex2;\n"
"DP4   oTex1.z, lightVec, iTex3;\n"
"DP4   oTex2.x, halfVec, iTex1;\n"
"DP4   oTex2.y, halfVec, iTex2;\n"
"DP4   oTex2.z, halfVec, iTex3;\n"
"MOV   oColor, iColor;\n"
"DP4   disttemp.x, modelview[2], iPos;\n"
"SUB   disttemp.x, fogparams.z, disttemp.x;\n"
"MUL   oFog.x, disttemp.x, fogparams.w;\n"
"END";

static char fragmentprogram[] =
"!!ARBfp1.0\n"
"ATTRIB tex0 = fragment.texcoord[0];\n"
"ATTRIB tex1 = fragment.texcoord[1];\n"
"ATTRIB tex2 = fragment.texcoord[2];\n"
"ATTRIB tex3 = fragment.texcoord[3];\n"
"ATTRIB col = fragment.color.primary;\n"
"PARAM scaler = { 16, 8, 2, -1 };\n"
"OUTPUT outColor = result.color;\n"
"TEMP normalmap, lightvec, halfvec, colormap, atten;\n"
"TEMP diffdot, specdot, selfshadow\n;"
"TEX normalmap, tex0, texture[0], 2D;\n"
"MAD normalmap.rgb, normalmap, scaler.b, scaler.a;\n"
"TEX lightvec, tex1, texture[1], CUBE;\n"
"MAD lightvec, lightvec, scaler.b, scaler.a;\n"
"TEX halfvec, tex2, texture[2], CUBE;\n"
"MAD halfvec, halfvec, scaler.b, scaler.a;\n"
"TEX colormap, tex0, texture[3], 2D;\n"
"TEX atten, tex3, texture[4], 3D;\n"
"DP3_SAT diffdot, normalmap, lightvec;\n"
"MUL_SAT selfshadow.r, lightvec.z, scaler.g;\n"
"DP3_SAT specdot.a, normalmap, halfvec;\n"
"MUL diffdot, diffdot, colormap;\n"
"POW specdot.a, specdot.a, scaler.r;\n"
"MUL_SAT diffdot, diffdot, selfshadow.r;\n"
"MUL_SAT specdot.a, specdot.a, normalmap.a;\n"
"MUL atten, col, atten;\n"
"ADD diffdot, diffdot, specdot.a;\n"
"MUL_SAT outColor, diffdot, atten;\n"
"END";


static char fragmentprogram2[] =
"!!ARBfp1.0\n"
"ATTRIB tex0 = fragment.texcoord[0];\n"
"ATTRIB tex1 = fragment.texcoord[1];\n"
"ATTRIB tex2 = fragment.texcoord[2];\n"
"ATTRIB tex3 = fragment.texcoord[3];\n"
"ATTRIB tex4 = fragment.texcoord[4];\n"
"ATTRIB col = fragment.color.primary;\n"
"PARAM scaler = { 16, 8, 2, -1 };\n"
"OUTPUT outColor = result.color;\n"
"TEMP normalmap, lightvec, halfvec, colormap, atten, filter;\n"
"TEMP diffdot, specdot, selfshadow\n;"
"TEX normalmap, tex0, texture[0], 2D;\n"
"MAD normalmap.rgb, normalmap, scaler.b, scaler.a;\n"
"TEX lightvec, tex1, texture[1], CUBE;\n"
"MAD lightvec, lightvec, scaler.b, scaler.a;\n"
"TEX halfvec, tex2, texture[2], CUBE;\n"
"MAD halfvec, halfvec, scaler.b, scaler.a;\n"
"TEX colormap, tex0, texture[3], 2D;\n"
"TEX atten, tex3, texture[4], 3D;\n"
"TEX filter, tex4, texture[5], CUBE;\n"
"DP3_SAT diffdot, normalmap, lightvec;\n"
"MUL_SAT selfshadow.r, lightvec.z, scaler.g;\n"
"DP3_SAT specdot.a, normalmap, halfvec;\n"
"MUL diffdot, diffdot, colormap;\n"
"POW specdot.a, specdot.a, scaler.r;\n"
"MUL_SAT diffdot, diffdot, selfshadow.r;\n"
"MUL_SAT specdot.a, specdot.a, normalmap.a;\n"
"MUL atten, col, atten;\n"
"ADD diffdot, diffdot, specdot.a;\n"
"MUL diffdot, diffdot, atten;\n"
"MUL_SAT outColor, diffdot, filter;\n"
"END";


static GLuint fragment_programs[2];
static GLuint vertex_programs[2];


#define ARBDEBUG

#if defined(ARBDEBUG) && defined(_WIN32)
static void checkerror()
{
    GLuint error = glGetError();
    if ( error != GL_NO_ERROR )
    {
        int line;
        const char* err;
        
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &line);
        err = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
        _asm { int 3 };
    }
}
#else

#define checkerror() do { } while(0)

#endif


void GL_CreateShadersARB()
{
    float scaler[4] = {0.5f, 0.5f, 0.5f, 0.5f};

#if !defined(__APPLE__) && !defined (MACOSX)
    SAFE_GET_PROC(qglVertexAttrib1sARB,glVertexAttrib1sARBPROC,"glVertexAttrib1sARB");
    SAFE_GET_PROC(qglVertexAttrib1fARB,glVertexAttrib1fARBPROC,"glVertexAttrib1fARB");
    SAFE_GET_PROC(qglVertexAttrib1dARB,glVertexAttrib1dARBPROC,"glVertexAttrib1dARB");
    SAFE_GET_PROC(qglVertexAttrib2sARB,glVertexAttrib2sARBPROC,"glVertexAttrib2sARB");
    SAFE_GET_PROC(qglVertexAttrib2fARB,glVertexAttrib2fARBPROC,"glVertexAttrib2fARB");
    SAFE_GET_PROC(qglVertexAttrib2dARB,glVertexAttrib2dARBPROC,"glVertexAttrib2dARB");
    SAFE_GET_PROC(qglVertexAttrib3sARB,glVertexAttrib3sARBPROC,"glVertexAttrib3sARB");
    SAFE_GET_PROC(qglVertexAttrib3fARB,glVertexAttrib3fARBPROC,"glVertexAttrib3fARB");
    SAFE_GET_PROC(qglVertexAttrib3dARB,glVertexAttrib3dARBPROC,"glVertexAttrib3dARB");
    SAFE_GET_PROC(qglVertexAttrib4sARB,glVertexAttrib4sARBPROC,"glVertexAttrib4sARB");
    SAFE_GET_PROC(qglVertexAttrib4fARB,glVertexAttrib4fARBPROC,"glVertexAttrib4fARB");
    SAFE_GET_PROC(qglVertexAttrib4dARB,glVertexAttrib4dARBPROC,"glVertexAttrib4dARB");
    SAFE_GET_PROC(qglVertexAttrib4NubARB,glVertexAttrib4NubARBPROC,"glVertexAttrib4NubARB");
    SAFE_GET_PROC(qglVertexAttrib1svARB,glVertexAttrib1svARBPROC,"glVertexAttrib1svARB");
    SAFE_GET_PROC(qglVertexAttrib1fvARB,glVertexAttrib1fvARBPROC,"glVertexAttrib1fvARB");
    SAFE_GET_PROC(qglVertexAttrib1dvARB,glVertexAttrib1dvARBPROC,"glVertexAttrib1dvARB");
    SAFE_GET_PROC(qglVertexAttrib2svARB,glVertexAttrib2svARBPROC,"glVertexAttrib2svARB");
    SAFE_GET_PROC(qglVertexAttrib2fvARB,glVertexAttrib2fvARBPROC,"glVertexAttrib2fvARB");
    SAFE_GET_PROC(qglVertexAttrib2dvARB,glVertexAttrib2dvARBPROC,"glVertexAttrib2dvARB");
    SAFE_GET_PROC(qglVertexAttrib3svARB,glVertexAttrib3svARBPROC,"glVertexAttrib3svARB");
    SAFE_GET_PROC(qglVertexAttrib3fvARB,glVertexAttrib3fvARBPROC,"glVertexAttrib3fvARB");
    SAFE_GET_PROC(qglVertexAttrib3dvARB,glVertexAttrib3dvARBPROC,"glVertexAttrib3dvARB");
    SAFE_GET_PROC(qglVertexAttrib4bvARB,glVertexAttrib4bvARBPROC,"glVertexAttrib4bvARB");
    SAFE_GET_PROC(qglVertexAttrib4svARB,glVertexAttrib4svARBPROC,"glVertexAttrib4svARB");
    SAFE_GET_PROC(qglVertexAttrib4ivARB,glVertexAttrib4ivARBPROC,"glVertexAttrib4ivARB");
    SAFE_GET_PROC(qglVertexAttrib4ubvARB,glVertexAttrib4ubvARBPROC,"glVertexAttrib4ubvARB");
    SAFE_GET_PROC(qglVertexAttrib4usvARB,glVertexAttrib4usvARBPROC,"glVertexAttrib4usvARB");
    SAFE_GET_PROC(qglVertexAttrib4uivARB,glVertexAttrib4uivARBPROC,"glVertexAttrib4uivARB");
    SAFE_GET_PROC(qglVertexAttrib4fvARB,glVertexAttrib4fvARBPROC,"glVertexAttrib4fvARB");
    SAFE_GET_PROC(qglVertexAttrib4dvARB,glVertexAttrib4dvARBPROC,"glVertexAttrib4dvARB");
    SAFE_GET_PROC(qglVertexAttrib4NbvARB,glVertexAttrib4NbvARBPROC,"glVertexAttrib4NbvARB");
    SAFE_GET_PROC(qglVertexAttrib4NsvARB,glVertexAttrib4NsvARBPROC,"glVertexAttrib4NsvARB");
    SAFE_GET_PROC(qglVertexAttrib4NivARB,glVertexAttrib4NivARBPROC,"glVertexAttrib4NivARB");
    SAFE_GET_PROC(qglVertexAttrib4NubvARB,glVertexAttrib4NubvARBPROC,"glVertexAttrib4NubvARB");
    SAFE_GET_PROC(qglVertexAttrib4NusvARB,glVertexAttrib4NusvARBPROC,"glVertexAttrib4NusvARB");
    SAFE_GET_PROC(qglVertexAttrib4NuivARB,glVertexAttrib4NuivARBPROC,"glVertexAttrib4NuivARB");
    SAFE_GET_PROC(qglVertexAttribPointerARB,glVertexAttribPointerARBPROC,"glVertexAttribPointerARB");
    SAFE_GET_PROC(qglEnableVertexAttribArrayARB,glEnableVertexAttribArrayARBPROC,"glEnableVertexAttribArrayARB");
    SAFE_GET_PROC(qglDisableVertexAttribArrayARB,glDisableVertexAttribArrayARBPROC,"glDisableVertexAttribArrayARB");
    SAFE_GET_PROC(qglProgramStringARB,glProgramStringARBPROC,"glProgramStringARB");
    SAFE_GET_PROC(qglBindProgramARB,glBindProgramARBPROC,"glBindProgramARB");
    SAFE_GET_PROC(qglDeleteProgramsARB,glDeleteProgramsARBPROC,"glDeleteProgramsARB");
    SAFE_GET_PROC(qglGenProgramsARB,glGenProgramsARBPROC,"glGenProgramsARB");
    SAFE_GET_PROC(qglProgramEnvParameter4dARB,glProgramEnvParameter4dARBPROC,"glProgramEnvParameter4dARB");
    SAFE_GET_PROC(qglProgramEnvParameter4dvARB,glProgramEnvParameter4dvARBPROC,"glProgramEnvParameter4dvARB");
    SAFE_GET_PROC(qglProgramEnvParameter4fARB,glProgramEnvParameter4fARBPROC,"glProgramEnvParameter4fARB");
    SAFE_GET_PROC(qglProgramEnvParameter4fvARB,glProgramEnvParameter4fvARBPROC,"glProgramEnvParameter4fvARB");
    SAFE_GET_PROC(qglProgramLocalParameter4dARB,glProgramLocalParameter4dARBPROC,"glProgramLocalParameter4dARB");
    SAFE_GET_PROC(qglProgramLocalParameter4dvARB,glProgramLocalParameter4dvARBPROC,"glProgramLocalParameter4dvARB");
    SAFE_GET_PROC(qglProgramLocalParameter4fARB,glProgramLocalParameter4fARBPROC,"glProgramLocalParameter4fARB");
    SAFE_GET_PROC(qglProgramLocalParameter4fvARB,glProgramLocalParameter4fvARBPROC,"glProgramLocalParameter4fvARB");
    SAFE_GET_PROC(qglGetProgramEnvParameterdvARB,glGetProgramEnvParameterdvARBPROC,"glGetProgramEnvParameterdvARB");
    SAFE_GET_PROC(qglGetProgramEnvParameterfvARB,glGetProgramEnvParameterfvARBPROC,"glGetProgramEnvParameterfvARB");
    SAFE_GET_PROC(qglGetProgramLocalParameterdvARB,glGetProgramLocalParameterdvARBPROC,"glGetProgramLocalParameterdvARB");
    SAFE_GET_PROC(qglGetProgramLocalParameterfvARB,glGetProgramLocalParameterfvARBPROC,"glGetProgramLocalParameterfvARB");
    SAFE_GET_PROC(qglGetProgramivARB,glGetProgramivARBPROC,"glGetProgramivARB");
    SAFE_GET_PROC(qglGetProgramStringARB,glGetProgramStringARBPROC,"glGetProgramStringARB");
    SAFE_GET_PROC(qglGetVertexAttribdvARB,glGetVertexAttribdvARBPROC,"glGetVertexAttribdvARB");
    SAFE_GET_PROC(qglGetVertexAttribfvARB,glGetVertexAttribfvARBPROC,"glGetVertexAttribfvARB");
    SAFE_GET_PROC(qglGetVertexAttribivARB,glGetVertexAttribivARBPROC,"glGetVertexAttribivARB");
    SAFE_GET_PROC(qglGetVertexAttribPointervARB,glGetVertexAttribPointervARBPROC,"glGetVertexAttribPointervARB");
    SAFE_GET_PROC(qglIsProgramARB,glIsProgramARBPROC,"glIsProgramARB");

    SAFE_GET_PROC( qglPNTrianglesiATI, PFNGLPNTRIANGLESIATIPROC, "glPNTrianglesiATI");
    SAFE_GET_PROC( qglPNTrianglesfATI, PFNGLPNTRIANGLESFATIPROC, "glPNTrianglesfATI");

    SAFE_GET_PROC( qglStencilOpSeparateATI, PFNGLSTENCILOPSEPARATEATIPROC, "glStencilOpSeparateATI");
    SAFE_GET_PROC( qglStencilFuncSeparateATI, PFNGLSTENCILFUNCSEPARATEATIPROC, "glStencilFuncSeparateATI");
#endif /* !__APPLE__ && !MACOSX */

    glEnable(GL_VERTEX_PROGRAM_ARB);
    checkerror();

    qglGenProgramsARB(2, &vertex_programs[0]);
    checkerror();
    qglBindProgramARB(GL_VERTEX_PROGRAM_ARB, vertex_programs[0]);
    checkerror();
    qglProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
		        sizeof(vertexprogram)-1, vertexprogram);
    checkerror();

    qglBindProgramARB(GL_VERTEX_PROGRAM_ARB, vertex_programs[1]);
    checkerror();
    qglProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
		        sizeof(vertexprogram2)-1, vertexprogram2);
    checkerror();

    glDisable(GL_VERTEX_PROGRAM_ARB);
    checkerror();

    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    checkerror();

    qglGenProgramsARB(2, &fragment_programs[0]);
    checkerror();
    qglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fragment_programs[0]);
    checkerror();
    qglProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
		                sizeof(fragmentprogram)-1, fragmentprogram);
    checkerror();

    qglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fragment_programs[1]);
    checkerror();
    qglProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
		                sizeof(fragmentprogram2)-1, fragmentprogram2);
    checkerror();

    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    checkerror();
}

void GL_DisableDiffuseShaderARB()
{
    //tex 0 = normal map
    //tex 1 = normalization cube map (tangent space light vector)
    //tex 2 = normalization cube map (tangent space half vector)
    //tex 3 = color map
    //tex 4 = attenuation
    //tex 5 = (optional light filter, depends on light settings)

    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    glDisable(GL_VERTEX_PROGRAM_ARB);

    GL_SelectTexture(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_CUBE_MAP_ARB);

    GL_SelectTexture(GL_TEXTURE2_ARB);
    glDisable(GL_TEXTURE_CUBE_MAP_ARB);

    GL_SelectTexture(GL_TEXTURE3_ARB);
    glDisable(GL_TEXTURE_2D);

    GL_SelectTexture(GL_TEXTURE4_ARB);
    glDisable(GL_TEXTURE_3D);
    glPopMatrix();

    if (currentshadowlight->filtercube)
    {
	GL_SelectTexture(GL_TEXTURE5_ARB);
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);
	glPopMatrix();
    }
    glMatrixMode(GL_MODELVIEW);

    GL_SelectTexture(GL_TEXTURE0_ARB);
}


void GL_EnableDiffuseSpecularShaderARB(const transform_t *tr, vec3_t lightOrig,
				       qboolean alias)
{
    float invrad = 1/currentshadowlight->radius;

    //tex 0 = normal map
    //tex 1 = normalization cube map (tangent space light vector)
    //tex 2 = normalization cube map (tangent space half vector)
    //tex 3 = color map
    //tex 4 = attenuation
    //tex 5 = (optional light filter, depends on light settings)

    GL_SelectTexture(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_CUBE_MAP_ARB);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, normcube_texture_object);

    GL_SelectTexture(GL_TEXTURE2_ARB);
    glEnable(GL_TEXTURE_CUBE_MAP_ARB);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, normcube_texture_object);

    GL_SelectTexture(GL_TEXTURE3_ARB);
    glEnable(GL_TEXTURE_2D);

    GL_SelectTexture(GL_TEXTURE4_ARB);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, atten3d_texture_object);

    glTranslatef(0.5,0.5,0.5);
    glScalef(0.5,0.5,0.5);
    glScalef(invrad, invrad, invrad);
    glTranslatef(-lightOrig[0], -lightOrig[1], -lightOrig[2]);

    glGetError();
    glEnable(GL_VERTEX_PROGRAM_ARB);
    checkerror();
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    checkerror();

    if (currentshadowlight->filtercube)
    {
	GL_SelectTexture(GL_TEXTURE5_ARB);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

        glEnable(GL_TEXTURE_CUBE_MAP_ARB);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, currentshadowlight->filtercube);
        GL_SetupCubeMapMatrix(tr);

	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, vertex_programs[1] );
	checkerror();
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, fragment_programs[1] );
	checkerror();
    }
    else
    {
	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, vertex_programs[0] );
	checkerror();
	qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, fragment_programs[0] );
	checkerror();
    }

    GL_SelectTexture(GL_TEXTURE0_ARB);

    qglProgramEnvParameter4fARB( GL_VERTEX_PROGRAM_ARB, 0, currentshadowlight->origin[0],
                                 currentshadowlight->origin[1],  currentshadowlight->origin[2], 1.0);
    qglProgramEnvParameter4fARB( GL_VERTEX_PROGRAM_ARB, 1, r_refdef.vieworg[0],
    		                 r_refdef.vieworg[1],  r_refdef.vieworg[2], 1.0);

}

/************************

Shader utility routines

*************************/

void ARB_SetupTcMod(tcmod_t *tc)
{
    switch (tc->type)
    {
    case TCMOD_ROTATE:
	glTranslatef(0.5,0.5,0.0);
	glRotatef(cl.time * tc->params[0],0,0,1);
	glTranslatef(-0.5, -0.5, 0.0);
	break;
    case TCMOD_SCROLL:
	glTranslatef(cl.time * tc->params[0], cl.time * tc->params[1], 0.0);
	break;
    case TCMOD_SCALE:
	glScalef(tc->params[0],tc->params[1],1.0);
	break;
    case TCMOD_STRETCH:
	//PENTA: fixme
	glScalef(1.0, 1.0, 1.0);
	break;
    }
}


void ARB_SetupSimpleStage(stage_t *s)
{
    tcmod_t *tc;
    int i;

    if (s->type != STAGE_SIMPLE)
    {
	Con_Printf("Non simple stage, in simple stage list");
	return;
    }

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();

    for (i=0; i<s->numtcmods; i++)
    {
	ARB_SetupTcMod(&s->tcmods[i]);	
    }

    if (s->src_blend > 0)
    {
	glBlendFunc(s->src_blend, s->dst_blend);
	glEnable(GL_BLEND);
    }

    if (s->alphatresh > 0)
    {
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, s->alphatresh);
    }

    if ((s->numtextures > 0) && (s->texture[0]))
	GL_Bind(s->texture[0]->texnum);
}

/************************

Generic triangle list routines

*************************/

void FormatError(); // In gl_bumpgf.c

void ARB_sendTriangleListWV(const vertexdef_t *verts, int *indecies,
			    int numIndecies)
{

    glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glTexCoordPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    //draw them
    glDrawElements(GL_TRIANGLES, numIndecies, GL_UNSIGNED_INT, indecies);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void ARB_sendTriangleListTA(const vertexdef_t *verts, int *indecies,
			    int numIndecies)
{
    glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    if (!verts->texcoords)
	FormatError();
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    if (!verts->tangents)
	FormatError();
    qglClientActiveTextureARB(GL_TEXTURE1_ARB);
    glTexCoordPointer(3, GL_FLOAT, verts->tangentstride, verts->tangents);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    if (!verts->binormals)
	FormatError();
    qglClientActiveTextureARB(GL_TEXTURE2_ARB);
    glTexCoordPointer(3, GL_FLOAT, verts->binormalstride, verts->binormals);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    if (!verts->normals)
	FormatError();
    qglClientActiveTextureARB(GL_TEXTURE3_ARB);
    glTexCoordPointer(3, GL_FLOAT, verts->normalstride, verts->normals);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    //draw them
    glDrawElements(GL_TRIANGLES, numIndecies, GL_UNSIGNED_INT, indecies);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE2_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE1_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    GL_SelectTexture(GL_TEXTURE0_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void ARB_drawTriangleListBump (const vertexdef_t *verts, int *indecies,
			       int numIndecies, shader_t *shader,
			       const transform_t *tr)
{
    GL_AddColor();
    glColor3fv(&currentshadowlight->color[0]);

    GL_EnableDiffuseSpecularShaderARB(tr,currentshadowlight->origin,true);
    //bind the correct texture
    GL_SelectTexture(GL_TEXTURE0_ARB);
    if (shader->numbumpstages > 0)
	GL_Bind(shader->bumpstages[0].texture[0]->texnum);
    GL_SelectTexture(GL_TEXTURE3_ARB);
    if (shader->numcolorstages > 0)
	GL_Bind(shader->colorstages[0].texture[0]->texnum);

    ARB_sendTriangleListTA(verts,indecies,numIndecies);
    GL_DisableDiffuseShaderARB();
}

void ARB_drawTriangleListBase (vertexdef_t *verts, int *indecies,
			       int numIndecies, shader_t *shader)
{
    int i;

    glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    for ( i = 0; i < shader->numstages; i++)
    {
	ARB_SetupSimpleStage(&shader->stages[i]);
	glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
	glPopMatrix();
    }
    glMatrixMode(GL_MODELVIEW);

    if (verts->colors)
    {
	glColorPointer(3, GL_UNSIGNED_BYTE, verts->colorstride, verts->colors);
	glEnableClientState(GL_COLOR_ARRAY);
	glShadeModel(GL_SMOOTH);

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

	if (shader->numcolorstages)
	{
	    if (shader->colorstages[0].numtextures)
		GL_Bind(shader->colorstages[0].texture[0]->texnum);

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
	glColor3f(0,0,0);
	glDisable(GL_TEXTURE_2D);
	glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
	glEnable(GL_TEXTURE_2D);
    }

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}

/*************************

Generic world surfaces routines

**************************/

void ARB_sendSurfacesBase(msurface_t **surfs, int numSurfaces,
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

void ARB_drawSurfaceListBase (vertexdef_t* verts, msurface_t** surfs,
			      int numSurfaces, shader_t* shader)
{
    int i;

    glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glColor3ub(255,255,255);

    if (!shader->cull)
    {
	glDisable(GL_CULL_FACE);
	//Con_Printf("Cullstuff %s\n",shader->name);
    }

    for (i = 0; i < shader->numstages; i++)
    {
	ARB_SetupSimpleStage(&shader->stages[i]);
	ARB_sendSurfacesBase(surfs, numSurfaces, false);
	glPopMatrix();
    }

    if (verts->lightmapcoords && (shader->flags & SURF_PPLIGHT))
    {
	qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	glTexCoordPointer(2, GL_FLOAT, verts->lightmapstride,
			  verts->lightmapcoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

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

	if (shader->numcolorstages)
	{
	    if (shader->colorstages[0].numtextures)
		GL_Bind(shader->colorstages[0].texture[0]->texnum);

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

	ARB_sendSurfacesBase(surfs, numSurfaces, true);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	GL_DisableMultitexture();
	qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    }

    if (!shader->cull)
    {
	glEnable(GL_CULL_FACE);
    }

    glDisable(GL_ALPHA_TEST);
    glMatrixMode(GL_MODELVIEW);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}

void ARB_sendSurfacesTA(msurface_t** surfs, int numSurfaces)
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
	    if (shader->numbumpstages > 0)
		GL_Bind(shader->bumpstages[0].texture[0]->texnum);
	    GL_SelectTexture(GL_TEXTURE3_ARB);
	    if (shader->numcolorstages > 0)
		GL_Bind(shader->colorstages[0].texture[0]->texnum);
	    lastshader = shader;
	}

	//Note: texture coords out of begin-end are not a problem...
	qglMultiTexCoord3fvARB(GL_TEXTURE1_ARB, &surf->tangent[0]);
	qglMultiTexCoord3fvARB(GL_TEXTURE2_ARB, &surf->binormal[0]);
	qglMultiTexCoord3fvARB(GL_TEXTURE3_ARB, &surf->plane->normal[0]);
	/*
	  glBegin(GL_POLYGON);
	  v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
	  for (j=0; j<p->numverts; j++, v+= VERTEXSIZE) {
	  qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
	  glVertex3fv(&v[0]);
	  }
	  glEnd();
	*/
	glDrawElements(GL_TRIANGLES, p->numindecies, GL_UNSIGNED_INT, &p->indecies[0]);
    }

    if (!cull)
	glEnable(GL_CULL_FACE);
}


void ARB_sendSurfacesPlain(msurface_t** surfs, int numSurfaces)
{
    int i,j;
    glpoly_t *p;
    msurface_t *surf;
    shader_t *shader, *lastshader;
    float *v;
    qboolean cull;
    lastshader = NULL;

    cull = true;

    for (i = 0; i < numSurfaces; i++)
    {
	surf = surfs[i];
		
	if (surf->visframe != r_framecount)
	    continue;

	if (!(surf->flags & SURF_PPLIGHT))
	    continue;

	p = surf->polys;

	shader = surf->shader->shader;

	//less state changes
	if (lastshader != shader)
	{
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
	    lastshader = shader;
	}
	/*		
		glBegin(GL_POLYGON);
		v = (float *)(&globalVertexTable[surf->polys->firstvertex]);
		for (j=0; j<p->numverts; j++, v+= VERTEXSIZE) {
		//qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, v[3], v[4]);
		glVertex3fv(&v[0]);
		}
		glEnd();
	*/
	glDrawElements(GL_TRIANGLES, p->numindecies, GL_UNSIGNED_INT,
		       &p->indecies[0]);
    }

    if (!cull)
	glEnable(GL_CULL_FACE);
}

void ARB_drawSurfaceListBump (vertexdef_t *verts, msurface_t **surfs,
			      int numSurfaces,const transform_t *tr)
{
    glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    GL_AddColor();
    glColor3fv(&currentshadowlight->color[0]);

    GL_EnableDiffuseSpecularShaderARB(tr,currentshadowlight->origin,true);

    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
    ARB_sendSurfacesTA(surfs,numSurfaces);
    GL_DisableDiffuseShaderARB();

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


typedef struct allocchain_s
{
    struct allocchain_s* next;
    char data[1];//variable sized
} allocchain_t;

static allocchain_t* allocChain = NULL;

void* ARB_getDriverMem(size_t size, drivermem_t hint)
{
    allocchain_t *r = (allocchain_t *)malloc(size+sizeof(void *));
    r->next = allocChain;
    allocChain = r;
    return &r->data[0];
}

void ARB_freeAllDriverMem(void)
{
    allocchain_t *r = allocChain;
    allocchain_t *next;

    while (r)
    {
	next = r->next;
	free(r);
	r = next;
    }
}

void ARB_freeDriver(void)
{
    //nothing here...
}


void BUMP_InitARB(void)
{
    GLint errPos, errCode;
    const GLubyte *errString;

    if ( gl_cardtype != ARB ) return;

    GL_CreateShadersARB();


    //bind the correct stuff to the bump mapping driver
    gl_bumpdriver.drawSurfaceListBase = ARB_drawSurfaceListBase;
    gl_bumpdriver.drawSurfaceListBump = ARB_drawSurfaceListBump;
    gl_bumpdriver.drawTriangleListBase = ARB_drawTriangleListBase;
    gl_bumpdriver.drawTriangleListBump = ARB_drawTriangleListBump;
    gl_bumpdriver.getDriverMem = ARB_getDriverMem;
    gl_bumpdriver.freeAllDriverMem = ARB_freeAllDriverMem;
    gl_bumpdriver.freeDriver = ARB_freeDriver;
}
