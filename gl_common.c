/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// gl_common.c -- OpenGL common routines

#include "quakedef.h"

const char     *gl_vendor;
const char     *gl_renderer;
const char     *gl_version;
const char     *gl_extensions;
qboolean fullsbardraw = false;


float           vid_gamma = 1.0;

qboolean        isPermedia = false;
qboolean        gl_mtexable = false;
qcardtype       gl_cardtype = GENERIC;
qboolean        gl_var = false; //PENTA: vertex array range is available
qboolean        gl_texcomp = false; // JP: texture compression available


//void (*qglColorTableEXT) (int, int, int, int, int, const void*);
//void (*qgl3DfxSetPaletteEXT) (GLuint *);

typedef void (*GLCOLORTABLEEXTPFN) (int, int, int, int, int, const void*) ;
GLCOLORTABLEEXTPFN qglColorTableEXT;

typedef void (*GL3DFXSETPALETTEEXTPFN) (GLuint *) ;
GL3DFXSETPALETTEEXTPFN qgl3DfxSetPaletteEXT;

PFNBLENDCOLORPROC qglBlendColorEXT;



//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

float		gldepthmin, gldepthmax;


//cvar_t	gl_ztrick = {"gl_ztrick","1"}; PENTA: Removed

// <AWE> Two more extensions. Added already check for paletted texture to gl_vidnt.c.
//	 However any code for anisotropic texture filtering has still to be added to gl_vidnt.c.

// <AWE> true for "GL_EXT_texture_filter_anisotropic".
qboolean	gl_texturefilteranisotropic = false;
// <AWE> anistropic texture level [= 1.0f or max. value].//Penta?? Changed to 2.0 because 1.0 is just isotropic filtering 
GLfloat		gl_textureanisotropylevel = 2.0f; 
// <AWE> On MacOSX X we use this var to store the state. 0 = off, 1 = on.
//cvar_t	gl_anisotropic = { "gl_anisotropic", "0", 1 }; 

qboolean	gl_occlusiontest = false;
occlusion_cut_meshes;
occlusion_cut_entities;
occlusion_cut_lights;

unsigned short  d_8to16table[256];
unsigned        d_8to24table[256];
unsigned char   d_15to8table[65536];
unsigned char   d_8to8graytable[256];


/*-----------------------------------------------------------------------*/

void GL_checkerror(char *file, int line)
{
    GLuint error = glGetError();
    while ( error != GL_NO_ERROR )
    {
        const char* err;
		err = gluErrorString(error);
#ifdef _WIN32
		Con_Printf("Gl error: %s\n (Triggered at line %d in file %s\n)",err,line,file); //So it gets logged if condebug is used
        //_asm { int 3 };
#else
		Sys_Error("Gl error: %s\n (Triggered at line %d in file %s\n)",err,line,file);
#endif
    }
}

int Q_strncasecmp (char *s1,char *s2,int count);


void VID_SetPalette (unsigned char *palette)
{
     byte           *pal;
     unsigned        r,g,b;
     unsigned        a;
     unsigned        v;
     int             r1,g1,b1;
     int		j,k,l,m;
     unsigned short  i;
     unsigned       *table;
     unsigned char  *shade;
     int             dist, bestdist;

//
// 8 8 8 encoding
//
     pal = palette;
     table = d_8to24table;
     shade = d_8to8graytable;
     for (i=0 ; i<256 ; i++)
     {
          r = pal[0];
          g = pal[1];
          b = pal[2];
          pal += 3;
		
          //PENTA: fullbright colors
          //a = i;//(i >= 192) ? 255 : 0;
          a = 255;
          v = (a<<24) + (r<<0) + (g<<8) + (b<<16);
          *table++ = v;
          //PENA: Grayscale conversion for bump maps
          *shade++ = ((r+g+b)/3);
     }
     d_8to24table[255] &= 0xffffff;	// 255 is transparent

     // JACK: 3D distance calcs - k is last closest, l is the distance.
     // FIXME: Precalculate this and cache to disk.
     for (i=0; i < (1<<15); i++) {
          /* Maps
             000000000000000
             000000000011111 = Red  = 0x1F
             000001111100000 = Blue = 0x03E0
             111110000000000 = Grn  = 0x7C00
          */
          r = ((i & 0x1F) << 3)+4;
          g = ((i & 0x03E0) >> 2)+4;
          b = ((i & 0x7C00) >> 7)+4;
          pal = (unsigned char *)d_8to24table;
          for (v=0,k=0,bestdist=10000*10000; v<256; v++,pal+=4) {
               r1 = (int)r - (int)pal[0];
               g1 = (int)g - (int)pal[1];
               b1 = (int)b - (int)pal[2];
               dist = (r1*r1)+(g1*g1)+(b1*b1);
               if (dist < bestdist) {
                    k=v;
                    bestdist = dist;
               }
          }
          d_15to8table[i]=k;
     }
}

