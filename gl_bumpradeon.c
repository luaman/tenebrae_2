/*
Copyright (C) 2001-2002 Charles Hollemeersch
Radeon Version (C) 2002-2003 Jarno Paananen

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

Same as gl_bumpmap.c but Radeon 8500+ optimized 
These routines require 6 texture units, vertex shader and pixel shader

This could be further optimized for Radeon 9700 (specular exponent for
example), but would need better documentation and extension.

All lights not require only 1 pass:
1 diffuse + specular with optional light filter

*/

#include "quakedef.h"

#include "glATI.h"

PFNGLGENFRAGMENTSHADERSATIPROC		qglGenFragmentShadersATI = NULL;
PFNGLBINDFRAGMENTSHADERATIPROC		qglBindFragmentShaderATI = NULL;
PFNGLDELETEFRAGMENTSHADERATIPROC 	qglDeleteFragmentShaderATI = NULL;
PFNGLBEGINFRAGMENTSHADERATIPROC 	qglBeginFragmentShaderATI = NULL;
PFNGLENDFRAGMENTSHADERATIPROC 		qglEndFragmentShaderATI = NULL;
PFNGLPASSTEXCOORDATIPROC		qglPassTexCoordATI = NULL;
PFNGLSAMPLEMAPATIPROC			qglSampleMapATI = NULL;
PFNGLCOLORFRAGMENTOP1ATIPROC		qglColorFragmentOp1ATI = NULL;
PFNGLCOLORFRAGMENTOP2ATIPROC		qglColorFragmentOp2ATI = NULL;
PFNGLCOLORFRAGMENTOP3ATIPROC		qglColorFragmentOp3ATI = NULL;
PFNGLALPHAFRAGMENTOP1ATIPROC		qglAlphaFragmentOp1ATI = NULL;
PFNGLALPHAFRAGMENTOP2ATIPROC		qglAlphaFragmentOp2ATI = NULL;
PFNGLALPHAFRAGMENTOP3ATIPROC		qglAlphaFragmentOp3ATI = NULL;
PFNGLSETFRAGMENTSHADERCONSTANTATIPROC	qglSetFragmentShaderConstantATI = NULL;
PFNGLBEGINVERTEXSHADEREXTPROC		qglBeginVertexShaderEXT = NULL;
PFNGLENDVERTEXSHADEREXTPROC		qglEndVertexShaderEXT = NULL;
PFNGLBINDVERTEXSHADEREXTPROC		qglBindVertexShaderEXT = NULL;
PFNGLGENVERTEXSHADERSEXTPROC		qglGenVertexShadersEXT = NULL;
PFNGLDELETEVERTEXSHADEREXTPROC		qglDeleteVertexShaderEXT = NULL;
PFNGLSHADEROP1EXTPROC			qglShaderOp1EXT = NULL;
PFNGLSHADEROP2EXTPROC			qglShaderOp2EXT = NULL;
PFNGLSHADEROP3EXTPROC			qglShaderOp3EXT = NULL;
PFNGLSWIZZLEEXTPROC			qglSwizzleEXT = NULL;
PFNGLWRITEMASKEXTPROC			qglWriteMaskEXT = NULL;
PFNGLINSERTCOMPONENTEXTPROC		qglInsertComponentEXT = NULL;
PFNGLEXTRACTCOMPONENTEXTPROC		qglExtractComponentEXT = NULL;
PFNGLGENSYMBOLSEXTPROC			qglGenSymbolsEXT = NULL;
PFNGLSETINVARIANTEXTPROC		qglSetInvariantEXT = NULL;
PFNGLSETLOCALCONSTANTEXTPROC		qglSetLocalConstantEXT = NULL;
PFNGLVARIANTBVEXTPROC			qglVariantbvEXT = NULL;
PFNGLVARIANTSVEXTPROC			qglVariantsvEXT = NULL;
PFNGLVARIANTIVEXTPROC			qglVariantivEXT = NULL;
PFNGLVARIANTFVEXTPROC			qglVariantfvEXT = NULL;
PFNGLVARIANTDVEXTPROC			qglVariantdvEXT = NULL;
PFNGLVARIANTUBVEXTPROC			qglVariantubvEXT = NULL;
PFNGLVARIANTUSVEXTPROC			qglVariantusvEXT = NULL;
PFNGLVARIANTUIVEXTPROC			qglVariantuivEXT = NULL;
PFNGLVARIANTPOINTEREXTPROC		qglVariantPointerEXT = NULL;
PFNGLENABLEVARIANTCLIENTSTATEEXTPROC	qglEnableVariantClientStateEXT = NULL;
PFNGLDISABLEVARIANTCLIENTSTATEEXTPROC	qglDisableVariantClientStateEXT = NULL;
PFNGLBINDLIGHTPARAMETEREXTPROC		qglBindLightParameterEXT = NULL;
PFNGLBINDMATERIALPARAMETEREXTPROC	qglBindMaterialParameterEXT = NULL;
PFNGLBINDTEXGENPARAMETEREXTPROC		qglBindTexGenParameterEXT = NULL;
PFNGLBINDTEXTUREUNITPARAMETEREXTPROC	qglBindTextureUnitParameterEXT = NULL;
PFNGLBINDPARAMETEREXTPROC		qglBindParameterEXT = NULL;
PFNGLISVARIANTENABLEDEXTPROC		qglIsVariantEnabledEXT = NULL;
PFNGLGETVARIANTBOOLEANVEXTPROC		qglGetVariantBooleanvEXT = NULL;
PFNGLGETVARIANTINTEGERVEXTPROC		qglGetVariantIntegervEXT = NULL;
PFNGLGETVARIANTFLOATVEXTPROC		qglGetVariantFloatvEXT = NULL;
PFNGLGETVARIANTPOINTERVEXTPROC		qglGetVariantPointervEXT = NULL;
PFNGLGETINVARIANTBOOLEANVEXTPROC	qglGetInvariantBooleanvEXT = NULL;
PFNGLGETINVARIANTINTEGERVEXTPROC	qglGetInvariantIntegervEXT = NULL;
PFNGLGETINVARIANTFLOATVEXTPROC		qglGetInvariantFloatvEXT = NULL;
PFNGLGETLOCALCONSTANTBOOLEANVEXTPROC	qglGetLocalConstantBooleanvEXT = NULL;
PFNGLGETLOCALCONSTANTINTEGERVEXTPROC	qglGetLocalConstantIntegervEXT = NULL;
PFNGLGETLOCALCONSTANTFLOATVEXTPROC	qglGetLocalConstantFloatvEXT = NULL;

