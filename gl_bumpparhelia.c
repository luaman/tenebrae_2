/*
Copyright (C) 2001-2002 Charles Hollemeersch
Parhelia version Copyright (C) 2002-2003 Jarno Paananen

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

Same as gl_bumpmap.c but Matrox Parhelia optimized 
These routines require 4 texture units and EXT_vertex_shader and
MTX_fragment shader extensions.

Most lights reqire 2 passes this way
1 diffuse
2 specular

If a light has a cubemap filter it requires 3 passes
1 attenuation
2 diffuse
3 specular
*/

#include "quakedef.h"

#include "glATI.h" // Yes, for EXT_vertex_shader

static GLuint vertex_shaders;
static GLuint fragment_shaders;
static GLuint lightPos, eyePos; // vertex shader constants

//#define PARHELIADEBUG

#if defined(PARHELIADEBUG) && defined(_WIN32)
#define Parhelia_checkerror() checkrealerror(__LINE__)
static void checkrealerror(int line)
{
    GLuint error = glGetError();
    if ( error != GL_NO_ERROR )
    {
        char    buf[128];
	wsprintf(buf, "ERROR (%08lx) on line %d\n", error, line );
	MessageBox( NULL, buf, "ERROR", MB_OK );
    }
}
#else

#define Parhelia_checkerror() do { } while(0)

#endif


// --- MTX_Fragment Shader Extension ---
#define GL_FRAGMENT_SHADER_MTX          0x8A80

// Get Parameters
#define GL_FS_BINDING_MTX               0x8A81
#define GL_MAX_TA_OPS_MTX               0x8A82
#define GL_OPT_TA_OPS_MTX               0x8A83
#define GL_TA_OPS_MTX                   0x8A84
#define GL_MAX_FP_OPS_MTX               0x8A85
#define GL_MAX_FP_TEXTURES_MTX          0x8A86
#define GL_MAX_FP_COLOURS_MTX           0x8A87
#define GL_OPT_FP_OPS_MTX               0x8A88
#define GL_OPT_FP_TEXTURES_MTX          0x8A89
#define GL_OPT_FP_COLOURS_MTX           0x8A8A
#define GL_FP_OPS_MTX                   0x8A8B
#define GL_FP_TEXTURES_MTX              0x8A8C
#define GL_FP_COLOURS_MTX               0x8A8D

// Texture Opcodes
#define GL_OP_TEX_NOP_MTX               0x8A8E
#define GL_OP_TEX_MTX                   0x8A8F
#define GL_OP_TEX_COORD_MTX             0x8A90
#define GL_OP_TEXKILL_GE_MTX            0x8A91
#define GL_OP_TEXKILL_LT_MTX            0x8A92

#define GL_OP_TEXBEM_MTX                0x8A93
#define GL_OP_TEXDEP_AR_MTX             0x8A94
#define GL_OP_TEXDEP_GB_MTX             0x8A95
#define GL_OP_TEXDEP_RGB_MTX            0x8A96
#define GL_OP_TEXDOT3_COORD_MTX         0x8A97
#define GL_OP_TEXDOT3_MTX               0x8A98

#define GL_OP_TEXM3X2_MTX               0x8A99
#define GL_OP_TEXM3X2_DEPTH_MTX         0x8A9A

#define GL_OP_TEXM3X3_MTX               0x8A9B
#define GL_OP_TEXM3X3_COORD_MTX         0x8A9C
#define GL_OP_TEXM3X3_REFLECT_MTX       0x8A9D

#define GL_TEXBEM_MATRIX_MTX            0x8A9E
#define GL_TEXTURE_FORMAT_MTX           0x8A9F

// Texture Formats
#define GL_TEX_UNSIGNED_MTX             0x8AC0
#define GL_TEX_SIGNED_MTX               0x8AC1
#define GL_TEX_BIASED_MTX               0x8AC2

// Hints
#define GL_HINT_NEAREST_MTX             0x0001
#define GL_HINT_BILINEAR_MTX            0x0002
#define GL_HINT_TRILINEAR_MTX           0x0004
#define GL_HINT_ANISOTROPIC_MTX         0x0008
#define GL_HINT_TEXTURE_1D_MTX          0x0010
#define GL_HINT_TEXTURE_2D_MTX          0x0020
#define GL_HINT_TEXTURE_3D_MTX          0x0040
#define GL_HINT_TEXTURE_CUBE_MTX        0x0080
#define GL_HINT_TEXTURE_BEML_MTX        0x0100
#define GL_HINT_TEXTURE_BEM_MTX         0x0200

// Colour Opcodes
#define GL_OP_MOV_MTX                   0x8AA0
#define GL_OP_ADD_MTX                   0x8AA1
#define GL_OP_SUB_MTX                   0x8AA2
#define GL_OP_MUL_MTX                   0x8AA3
#define GL_OP_DP3_MTX                   0x8AA4
#define GL_OP_DP4_MTX                   0x8AA5
#define GL_OP_MAD_MTX                   0x8AA6
#define GL_OP_LRP_MTX                   0x8AA7
#define GL_OP_CND_MTX                   0x8AA8
#define GL_OP_CMV_MTX                   0x8AA9
#define GL_OP_CMP_MTX                   0x8AAA

// Result
#define GL_COLOR0_MTX                   0x0B00 // GL_CURRENT_COLOR
#define GL_COLOR1_MTX                   0x8459 // GL_CURRENT_SECONDARY_COLOR
#define GL_TEXTURE0                     0x84C0 // GL_TEXTURE0_ARB
#define GL_TEXTURE1                     0x84C1 // GL_TEXTURE1_ARB
#define GL_TEXTURE2                     0x84C2 // GL_TEXTURE2_ARB
#define GL_TEXTURE3                     0x84C3 // GL_TEXTURE3_ARB
#define GL_TEMPORARY0_MTX               0x8AB0
#define GL_TEMPORARY1_MTX               0x8AB1
#define GL_IGNORED_MTX                  0x8AB2
#define GL_OUTPUT_COLOR_MTX             0x8AB3

// Target
#define GL_RESULT_MTX                   0x8AC3
#define GL_ARGUMENT0_MTX                0x8AC4
#define GL_ARGUMENT1_MTX                0x8AC5
#define GL_ARGUMENT2_MTX                0x8AC6

// Result Modifier
#define GL_SATURATE_MTX                 0x8AC7
#define GL_SCALE_MTX                    0x8AC8

// Arguments Modifier
#define GL_REPLICATE_MTX                0x8AC9
#define GL_CONVERT_MTX                  0x8ACA

#define GL_NONE_MTX                     0x8ACB

// GL_SCALE_MTX
#define GL_SCALE_X2_MTX                 0x8ACC
#define GL_SCALE_X4_MTX                 0x8ACD
#define GL_SCALE_X8_MTX                 0x8ACE
#define GL_SCALE_D2_MTX                 0x8ACF
#define GL_SCALE_D4_MTX                 0x8AD0

// GL_CONVERT_MTX
#define GL_ARG_INVERT_MTX               0x8AB4
#define GL_ARG_NEGATE_MTX               0x8AB5
#define GL_ARG_BIAS_MTX                 0x8AB6
#define GL_ARG_BIAS_NEGATE_MTX          0x8AB7
#define GL_ARG_SIGNED_MTX               0x8AB8
#define GL_ARG_SIGNED_NEGATE_MTX        0x8AB9

// GL_REPLICATE_MTX
#define GL_RED_MTX                      0x8ABA
#define GL_GREEN_MTX                    0x8ABB
#define GL_BLUE_MTX                     0x8ABC
#define GL_ALPHA_MTX                    0x8ABD