//BOOL	gammaworks;

void	VID_ShiftPalette (unsigned char *palette)
{
//	extern	byte ramps[3][256];
	
//	VID_SetPalette (palette);

//	gammaworks = SetDeviceGammaRamp (maindc, ramps);
}



/*
  BINDTEXFUNCPTR bindTexFunc;

  #define TEXTURE_EXT_STRING "GL_EXT_texture_object"

/*
PENTA; not used anymore
*/
/*
  void CheckArrayExtensions (void)
  {
  char		*tmp;

  // check for texture extension 
  tmp = (unsigned char *)glGetString(GL_EXTENSIONS);
  while (*tmp)
  {
  if (strncmp((const char*)tmp, "GL_EXT_vertex_array", strlen("GL_EXT_vertex_array")) == 0)
  {
  if (
  ((SAFE_GET_PROC (glArrayElementEXT, (void *), "glArrayElementEXT")) == NULL) ||
  ((SAFE_GET_PROC (glColorPointerEXT, (void *), "glColorPointerEXT")) == NULL) ||
  ((SAFE_GET_PROC (glTexCoordPointerEXT, (void *), "glTexCoordPointerEXT")) == NULL) ||
  ((SAFE_GET_PROC (glVertexPointerEXT, (void *), "glVertexPointerEXT")) == NULL) )
  {
  Sys_Error ("GetProcAddress for vertex extension failed");
  return;
  }
  return;
  }
  tmp++;
  }

  Sys_Error ("Vertex array extension not present");
  }
*/

void CheckMultiTextureExtensions(void)
{

     if (strstr(gl_extensions, "GL_ARB_multitexture")) {

          SAFE_GET_PROC (qglActiveTextureARB,PFNGLACTIVETEXTUREARBPROC,"glActiveTextureARB");
          SAFE_GET_PROC (qglClientActiveTextureARB,PFNGLCLIENTACTIVETEXTUREARBPROC,"glClientActiveTextureARB");
          SAFE_GET_PROC (qglMultiTexCoord1fARB,PFNGLMULTITEXCOORD1FARBPROC,"glMultiTexCoord1fARB");
          SAFE_GET_PROC (qglMultiTexCoord2fARB,PFNGLMULTITEXCOORD2FARBPROC,"glMultiTexCoord2fARB");
          SAFE_GET_PROC (qglMultiTexCoord2fvARB,PFNGLMULTITEXCOORD2FVARBPROC,"glMultiTexCoord2fvARB");
          SAFE_GET_PROC (qglMultiTexCoord3fARB,PFNGLMULTITEXCOORD3FARBPROC,"glMultiTexCoord3fARB");
          SAFE_GET_PROC (qglMultiTexCoord3fvARB,PFNGLMULTITEXCOORD3FVARBPROC,"glMultiTexCoord3fvARB");
          gl_mtexable = true;

     } else {
          Sys_Error ("No multitexturing found.\nProbably your 3d-card is not supported.\n");
     }
}