PFNGLPNTRIANGLESIATIPROC qglPNTrianglesiATI = NULL;
PFNGLPNTRIANGLESFATIPROC qglPNTrianglesfATI = NULL;

PFNGLSTENCILOPSEPARATEATIPROC qglStencilOpSeparateATI = NULL;
PFNGLSTENCILFUNCSEPARATEATIPROC qglStencilFuncSeparateATI = NULL;

static unsigned int fragment_shaders;
static unsigned int vertex_shaders;
static GLuint lightPos, eyePos; // vertex shader constants

//#define RADEONDEBUG

#ifdef RADEONDEBUG
void Radeon_checkerror()
{
    GLuint error = glGetError();
    if ( error != GL_NO_ERROR )
    {
        _asm { int 3 };
    }
}
#else

#define Radeon_checkerror() do { } while(0)

#endif

// <AWE> We look up the symbols here, this way we don't need to prototype all the "qgl" functions!
#if defined (__APPLE__) || defined (MACOSX)

extern void *	GL_GetProcAddress (const char *theName, qboolean theSafeMode);

qboolean	Radeon_LookupSymbols (void)
{
    qglGenFragmentShadersATI = GL_GetProcAddress ("glGenFragmentShadersATI", false);
    qglBindFragmentShaderATI = GL_GetProcAddress ("glBindFragmentShaderATI", false);
    qglDeleteFragmentShaderATI = GL_GetProcAddress ("glDeleteFragmentShaderATI", false);
    qglBeginFragmentShaderATI = GL_GetProcAddress ("glBeginFragmentShaderATI", false);
    qglEndFragmentShaderATI = GL_GetProcAddress ("glEndFragmentShaderATI", false);
    qglPassTexCoordATI = GL_GetProcAddress ("glPassTexCoordATI", false);
    qglSampleMapATI = GL_GetProcAddress ("glSampleMapATI", false);
    qglColorFragmentOp1ATI = GL_GetProcAddress ("glColorFragmentOp1ATI", false);
    qglColorFragmentOp2ATI = GL_GetProcAddress ("glColorFragmentOp2ATI", false);
    qglColorFragmentOp3ATI = GL_GetProcAddress ("glColorFragmentOp3ATI", false);
    qglAlphaFragmentOp1ATI = GL_GetProcAddress ("glAlphaFragmentOp1ATI", false);
    qglAlphaFragmentOp2ATI = GL_GetProcAddress ("glAlphaFragmentOp2ATI", false);
    qglAlphaFragmentOp3ATI = GL_GetProcAddress ("glAlphaFragmentOp3ATI", false);
    qglSetFragmentShaderConstantATI = GL_GetProcAddress ("glSetFragmentShaderConstantATI", false);
    qglBeginVertexShaderEXT = GL_GetProcAddress ("glBeginVertexShaderEXT", false);            
    qglEndVertexShaderEXT = GL_GetProcAddress ("glEndVertexShaderEXT", false);          
    qglBindVertexShaderEXT = GL_GetProcAddress ("glBindVertexShaderEXT", false);               
    qglGenVertexShadersEXT = GL_GetProcAddress ("glGenVertexShadersEXT", false);               
    qglDeleteVertexShaderEXT = GL_GetProcAddress ("glDeleteVertexShaderEXT", false);         
    qglShaderOp1EXT = GL_GetProcAddress ("glShaderOp1EXT", false);            
    qglShaderOp2EXT = GL_GetProcAddress ("glShaderOp2EXT", false);            
    qglShaderOp3EXT = GL_GetProcAddress ("glShaderOp3EXT", false);            
    qglSwizzleEXT = GL_GetProcAddress ("glSwizzleEXT", false);          
    qglWriteMaskEXT = GL_GetProcAddress ("glWriteMaskEXT", false);            
    qglInsertComponentEXT = GL_GetProcAddress ("glInsertComponentEXT", false);          
    qglExtractComponentEXT = GL_GetProcAddress ("glExtractComponentEXT", false);               
    qglGenSymbolsEXT = GL_GetProcAddress ("glGenSymbolsEXT", false);         
    qglSetInvariantEXT = GL_GetProcAddress ("glSetInvariantEXT", false);           
    qglSetLocalConstantEXT = GL_GetProcAddress ("glSetLocalConstantEXT", false);               
    qglVariantbvEXT = GL_GetProcAddress ("glVariantbvEXT", false);            
    qglVariantsvEXT = GL_GetProcAddress ("glVariantsvEXT", false);            
    qglVariantivEXT = GL_GetProcAddress ("glVariantivEXT", false);            
    qglVariantfvEXT = GL_GetProcAddress ("glVariantfvEXT", false);            
    qglVariantdvEXT = GL_GetProcAddress ("glVariantdvEXT", false);            
    qglVariantubvEXT = GL_GetProcAddress ("glVariantubvEXT", false);         
    qglVariantusvEXT = GL_GetProcAddress ("glVariantusvEXT", false);         
    qglVariantuivEXT = GL_GetProcAddress ("glVariantuivEXT", false);         
    qglVariantPointerEXT = GL_GetProcAddress ("glVariantPointerEXT", false);             
    qglEnableVariantClientStateEXT = GL_GetProcAddress ("glEnableVariantClientStateEXT", false);
    qglDisableVariantClientStateEXT = GL_GetProcAddress ("glDisableVariantClientStateEXT", false);
    qglBindLightParameterEXT = GL_GetProcAddress ("glBindLightParameterEXT", false);
    qglBindMaterialParameterEXT = GL_GetProcAddress ("glBindMaterialParameterEXT", false);
    qglBindTexGenParameterEXT = GL_GetProcAddress ("glBindTexGenParameterEXT", false);
    qglBindTextureUnitParameterEXT = GL_GetProcAddress ("glBindTextureUnitParameterEXT", false);
    qglBindParameterEXT = GL_GetProcAddress ("glBindParameterEXT", false);
    qglIsVariantEnabledEXT = GL_GetProcAddress ("glIsVariantEnabledEXT", false);
    qglGetVariantBooleanvEXT = GL_GetProcAddress ("glGetVariantBooleanvEXT", false);
    qglGetVariantIntegervEXT = GL_GetProcAddress ("glGetVariantIntegervEXT", false);
    qglGetVariantFloatvEXT = GL_GetProcAddress ("glGetVariantFloatvEXT", false);
    qglGetVariantPointervEXT = GL_GetProcAddress ("glGetVariantPointervEXT", false);
    qglGetInvariantBooleanvEXT = GL_GetProcAddress ("glGetInvariantBooleanvEXT", false);
    qglGetInvariantIntegervEXT = GL_GetProcAddress ("glGetInvariantIntegervEXT", false);
    qglGetInvariantFloatvEXT = GL_GetProcAddress ("glGetInvariantFloatvEXT", false);
    qglGetLocalConstantBooleanvEXT = GL_GetProcAddress ("glGetLocalConstantBooleanvEXT", false);
    qglGetLocalConstantIntegervEXT = GL_GetProcAddress ("glGetLocalConstantIntegervEXT", false);
    qglGetLocalConstantFloatvEXT = GL_GetProcAddress ("glGetLocalConstantFloatvEXT", false);

    if (qglGenFragmentShadersATI != NULL &&
        qglBindFragmentShaderATI != NULL &&
        qglDeleteFragmentShaderATI != NULL &&
        qglBeginFragmentShaderATI != NULL &&
        qglEndFragmentShaderATI != NULL &&
        qglPassTexCoordATI != NULL &&
        qglSampleMapATI != NULL &&
        qglColorFragmentOp1ATI != NULL &&
        qglColorFragmentOp2ATI != NULL &&
        qglColorFragmentOp3ATI != NULL &&
        qglAlphaFragmentOp1ATI != NULL &&
        qglAlphaFragmentOp2ATI != NULL &&
        qglAlphaFragmentOp3ATI != NULL &&
        qglSetFragmentShaderConstantATI != NULL &&
        qglBeginVertexShaderEXT != NULL &&          
        qglEndVertexShaderEXT != NULL &&         
        qglBindVertexShaderEXT != NULL &&               
        qglGenVertexShadersEXT != NULL &&               
        qglDeleteVertexShaderEXT != NULL &&         
        qglShaderOp1EXT != NULL &&           
        qglShaderOp2EXT != NULL &&            
        qglShaderOp3EXT != NULL &&           
        qglSwizzleEXT != NULL &&         
        qglWriteMaskEXT != NULL &&             
        qglInsertComponentEXT != NULL &&           
        qglExtractComponentEXT != NULL &&               
        qglGenSymbolsEXT != NULL &&        
        qglSetInvariantEXT != NULL &&            
        qglSetLocalConstantEXT != NULL &&                
        qglVariantbvEXT != NULL &&             
        qglVariantsvEXT != NULL &&            
        qglVariantivEXT != NULL &&             
        qglVariantfvEXT != NULL &&             
        qglVariantdvEXT != NULL &&            
        qglVariantubvEXT != NULL &&          
        qglVariantusvEXT != NULL &&         
        qglVariantuivEXT != NULL &&         
        qglVariantPointerEXT != NULL &&             
        qglEnableVariantClientStateEXT != NULL && 
        qglDisableVariantClientStateEXT != NULL && 
        qglBindLightParameterEXT != NULL && 
        qglBindMaterialParameterEXT != NULL && 
        qglBindTexGenParameterEXT != NULL && 
        qglBindTextureUnitParameterEXT != NULL && 
        qglBindParameterEXT != NULL && 
        qglIsVariantEnabledEXT != NULL && 
        qglGetVariantBooleanvEXT != NULL && 
        qglGetVariantIntegervEXT != NULL && 
        qglGetVariantFloatvEXT != NULL && 
        qglGetVariantPointervEXT != NULL && 
        qglGetInvariantBooleanvEXT != NULL && 
        qglGetInvariantIntegervEXT != NULL && 
        qglGetInvariantFloatvEXT != NULL && 
        qglGetLocalConstantBooleanvEXT != NULL && 
        qglGetLocalConstantIntegervEXT != NULL && 
        qglGetLocalConstantFloatvEXT != NULL)
    {
        return (true);
    }
    
    return (false);
}