/* Fragment_Shader_ext */
typedef GLvoid    (APIENTRY * PFNGLBEGINFRAGSHADEREXTPROC)          ( GLvoid );
typedef GLvoid    (APIENTRY * PFNGLBINDFRAGSHADEREXTPROC)           ( GLuint id);
typedef GLvoid    (APIENTRY * PFNGLDELETEFRAGSHADERSEXTPROC)        ( GLsizei n, const GLuint *shaders );
typedef GLvoid    (APIENTRY * PFNGLENDFRAGSHADEREXTPROC)            ( GLvoid );
typedef GLvoid    (APIENTRY * PFNGLFRAGPROCALPHAOP1EXTPROC)         ( GLenum op, GLenum res, GLenum arg0 );
typedef GLvoid    (APIENTRY * PFNGLFRAGPROCALPHAOP2EXTPROC)         ( GLenum op, GLenum res, GLenum arg0, GLenum arg1 );
typedef GLvoid    (APIENTRY * PFNGLFRAGPROCALPHAOP3EXTPROC)         ( GLenum op, GLenum res, GLenum arg0, GLenum arg1, GLenum arg2 );
typedef GLvoid    (APIENTRY * PFNGLFRAGPROCOPPARAMMTXEXTPROC)       ( GLenum target, GLenum pname, GLenum param );
typedef GLvoid    (APIENTRY * PFNGLFRAGPROCRGBAOP1EXTPROC)          ( GLenum op, GLenum res, GLenum arg0 );
typedef GLvoid    (APIENTRY * PFNGLFRAGPROCRGBAOP2EXTPROC)          ( GLenum op, GLenum res, GLenum arg0, GLenum arg1 );
typedef GLvoid    (APIENTRY * PFNGLFRAGPROCRGBAOP3EXTPROC)          ( GLenum op, GLenum res, GLenum arg0, GLenum arg1, GLenum arg2 );
typedef GLuint    (APIENTRY * PFNGLGENFRAGPROCINVARIANTBVEXTPROC)   ( GLbyte* values);
typedef GLuint    (APIENTRY * PFNGLGENFRAGPROCINVARIANTDVEXTPROC)   ( GLdouble* values);
typedef GLuint    (APIENTRY * PFNGLGENFRAGPROCINVARIANTFVEXTPROC)   ( GLfloat* values);
typedef GLuint    (APIENTRY * PFNGLGENFRAGPROCINVARIANTIVEXTPROC)   ( GLint* values);
typedef GLuint    (APIENTRY * PFNGLGENFRAGPROCINVARIANTSVEXTPROC)   ( GLshort* values);
typedef GLuint    (APIENTRY * PFNGLGENFRAGSHADERSEXTPROC)           ( GLsizei n );
typedef GLboolean (APIENTRY * PFNGLISFRAGSHADEREXTPROC)             ( GLuint id );
typedef GLvoid    (APIENTRY * PFNGLSETFRAGPROCINVARIANTBVPROC)      ( GLuint id, const GLbyte* values );
typedef GLvoid    (APIENTRY * PFNGLSETFRAGPROCINVARIANTDVPROC)      ( GLuint id, const GLdouble* values );
typedef GLvoid    (APIENTRY * PFNGLSETFRAGPROCINVARIANTFVPROC)      ( GLuint id, const GLfloat* values );
typedef GLvoid    (APIENTRY * PFNGLSETFRAGPROCINVARIANTIVPROC)      ( GLuint id, const GLint* values );
typedef GLvoid    (APIENTRY * PFNGLSETFRAGPROCINVARIANTSVPROC)      ( GLuint id, const GLshort* values );
typedef GLvoid    (APIENTRY * PFNGLTEXADDRESSHINTSEXTPROC)          ( GLenum target, GLbitfield hints );
typedef GLvoid    (APIENTRY * PFNGLTEXADDRESSOP1EXTPROC)            ( GLenum op, GLenum arg0 );
typedef GLvoid    (APIENTRY * PFNGLTEXADDRESSOP2EXTPROC)            ( GLenum op, GLenum arg0, GLenum arg1);
typedef GLvoid    (APIENTRY * PFNGLTEXADDRESSOP3EXTPROC)            ( GLenum op, GLenum arg0, GLenum arg1, GLenum arg2 );
typedef GLvoid    (APIENTRY * PFNGLTEXADDRESSOP4EXTPROC)            ( GLenum op, GLenum arg0, GLenum arg1, GLenum arg2, GLenum arg3 );
typedef GLvoid    (APIENTRY * PFNGLTEXADDRESSPARAMFVEXTPROC)        ( GLenum target, GLenum pname, const GLfloat* param );
typedef GLvoid    (APIENTRY * PFNGLTEXADDRESSPARAMUIEXTPROC)        ( GLenum target, GLenum pname, GLuint param );
typedef GLvoid    (APIENTRY * PFNGLGETFRAGPROCINVARIANTBVEXTPROC)   ( GLuint id, GLbyte* values);
typedef GLvoid    (APIENTRY * PFNGLGETFRAGPROCINVARIANTDVEXTPROC)   ( GLuint id, GLdouble* values);
typedef GLvoid    (APIENTRY * PFNGLGETFRAGPROCINVARIANTFVEXTPROC)   ( GLuint id, GLfloat* values);
typedef GLvoid    (APIENTRY * PFNGLGETFRAGPROCINVARIANTIVEXTPROC)   ( GLuint id, GLint* values);
typedef GLvoid    (APIENTRY * PFNGLGETFRAGPROCINVARIANTSVEXTPROC)   ( GLuint id, GLshort* values);
typedef GLvoid    (APIENTRY * PFNGLGETTEXADDRESSPARAMFVEXTPROC)     ( GLenum target, GLenum pname, GLfloat* param);
typedef GLvoid    (APIENTRY * PFNGLGETTEXADDRESSPARAMUIVEXTPROC)    ( GLenum target, GLenum pname, GLuint* param);


// --- End of MTX_Fragment Shader Extension ---

// Fragment Shader Extension Entry Points (pointers to the extension functions)
// ----------------------------------------------------------------------------

PFNGLBEGINFRAGSHADEREXTPROC qglBeginFragShaderMTX = NULL;
PFNGLBINDFRAGSHADEREXTPROC  qglBindFragShaderMTX = NULL;
PFNGLDELETEFRAGSHADERSEXTPROC qglDeleteFragShadersMTX = NULL;
PFNGLENDFRAGSHADEREXTPROC qglEndFragShaderMTX = NULL;
PFNGLFRAGPROCALPHAOP1EXTPROC qglFragProcAlphaOp1MTX = NULL;
PFNGLFRAGPROCALPHAOP2EXTPROC qglFragProcAlphaOp2MTX = NULL;
PFNGLFRAGPROCALPHAOP3EXTPROC qglFragProcAlphaOp3MTX = NULL;
PFNGLFRAGPROCOPPARAMMTXEXTPROC qglFragProcOpParamMTX = NULL;
PFNGLFRAGPROCRGBAOP1EXTPROC qglFragProcRGBAOp1MTX = NULL;
PFNGLFRAGPROCRGBAOP2EXTPROC qglFragProcRGBAOp2MTX = NULL;
PFNGLFRAGPROCRGBAOP3EXTPROC qglFragProcRGBAOp3MTX = NULL;
PFNGLGENFRAGPROCINVARIANTBVEXTPROC qglGenFragProcInvariantBVMTX = NULL;
PFNGLGENFRAGPROCINVARIANTDVEXTPROC qglGenFragProcInvariantDVMTX = NULL;
PFNGLGENFRAGPROCINVARIANTFVEXTPROC qglGenFragProcInvariantFVMTX = NULL;
PFNGLGENFRAGPROCINVARIANTIVEXTPROC qglGenFragProcInvariantIVMTX = NULL;
PFNGLGENFRAGPROCINVARIANTSVEXTPROC qglGenFragProcInvariantSVMTX = NULL;
PFNGLGENFRAGSHADERSEXTPROC qglGenFragShadersMTX = NULL;
PFNGLISFRAGSHADEREXTPROC qglIsFragShaderMTX = NULL;
PFNGLSETFRAGPROCINVARIANTBVPROC qglSetFragProcInvariantBVMTX = NULL;
PFNGLSETFRAGPROCINVARIANTDVPROC qglSetFragProcInvariantDVMTX = NULL;
PFNGLSETFRAGPROCINVARIANTFVPROC qglSetFragProcInvariantFVMTX = NULL;
PFNGLSETFRAGPROCINVARIANTIVPROC qglSetFragProcInvariantIVMTX = NULL;
PFNGLSETFRAGPROCINVARIANTSVPROC qglSetFragProcInvariantSVMTX = NULL;
PFNGLTEXADDRESSHINTSEXTPROC qglTexAddressHintsMTX = NULL;
PFNGLTEXADDRESSOP1EXTPROC qglTexAddressOp1MTX = NULL;
PFNGLTEXADDRESSOP2EXTPROC qglTexAddressOp2MTX = NULL;
PFNGLTEXADDRESSOP3EXTPROC qglTexAddressOp3MTX = NULL;
PFNGLTEXADDRESSOP4EXTPROC qglTexAddressOp4MTX = NULL;
PFNGLTEXADDRESSPARAMFVEXTPROC qglTexAddressParamFVMTX = NULL;
PFNGLTEXADDRESSPARAMUIEXTPROC qglTexAddressParamUIMTX = NULL;
PFNGLGETFRAGPROCINVARIANTBVEXTPROC qglGetFragProcInvariantBVMTX = NULL;
PFNGLGETFRAGPROCINVARIANTDVEXTPROC qglGetFragProcInvariantDVMTX = NULL;
PFNGLGETFRAGPROCINVARIANTFVEXTPROC qglGetFragProcInvariantFVMTX = NULL;
PFNGLGETFRAGPROCINVARIANTIVEXTPROC qglGetFragProcInvariantIVMTX = NULL;
PFNGLGETFRAGPROCINVARIANTSVEXTPROC qglGetFragProcInvariantSVMTX = NULL;
PFNGLGETTEXADDRESSPARAMFVEXTPROC qglGetTexAddressParamFVMTX = NULL;
PFNGLGETTEXADDRESSPARAMUIVEXTPROC qglGetTexAddressParamUIVMTX = NULL;