/*
  PENTA: If we don't have these extensions then we don't continue
  (how would we ever draw bump maps is these simpel ext's aren't supported)
*/
void CheckDiffuseBumpMappingExtensions(void)
{
     if (!strstr(gl_extensions, "GL_EXT_texture_env_combine") &&
         !strstr(gl_extensions, "GL_ARB_texture_env_combine") ) {
          Sys_Error ("EXT_texture_env_combine not found.\nProbably your 3d-card is not supported.\n");
     }

     if (!strstr(gl_extensions, "GL_ARB_texture_env_dot3")) {
          Sys_Error ("ARB_texture_env_dot3 not found.\nProbably your 3d-card is not supported.\n");
     }

     if (!strstr(gl_extensions, "GL_ARB_texture_cube_map")) {
          Sys_Error ("ARB_texture_cube_map not found.\nProbably your 3d-card is not supported.\n");
     }

     //Just spit a warning user prob has gl-1.2 or something
     if (!strstr(gl_extensions, "GL_SGI_texture_edge_clamp") &&
         !strstr(gl_extensions, "GL_EXT_texture_edge_clamp")) {
          Con_Printf("Warning no edge_clamp extension found");
     }
}

/*
  PENTA: if we don't have these we continue with less eficient specular
*/
void CheckSpecularBumpMappingExtensions(void)
{
  
     if ( (strstr(gl_extensions, "GL_NV_register_combiners")) && (!COM_CheckParm ("-forcenonv")) ) {
          gl_cardtype = GEFORCE; // GEFORCE3 checked later
          SAFE_GET_PROC (qglCombinerParameterfvNV,PFNGLCOMBINERPARAMETERFVNVPROC,"glCombinerParameterfvNV");
          SAFE_GET_PROC (qglCombinerParameterivNV,PFNGLCOMBINERPARAMETERIVNVPROC,"glCombinerParameterivNV");
          SAFE_GET_PROC (qglCombinerParameterfNV,PFNGLCOMBINERPARAMETERFNVPROC,"glCombinerParameterfNV");
          SAFE_GET_PROC (qglCombinerParameteriNV,PFNGLCOMBINERPARAMETERINVPROC,"glCombinerParameteriNV");
          SAFE_GET_PROC (qglCombinerInputNV,PFNGLCOMBINERINPUTNVPROC,"glCombinerInputNV");
          SAFE_GET_PROC (qglCombinerOutputNV,PFNGLCOMBINEROUTPUTNVPROC,"glCombinerOutputNV");
          SAFE_GET_PROC (qglFinalCombinerInputNV,PFNGLFINALCOMBINERINPUTNVPROC,"glFinalCombinerInputNV");
          SAFE_GET_PROC (qglGetCombinerInputParameterfvNV,PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC,"glGetCombinerInputParameterfvNV");
          SAFE_GET_PROC (qglGetCombinerInputParameterivNV,PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC,"glGetCombinerInputParameterivNV");
          SAFE_GET_PROC (qglGetCombinerOutputParameterfvNV,PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC,"glGetCombinerOutputParameterfvNV");
          SAFE_GET_PROC (qglGetCombinerOutputParameterivNV,PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC,"glGetCombinerOutputParameterivNV");
          SAFE_GET_PROC (qglGetFinalCombinerInputParameterfvNV,PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC,"glGetFinalCombinerInputParameterfvNV"); 
          SAFE_GET_PROC (qglGetFinalCombinerInputParameterivNV,PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC,"glGetFinalCombinerInputParameterivNV"); 
     } 

	 //PENTA: Cg uses register combiners2 also, they only add some constant registers tough...
	 if (strstr(gl_extensions, "GL_NV_register_combiners2")) {
          SAFE_GET_PROC (qglCombinerStageParameterfvNV,PFNGLCOMBINERSTAGEPARAMETERFVNVPROC,"glCombinerStageParameterfvNV"); 
          SAFE_GET_PROC (qglGetCombinerStageParameterfvNV,PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC,"glGetCombinerStageParameterfvNV"); 
	 }
}