#endif /* __APPLE__ || MACOSX */

void Radeon_CreateShaders()
{
    float scaler[4] = {0.5f, 0.5f, 0.5f, 0.5f};
    int i;
    GLuint mvp, modelview, zcomp, tempvec;
    GLuint texturematrix, texturematrix2;
    GLuint vertex;
    GLuint texcoord0;
    GLuint texcoord1;
    GLuint texcoord2;
    GLuint texcoord3;
    GLuint color;
    GLuint supportedTmu;
    GLuint disttemp, disttemp2;
    GLuint fogstart, fogend;

#if !defined(__APPLE__) && !defined (MACOSX)
    SAFE_GET_PROC( qglGenFragmentShadersATI, PFNGLGENFRAGMENTSHADERSATIPROC, "glGenFragmentShadersATI");
    SAFE_GET_PROC( qglBindFragmentShaderATI, PFNGLBINDFRAGMENTSHADERATIPROC, "glBindFragmentShaderATI");
    SAFE_GET_PROC( qglDeleteFragmentShaderATI, PFNGLDELETEFRAGMENTSHADERATIPROC, "glDeleteFragmentShaderATI");
    SAFE_GET_PROC( qglBeginFragmentShaderATI, PFNGLBEGINFRAGMENTSHADERATIPROC, "glBeginFragmentShaderATI");
    SAFE_GET_PROC( qglEndFragmentShaderATI, PFNGLENDFRAGMENTSHADERATIPROC, "glEndFragmentShaderATI");
    SAFE_GET_PROC( qglPassTexCoordATI, PFNGLPASSTEXCOORDATIPROC, "glPassTexCoordATI");
    SAFE_GET_PROC( qglSampleMapATI, PFNGLSAMPLEMAPATIPROC, "glSampleMapATI");
    SAFE_GET_PROC( qglColorFragmentOp1ATI, PFNGLCOLORFRAGMENTOP1ATIPROC, "glColorFragmentOp1ATI");
    SAFE_GET_PROC( qglColorFragmentOp2ATI, PFNGLCOLORFRAGMENTOP2ATIPROC, "glColorFragmentOp2ATI");
    SAFE_GET_PROC( qglColorFragmentOp3ATI, PFNGLCOLORFRAGMENTOP3ATIPROC, "glColorFragmentOp3ATI");
    SAFE_GET_PROC( qglAlphaFragmentOp1ATI, PFNGLALPHAFRAGMENTOP1ATIPROC, "glAlphaFragmentOp1ATI");
    SAFE_GET_PROC( qglAlphaFragmentOp2ATI, PFNGLALPHAFRAGMENTOP2ATIPROC, "glAlphaFragmentOp2ATI");
    SAFE_GET_PROC( qglAlphaFragmentOp3ATI, PFNGLALPHAFRAGMENTOP3ATIPROC, "glAlphaFragmentOp3ATI");
    SAFE_GET_PROC( qglSetFragmentShaderConstantATI, PFNGLSETFRAGMENTSHADERCONSTANTATIPROC, "glSetFragmentShaderConstantATI");
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

    glEnable(GL_FRAGMENT_SHADER_ATI);

    fragment_shaders = qglGenFragmentShadersATI(2);

    // combined diffuse & specular shader w/ vertex color
    qglBindFragmentShaderATI(fragment_shaders);
    Radeon_checkerror();
    qglBeginFragmentShaderATI();
    Radeon_checkerror();

    qglSetFragmentShaderConstantATI(GL_CON_0_ATI, &scaler[0]);
    Radeon_checkerror();

    // texld r0, t0
    qglSampleMapATI (GL_REG_0_ATI, GL_TEXTURE0_ARB, GL_SWIZZLE_STR_ATI);
    Radeon_checkerror();
    // texld r1, t1
    qglSampleMapATI (GL_REG_1_ATI, GL_TEXTURE1_ARB, GL_SWIZZLE_STR_ATI);
    Radeon_checkerror();
    // texld r2, t2
    qglSampleMapATI (GL_REG_2_ATI, GL_TEXTURE2_ARB, GL_SWIZZLE_STR_ATI);
    Radeon_checkerror();
    // texld r3, t3
    qglSampleMapATI (GL_REG_3_ATI, GL_TEXTURE3_ARB, GL_SWIZZLE_STR_ATI);
    Radeon_checkerror();
    // texld r4, t4
    qglSampleMapATI (GL_REG_4_ATI, GL_TEXTURE4_ARB, GL_SWIZZLE_STR_ATI);
    Radeon_checkerror();

    // gloss * atten * light color * specular +
    // dot * color * atten * light color * self shadow =
    // (gloss * specular + dot * color * self shadow ) * atten * light color
    // Alpha ops rule :-)

    // dp3_sat r2.rgb, r0_bx2.rgb, r2_bx2.rgb   // specular
    qglColorFragmentOp2ATI(GL_DOT3_ATI,
                           GL_REG_2_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_SATURATE_BIT_ATI,
                           GL_REG_0_ATI, GL_NONE, GL_2X_BIT_ATI|GL_BIAS_BIT_ATI,
                           GL_REG_2_ATI, GL_NONE, GL_2X_BIT_ATI|GL_BIAS_BIT_ATI);
    Radeon_checkerror();

    // +mov_x8_sat r2.a, r1_bx2.b               // self shadow term
    qglAlphaFragmentOp1ATI(GL_MOV_ATI,
                           GL_REG_2_ATI, GL_8X_BIT_ATI|GL_SATURATE_BIT_ATI,
                           GL_REG_1_ATI, GL_BLUE, GL_2X_BIT_ATI|GL_BIAS_BIT_ATI);
    Radeon_checkerror();

    // dp3_sat r1.rgb, r0_bx2.rgb, r1_bx2.rgb   // diffuse
    qglColorFragmentOp2ATI(GL_DOT3_ATI,
                           GL_REG_1_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_SATURATE_BIT_ATI,
                           GL_REG_0_ATI, GL_NONE, GL_2X_BIT_ATI|GL_BIAS_BIT_ATI,
                           GL_REG_1_ATI, GL_NONE, GL_2X_BIT_ATI|GL_BIAS_BIT_ATI);
    Radeon_checkerror();

    // +mad_x2_sat r1.a, r2.b, r2.b, -c0.b      // specular exponent
    qglAlphaFragmentOp3ATI(GL_MAD_ATI,
                           GL_REG_1_ATI, GL_2X_BIT_ATI|GL_SATURATE_BIT_ATI,
                           GL_REG_2_ATI, GL_BLUE, GL_NONE,
                           GL_REG_2_ATI, GL_BLUE, GL_NONE,
                           GL_CON_0_ATI, GL_BLUE, GL_NEGATE_BIT_ATI);
    Radeon_checkerror();

    // mul r1.rgb, r1.rgb, r3.rgb               // diffuse color * diffuse bump
    qglColorFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_1_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE,
                           GL_REG_3_ATI, GL_NONE, GL_NONE);
    Radeon_checkerror();

    // +mul r1.a, r1.a, r1.a                    // raise exponent
    qglAlphaFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_1_ATI, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE);
    Radeon_checkerror();

    // mul r4.rgb, r4.rgb, v0.rgb               // atten * light color
    qglColorFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_4_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_NONE,
                           GL_REG_4_ATI, GL_NONE, GL_NONE,
                           GL_PRIMARY_COLOR_ARB, GL_NONE, GL_NONE);
    Radeon_checkerror();

    // +mul r1.a, r1.a, r1.a                    // raise exponent
    qglAlphaFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_1_ATI, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE);
    Radeon_checkerror();

    // mul r1.rgb, r1.rgb, r2.a                 // self shadow * diffuse
    qglColorFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_1_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE,
                           GL_REG_2_ATI, GL_ALPHA, GL_NONE);
    Radeon_checkerror();

    // +mul r0.a, r1.a, r0.a                    // specular * gloss map
    qglAlphaFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_0_ATI, GL_SATURATE_BIT_ATI,
                           GL_REG_1_ATI, GL_NONE, GL_NONE,
                           GL_REG_0_ATI, GL_NONE, GL_NONE);
    Radeon_checkerror();

    // add r0.rgb, r1.rgb, r0.a                 // diffuse + specular
    qglColorFragmentOp2ATI(GL_ADD_ATI,
                           GL_REG_0_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE,
                           GL_REG_0_ATI, GL_ALPHA, GL_NONE);
    Radeon_checkerror();

    // mul_sat r0.rgb, r0.rgb, r4.rgb           // (diffuse + specular)*atten*color
    qglColorFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
                           GL_REG_0_ATI, GL_NONE, GL_NONE,
                           GL_REG_4_ATI, GL_NONE, GL_NONE);
    Radeon_checkerror();


    qglEndFragmentShaderATI();
    Radeon_checkerror();

    // combined diffuse & specular shader w/ vertex color and colored gloss
    qglBindFragmentShaderATI(fragment_shaders+1);
    Radeon_checkerror();
    qglBeginFragmentShaderATI();
    Radeon_checkerror();

    qglSetFragmentShaderConstantATI(GL_CON_0_ATI, &scaler[0]);
    Radeon_checkerror();

    // texld r0, t0
    qglSampleMapATI (GL_REG_0_ATI, GL_TEXTURE0_ARB, GL_SWIZZLE_STR_ATI);
    Radeon_checkerror();
    // texld r1, t1
    qglSampleMapATI (GL_REG_1_ATI, GL_TEXTURE1_ARB, GL_SWIZZLE_STR_ATI);
    Radeon_checkerror();
    // texld r2, t2
    qglSampleMapATI (GL_REG_2_ATI, GL_TEXTURE2_ARB, GL_SWIZZLE_STR_ATI);
    Radeon_checkerror();
    // texld r3, t3
    qglSampleMapATI (GL_REG_3_ATI, GL_TEXTURE3_ARB, GL_SWIZZLE_STR_ATI);
    Radeon_checkerror();
    // texld r4, t4
    qglSampleMapATI (GL_REG_4_ATI, GL_TEXTURE4_ARB, GL_SWIZZLE_STR_ATI);
    Radeon_checkerror();
    // texld r5, t5
    qglSampleMapATI (GL_REG_5_ATI, GL_TEXTURE5_ARB, GL_SWIZZLE_STR_ATI);
    Radeon_checkerror();

    // gloss * atten * light color * specular +
    // dot * color * atten * light color * self shadow =
    // (gloss * specular + dot * color * self shadow ) * atten * light color
    // Alpha ops rule :-)

    // dp3_sat r2.rgb, r0_bx2.rgb, r2_bx2.rgb   // specular
    qglColorFragmentOp2ATI(GL_DOT3_ATI,
                           GL_REG_2_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_SATURATE_BIT_ATI,
                           GL_REG_0_ATI, GL_NONE, GL_2X_BIT_ATI|GL_BIAS_BIT_ATI,
                           GL_REG_2_ATI, GL_NONE, GL_2X_BIT_ATI|GL_BIAS_BIT_ATI);
    Radeon_checkerror();

    // +mov_x8_sat r2.a, r1_bx2.b               // self shadow term
    qglAlphaFragmentOp1ATI(GL_MOV_ATI,
                           GL_REG_2_ATI, GL_8X_BIT_ATI|GL_SATURATE_BIT_ATI,
                           GL_REG_1_ATI, GL_BLUE, GL_2X_BIT_ATI|GL_BIAS_BIT_ATI);
    Radeon_checkerror();

    // dp3_sat r1.rgb, r0_bx2.rgb, r1_bx2.rgb   // diffuse
    qglColorFragmentOp2ATI(GL_DOT3_ATI,
                           GL_REG_1_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_SATURATE_BIT_ATI,
                           GL_REG_0_ATI, GL_NONE, GL_2X_BIT_ATI|GL_BIAS_BIT_ATI,
                           GL_REG_1_ATI, GL_NONE, GL_2X_BIT_ATI|GL_BIAS_BIT_ATI);
    Radeon_checkerror();

    // +mad_x2_sat r1.a, r2.b, r2.b, -c0.b      // specular exponent
    qglAlphaFragmentOp3ATI(GL_MAD_ATI,
                           GL_REG_1_ATI, GL_2X_BIT_ATI|GL_SATURATE_BIT_ATI,
                           GL_REG_2_ATI, GL_BLUE, GL_NONE,
                           GL_REG_2_ATI, GL_BLUE, GL_NONE,
                           GL_CON_0_ATI, GL_BLUE, GL_NEGATE_BIT_ATI);
    Radeon_checkerror();

    // mul r1.rgb, r1.rgb, r3.rgb               // diffuse color * diffuse bump
    qglColorFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_1_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE,
                           GL_REG_3_ATI, GL_NONE, GL_NONE);
    Radeon_checkerror();

    // +mul r1.a, r1.a, r1.a                    // raise exponent
    qglAlphaFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_1_ATI, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE);
    Radeon_checkerror();

    // mul r4.rgb, r4.rgb, v0.rgb               // atten * light color
    qglColorFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_4_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_NONE,
                           GL_REG_4_ATI, GL_NONE, GL_NONE,
                           GL_PRIMARY_COLOR_ARB, GL_NONE, GL_NONE);
    Radeon_checkerror();

    // +mul r1.a, r1.a, r1.a                    // raise exponent
    qglAlphaFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_1_ATI, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE);
    Radeon_checkerror();

    // mul r1.rgb, r1.rgb, r2.a                 // self shadow * diffuse
    qglColorFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_1_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE,
                           GL_REG_2_ATI, GL_ALPHA, GL_NONE);
    Radeon_checkerror();

    // mad_sat r0.rgb, r1.a, r5.rgb, r1.rgb     // specular * gloss + diffuse
    qglColorFragmentOp3ATI(GL_MAD_ATI,
                           GL_REG_0_ATI, GL_RED_BIT_ATI|GL_GREEN_BIT_ATI|GL_BLUE_BIT_ATI, GL_SATURATE_BIT_ATI,
                           GL_REG_1_ATI, GL_ALPHA, GL_NONE,
                           GL_REG_5_ATI, GL_NONE, GL_NONE,
                           GL_REG_1_ATI, GL_NONE, GL_NONE);
    Radeon_checkerror();

    // mul_sat r0.rgb, r0.rgb, r4.rgb           // (diffuse + specular)*atten*color
    qglColorFragmentOp2ATI(GL_MUL_ATI,
                           GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
                           GL_REG_0_ATI, GL_NONE, GL_NONE,
                           GL_REG_4_ATI, GL_NONE, GL_NONE);
    Radeon_checkerror();

    qglEndFragmentShaderATI();
    Radeon_checkerror();

    glDisable(GL_FRAGMENT_SHADER_ATI);


    glEnable(GL_VERTEX_SHADER_EXT);

    vertex_shaders = qglGenVertexShadersEXT(1);
    Radeon_checkerror();
    lightPos = qglGenSymbolsEXT(GL_VECTOR_EXT, GL_INVARIANT_EXT, 
				GL_FULL_RANGE_EXT, 1);
    eyePos = qglGenSymbolsEXT(GL_VECTOR_EXT, GL_INVARIANT_EXT, 
			      GL_FULL_RANGE_EXT, 1);


    qglBindVertexShaderEXT(vertex_shaders);
    Radeon_checkerror();
    qglBeginVertexShaderEXT();
    Radeon_checkerror();

    // Generates a necessary input for the diffuse bumpmapping registers
    mvp           = qglBindParameterEXT( GL_MVP_MATRIX_EXT );
    Radeon_checkerror();
    modelview     = qglBindParameterEXT( GL_MODELVIEW_MATRIX );
    Radeon_checkerror();
    vertex        = qglBindParameterEXT( GL_CURRENT_VERTEX_EXT );
    Radeon_checkerror();
    color         = qglBindParameterEXT( GL_CURRENT_COLOR );
    Radeon_checkerror();
    texturematrix = qglBindTextureUnitParameterEXT( GL_TEXTURE4_ARB, GL_TEXTURE_MATRIX );
    Radeon_checkerror();
    texcoord0     = qglBindTextureUnitParameterEXT( GL_TEXTURE0_ARB, GL_CURRENT_TEXTURE_COORDS );
    Radeon_checkerror();
    texcoord1     = qglBindTextureUnitParameterEXT( GL_TEXTURE1_ARB, GL_CURRENT_TEXTURE_COORDS );
    Radeon_checkerror();
    texcoord2     = qglBindTextureUnitParameterEXT( GL_TEXTURE2_ARB, GL_CURRENT_TEXTURE_COORDS );
    Radeon_checkerror();
    texcoord3     = qglBindTextureUnitParameterEXT( GL_TEXTURE3_ARB, GL_CURRENT_TEXTURE_COORDS );
    Radeon_checkerror();
    disttemp      = qglGenSymbolsEXT(GL_SCALAR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);
    Radeon_checkerror();
    disttemp2     = qglGenSymbolsEXT(GL_SCALAR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);
    Radeon_checkerror();
    zcomp         = qglGenSymbolsEXT(GL_VECTOR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);
    Radeon_checkerror();
    tempvec       = qglGenSymbolsEXT(GL_VECTOR_EXT, GL_LOCAL_EXT, GL_FULL_RANGE_EXT, 1);
    Radeon_checkerror();
    fogstart      = qglBindParameterEXT( GL_FOG_START );
    Radeon_checkerror();
    fogend        = qglBindParameterEXT( GL_FOG_END );
    Radeon_checkerror();

    // Transform vertex to view-space
    qglShaderOp2EXT( GL_OP_MULTIPLY_MATRIX_EXT, GL_OUTPUT_VERTEX_EXT, mvp, vertex );
    Radeon_checkerror();
    
    // Transform vertex by texture matrix and copy to output
    qglShaderOp2EXT( GL_OP_MULTIPLY_MATRIX_EXT, GL_OUTPUT_TEXTURE_COORD4_EXT, texturematrix, vertex );
    Radeon_checkerror();

    // copy tex coords of unit 0 to unit 3
    qglShaderOp1EXT( GL_OP_MOV_EXT, GL_OUTPUT_TEXTURE_COORD0_EXT, texcoord0);
    Radeon_checkerror();
    qglShaderOp1EXT( GL_OP_MOV_EXT, GL_OUTPUT_TEXTURE_COORD3_EXT, texcoord0);
    Radeon_checkerror();
    qglShaderOp1EXT( GL_OP_MOV_EXT, GL_OUTPUT_TEXTURE_COORD5_EXT, texcoord0);
    Radeon_checkerror();
    qglShaderOp1EXT( GL_OP_MOV_EXT, GL_OUTPUT_COLOR0_EXT, color);
    Radeon_checkerror();

    // Transform vertex and take z for fog
    qglExtractComponentEXT( zcomp, modelview, 2);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, zcomp, vertex );
    Radeon_checkerror();

    // calculate fog values end - z and end - start
    qglShaderOp2EXT( GL_OP_SUB_EXT, disttemp, fogend, disttemp);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_SUB_EXT, disttemp2, fogend, fogstart);
    Radeon_checkerror();

    // divide end - z by end - start, that's it
    qglShaderOp1EXT( GL_OP_RECIP_EXT, disttemp2, disttemp2);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_MUL_EXT, GL_OUTPUT_FOG_EXT, disttemp, disttemp2);
    Radeon_checkerror();

    // calculate light position
    qglShaderOp2EXT( GL_OP_SUB_EXT, tempvec, lightPos, vertex);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_DOT3_EXT, disttemp, tempvec, tempvec);
    Radeon_checkerror();
    qglShaderOp1EXT( GL_OP_RECIP_SQRT_EXT, disttemp, disttemp);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_MUL_EXT, tempvec, tempvec, disttemp);
    Radeon_checkerror();
    // Normalized light vec now in tempvec, transform to tex1
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord1);
    Radeon_checkerror();
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD1_EXT, disttemp, 0);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord2);
    Radeon_checkerror();
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD1_EXT, disttemp, 1);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord3);
    Radeon_checkerror();
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD1_EXT, disttemp, 2);
    Radeon_checkerror();

    // Now, calculate halfvec
    qglShaderOp2EXT( GL_OP_SUB_EXT, zcomp, eyePos, vertex);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_ADD_EXT, tempvec, tempvec, zcomp);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_DOT3_EXT, disttemp, tempvec, tempvec);
    Radeon_checkerror();
    qglShaderOp1EXT( GL_OP_RECIP_SQRT_EXT, disttemp, disttemp);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_MUL_EXT, tempvec, tempvec, disttemp);
    Radeon_checkerror();
    // Normalized half vec now in tempvec, transform to tex1
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord1);
    Radeon_checkerror();
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD2_EXT, disttemp, 0);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord2);
    Radeon_checkerror();
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD2_EXT, disttemp, 1);
    Radeon_checkerror();
    qglShaderOp2EXT( GL_OP_DOT4_EXT, disttemp, tempvec, texcoord3);
    Radeon_checkerror();
    qglInsertComponentEXT( GL_OUTPUT_TEXTURE_COORD2_EXT, disttemp, 2);

    qglEndVertexShaderEXT();
    Radeon_checkerror();

    glDisable(GL_VERTEX_SHADER_EXT);
}