// EXT_vertex_shader stuff is in gl_bumpradeon.c
extern PFNGLBEGINVERTEXSHADEREXTPROC           qglBeginVertexShaderEXT;
extern PFNGLENDVERTEXSHADEREXTPROC             qglEndVertexShaderEXT;
extern PFNGLBINDVERTEXSHADEREXTPROC            qglBindVertexShaderEXT;
extern PFNGLGENVERTEXSHADERSEXTPROC            qglGenVertexShadersEXT;
extern PFNGLDELETEVERTEXSHADEREXTPROC          qglDeleteVertexShaderEXT;
extern PFNGLSHADEROP1EXTPROC                   qglShaderOp1EXT;
extern PFNGLSHADEROP2EXTPROC                   qglShaderOp2EXT;
extern PFNGLSHADEROP3EXTPROC                   qglShaderOp3EXT;
extern PFNGLSWIZZLEEXTPROC                     qglSwizzleEXT;
extern PFNGLWRITEMASKEXTPROC                   qglWriteMaskEXT;
extern PFNGLINSERTCOMPONENTEXTPROC             qglInsertComponentEXT;
extern PFNGLEXTRACTCOMPONENTEXTPROC            qglExtractComponentEXT;
extern PFNGLGENSYMBOLSEXTPROC                  qglGenSymbolsEXT;
extern PFNGLSETINVARIANTEXTPROC                qglSetInvariantEXT;
extern PFNGLSETLOCALCONSTANTEXTPROC            qglSetLocalConstantEXT;
extern PFNGLVARIANTBVEXTPROC                   qglVariantbvEXT;
extern PFNGLVARIANTSVEXTPROC                   qglVariantsvEXT;
extern PFNGLVARIANTIVEXTPROC                   qglVariantivEXT;
extern PFNGLVARIANTFVEXTPROC                   qglVariantfvEXT;
extern PFNGLVARIANTDVEXTPROC                   qglVariantdvEXT;
extern PFNGLVARIANTUBVEXTPROC                  qglVariantubvEXT;
extern PFNGLVARIANTUSVEXTPROC                  qglVariantusvEXT;
extern PFNGLVARIANTUIVEXTPROC                  qglVariantuivEXT;
extern PFNGLVARIANTPOINTEREXTPROC              qglVariantPointerEXT;
extern PFNGLENABLEVARIANTCLIENTSTATEEXTPROC    qglEnableVariantClientStateEXT;
extern PFNGLDISABLEVARIANTCLIENTSTATEEXTPROC   qglDisableVariantClientStateEXT;
extern PFNGLBINDLIGHTPARAMETEREXTPROC          qglBindLightParameterEXT;
extern PFNGLBINDMATERIALPARAMETEREXTPROC       qglBindMaterialParameterEXT;
extern PFNGLBINDTEXGENPARAMETEREXTPROC         qglBindTexGenParameterEXT;
extern PFNGLBINDTEXTUREUNITPARAMETEREXTPROC    qglBindTextureUnitParameterEXT;
extern PFNGLBINDPARAMETEREXTPROC               qglBindParameterEXT;
extern PFNGLISVARIANTENABLEDEXTPROC            qglIsVariantEnabledEXT;
extern PFNGLGETVARIANTBOOLEANVEXTPROC          qglGetVariantBooleanvEXT;
extern PFNGLGETVARIANTINTEGERVEXTPROC          qglGetVariantIntegervEXT;
extern PFNGLGETVARIANTFLOATVEXTPROC            qglGetVariantFloatvEXT;
extern PFNGLGETVARIANTPOINTERVEXTPROC          qglGetVariantPointervEXT;
extern PFNGLGETINVARIANTBOOLEANVEXTPROC        qglGetInvariantBooleanvEXT;
extern PFNGLGETINVARIANTINTEGERVEXTPROC        qglGetInvariantIntegervEXT;
extern PFNGLGETINVARIANTFLOATVEXTPROC          qglGetInvariantFloatvEXT;
extern PFNGLGETLOCALCONSTANTBOOLEANVEXTPROC    qglGetLocalConstantBooleanvEXT;
extern PFNGLGETLOCALCONSTANTINTEGERVEXTPROC    qglGetLocalConstantIntegervEXT;
extern PFNGLGETLOCALCONSTANTFLOATVEXTPROC      qglGetLocalConstantFloatvEXT;