/*
  PENTA: if we have these we draw optimized
*/
void CheckGeforce3Extensions(void)
{
     int supportedTmu;
     glGetIntegerv(GL_MAX_ACTIVE_TEXTURES_ARB,&supportedTmu);
     //Con_Printf("%i texture units\n",supportedTmu);

     if (strstr(gl_extensions, "GL_EXT_texture3D")
         && (supportedTmu >= 4)  && (!COM_CheckParm ("-forcegf2"))
         && (gl_cardtype == GEFORCE)
         && strstr(gl_extensions, "GL_NV_vertex_program1_1")
         && strstr(gl_extensions, "GL_NV_vertex_array_range")
		 && strstr(gl_extensions, "ARB_imaging"))
     {
          gl_cardtype = GEFORCE3;
          SAFE_GET_PROC (qglTexImage3DEXT,PFNGLTEXIMAGE3DEXT,"glTexImage3DEXT");

          //get vertex_program pointers
          SAFE_GET_PROC (qglAreProgramsResidentNV,PFNGLAREPROGRAMSRESIDENTNVPROC,"glAreProgramsResidentNV");
          SAFE_GET_PROC (qglBindProgramNV,PFNGLBINDPROGRAMNVPROC,"glBindProgramNV");
          SAFE_GET_PROC (qglDeleteProgramsNV,PFNGLDELETEPROGRAMSNVPROC,"glDeleteProgramsNV");
          SAFE_GET_PROC (qglExecuteProgramNV,PFNGLEXECUTEPROGRAMNVPROC,"glExecuteProgramNV");
          SAFE_GET_PROC (qglGenProgramsNV,PFNGLGENPROGRAMSNVPROC,"glGenProgramsNV");
          SAFE_GET_PROC (qglGetProgramParameterdvNV,PFNGLGETPROGRAMPARAMETERDVNVPROC,"glGetProgramParameterdvNV");
          SAFE_GET_PROC (qglGetProgramParameterfvNV,PFNGLGETPROGRAMPARAMETERFVNVPROC,"glGetProgramParameterfvNV");
          SAFE_GET_PROC (qglGetProgramivNV,PFNGLGETPROGRAMIVNVPROC,"glGetProgramivNV");
          SAFE_GET_PROC (qglGetProgramStringNV,PFNGLGETPROGRAMSTRINGNVPROC,"glGetProgramStringNV");
          SAFE_GET_PROC (qglGetTrackMatrixivNV,PFNGLGETTRACKMATRIXIVNVPROC,"glGetTrackMatrixivNV");
          SAFE_GET_PROC (qglGetVertexAttribdvNV,PFNGLGETVERTEXATTRIBDVNVPROC,"glGetVertexAttribdvNV");
          SAFE_GET_PROC (qglGetVertexAttribfvNV,PFNGLGETVERTEXATTRIBFVNVPROC,"glGetVertexAttribfvNV");
          SAFE_GET_PROC (qglGetVertexAttribivNV,PFNGLGETVERTEXATTRIBIVNVPROC,"glGetVertexAttribivNV");
          SAFE_GET_PROC (qglGetVertexAttribPointervNV,PFNGLGETVERTEXATTRIBPOINTERVNVPROC,"glGetVertexAttribPointervNV");
          SAFE_GET_PROC (qglGetVertexAttribPointervNV,PFNGLGETVERTEXATTRIBPOINTERVNVPROC,"glGetVertexAttribPointerNV");
          SAFE_GET_PROC (qglIsProgramNV,PFNGLISPROGRAMNVPROC,"glIsProgramNV");
          SAFE_GET_PROC (qglLoadProgramNV,PFNGLLOADPROGRAMNVPROC,"glLoadProgramNV");
          SAFE_GET_PROC (qglProgramParameter4dNV,PFNGLPROGRAMPARAMETER4DNVPROC,"glProgramParameter4dNV");
          SAFE_GET_PROC (qglProgramParameter4dvNV,PFNGLPROGRAMPARAMETER4DVNVPROC,"glProgramParameter4dvNV");
          SAFE_GET_PROC (qglProgramParameter4fNV,PFNGLPROGRAMPARAMETER4FNVPROC,"glProgramParameter4fNV");
          SAFE_GET_PROC (qglProgramParameter4fvNV,PFNGLPROGRAMPARAMETER4FVNVPROC,"glProgramParameter4fvNV");
          SAFE_GET_PROC (qglProgramParameters4dvNV,PFNGLPROGRAMPARAMETERS4DVNVPROC,"glProgramParameters4dvNV");
          SAFE_GET_PROC (qglProgramParameters4fvNV,PFNGLPROGRAMPARAMETERS4FVNVPROC,"glProgramParameters4fvNV");
          SAFE_GET_PROC (qglRequestResidentProgramsNV,PFNGLREQUESTRESIDENTPROGRAMSNVPROC,"glRequestResidentProgramsNV");
          SAFE_GET_PROC (qglTrackMatrixNV,PFNGLTRACKMATRIXNVPROC,"glTrackMatrixNV");

		  //get blend color pointers
		  SAFE_GET_PROC (qglBlendColorEXT,PFNBLENDCOLORPROC,"glBlendColor");


          //default to trilinear filtering on gf3
          gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
          gl_filter_max = GL_LINEAR;

     }
}