void Radeon_DisableBumpShader(shader_t* shader)
{
    //tex 0 = normal map
    //tex 1 = normalization cube map (tangent space light vector)
    //tex 2 = normalization cube map (tangent space half vector)
    //tex 3 = color map
    //tex 4 = (attenuation or light filter, depends on light settings)

    glDisable(GL_VERTEX_SHADER_EXT);
    glDisable(GL_FRAGMENT_SHADER_ATI);

    GL_SelectTexture(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_CUBE_MAP_ARB);

    GL_SelectTexture(GL_TEXTURE2_ARB);
    glDisable(GL_TEXTURE_CUBE_MAP_ARB);

    GL_SelectTexture(GL_TEXTURE3_ARB);
    glDisable(GL_TEXTURE_2D);

    GL_SelectTexture(GL_TEXTURE4_ARB);
    if (currentshadowlight->shader->numstages)
    {        
        if (currentshadowlight->shader->stages[0].texture[0]->gltype == GL_TEXTURE_CUBE_MAP_ARB)
        {
            glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        }
        else
        {
            glDisable(GL_TEXTURE_2D);
        }
        glPopMatrix();
        GL_SelectTexture(GL_TEXTURE5_ARB);
        if ( shader->glossstages[0].type == STAGE_GLOSS )
        {
            glDisable(GL_TEXTURE_2D);
        }
    }
    else
    {
        glDisable(GL_TEXTURE_3D);
        glPopMatrix();
        if ( shader->glossstages[0].type == STAGE_GLOSS )
        {
            GL_SelectTexture(GL_TEXTURE5_ARB);
            glDisable(GL_TEXTURE_2D);
        }
    }
    glMatrixMode(GL_MODELVIEW);
    GL_SelectTexture(GL_TEXTURE0_ARB);
}