void Parhelia_CreateShaders()
{
    GLuint mvp, modelview, zcomp, tempvec;
    GLuint texturematrix;
    GLuint vertex, normal;
    GLuint texcoord0;
    GLuint texcoord1;
    GLuint texcoord2;
    GLuint texcoord3;
    GLuint color;
    GLuint disttemp, disttemp2;
    GLuint fogstart, fogend;
    GLuint scaler;
    GLfloat scalervalues[4] = { 0.5, 0.5, 0.5, 0.5 };
    int err;

#if !defined(__APPLE__) && !defined (MACOSX)
    SAFE_GET_PROC( qglBeginVertexShaderEXT, PFNGLBEGINVERTEXSHADEREXTPROC, "glBeginVertexShaderEXT");            
    SAFE_GET_PROC( qglEndVertexShaderEXT, PFNGLENDVERTEXSHADEREXTPROC, "glEndVertexShaderEXT");          
    SAFE_GET_PROC( qglBindVertexShaderEXT, PFNGLBINDVERTEXSHADEREXTPROC, "glBindVertexShaderEXT");               
    SAFE_GET_PROC( qglGenVertexShadersEXT, PFNGLGENVERTEXSHADERSEXTPROC, "glGenVertexShadersEXT");               
    SAFE_GET_PROC( qglDeleteVertexShaderEXT, PFNGLDELETEVERTEXSHADEREXTPROC, "glDeleteVertexShaderEXT");         
    SAFE_GET_PROC( qglShaderOp1EXT, PFNGLSHADEROP1EXTPROC, "glShaderOp1EXT");            
    SAFE_GET_PROC( qglShaderOp2EXT, PFNGLSHADEROP2EXTPROC, "glShaderOp2EXT");            
    SAFE_GET_PROC( qglShaderOp3EXT, PFNGLSHADEROP3EXTPROC, "glShaderOp3EXT");            
    SAFE_GET_PROC( qglSwizzleEXT, PFNGLSWIZZLEEXTPROC, "glSwizzleEXT");          
    SAFE_GET_PROC( qglWriteMaskEXT, PFNGLWRITEMASKEXTPROC, "glWriteMaskEXT");            
    SAFE_GET_PROC( qglInsertComponentEXT, PFNGLINSERTCOMPONENTEXTPROC, "glInsertComponentEXT");          
    SAFE_GET_PROC( qglExtractComponentEXT, PFNGLEXTRACTCOMPONENTEXTPROC, "glExtractComponentEXT");               
    SAFE_GET_PROC( qglGenSymbolsEXT, PFNGLGENSYMBOLSEXTPROC, "glGenSymbolsEXT");         
    SAFE_GET_PROC( qglSetInvariantEXT, PFNGLSETINVARIANTEXTPROC, "glSetInvariantEXT");           
    SAFE_GET_PROC( qglSetLocalConstantEXT, PFNGLSETLOCALCONSTANTEXTPROC, "glSetLocalConstantEXT");               
    SAFE_GET_PROC( qglVariantbvEXT, PFNGLVARIANTBVEXTPROC, "glVariantbvEXT");            
    SAFE_GET_PROC( qglVariantsvEXT, PFNGLVARIANTSVEXTPROC, "glVariantsvEXT");            
    SAFE_GET_PROC( qglVariantivEXT, PFNGLVARIANTIVEXTPROC, "glVariantivEXT");            
    SAFE_GET_PROC( qglVariantfvEXT, PFNGLVARIANTFVEXTPROC, "glVariantfvEXT");            
    SAFE_GET_PROC( qglVariantdvEXT, PFNGLVARIANTDVEXTPROC, "glVariantdvEXT");            
    SAFE_GET_PROC( qglVariantubvEXT, PFNGLVARIANTUBVEXTPROC, "glVariantubvEXT");         
    SAFE_GET_PROC( qglVariantusvEXT, PFNGLVARIANTUSVEXTPROC, "glVariantusvEXT");         
    SAFE_GET_PROC( qglVariantuivEXT, PFNGLVARIANTUIVEXTPROC, "glVariantuivEXT");         
    SAFE_GET_PROC( qglVariantPointerEXT, PFNGLVARIANTPOINTEREXTPROC, "glVariantPointerEXT");             
    SAFE_GET_PROC( qglEnableVariantClientStateEXT, PFNGLENABLEVARIANTCLIENTSTATEEXTPROC, "glEnableVariantClientStateEXT");
    SAFE_GET_PROC( qglDisableVariantClientStateEXT, PFNGLDISABLEVARIANTCLIENTSTATEEXTPROC, "glDisableVariantClientStateEXT");
    SAFE_GET_PROC( qglBindLightParameterEXT, PFNGLBINDLIGHTPARAMETEREXTPROC, "glBindLightParameterEXT");
    SAFE_GET_PROC( qglBindMaterialParameterEXT, PFNGLBINDMATERIALPARAMETEREXTPROC, "glBindMaterialParameterEXT");
    SAFE_GET_PROC( qglBindTexGenParameterEXT, PFNGLBINDTEXGENPARAMETEREXTPROC, "glBindTexGenParameterEXT");
    SAFE_GET_PROC( qglBindTextureUnitParameterEXT, PFNGLBINDTEXTUREUNITPARAMETEREXTPROC, "glBindTextureUnitParameterEXT");
    SAFE_GET_PROC( qglBindParameterEXT, PFNGLBINDPARAMETEREXTPROC, "glBindParameterEXT");
    SAFE_GET_PROC( qglIsVariantEnabledEXT, PFNGLISVARIANTENABLEDEXTPROC, "glIsVariantEnabledEXT");
    SAFE_GET_PROC( qglGetVariantBooleanvEXT, PFNGLGETVARIANTBOOLEANVEXTPROC, "glGetVariantBooleanvEXT");
    SAFE_GET_PROC( qglGetVariantIntegervEXT, PFNGLGETVARIANTINTEGERVEXTPROC, "glGetVariantIntegervEXT");
    SAFE_GET_PROC( qglGetVariantFloatvEXT, PFNGLGETVARIANTFLOATVEXTPROC, "glGetVariantFloatvEXT");
    SAFE_GET_PROC( qglGetVariantPointervEXT, PFNGLGETVARIANTPOINTERVEXTPROC, "glGetVariantPointervEXT");
    SAFE_GET_PROC( qglGetInvariantBooleanvEXT, PFNGLGETINVARIANTBOOLEANVEXTPROC, "glGetInvariantBooleanvEXT");
    SAFE_GET_PROC( qglGetInvariantIntegervEXT, PFNGLGETINVARIANTINTEGERVEXTPROC, "glGetInvariantIntegervEXT");
    SAFE_GET_PROC( qglGetInvariantFloatvEXT, PFNGLGETINVARIANTFLOATVEXTPROC, "glGetInvariantFloatvEXT");
    SAFE_GET_PROC( qglGetLocalConstantBooleanvEXT, PFNGLGETLOCALCONSTANTBOOLEANVEXTPROC, "glGetLocalConstantBooleanvEXT");
    SAFE_GET_PROC( qglGetLocalConstantIntegervEXT, PFNGLGETLOCALCONSTANTINTEGERVEXTPROC, "glGetLocalConstantIntegervEXT");
    SAFE_GET_PROC( qglGetLocalConstantFloatvEXT, PFNGLGETLOCALCONSTANTFLOATVEXTPROC, "glGetLocalConstantFloatvEXT");

    SAFE_GET_PROC( qglBeginFragShaderMTX, PFNGLBEGINFRAGSHADEREXTPROC, "glBeginFragShaderMTX");
    SAFE_GET_PROC( qglBindFragShaderMTX, PFNGLBINDFRAGSHADEREXTPROC, "glBindFragShaderMTX");
    SAFE_GET_PROC( qglDeleteFragShadersMTX, PFNGLDELETEFRAGSHADERSEXTPROC, "glDeleteFragShadersMTX");
    SAFE_GET_PROC( qglEndFragShaderMTX, PFNGLENDFRAGSHADEREXTPROC, "glEndFragShaderMTX");
    SAFE_GET_PROC( qglFragProcAlphaOp1MTX, PFNGLFRAGPROCALPHAOP1EXTPROC, "glFragProcAlphaOp1MTX");
    SAFE_GET_PROC( qglFragProcAlphaOp2MTX, PFNGLFRAGPROCALPHAOP2EXTPROC, "glFragProcAlphaOp2MTX");
    SAFE_GET_PROC( qglFragProcAlphaOp3MTX, PFNGLFRAGPROCALPHAOP3EXTPROC, "glFragProcAlphaOp3MTX");
    SAFE_GET_PROC( qglFragProcOpParamMTX, PFNGLFRAGPROCOPPARAMMTXEXTPROC, "glFragProcOpParamMTX");
    SAFE_GET_PROC( qglFragProcRGBAOp1MTX, PFNGLFRAGPROCRGBAOP1EXTPROC, "glFragProcRGBAOp1MTX");
    SAFE_GET_PROC( qglFragProcRGBAOp2MTX, PFNGLFRAGPROCRGBAOP2EXTPROC, "glFragProcRGBAOp2MTX");
    SAFE_GET_PROC( qglFragProcRGBAOp3MTX, PFNGLFRAGPROCRGBAOP3EXTPROC, "glFragProcRGBAOp3MTX");
    SAFE_GET_PROC( qglGenFragProcInvariantBVMTX, PFNGLGENFRAGPROCINVARIANTBVEXTPROC, "glGenFragProcInvariantbvMTX");
    SAFE_GET_PROC( qglGenFragProcInvariantDVMTX, PFNGLGENFRAGPROCINVARIANTDVEXTPROC, "glGenFragProcInvariantdvMTX");
    SAFE_GET_PROC( qglGenFragProcInvariantFVMTX, PFNGLGENFRAGPROCINVARIANTFVEXTPROC, "glGenFragProcInvariantfvMTX");
    SAFE_GET_PROC( qglGenFragProcInvariantIVMTX, PFNGLGENFRAGPROCINVARIANTIVEXTPROC, "glGenFragProcInvariantivMTX");
    SAFE_GET_PROC( qglGenFragProcInvariantSVMTX, PFNGLGENFRAGPROCINVARIANTSVEXTPROC, "glGenFragProcInvariantsvMTX");
    SAFE_GET_PROC( qglGenFragShadersMTX, PFNGLGENFRAGSHADERSEXTPROC, "glGenFragShadersMTX");
    SAFE_GET_PROC( qglIsFragShaderMTX, PFNGLISFRAGSHADEREXTPROC, "glIsFragShaderMTX");
    SAFE_GET_PROC( qglSetFragProcInvariantBVMTX, PFNGLSETFRAGPROCINVARIANTBVPROC, "glSetFragProcInvariantbvMTX");
    SAFE_GET_PROC( qglSetFragProcInvariantDVMTX, PFNGLSETFRAGPROCINVARIANTDVPROC, "glSetFragProcInvariantdvMTX");
    SAFE_GET_PROC( qglSetFragProcInvariantFVMTX, PFNGLSETFRAGPROCINVARIANTFVPROC, "glSetFragProcInvariantfvMTX");
    SAFE_GET_PROC( qglSetFragProcInvariantIVMTX, PFNGLSETFRAGPROCINVARIANTIVPROC, "glSetFragProcInvariantivMTX");
    SAFE_GET_PROC( qglSetFragProcInvariantSVMTX, PFNGLSETFRAGPROCINVARIANTSVPROC, "glSetFragProcInvariantsvMTX");
    SAFE_GET_PROC( qglTexAddressHintsMTX, PFNGLTEXADDRESSHINTSEXTPROC, "glTexAddressHintsMTX");
    SAFE_GET_PROC( qglTexAddressOp1MTX, PFNGLTEXADDRESSOP1EXTPROC, "glTexAddressOp1MTX");
    SAFE_GET_PROC( qglTexAddressOp2MTX, PFNGLTEXADDRESSOP2EXTPROC, "glTexAddressOp2MTX");
    SAFE_GET_PROC( qglTexAddressOp3MTX, PFNGLTEXADDRESSOP3EXTPROC, "glTexAddressOp3MTX");
    SAFE_GET_PROC( qglTexAddressOp4MTX, PFNGLTEXADDRESSOP4EXTPROC, "glTexAddressOp4MTX");
    SAFE_GET_PROC( qglTexAddressParamFVMTX, PFNGLTEXADDRESSPARAMFVEXTPROC, "glTexAddressParamfvMTX");
    SAFE_GET_PROC( qglTexAddressParamUIMTX, PFNGLTEXADDRESSPARAMUIEXTPROC, "glTexAddressParamuiMTX");
    SAFE_GET_PROC( qglGetFragProcInvariantBVMTX, PFNGLGETFRAGPROCINVARIANTBVEXTPROC, "glGetFragProcInvariantbvMTX");
    SAFE_GET_PROC( qglGetFragProcInvariantDVMTX, PFNGLGETFRAGPROCINVARIANTDVEXTPROC, "glGetFragProcInvariantdvMTX");
    SAFE_GET_PROC( qglGetFragProcInvariantFVMTX, PFNGLGETFRAGPROCINVARIANTFVEXTPROC, "glGetFragProcInvariantfvMTX");
    SAFE_GET_PROC( qglGetFragProcInvariantIVMTX, PFNGLGETFRAGPROCINVARIANTIVEXTPROC, "glGetFragProcInvariantivMTX");
    SAFE_GET_PROC( qglGetFragProcInvariantSVMTX, PFNGLGETFRAGPROCINVARIANTSVEXTPROC, "glGetFragProcInvariantsvMTX");

    SAFE_GET_PROC( qglGetTexAddressParamFVMTX, PFNGLGETTEXADDRESSPARAMFVEXTPROC, "glGetTexAddressParamfvMTX");
    SAFE_GET_PROC( qglGetTexAddressParamUIVMTX, PFNGLGETTEXADDRESSPARAMUIVEXTPROC, "glGetTexAddressParamuivMTX");
#endif

    //DEBUG
    // Con_Printf("Errors before creating shaders...\n%s\n",gluErrorString(glGetError()));
    // Con_Printf("----Fragment Shaders----\n");

	
    // Generate two shaders, diffuse and specular, with two variants for cube and 3d atten texture
    glEnable(GL_FRAGMENT_SHADER_MTX);
    
    //DEBUG
    // Con_Printf("Enabled GL_MTX_FRAGMENT_SHADER...\n%s\n",gluErrorString(glGetError()));


    fragment_shaders = qglGenFragShadersMTX(4);
    
    /*
      if (fragment_shaders == 0)
      {
      Con_Printf("Error: unable to allocate shaders: %s\n", gluErrorString(glGetError()));
      } else
      {
      Con_Printf("Allocated 4 fragment shaders\n");
      };	
    */


    // Diffuse shader 1 (3D atten)
    qglBindFragShaderMTX(fragment_shaders);
	
    //DEBUG
    // Con_Printf("Shader %i Bind Status: %s\n",fragment_shaders,gluErrorString(glGetError()));


    
    qglBeginFragShaderMTX();
    
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_TEMPORARY1_MTX, GL_COLOR0_MTX); //Matrox Work Around

    // tex t0
    qglTexAddressHintsMTX( GL_TEXTURE0, GL_HINT_TEXTURE_2D_MTX|GL_HINT_TRILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE0, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE0 );

    // tex t1
    qglTexAddressHintsMTX( GL_TEXTURE1, GL_HINT_TEXTURE_CUBE_MTX|GL_HINT_BILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE1, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE1 );

    // tex t2
    qglTexAddressHintsMTX( GL_TEXTURE2, GL_HINT_TEXTURE_2D_MTX|GL_HINT_TRILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE2, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE2 );

    // tex t3 (3D)
    qglTexAddressHintsMTX( GL_TEXTURE3, GL_HINT_TEXTURE_3D_MTX|GL_HINT_BILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE3, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE3 );

    //dp3_sat r0, t0_bx2, t1_bx2
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SATURATE_MTX, GL_TRUE);
    qglFragProcOpParamMTX(GL_ARGUMENT0_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcRGBAOp2MTX(GL_OP_DP3_MTX, GL_TEMPORARY0_MTX, GL_TEXTURE0, GL_TEXTURE1);

    //mul r0.rgb, r0, t2
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY0_MTX, GL_TEXTURE2);
    
    //+add_x4 r1.a, t0_bx2.b, t0_bx2.b
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SATURATE_MTX, GL_TRUE);
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SCALE_MTX, GL_SCALE_X4_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT0_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT0_MTX, GL_REPLICATE_MTX, GL_BLUE_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_REPLICATE_MTX, GL_BLUE_MTX);
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_IGNORED_MTX, GL_TEXTURE0); // [JPS] insert this
    qglFragProcAlphaOp2MTX(GL_OP_ADD_MTX, GL_TEMPORARY1_MTX, GL_TEXTURE0, GL_TEXTURE0);
    
    //mul r0, r0, v0
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY0_MTX, GL_COLOR0_MTX);
    
    //mul r0, r0, t3
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY0_MTX, GL_TEXTURE3);
    
    //mul r0, r0, r1.a
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SATURATE_MTX, GL_TRUE);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_REPLICATE_MTX, GL_ALPHA_MTX);
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_OUTPUT_COLOR_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY1_MTX);
    
    qglEndFragShaderMTX();
    
    // DEBUG - display status of shader creation
    // Con_Printf("Diffuse 1 Status: %s\n",gluErrorString(glGetError()));


    // Diffuse shader 2 (cube atten)
    qglBindFragShaderMTX(fragment_shaders+1);

    //DEBUG
    // Con_Printf("Shader %i Bind Status: %s\n",fragment_shaders+1,gluErrorString(glGetError()));

    qglBeginFragShaderMTX();
    
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_TEMPORARY1_MTX, GL_COLOR0_MTX); //Matrox Work Around
    // tex t0
    qglTexAddressHintsMTX( GL_TEXTURE0, GL_HINT_TEXTURE_2D_MTX|GL_HINT_TRILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE0, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE0 );

    // tex t1
    qglTexAddressHintsMTX( GL_TEXTURE1, GL_HINT_TEXTURE_CUBE_MTX|GL_HINT_BILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE1, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE1 );

    // tex t2
    qglTexAddressHintsMTX( GL_TEXTURE2, GL_HINT_TEXTURE_2D_MTX|GL_HINT_TRILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE2, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE2 );

    // tex t3 (cube)
    qglTexAddressHintsMTX( GL_TEXTURE3, GL_HINT_TEXTURE_CUBE_MTX|GL_HINT_BILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE3, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE3 );

    //dp3_sat r0, t0_bx2, t1_bx2
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SATURATE_MTX, GL_TRUE);
    qglFragProcOpParamMTX(GL_ARGUMENT0_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcRGBAOp2MTX(GL_OP_DP3_MTX, GL_TEMPORARY0_MTX, GL_TEXTURE0, GL_TEXTURE1);

    //mul r0.rgb, r0, t2
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY0_MTX, GL_TEXTURE2);

    //+add_x4 r1.a, t0_bx2.b, t0_bx2.b
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SATURATE_MTX, GL_TRUE);
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SCALE_MTX, GL_SCALE_X4_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT0_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT0_MTX, GL_REPLICATE_MTX, GL_BLUE_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_REPLICATE_MTX, GL_BLUE_MTX);
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_IGNORED_MTX, GL_TEXTURE0); // [JPS] insert this
    qglFragProcAlphaOp2MTX(GL_OP_ADD_MTX, GL_TEMPORARY1_MTX, GL_TEXTURE0, GL_TEXTURE0);

    //mul r0, r0, v0
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY0_MTX, GL_COLOR0_MTX);

    //mul r0, r0, t3
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY0_MTX, GL_TEXTURE3);

    //mul r0, r0, r1.a
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SATURATE_MTX, GL_TRUE);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_REPLICATE_MTX, GL_ALPHA_MTX);
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_IGNORED_MTX, GL_TEMPORARY0_MTX); // [JPS] insert this
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_OUTPUT_COLOR_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY1_MTX);

    qglEndFragShaderMTX();

    // DEBUG - display status of shader creation
    // Con_Printf("Diffuse 2 Status: %s\n",gluErrorString(glGetError()));

    // Specular shader 1 (3D atten)
    qglBindFragShaderMTX(fragment_shaders+2);
	
    //DEBUG
    // Con_Printf("Shader %i Bind Status: %s\n",fragment_shaders+2,gluErrorString(glGetError()));

    qglBeginFragShaderMTX();
    
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_TEMPORARY1_MTX, GL_COLOR0_MTX); //Matrox Work Around

    // tex t0
    qglTexAddressHintsMTX( GL_TEXTURE0, GL_HINT_TEXTURE_2D_MTX|GL_HINT_TRILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE0, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE0 );

    // tex t1
    qglTexAddressHintsMTX( GL_TEXTURE1, GL_HINT_TEXTURE_CUBE_MTX|GL_HINT_BILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE1, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE1 );

    // tex t2
    qglTexAddressHintsMTX( GL_TEXTURE2, GL_HINT_TEXTURE_2D_MTX|GL_HINT_TRILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE2, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE2 );

    // tex t3 (3D)
    qglTexAddressHintsMTX( GL_TEXTURE3, GL_HINT_TEXTURE_3D_MTX|GL_HINT_BILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE3, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE3 );

    //dp3_sat r0, t0_bx2, t1_bx2
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SATURATE_MTX, GL_TRUE);
    qglFragProcOpParamMTX(GL_ARGUMENT0_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcRGBAOp2MTX(GL_OP_DP3_MTX, GL_TEMPORARY0_MTX, GL_TEXTURE0, GL_TEXTURE1);

    //mul r1.rgb, t3, t0.a
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_REPLICATE_MTX, GL_ALPHA_MTX);
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY1_MTX, GL_TEXTURE3, GL_TEXTURE0);

    //+mad_x2_sat r1.a, r0.b, r0.b, c0
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SATURATE_MTX, GL_TRUE);
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SCALE_MTX, GL_SCALE_X2_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT0_MTX, GL_REPLICATE_MTX, GL_BLUE_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_REPLICATE_MTX, GL_BLUE_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT2_MTX, GL_CONVERT_MTX, GL_ARG_NEGATE_MTX);
    scaler = qglGenFragProcInvariantFVMTX(scalervalues);
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_IGNORED_MTX, GL_TEXTURE0); // [JPS] insert this
    qglFragProcAlphaOp3MTX(GL_OP_MAD_MTX, GL_TEMPORARY1_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY0_MTX, scaler);

    //mul r0, r1, v0
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY1_MTX, GL_COLOR0_MTX);

    //+mul r1.a, r1.a, r1.a
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_IGNORED_MTX, GL_TEXTURE0); // [JPS] insert this
    qglFragProcAlphaOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY1_MTX, GL_TEMPORARY1_MTX, GL_TEMPORARY1_MTX);

    //mul r1.a, r1.a, r1.a
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_IGNORED_MTX, GL_TEXTURE0); // [JPS] insert this
    qglFragProcAlphaOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY1_MTX, GL_TEMPORARY1_MTX, GL_TEMPORARY1_MTX);


    //mul r0, r0, r1.a
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_REPLICATE_MTX, GL_ALPHA_MTX);
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_OUTPUT_COLOR_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY1_MTX);

    qglEndFragShaderMTX();

    // DEBUG - display status of shader creation
    // Con_Printf("Specular 1 Status: %s\n",gluErrorString(glGetError()));

    // Specular shader 2 (cube atten)
    qglBindFragShaderMTX(fragment_shaders+3);

    //DEBUG
    // Con_Printf("Shader %i Bind Status: %s\n",fragment_shaders+3,gluErrorString(glGetError()));

    qglBeginFragShaderMTX();
	
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_TEMPORARY1_MTX, GL_COLOR0_MTX); //Matrox Work Around    
    // tex t0
    qglTexAddressHintsMTX( GL_TEXTURE0, GL_HINT_TEXTURE_2D_MTX|GL_HINT_TRILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE0, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE0 );

    // tex t1
    qglTexAddressHintsMTX( GL_TEXTURE1, GL_HINT_TEXTURE_CUBE_MTX|GL_HINT_BILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE1, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE1 );

    // tex t2
    qglTexAddressHintsMTX( GL_TEXTURE2, GL_HINT_TEXTURE_2D_MTX|GL_HINT_TRILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE2, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE2 );

    // tex t3 (cube)
    qglTexAddressHintsMTX( GL_TEXTURE3, GL_HINT_TEXTURE_CUBE_MTX|GL_HINT_BILINEAR_MTX);
    qglTexAddressParamUIMTX( GL_TEXTURE3, GL_TEXTURE_FORMAT_MTX, GL_TEX_UNSIGNED_MTX);
    qglTexAddressOp1MTX( GL_OP_TEX_MTX, GL_TEXTURE3 );

    //dp3_sat r0, t0_bx2, t1_bx2
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SATURATE_MTX, GL_TRUE);
    qglFragProcOpParamMTX(GL_ARGUMENT0_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_CONVERT_MTX, GL_ARG_SIGNED_MTX);
    qglFragProcRGBAOp2MTX(GL_OP_DP3_MTX, GL_TEMPORARY0_MTX, GL_TEXTURE0, GL_TEXTURE1);


    //mul r1.rgb, t3, t0.a
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_REPLICATE_MTX, GL_ALPHA_MTX);
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY1_MTX, GL_TEXTURE3, GL_TEXTURE0);

    //+mad_x2_sat r1.a, r0.b, r0.b, c0
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SATURATE_MTX, GL_TRUE);
    qglFragProcOpParamMTX(GL_RESULT_MTX, GL_SCALE_MTX, GL_SCALE_X2_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT0_MTX, GL_REPLICATE_MTX, GL_BLUE_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_REPLICATE_MTX, GL_BLUE_MTX);
    qglFragProcOpParamMTX(GL_ARGUMENT2_MTX, GL_CONVERT_MTX, GL_ARG_NEGATE_MTX);
    scaler = qglGenFragProcInvariantFVMTX(scalervalues);
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_IGNORED_MTX, GL_TEXTURE0); // [JPS] insert this
    qglFragProcAlphaOp3MTX(GL_OP_MAD_MTX, GL_TEMPORARY1_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY0_MTX, scaler);

    //mul r0, r1, v0
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY1_MTX, GL_COLOR0_MTX);

    //+mul r1.a, r1.a, r1.a
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_IGNORED_MTX, GL_TEXTURE0); // [JPS] insert this
    qglFragProcAlphaOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY1_MTX, GL_TEMPORARY1_MTX, GL_TEMPORARY1_MTX);
    
    //mul r1.a, r1.a, r1.a
    qglFragProcRGBAOp1MTX(GL_OP_MOV_MTX, GL_IGNORED_MTX, GL_TEXTURE0); // [JPS] insert this
    qglFragProcAlphaOp2MTX(GL_OP_MUL_MTX, GL_TEMPORARY1_MTX, GL_TEMPORARY1_MTX, GL_TEMPORARY1_MTX);

    //mul r0, r0, r1.a
    qglFragProcOpParamMTX(GL_ARGUMENT1_MTX, GL_REPLICATE_MTX, GL_ALPHA_MTX);
    qglFragProcRGBAOp2MTX(GL_OP_MUL_MTX, GL_OUTPUT_COLOR_MTX, GL_TEMPORARY0_MTX, GL_TEMPORARY1_MTX);

    qglEndFragShaderMTX();
	
    // DEBUG - display status of shader creation
    // Con_Printf("Specular 2 Status: %s\n",gluErrorString(glGetError()));
	
    glDisable(GL_FRAGMENT_SHADER_MTX);
    // Parhelia_checkerror();
    // Con_Printf("----Vertex Shaders----\n");
    // Generate vertex shader
    glEnable(GL_VERTEX_SHADER_EXT);
		
		
    //DEBUG
    // Con_Printf("Enabled GL_VERTEX_SHADER_EXT...\n%s\n",gluErrorString(glGetError()));

    vertex_shaders = qglGenVertexShadersEXT(2);

    lightPos = qglGenSymbolsEXT(GL_VECTOR_EXT, GL_INVARIANT_EXT, 
				GL_FULL_RANGE_EXT, 1);
    eyePos = qglGenSymbolsEXT(GL_VECTOR_EXT, GL_INVARIANT_EXT, 
			      GL_FULL_RANGE_EXT, 1);

     	
    /*
    //DEBUG
    if (fragment_shaders == 0)
    {
    Con_Printf("Error: unable to allocate shaders: %s\n", gluErrorString(glGetError()));
    } else
    {
    Con_Printf("Allocated 1 vertex shader\n");
    };
    */

    qglBindVertexShaderEXT(vertex_shaders);
    qglBeginVertexShaderEXT();

    //DEBUG
    // Con_Printf("Shader %i Bind Status: %s\n",fragment_shaders,gluErrorString(glGetError()));

    mvp           = qglBindParameterEXT( GL_MVP_MATRIX_EXT );
    modelview     = qglBindParameterEXT( GL_MODELVIEW_MATRIX );
    vertex        = qglBindParameterEXT( GL_CURRENT_VERTEX_EXT );
    normal        = qglBindParameterEXT( GL_CURRENT_NORMAL );
    color         = qglBindParameterEXT( GL_CURRENT_COLOR );
    texturematrix = qglBindTextureUnitParameterEXT( GL_TEXTURE3_ARB, GL_TEXTURE_MATRIX );
    texcoord0     = qglBindTextureUnitParameterEXT( GL_TEXTURE0_ARB, GL_CURRENT_TEXTURE_COORDS );
    texcoord1     = qglBindTextureUnitParameterEXT( GL_TEXTURE1_ARB, GL_CURRENT_TEXTURE_COORDS );
    texcoord2     = qglBindTextureUnitParameterEXT( GL_TEXTURE2_ARB, GL_CURRENT_TEXTURE_COORDS );
    texcoord3     = qglBindTextureUnitParameterEXT( GL_TEXTURE3_ARB, GL_CURRENT_TEXTURE_COORDS );
    fogstart      = qglBindParameterEXT( GL_FOG_START );
    Parhelia_checkerror();
    fogend        = qglBindParameterEXT( GL_FOG_END );
    Parhelia_checkerror();


    disttemp      = qglGenSymbolsEXT(GL_SCALAR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);
    disttemp2     = qglGenSymbolsEXT(GL_SCALAR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);
    zcomp         = qglGenSymbolsEXT(GL_VECTOR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);
    tempvec       = qglGenSymbolsEXT(GL_VECTOR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);


    // Generates a necessary input for the diffuse bumpmapping registers

    // Transform vertex to view-space
    qglShaderOp2EXT( GL_OP_MULTIPLY_MATRIX_EXT, GL_OUTPUT_VERTEX_EXT, mvp, vertex );
    
    // Transform vertex by texture matrix and copy to output
    qglShaderOp2EXT( GL_OP_MULTIPLY_MATRIX_EXT, GL_OUTPUT_TEXTURE_COORD3_EXT, texturematrix, vertex );

    // copy tex coords of unit 0 to unit 2
    qglShaderOp1EXT( GL_OP_MOV_EXT, GL_OUTPUT_TEXTURE_COORD0_EXT, texcoord0);
    qglShaderOp1EXT( GL_OP_MOV_EXT, GL_OUTPUT_TEXTURE_COORD2_EXT, texcoord0);
    qglShaderOp1EXT( GL_OP_MOV_EXT, GL_OUTPUT_COLOR0_EXT, color);

    // Transform vertex and take z for fog
    qglExtractComponentEXT( zcomp, modelview, 2);
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, zcomp, vertex );

    // calculate fog values end - z and end - start
    qglShaderOp2EXT( GL_OP_SUB_EXT, disttemp, fogend, disttemp);
    qglShaderOp2EXT( GL_OP_SUB_EXT, disttemp2, fogend, fogstart);

    // divide end - z by end - start, that's it
    qglShaderOp1EXT( GL_OP_RECIP_EXT, disttemp2, disttemp2);
    qglShaderOp2EXT( GL_OP_MUL_EXT, GL_OUTPUT_FOG_EXT, disttemp, disttemp2);

    // calculate light position
    qglShaderOp2EXT( GL_OP_SUB_EXT, zcomp, lightPos, vertex);
    qglShaderOp2EXT( GL_OP_DOT3_EXT, disttemp, zcomp, zcomp);
    qglShaderOp1EXT( GL_OP_RECIP_SQRT_EXT, disttemp, disttemp);
    qglShaderOp2EXT( GL_OP_MUL_EXT, tempvec, zcomp, disttemp);

    // Normalized light vec now in tempvec, transform to tex1
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord1);
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD1_EXT, disttemp, 0);
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord2);
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD1_EXT, disttemp, 1);
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord3);
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD1_EXT, disttemp, 2);

    qglEndVertexShaderEXT();


    // The same for specular
    qglBindVertexShaderEXT(vertex_shaders+1);
    qglBeginVertexShaderEXT();

    //DEBUG
    // Con_Printf("Shader %i Bind Status: %s\n",fragment_shaders,gluErrorString(glGetError()));

    mvp           = qglBindParameterEXT( GL_MVP_MATRIX_EXT );
    modelview     = qglBindParameterEXT( GL_MODELVIEW_MATRIX );
    vertex        = qglBindParameterEXT( GL_CURRENT_VERTEX_EXT );
    normal        = qglBindParameterEXT( GL_CURRENT_NORMAL );
    color         = qglBindParameterEXT( GL_CURRENT_COLOR );
    texturematrix = qglBindTextureUnitParameterEXT( GL_TEXTURE3_ARB, GL_TEXTURE_MATRIX );
    texcoord0     = qglBindTextureUnitParameterEXT( GL_TEXTURE0_ARB, GL_CURRENT_TEXTURE_COORDS );
    texcoord1     = qglBindTextureUnitParameterEXT( GL_TEXTURE1_ARB, GL_CURRENT_TEXTURE_COORDS );
    texcoord2     = qglBindTextureUnitParameterEXT( GL_TEXTURE2_ARB, GL_CURRENT_TEXTURE_COORDS );
    texcoord3     = qglBindTextureUnitParameterEXT( GL_TEXTURE3_ARB, GL_CURRENT_TEXTURE_COORDS );
    fogstart      = qglBindParameterEXT( GL_FOG_START );
    Parhelia_checkerror();
    fogend        = qglBindParameterEXT( GL_FOG_END );
    Parhelia_checkerror();


    disttemp      = qglGenSymbolsEXT(GL_SCALAR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);
    disttemp2     = qglGenSymbolsEXT(GL_SCALAR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);
    zcomp         = qglGenSymbolsEXT(GL_VECTOR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);
    tempvec       = qglGenSymbolsEXT(GL_VECTOR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);


    // Generates a necessary input for the diffuse bumpmapping registers

    // Transform vertex to view-space
    qglShaderOp2EXT( GL_OP_MULTIPLY_MATRIX_EXT, GL_OUTPUT_VERTEX_EXT, mvp, vertex );
    
    // Transform vertex by texture matrix and copy to output
    qglShaderOp2EXT( GL_OP_MULTIPLY_MATRIX_EXT, GL_OUTPUT_TEXTURE_COORD3_EXT, texturematrix, vertex );

    // copy tex coords of unit 0 to unit 2
    qglShaderOp1EXT( GL_OP_MOV_EXT, GL_OUTPUT_TEXTURE_COORD0_EXT, texcoord0);
    qglShaderOp1EXT( GL_OP_MOV_EXT, GL_OUTPUT_TEXTURE_COORD2_EXT, texcoord0);
    qglShaderOp1EXT( GL_OP_MOV_EXT, GL_OUTPUT_COLOR0_EXT, color);

    // Transform vertex and take z for fog
    qglExtractComponentEXT( zcomp, modelview, 2);
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, zcomp, vertex );

    // calculate fog values end - z and end - start
    qglShaderOp2EXT( GL_OP_SUB_EXT, disttemp, fogend, disttemp);
    qglShaderOp2EXT( GL_OP_SUB_EXT, disttemp2, fogend, fogstart);

    // divide end - z by end - start, that's it
    qglShaderOp1EXT( GL_OP_RECIP_EXT, disttemp2, disttemp2);
    qglShaderOp2EXT( GL_OP_MUL_EXT, GL_OUTPUT_FOG_EXT, disttemp, disttemp2);

    // calculate halfvec
    qglShaderOp2EXT( GL_OP_SUB_EXT, tempvec, lightPos, vertex);
    qglShaderOp2EXT( GL_OP_DOT3_EXT, disttemp, tempvec, tempvec);
    qglShaderOp1EXT( GL_OP_RECIP_SQRT_EXT, disttemp, disttemp);
    qglShaderOp2EXT( GL_OP_MUL_EXT, tempvec, tempvec, disttemp);
    qglShaderOp2EXT( GL_OP_SUB_EXT, zcomp, eyePos, vertex);
    qglShaderOp2EXT( GL_OP_ADD_EXT, tempvec, tempvec, zcomp);
    qglShaderOp2EXT( GL_OP_DOT3_EXT, disttemp, tempvec, tempvec);
    qglShaderOp1EXT( GL_OP_RECIP_SQRT_EXT, disttemp, disttemp);
    qglShaderOp2EXT( GL_OP_MUL_EXT, tempvec, tempvec, disttemp);

    // Normalized half vec now in tempvec, transform to tex1
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord1);
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD1_EXT, disttemp, 0);
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord2);
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD1_EXT, disttemp, 1);
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord3);
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD1_EXT, disttemp, 2);

    qglEndVertexShaderEXT();



    // DEBUG - display status of shader creation
    // Con_Printf("Vertex Status: %s\n",gluErrorString(glGetError()));
        
		
    glDisable(GL_VERTEX_SHADER_EXT);
    // Parhelia_checkerror();

}