//program wide pointer to the allocated agp mem block
void *AGP_Buffer;


void CheckVertexArrayRange(void)
{
/*
  if (strstr(gl_extensions, "GL_NV_vertex_array_range"))
  {
  //get VAR pointers
  SAFE_GET_PROC (glFlushVertexArrayRangeNV, PFNGLFLUSHVERTEXARRAYRANGENVPROC, "glFlushVertexArrayRangeNV");
  SAFE_GET_PROC (glVertexArrayRangeNV, PFNGLVERTEXARRAYRANGENVPROC, "glVertexArrayRangeNV");
  SAFE_GET_PROC (wglAllocateMemoryNV, PFNWGLALLOCATEMEMORYNVPROC, "wglAllocateMemoryNV");
  SAFE_GET_PROC (wglFreeMemoryNV, PFNWGLFREEMEMORYNVPROC, "wglFreeMemoryNV");

  if (wglAllocateMemoryNV != NULL) {
  AGP_Buffer = wglAllocateMemoryNV(AGP_BUFFER_SIZE, 0.0, 0.0, 0.85);
  if (AGP_Buffer) {
  Con_Printf("Using %ikb AGP mem.\n",AGP_BUFFER_SIZE/1024);
  gl_var = true;
  } else {
  Con_Printf("Failed to allocate %ikb AGP mem.\n",AGP_BUFFER_SIZE/1024);
  gl_var = false;
  }
  }
  } else {
*/
     gl_var = false;
//	}

}


/*
  PA: if we have these we draw optimized
*/

void CheckRadeonExtensions(void)
{
     int supportedTmu;
     glGetIntegerv(GL_MAX_ACTIVE_TEXTURES_ARB,&supportedTmu);

     if (strstr(gl_extensions, "GL_EXT_texture3D")
         && (supportedTmu >= 4)  && (!COM_CheckParm ("-forcegeneric"))
         && strstr(gl_extensions, "GL_ATI_fragment_shader")
         && strstr(gl_extensions, "GL_EXT_vertex_shader"))
     {
          Con_Printf("Using Radeon path.\n");
          gl_cardtype = RADEON;

          //get TEX3d pointers
          SAFE_GET_PROC (qglTexImage3DEXT,PFNGLTEXIMAGE3DEXT,"glTexImage3DEXT");

          //default to trilinear filtering on Radeon
          gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
          gl_filter_max = GL_LINEAR;
     } 
}

void CheckParheliaExtensions(void) 
{
     int supportedTmu;
     glGetIntegerv(GL_MAX_ACTIVE_TEXTURES_ARB,&supportedTmu); 

     if (strstr(gl_extensions, "GL_EXT_texture3D")
         && (supportedTmu >= 4)  && (COM_CheckParm ("-forceparhelia"))
         && strstr(gl_extensions, "GL_MTX_fragment_shader")
         && strstr(gl_extensions, "GL_EXT_vertex_shader"))
     {
          gl_cardtype = PARHELIA;

          //get TEX3d pointers
          SAFE_GET_PROC (qglTexImage3DEXT,PFNGLTEXIMAGE3DEXT,"glTexImage3DEXT");

          //default to trilinear filtering on Parhelia
          gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
          gl_filter_max = GL_LINEAR;
     }
}