void Radeon_SetupTcMods(stage_t *s);

void Radeon_EnableBumpShader(const transform_t *tr, const lightobject_t* lightOrig,
                             qboolean alias, shader_t* shader)
{
    GLfloat temp[4];

    //tex 0 = normal map
    //tex 1 = normalization cube map (tangent space light vector)
    //tex 2 = normalization cube map (tangent space half vector)
    //tex 3 = color map
    //tex 4 = (attenuation or light filter, depends on light settings but the actual
    //tex 5 = colored gloss if used

    glGetError();
    GL_SelectTexture(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_CUBE_MAP_ARB);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, normcube_texture_object);

    GL_SelectTexture(GL_TEXTURE2_ARB);
    glEnable(GL_TEXTURE_CUBE_MAP_ARB);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, normcube_texture_object);

    GL_SelectTexture(GL_TEXTURE3_ARB);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_FRAGMENT_SHADER_ATI);
    glEnable(GL_VERTEX_SHADER_EXT);

    qglBindVertexShaderEXT( vertex_shaders );
    Radeon_checkerror();

    GL_SelectTexture(GL_TEXTURE4_ARB);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    if (currentshadowlight->shader->numstages)
    {        
        if (currentshadowlight->shader->stages[0].texture[0]->gltype == GL_TEXTURE_CUBE_MAP_ARB)
        {
            glEnable(GL_TEXTURE_CUBE_MAP_ARB);
  	    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, currentshadowlight->shader->stages[0].texture[0]->texnum); 
        }
        else
        {
            // 2D filter
            glEnable(GL_TEXTURE_2D);
            GL_BindAdvanced(currentshadowlight->shader->stages[0].texture[0]);
	    //Default = repeat the texture one time in the light's sphere
	    //Can be modified with the tcMod shader commands
	    glTranslatef(0.5,0.5,0.5);
	    glScalef(0.5,0.5,0.5);
	    glScalef(1.0f/(currentshadowlight->radiusv[0]), 
	  		   1.0f/(currentshadowlight->radiusv[1]),
			   1.0f/(currentshadowlight->radiusv[2]));
        }
        SH_SetupTcMods(&currentshadowlight->shader->stages[0]);
        GL_SetupCubeMapMatrix(tr);

        GL_SelectTexture(GL_TEXTURE5_ARB);
        if ( shader->glossstages[0].type == STAGE_GLOSS )
        {
            GL_BindAdvanced(shader->glossstages[0].texture[0]);
            glEnable(GL_TEXTURE_2D);
            qglBindFragmentShaderATI( fragment_shaders + 1 );
            Radeon_checkerror();
        }
        else
        {
            qglBindFragmentShaderATI( fragment_shaders );
            Radeon_checkerror();
        }
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

        if ( shader->glossstages[0].type == STAGE_GLOSS )
        {
            GL_SelectTexture(GL_TEXTURE5_ARB);
            GL_BindAdvanced(shader->glossstages[0].texture[0]);
            glEnable(GL_TEXTURE_2D);
            qglBindFragmentShaderATI( fragment_shaders + 1 );
            Radeon_checkerror();
        }
        else
        {
            qglBindFragmentShaderATI( fragment_shaders );
            Radeon_checkerror();
        }

    }

    GL_SelectTexture(GL_TEXTURE0_ARB);

    temp[0] = currentshadowlight->origin[0];
    temp[1] = currentshadowlight->origin[1];
    temp[2] = currentshadowlight->origin[2];
    temp[3] = 1.0f;
    qglSetInvariantEXT(lightPos, GL_FLOAT, &temp[0]);

    temp[0] = r_refdef.vieworg[0];
    temp[1] = r_refdef.vieworg[1];
    temp[2] = r_refdef.vieworg[2];
    qglSetInvariantEXT(eyePos, GL_FLOAT, &temp[0]);
}