/*
  Pixel shader for diffuse bump mapping does diffuse bumpmapping with norm
  cube, self shadowing & dist attent in 1 pass
*/
void Parhelia_EnableDiffuseShader(const transform_t *tr, const lightobject_t *lo)
{
	float temp[4];

    //tex 0 = normal map
    //tex 1 = normalization cube map (tangent space light vector)
    //tex 2 = color map
    //tex 3 = (attenuation or light filter, depends on light settings but the
    //        actual register combiner setup does not change only the bound 
    //        texture)

    glEnable(GL_VERTEX_SHADER_EXT);
    qglBindVertexShaderEXT( vertex_shaders );
    temp[0] = currentshadowlight->origin[0];
    temp[1] = currentshadowlight->origin[1];
    temp[2] = currentshadowlight->origin[2];
    temp[3] = 1.0f;
    qglSetInvariantEXT(lightPos, GL_FLOAT, &temp[0]);

    temp[0] = r_refdef.vieworg[0];
    temp[1] = r_refdef.vieworg[1];
    temp[2] = r_refdef.vieworg[2];
    qglSetInvariantEXT(eyePos, GL_FLOAT, &temp[0]);
    Parhelia_checkerror();

    glEnable(GL_FRAGMENT_SHADER_MTX );

    GL_SelectTexture(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_CUBE_MAP_ARB);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, normcube_texture_object);

    GL_SelectTexture(GL_TEXTURE2_ARB);
    glEnable(GL_TEXTURE_2D);

    GL_SelectTexture(GL_TEXTURE3_ARB);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    if (currentshadowlight->filtercube)
    {
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, currentshadowlight->filtercube);
	GL_SetupCubeMapMatrix(tr);

        qglBindFragShaderMTX( fragment_shaders + 1 );
        Parhelia_checkerror();
      
    }
    else
    {
	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, atten3d_texture_object);

	glTranslatef(0.5,0.5,0.5);
	glScalef(0.5,0.5,0.5);
        glScalef(1.0f/(currentshadowlight->radiusv[0]),
                 1.0f/(currentshadowlight->radiusv[1]),
	         1.0f/(currentshadowlight->radiusv[2]));
		GL_SetupAttenMatrix(tr);

        qglBindFragShaderMTX( fragment_shaders );
        Parhelia_checkerror();
    }

    GL_SelectTexture(GL_TEXTURE0_ARB);

}