void CheckARBFragmentExtensions(void) 
{
     int supportedTmu;
     glGetIntegerv(GL_MAX_ACTIVE_TEXTURES_ARB,&supportedTmu); 

     if (strstr(gl_extensions, "GL_EXT_texture3D")
         && (!COM_CheckParm ("-forcegeneric"))
         && (!COM_CheckParm ("-noarb"))
         && strstr(gl_extensions, "GL_ARB_fragment_program")
         && strstr(gl_extensions, "GL_ARB_vertex_program"))
     {
          gl_cardtype = ARB;

          //get TEX3d poiters                   wlgGetProcAddress
          SAFE_GET_PROC (qglTexImage3DEXT,PFNGLTEXIMAGE3DEXT,"glTexImage3DEXT");

          //default to trilinear filtering
          gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
          gl_filter_max = GL_LINEAR;
     }
}

void CheckNV3xFragmentExtensions(void) 
{
     int supportedTmu;
     glGetIntegerv(GL_MAX_ACTIVE_TEXTURES_ARB,&supportedTmu); 

     if (strstr(gl_extensions, "GL_EXT_texture3D")
         && (!COM_CheckParm ("-forcegeneric"))
         && (!COM_CheckParm ("-nonv3x"))
         && strstr(gl_extensions, "GL_NV_fragment_program")
         && strstr(gl_extensions, "GL_NV_vertex_program2"))
     {
          gl_cardtype = NV3x;

          //get TEX3d poiters                   wlgGetProcAddress
          SAFE_GET_PROC (qglTexImage3DEXT,PFNGLTEXIMAGE3DEXT,"glTexImage3DEXT");

          //get vertex_program pointers
          SAFE_GET_PROC (qglAreProgramsResidentNV,PFNGLAREPROGRAMSRESIDENTNVPROC,"glAreProgramsResidentNV");
          SAFE_GET_PROC (qglBindProgramNV,PFNGLBINDPROGRAMNVPROC,"glBindProgramNV");
          SAFE_GET_PROC (qglDeleteProgramsNV,PFNGLDELETEPROGRAMSNVPROC,"glDeleteProgramsNV");
          SAFE_GET_PROC (qglExecuteProgramNV,PFNGLEXECUTEPROGRAMNVPROC,"glExecuteProgramNV");
          SAFE_GET_PROC (qglGenProgramsNV,PFNGLGENPROGRAMSNVPROC,"glGenProgramsNV");
          SAFE_GET_PROC (qglGetProgramParameterdvNV,PFNGLGETPROGRAMPARAMETERDVNVPROC,"glGetProgramParameterdvNV");
          SAFE_GET_PROC (qglGetProgramParameterfvNV,PFNGLGETPROGRAMPARAMETERFVNVPROC,"glGetProgramParameterfvNV");
          SAFE_GET_PROC (qglGetProgramivNV,PFNGLGETPROGRAMIVNVPROC,"glGetProgramivNV");
          SAFE_GET_PROC (qglGetProgramStringNV,PFNGLGETPROGRAMSTRINGNVPROC,"glGetProgramStringNV");
          SAFE_GET_PROC (qglGetTrackMatrixivNV,PFNGLGETTRACKMATRIXIVNVPROC,"glGetTrackMatrixivNV");
          SAFE_GET_PROC (qglGetVertexAttribdvNV,PFNGLGETVERTEXATTRIBDVNVPROC,"glGetVertexAttribdvNV");
          SAFE_GET_PROC (qglGetVertexAttribfvNV,PFNGLGETVERTEXATTRIBFVNVPROC,"glGetVertexAttribfvNV");
          SAFE_GET_PROC (qglGetVertexAttribivNV,PFNGLGETVERTEXATTRIBIVNVPROC,"glGetVertexAttribivNV");
          SAFE_GET_PROC (qglGetVertexAttribPointervNV,PFNGLGETVERTEXATTRIBPOINTERVNVPROC,"glGetVertexAttribPointervNV");
          SAFE_GET_PROC (qglGetVertexAttribPointervNV,PFNGLGETVERTEXATTRIBPOINTERVNVPROC,"glGetVertexAttribPointerNV");
          SAFE_GET_PROC (qglIsProgramNV,PFNGLISPROGRAMNVPROC,"glIsProgramNV");
          SAFE_GET_PROC (qglLoadProgramNV,PFNGLLOADPROGRAMNVPROC,"glLoadProgramNV");
          SAFE_GET_PROC (qglProgramParameter4dNV,PFNGLPROGRAMPARAMETER4DNVPROC,"glProgramParameter4dNV");
          SAFE_GET_PROC (qglProgramParameter4dvNV,PFNGLPROGRAMPARAMETER4DVNVPROC,"glProgramParameter4dvNV");
          SAFE_GET_PROC (qglProgramParameter4fNV,PFNGLPROGRAMPARAMETER4FNVPROC,"glProgramParameter4fNV");
          SAFE_GET_PROC (qglProgramParameter4fvNV,PFNGLPROGRAMPARAMETER4FVNVPROC,"glProgramParameter4fvNV");
          SAFE_GET_PROC (qglProgramParameters4dvNV,PFNGLPROGRAMPARAMETERS4DVNVPROC,"glProgramParameters4dvNV");
          SAFE_GET_PROC (qglProgramParameters4fvNV,PFNGLPROGRAMPARAMETERS4FVNVPROC,"glProgramParameters4fvNV");
          SAFE_GET_PROC (qglRequestResidentProgramsNV,PFNGLREQUESTRESIDENTPROGRAMSNVPROC,"glRequestResidentProgramsNV");
          SAFE_GET_PROC (qglTrackMatrixNV,PFNGLTRACKMATRIXNVPROC,"glTrackMatrixNV");

          //default to trilinear filtering
          gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
          gl_filter_max = GL_LINEAR;
     }
}