void Radeon_EnableAttentShader(const transform_t *tr) {

	float invrad = 1/currentshadowlight->radius;

	GL_SelectTexture(GL_TEXTURE0_ARB);
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

void Radeon_DisableAttentShader() {

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_TEXTURE_3D);
	glEnable(GL_TEXTURE_2D);
}



/************************

Generic triangle list routines

*************************/

void FormatError(); // In gl_bumpgf.c

void Radeon_sendTriangleListWV(const vertexdef_t *verts, int *indecies,
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

void Radeon_sendTriangleListTA(const vertexdef_t *verts, int *indecies,
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

void Radeon_drawTriangleListBump (const vertexdef_t *verts, int *indecies,
				  int numIndecies, shader_t *shader,
				  const transform_t *tr,  const lightobject_t *lo)
{
    if (!(shader->flags & SURF_PPLIGHT)) return;

    if (currentshadowlight->shader->numstages )
    {
    	//draw attent into dest alpha
	GL_DrawAlpha();
	Radeon_EnableAttentShader(tr);
	Radeon_sendTriangleListWV(verts,indecies,numIndecies);
	Radeon_DisableAttentShader();
	GL_ModulateAlphaDrawColor();
    } else {
	GL_AddColor();
    }
    glColor3fv(&currentshadowlight->color[0]);

    Radeon_EnableBumpShader(tr, lo, true, shader);
    //bind the correct texture
    GL_SelectTexture(GL_TEXTURE0_ARB);
    if (shader->numbumpstages > 0)
	GL_BindAdvanced(shader->bumpstages[0].texture[0]);
    GL_SelectTexture(GL_TEXTURE3_ARB);
    if (shader->numcolorstages > 0)
	GL_BindAdvanced(shader->colorstages[0].texture[0]);

    Radeon_sendTriangleListTA(verts,indecies,numIndecies);
    Radeon_DisableBumpShader(shader);
}

void Radeon_drawTriangleListBase (vertexdef_t *verts, int *indecies,
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


    //PENTA: Added fix
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
    else if (shader->flags & SURF_PPLIGHT)
    {

		//PENTA: added fix
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

void Radeon_sendSurfacesBase(msurface_t **surfs, int numSurfaces,
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

void Radeon_drawSurfaceListBase (vertexdef_t* verts, msurface_t** surfs,
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
	Radeon_sendSurfacesBase(surfs, numSurfaces, false);
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

	Radeon_sendSurfacesBase(surfs, numSurfaces, true);

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

void Radeon_sendSurfacesTA(msurface_t** surfs, int numSurfaces, const transform_t *tr, const lightobject_t *lo)
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
            if ( lastshader && lastshader->glossstages[0].type != shader->glossstages[0].type )
            {
                // disable previous shader if switching between colored and mono gloss
                Radeon_DisableBumpShader(lastshader);
                Radeon_EnableBumpShader(tr, lo, true, shader);
            }
            else
            {
                if ( !lastshader )
                {
                    // Enable shader for the first surface
                    Radeon_EnableBumpShader(tr, lo, true, shader);
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
	    if (shader->numbumpstages > 0)
		GL_BindAdvanced(shader->bumpstages[0].texture[0]);
	    GL_SelectTexture(GL_TEXTURE3_ARB);
	    if (shader->numcolorstages > 0)
		GL_BindAdvanced(shader->colorstages[0].texture[0]);
            if ( shader->glossstages > 0 )
            {
                // Bind colored gloss
                GL_SelectTexture(GL_TEXTURE5_ARB);
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
        Radeon_DisableBumpShader(lastshader);
}


void Radeon_sendSurfacesPlain(msurface_t** surfs, int numSurfaces)
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
	glDrawElements(GL_TRIANGLES, p->numindecies, GL_UNSIGNED_INT,
		       &p->indecies[0]);
    }

    if (!cull)
	glEnable(GL_CULL_FACE);
}

void Radeon_drawSurfaceListBump (vertexdef_t *verts, msurface_t **surfs,
				 int numSurfaces, const transform_t *tr,
                                 const lightobject_t *lo)
{
    GL_VertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    if (currentshadowlight->shader->numstages)
    {
    	//draw attent into dest alpha
    	GL_DrawAlpha();
	Radeon_EnableAttentShader(tr);
	GL_TexCoordPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
	Radeon_sendSurfacesPlain(surfs,numSurfaces);
	Radeon_DisableAttentShader();
	GL_ModulateAlphaDrawColor();
    }
    else
    {
	GL_AddColor();
    }
    glColor3fv(&currentshadowlight->color[0]);

    GL_TexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
    Radeon_sendSurfacesTA(surfs, numSurfaces, tr, lo);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Radeon_freeDriver(void)
{
    //nothing here...
}


void BUMP_InitRadeon(void)
{
    GLint errPos, errCode;
    const GLubyte *errString;

    if ( gl_cardtype != RADEON ) return;
    
    Radeon_CreateShaders();


    //bind the correct stuff to the bump mapping driver
    gl_bumpdriver.drawSurfaceListBase = Radeon_drawSurfaceListBase;
    gl_bumpdriver.drawSurfaceListBump = Radeon_drawSurfaceListBump;
    gl_bumpdriver.drawTriangleListBase = Radeon_drawTriangleListBase;
    gl_bumpdriver.drawTriangleListBump = Radeon_drawTriangleListBump;
    gl_bumpdriver.freeDriver = Radeon_freeDriver;
}