void Parhelia_DisableDiffuseShader()
{
    //tex 0 = normal map
    //tex 1 = normalization cube map (tangent space light vector)
    //tex 2 = color map
    //tex 3 = (attenuation or light filter, depends on light settings)

    glDisable(GL_VERTEX_SHADER_EXT);
    glDisable(GL_FRAGMENT_SHADER_MTX );

    GL_SelectTexture(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_CUBE_MAP_ARB);

    GL_SelectTexture(GL_TEXTURE2_ARB);
    glDisable(GL_TEXTURE_2D);

    GL_SelectTexture(GL_TEXTURE3_ARB);
    if (currentshadowlight->filtercube)
    {
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);
    }
    else
    {
	glDisable(GL_TEXTURE_3D);
    }
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    GL_SelectTexture(GL_TEXTURE0_ARB);
}

void Parhelia_EnableSpecularShader(const transform_t *tr, const lightobject_t *lo,
				   qboolean alias)
{
    vec3_t scaler = {0.5f, 0.5f, 0.5f};
	float temp[4];

    glEnable(GL_VERTEX_SHADER_EXT);
    qglBindVertexShaderEXT( vertex_shaders+1 );
    temp[0] = currentshadowlight->origin[0];
    temp[1] = currentshadowlight->origin[1];
    temp[2] = currentshadowlight->origin[2];
    temp[3] = 1.0f;
    qglSetInvariantEXT(lightPos, GL_FLOAT, &temp[0]);

    temp[0] = r_refdef.vieworg[0];
    temp[1] = r_refdef.vieworg[1];
    temp[2] = r_refdef.vieworg[2];
    qglSetInvariantEXT(eyePos, GL_FLOAT, &temp[0]);
    Parhelia_checkerror();

    glEnable(GL_FRAGMENT_SHADER_MTX );

    //tex 0 = normal map
    //tex 1 = normalization cube map (tangent space half angle)
    //tex 2 = color map
    //tex 3 = (attenuation or light filter, depends on light settings)

    GL_SelectTexture(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_CUBE_MAP_ARB);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, normcube_texture_object);

    GL_SelectTexture(GL_TEXTURE2_ARB);
    glEnable(GL_TEXTURE_2D);

    GL_SelectTexture(GL_TEXTURE3_ARB);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();

    if (currentshadowlight->filtercube)
    {
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, currentshadowlight->filtercube);
	GL_SetupCubeMapMatrix(tr);

        qglBindFragShaderMTX( fragment_shaders + 3 );
        Parhelia_checkerror();
    }
    else
    {
	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, atten3d_texture_object);

	glTranslatef(0.5,0.5,0.5);
	glScalef(0.5,0.5,0.5);
        glScalef(1.0f/(currentshadowlight->radiusv[0]),
                 1.0f/(currentshadowlight->radiusv[1]),
	         1.0f/(currentshadowlight->radiusv[2]));
		GL_SetupAttenMatrix(tr);

        qglBindFragShaderMTX( fragment_shaders + 2 );
        Parhelia_checkerror();
    }

    GL_SelectTexture(GL_TEXTURE0_ARB);

}