void CheckAnisotropicExtension(void)
{
     if (strstr(gl_extensions, "GL_EXT_texture_filter_anisotropic") &&
         ( COM_CheckParm ("-anisotropic") || COM_CheckParm ("-anisotropy")) )
     {
          GLfloat maxanisotropy;

          if ( COM_CheckParm ("-anisotropy"))
               gl_textureanisotropylevel = Q_atoi(com_argv[COM_CheckParm("-anisotropy")+1]);
        
          glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxanisotropy);
          if ( gl_textureanisotropylevel >  maxanisotropy )
               gl_textureanisotropylevel = maxanisotropy;

          if ( gl_textureanisotropylevel < 1.0f )
               gl_textureanisotropylevel = 1.0f;

          Con_Printf("Anisotropic texture filter level %.0f\n", 
                     gl_textureanisotropylevel);
          gl_texturefilteranisotropic = true;
     }
}

void CheckTextureCompressionExtension(void)
{
     if (strstr(gl_extensions, "GL_ARB_texture_compression") )
     {
          Con_Printf("Texture compression available\n");
          gl_texcomp = true;
     }
}

void CheckOcclusionTest(void)
{
     if (strstr(gl_extensions, "GL_HP_occlusion_test") )
     {
          Con_Printf("Occlusion test available\n");
          gl_occlusiontest = true;
     }
}
/*
  ===============
  GL_Init
  ===============
*/
void GL_Init (void)
{
     int supportedTmu;

     gl_vendor = glGetString (GL_VENDOR);
     Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
     gl_renderer = glGetString (GL_RENDERER);
     Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

     gl_version = glGetString (GL_VERSION);
     Con_Printf ("GL_VERSION: %s\n", gl_version);
     gl_extensions = glGetString (GL_EXTENSIONS);
     Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

     Con_Printf ("%s %s\n", gl_renderer, gl_version);

     if (Q_strncasecmp ((char *)gl_renderer,"PowerVR",7)==0)
          fullsbardraw = true;

     if (Q_strncasecmp ((char *)gl_renderer,"Permedia",8)==0)
          isPermedia = true;

     Con_Printf ("Checking multitexture\n");
     CheckMultiTextureExtensions ();

     Con_Printf ("Checking diffuse bumpmap extensions\n");
     CheckDiffuseBumpMappingExtensions ();

     Con_Printf ("Checking NV3x extensions\n");
     CheckNV3xFragmentExtensions ();

     if ( gl_cardtype != NV3x )
     {
        Con_Printf ("Checking ARB extensions\n");
        CheckARBFragmentExtensions ();
     }

     if ( gl_cardtype != ARB && gl_cardtype != NV3x )
     {
          Con_Printf ("Checking GeForce 1/2/4-MX extensions\n");
          CheckSpecularBumpMappingExtensions (); 
          Con_Printf ("Checking GeForce 3/4 extensions\n");
          CheckGeforce3Extensions ();
          Con_Printf ("Checking Radeon 8500+ extensions\n");
          CheckRadeonExtensions ();
          Con_Printf ("Checking Parhelia extensions\n");
          CheckParheliaExtensions ();
     }

     Con_Printf ("Checking VAR\n");
     CheckVertexArrayRange ();
     Con_Printf ("Checking AF\n");
     CheckAnisotropicExtension ();
     Con_Printf ("Checking TC\n");
     CheckTextureCompressionExtension ();
     Con_Printf ("checking OT\n");
     CheckOcclusionTest();

     //if something goes wrong here throw an sys_error as we don't want to end up
     //having invalid function pointers called...
     switch (gl_cardtype)
     {
     case GENERIC:
          Con_Printf ("Using generic path.\n");
//	  Sys_Error("No generic path yet\n");
          BUMP_InitGeneric();
          break;
     case GEFORCE:
          Con_Printf ("Using GeForce 1/2/4-MX path\n");
//	  Sys_Error("No geforce2 path yet\n");
          BUMP_InitGeneric(); // has optimizations for GF cards (some day :)
          break;
     case GEFORCE3:
          Con_Printf ("Using GeForce 3/4 path\n");
          BUMP_InitGeforce3();
          break;
     case RADEON:
          Con_Printf ("Using Radeon path.\n");
	  BUMP_InitRadeon();
          break;
     case PARHELIA:
          Con_Printf ("Using Parhelia path.\n");
	  BUMP_InitParhelia();
          break;
     case ARB:
          Con_Printf ("Using ARB_fragment_program path.\n");
          BUMP_InitARB();
          break;
     case NV3x:
          Con_Printf ("Using NV_fragment_program path.\n");
          BUMP_InitNV3x();
          break;
     }
            
     glGetIntegerv (GL_MAX_ACTIVE_TEXTURES_ARB,&supportedTmu); 
     Con_Printf ("%i texture units\n",supportedTmu);
        
     //PENTA: enable mlook by default, people kept mailing me about how to do mlook
     Cbuf_AddText ("+mlook\n");

     glClearColor (0.5,0.5,0.5,0.5);
     glCullFace (GL_FRONT);
     glEnable (GL_TEXTURE_2D);

     glEnable (GL_ALPHA_TEST);
     glAlphaFunc (GL_GREATER, 0.666f);

     glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
     glShadeModel (GL_FLAT);

     glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
     glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
     glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

     glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
     glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#if 0
     CheckArrayExtensions ();

     glEnable (GL_VERTEX_ARRAY_EXT);
     glEnable (GL_TEXTURE_COORD_ARRAY_EXT);
     glVertexPointerEXT (3, GL_FLOAT, 0, 0, &glv.x);
     glTexCoordPointerEXT (2, GL_FLOAT, 0, 0, &glv.s);
     glColorPointerEXT (3, GL_FLOAT, 0, 0, &glv.r);
#endif

}