void Parhelia_EnableAttentShader(const transform_t *tr)
{
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.5,0.5,0.5);
    glScalef(0.5,0.5,0.5);
    glScalef(1.0f/(currentshadowlight->radiusv[0]),
             1.0f/(currentshadowlight->radiusv[1]),
             1.0f/(currentshadowlight->radiusv[2]));
	GL_SetupAttenMatrix(tr);
	
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, atten3d_texture_object);
}

void Parhelia_DisableAttentShader()
{
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_TEXTURE_3D);
    glEnable(GL_TEXTURE_2D);
}



/************************

Generic triangle list routines

*************************/

void FormatError(); // In gl_bumpgf.c

void Parhelia_sendTriangleListWV(const vertexdef_t *verts, int *indecies,
				 int numIndecies)
{

    GL_VertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    GL_TexCoordPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    //draw them
    glDrawElements(GL_TRIANGLES, numIndecies, GL_UNSIGNED_INT, indecies);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Parhelia_sendTriangleListTA(const vertexdef_t *verts, int *indecies,
				 int numIndecies)
{
    GL_VertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    //Check the input vertices
    if (IsNullDriver(verts->texcoords)) FormatError();
    if (IsNullDriver(verts->binormals)) FormatError();
    if (IsNullDriver(verts->tangents)) FormatError();
    if (IsNullDriver(verts->normals)) FormatError();

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    GL_TexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE1_ARB);
    GL_TexCoordPointer(3, GL_FLOAT, verts->tangentstride, verts->tangents);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE2_ARB);
    GL_TexCoordPointer(3, GL_FLOAT, verts->binormalstride, verts->binormals);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE3_ARB);
    GL_TexCoordPointer(3, GL_FLOAT, verts->normalstride, verts->normals);
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

void Parhelia_drawTriangleListBump (const vertexdef_t *verts, int *indecies,
				    int numIndecies, shader_t *shader,
				    const transform_t *tr, const lightobject_t *lo)
{
    if (currentshadowlight->filtercube)
    {
	//draw attent into dest alpha
	GL_DrawAlpha();
	Parhelia_EnableAttentShader(tr);
	Parhelia_sendTriangleListWV(verts,indecies,numIndecies);
	Parhelia_DisableAttentShader();
	GL_ModulateAlphaDrawColor();
    }
    else
    {
	GL_AddColor();
    }
    glColor3fv(&currentshadowlight->color[0]);

    Parhelia_EnableSpecularShader(tr, lo, true);
    //bind the correct texture
    GL_SelectTexture(GL_TEXTURE0_ARB);
    if (shader->numbumpstages > 0)
	GL_Bind(shader->bumpstages[0].texture[0]->texnum);
    GL_SelectTexture(GL_TEXTURE2_ARB);
    if (shader->numcolorstages > 0)
	GL_Bind(shader->colorstages[0].texture[0]->texnum);

    Parhelia_sendTriangleListTA(verts,indecies,numIndecies);
    Parhelia_DisableDiffuseShader();

    Parhelia_EnableDiffuseShader(tr, lo);
    //bind the correct texture
    GL_SelectTexture(GL_TEXTURE0_ARB);
    if (shader->numbumpstages > 0)
	GL_Bind(shader->bumpstages[0].texture[0]->texnum);
    GL_SelectTexture(GL_TEXTURE2_ARB);
    if (shader->numcolorstages > 0)
	GL_Bind(shader->colorstages[0].texture[0]->texnum);

    Parhelia_sendTriangleListTA(verts,indecies,numIndecies);
    Parhelia_DisableDiffuseShader();
}

void Parhelia_drawTriangleListBase (vertexdef_t *verts, int *indecies,
				    int numIndecies, shader_t *shader,
                                    int lightMapIndex)
{
    int i;

    GL_VertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

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
    else if (shader->flags & SURF_PPLIGHT)
    {
	glColor3f(0,0,0);

		if (shader->colorstages[0].src_blend >= 0) {
			glBlendFunc(shader->colorstages[0].src_blend, shader->colorstages[0].dst_blend);
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_BLEND);
		}

	glDisable(GL_TEXTURE_2D);
	glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
	glEnable(GL_TEXTURE_2D);
    }

    if (!shader->cull)
    { 
	glEnable(GL_CULL_FACE); 
    } 

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
}

/*************************

Generic world surfaces routines

**************************/

void Parhelia_sendSurfacesBase(msurface_t **surfs, int numSurfaces,
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

void Parhelia_drawSurfaceListBase (vertexdef_t* verts, msurface_t** surfs,
				   int numSurfaces, shader_t* shader)
{
    int i;

    GL_VertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    GL_TexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glColor3ub(255,255,255);

    if (!shader->cull)
    {
	glDisable(GL_CULL_FACE);
	//Con_Printf("Cullstuff %s\n",shader->name);
    }

    for (i = 0; i < shader->numstages; i++)
    {
	SH_SetupSimpleStage(&shader->stages[i]);
	Parhelia_sendSurfacesBase(surfs, numSurfaces, false);
	glPopMatrix();
    }

    if (!IsNullDriver(verts->lightmapcoords) && (shader->flags & SURF_PPLIGHT))
    {
	qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	GL_TexCoordPointer(2, GL_FLOAT, verts->lightmapstride,
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

	Parhelia_sendSurfacesBase(surfs, numSurfaces, true);

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

void Parhelia_sendSurfacesTA(msurface_t** surfs, int numSurfaces)
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
	    GL_SelectTexture(GL_TEXTURE2_ARB);
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


void Parhelia_sendSurfacesPlain(msurface_t** surfs, int numSurfaces)
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

void Parhelia_drawSurfaceListBump (vertexdef_t *verts, msurface_t **surfs,
				   int numSurfaces,const transform_t *tr, const lightobject_t *lo)
{
    GL_VertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    if (currentshadowlight->filtercube)
    {
	//draw attent into dest alpha
	GL_DrawAlpha();
	Parhelia_EnableAttentShader(tr);
		
	GL_TexCoordPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
	Parhelia_sendSurfacesPlain(surfs,numSurfaces);

	Parhelia_DisableAttentShader();
	GL_ModulateAlphaDrawColor();
    }
    else
    {
	GL_AddColor();
    }
    glColor3fv(&currentshadowlight->color[0]);

    Parhelia_EnableSpecularShader(tr, lo, true);

    GL_TexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
    Parhelia_sendSurfacesTA(surfs,numSurfaces);
    Parhelia_DisableDiffuseShader();

    Parhelia_EnableDiffuseShader(tr, lo);
    Parhelia_sendSurfacesTA(surfs,numSurfaces);
    Parhelia_DisableDiffuseShader();

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Parhelia_freeDriver(void)
{
    //nothing here...
}


void BUMP_InitParhelia(void)
{
    GLint errPos, errCode;
    const GLubyte *errString;

    if ( gl_cardtype != PARHELIA ) return;

    Parhelia_CreateShaders();

    //bind the correct stuff to the bump mapping driver
    gl_bumpdriver.drawSurfaceListBase = Parhelia_drawSurfaceListBase;
    gl_bumpdriver.drawSurfaceListBump = Parhelia_drawSurfaceListBump;
    gl_bumpdriver.drawTriangleListBase = Parhelia_drawTriangleListBase;
    gl_bumpdriver.drawTriangleListBump = Parhelia_drawTriangleListBump;
    gl_bumpdriver.freeDriver = Parhelia_freeDriver;
}
