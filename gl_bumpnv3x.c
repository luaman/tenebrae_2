/*
Copyright (C) 2001-2002 Charles Hollemeersch
NV_fragment_progam version (C) 2003 Jarno Paananen

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

Same as gl_bumpmap.c but NV30+ optimized as they can't quite cope with the
ARB path
These routines require 6 texture units, NV vertex shader and pixel shader extensions

All lights require 1 pass:
1 diffuse + specular with optional light filter

*/

#include "quakedef.h"


// NV_vertex_program is already in glquake.h

// NV_fragment_program
#ifndef GL_NV_fragment_program
#define GL_FRAGMENT_PROGRAM_NV            0x8870
#define GL_MAX_TEXTURE_COORDS_NV          0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_NV     0x8872
#define GL_FRAGMENT_PROGRAM_BINDING_NV    0x8873
#define GL_PROGRAM_ERROR_STRING_NV        0x8874
#define GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV 0x8868
#endif

#ifndef GL_NV_fragment_program
#define GL_NV_fragment_program 1
#ifdef GL_GLEXT_PROTOTYPES
extern void APIENTRY qglProgramNamedParameter4fNV(GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void APIENTRY qglProgramNamedParameter4dNV(GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void APIENTRY qglProgramNamedParameter4fvNV(GLuint id, GLsizei len, const GLubyte *name, const GLfloat v[]);
extern void APIENTRY qglProgramNamedParameter4dvNV(GLuint id, GLsizei len, const GLubyte *name, const GLdouble v[]);
extern void APIENTRY qglGetProgramNamedParameterfvNV(GLuint id, GLsizei len, const GLubyte *name, GLfloat *params);
extern void APIENTRY qglGetProgramNamedParameterdvNV(GLuint id, GLsizei len, const GLubyte *name, GLdouble *params);
#endif
typedef void (APIENTRY * PFNGLPROGRAMNAMEDPARAMETER4FNVPROC) (GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLPROGRAMNAMEDPARAMETER4DNVPROC) (GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLPROGRAMNAMEDPARAMETER4FVNVPROC) (GLuint id, GLsizei len, const GLubyte *name, const GLfloat v[]);
typedef void (APIENTRY * PFNGLPROGRAMNAMEDPARAMETER4DVNVPROC) (GLuint id, GLsizei len, const GLubyte *name, const GLdouble v[]);
typedef void (APIENTRY * PFNGLGETPROGRAMNAMEDPARAMETERFVNVPROC) (GLuint id, GLsizei len, const GLubyte *name, GLfloat *params);
typedef void (APIENTRY * PFNGLGETPROGRAMNAMEDPARAMETERDVNVPROC) (GLuint id, GLsizei len, const GLubyte *name, GLdouble *params);

typedef void (APIENTRY * glProgramLocalParameter4fARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
glProgramLocalParameter4fARBPROC qglProgramLocalParameter4fARB;
#endif

// EXT_stencil_two_side
#ifndef GL_EXT_stencil_two_side
#define GL_STENCIL_TEST_TWO_SIDE_EXT      0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT        0x8911
#endif

#ifndef GL_EXT_stencil_two_side
#define GL_EXT_stencil_two_side 1
typedef void (APIENTRY * PFNGLACTIVESTENCILFACEEXTPROC) (GLenum face);
PFNGLACTIVESTENCILFACEEXTPROC qglActiveStencilFaceEXT;
#endif

static char bump_vertex_program[] =
"!!VP1.1\n"
"OPTION NV_position_invariant;\n"
"DP4   o[TEX3].x, c[4], v[OPOS];\n"
"DP4   o[TEX3].y, c[5], v[OPOS];\n"
"DP4   o[TEX3].z, c[6], v[OPOS];\n"
"DP4   o[TEX3].w, c[7], v[OPOS];\n"
"MOV   o[TEX0], v[TEX0];\n"
"ADD   R0, c[0], -v[OPOS];\n"
"DP3   R0.w, R0, R0;\n"
"RSQ   R0.w, R0.w;\n"
"MUL   R0, R0, R0.w;\n"
"ADD   R1, c[1], -v[OPOS];\n"
"DP3   R1.w, R1, R1;\n"
"RSQ   R1.w, R1.w;\n"
"MUL   R1, R1, R1.w;\n"
"ADD   R1, R0, R1;\n"
"MUL   R1, R1, c[2];\n"
"DP3   o[TEX1].x, R0, v[TEX1];\n"
"DP3   o[TEX1].y, R0, v[TEX2];\n"
"DP3   o[TEX1].z, R0, v[TEX3];\n"
"DP3   o[TEX2].x, R1, v[TEX1];\n"
"DP3   o[TEX2].y, R1, v[TEX2];\n"
"DP3   o[TEX2].z, R1, v[TEX3];\n"
"MOV   o[COL0], v[COL0];\n"
"END";


static char bump_vertex_program2[] =
"!!VP1.1\n"
"OPTION NV_position_invariant;\n"
"DP4   o[TEX3].x, c[4], v[OPOS];\n"
"DP4   o[TEX3].y, c[5], v[OPOS];\n"
"DP4   o[TEX3].z, c[6], v[OPOS];\n"
"DP4   o[TEX3].w, c[7], v[OPOS];\n"
"DP4   o[TEX4].x, c[8], v[OPOS];\n"
"DP4   o[TEX4].y, c[9], v[OPOS];\n"
"DP4   o[TEX4].z, c[10], v[OPOS];\n"
"DP4   o[TEX4].w, c[11], v[OPOS];\n"
"MOV   o[TEX0], v[TEX0];\n"
"ADD   R0, c[0], -v[OPOS];\n"
"DP3   R0.w, R0, R0;\n"
"RSQ   R0.w, R0.w;\n"
"MUL   R0, R0, R0.w;\n"
"ADD   R1, c[1], -v[OPOS];\n"
"DP3   R1.w, R1, R1;\n"
"RSQ   R1.w, R1.w;\n"
"MUL   R1, R1, R1.w;\n"
"ADD   R1, R0, R1;\n"
"MUL   R1, R1, c[2];\n"
"DP3   o[TEX1].x, R0, v[TEX1];\n"
"DP3   o[TEX1].y, R0, v[TEX2];\n"
"DP3   o[TEX1].z, R0, v[TEX3];\n"
"DP3   o[TEX2].x, R1, v[TEX1];\n"
"DP3   o[TEX2].y, R1, v[TEX2];\n"
"DP3   o[TEX2].z, R1, v[TEX3];\n"
"MOV   o[COL0], v[COL0];\n"
"END";

static char delux_vertex_program[] = 
"!!VP1.1\n"
"OPTION NV_position_invariant;\n"
"MOV   o[TEX0], v[TEX0];\n"
"MOV   o[TEX1], v[TEX1];\n"
"MOV   o[TEX2], v[TEX2];\n"
"MOV   o[TEX3], v[TEX3];\n"
"MOV   o[TEX4], v[TEX4];\n"
"MOV   o[TEX5], v[OPOS];\n"
"END";


static char bump_fragment_program[] =
"!!FP1.0\n"
"TEX H0, f[TEX0], TEX0, 2D;\n"
"TEX H1, f[TEX1], TEX3, CUBE;\n"
"MADH H0.xyz, H0, 2, -1;\n"
"MADH H1.xyz, H1, 2, -1;\n"
"DP3H_SAT H2.x, H0, H1;\n"
"TEX H3, f[TEX0], TEX1, 2D;\n"
"MULH H3, H3, H2.x;\n"
"MULH H3, H3, f[COL0];\n"
"MULH_SAT H2.y, H1.z, 8;\n"
"MULH H2.x, H2.x, H2.y;\n"
"TEX H1, f[TEX2], TEX3, CUBE;\n"
"MADH H1.xyz, H1, 2, -1;\n"
"DP3H_SAT H2.z, H0, H1;\n"
"POWH H2.z, H2.z, 16;\n"
"MULX H2.z, H2.z, H0.w;\n"
"MADX H3, H3, H2.y, H2.z;\n"
"TEX H1, f[TEX3], TEX2, 3D;\n"
"MULX_SAT o[COLH], H3, H1;\n"
"END";

static char bump_fragment_program_colored[] =
"!!FP1.0\n"
"TEX H0, f[TEX0], TEX0, 2D;\n"
"TEX H1, f[TEX1], TEX3, CUBE;\n"
"MADH H0.xyz, H0, 2, -1;\n"
"MADH H1.xyz, H1, 2, -1;\n"
"DP3H_SAT H2.x, H0, H1;\n"
"TEX H3, f[TEX0], TEX1, 2D;\n"
"MULH H3, H3, H2.x;\n"
"MULH H3, H3, f[COL0];\n"
"MULH_SAT H2.y, H1.z, 8;\n"
"MULH H2.x, H2.x, H2.y;\n"
"TEX H1, f[TEX2], TEX3, CUBE;\n"
"MADH H1.xyz, H1, 2, -1;\n"
"DP3H_SAT H2.z, H0, H1;\n"
"POWH H2.z, H2.z, 16;\n"
"TEX H1, f[TEX3], TEX2, 3D;\n"
"TEX H0, f[TEX0], TEX4, 2D;\n"
"MULX H0, H2.z, H0;\n"
"MADX H3, H3, H2.y, H0;\n"
"MULX_SAT o[COLH], H3, H1;\n"
"END";

static char bump_fragment_program2[] =
"!!FP1.0\n"
"TEX H0, f[TEX0], TEX0, 2D;\n"
"TEX H1, f[TEX1], TEX3, CUBE;\n"
"MADH H0.xyz, H0, 2, -1;\n"
"MADH H1.xyz, H1, 2, -1;\n"
"DP3H_SAT H2.x, H0, H1;\n"
"TEX H3, f[TEX0], TEX1, 2D;\n"
"MULH H3, H3, H2.x;\n"
"MULH H3, H3, f[COL0];\n"
"MULH_SAT H2.y, H1.z, 8;\n"
"MULH H2.x, H2.x, H2.y;\n"
"TEX H1, f[TEX2], TEX3, CUBE;\n"
"MADH H1.xyz, H1, 2, -1;\n"
"DP3H_SAT H2.z, H0, H1;\n"
"POWH H2.z, H2.z, 16;\n"
"MULX H2.z, H2.z, H0.w;\n"
"MADX H3, H3, H2.y, H2.z;\n"
"TEX H1, f[TEX3], TEX2, 3D;\n"
"TEX H2, f[TEX4], TEX4, CUBE;\n"
"MULX H3, H3, H2;\n"
"MULX_SAT o[COLH], H3, H1;\n"
"END";

static char bump_fragment_program2_colored[] =
"!!FP1.0\n"
"TEX H0, f[TEX0], TEX0, 2D;\n"
"TEX H1, f[TEX1], TEX3, CUBE;\n"
"MADH H0.xyz, H0, 2, -1;\n"
"MADH H1.xyz, H1, 2, -1;\n"
"DP3H_SAT H2.x, H0, H1;\n"
"TEX H3, f[TEX0], TEX1, 2D;\n"
"MULH H3, H3, H2.x;\n"
"MULH H3, H3, f[COL0];\n"
"MULH_SAT H2.y, H1.z, 8;\n"
"MULH H2.x, H2.x, H2.y;\n"
"TEX H1, f[TEX2], TEX3, CUBE;\n"
"MADH H1.xyz, H1, 2, -1;\n"
"DP3H_SAT H2.z, H0, H1;\n"
"POWH H2.z, H2.z, 16;\n"
"TEX H0, f[TEX0], TEX5, CUBE;\n"
"MULX H0, H2.z, H0;\n"
"MADX H3, H3, H2.y, H0;\n"
"TEX H1, f[TEX3], TEX2, 3D;\n"
"TEX H2, f[TEX4], TEX4, CUBE;\n"
"MULX H3, H3, H2;\n"
"MULX_SAT o[COLH], H3, H1;\n"
"END";

static char delux_fragment_program[] =
"!!FP1.0\n"
"TEX R0, f[TEX1], TEX1, 2D;\n"
"TEX R1, f[TEX0], TEX2, 2D;\n"
"MADR R0.xyz, R0, 2, -1;\n"
"ADDR R3.xyz, p[0], -f[TEX5];\n"
"DP3R R2.x, R0, f[TEX2];\n"
"DP3R R2.y, R0, f[TEX3];\n"
"DP3R R2.z, R0, f[TEX4];\n"
"DP3R R4.x, R3, f[TEX2];\n"
"DP3R R4.y, R3, f[TEX3];\n"
"DP3R R4.z, R3, f[TEX4];\n"
"DP3R R0.x, R2, R2;\n"
"RSQR R0.x, R0.x;\n"
"MULR R2.xyz, R0.x, R2;\n"
"DP3R R0.x, R4, R4;\n"
"RSQR R0.x, R0.x;\n"
"MADR R4.xyz, R0.x, R4, R2;\n"
"MULR R4.xyz, R4, 0.5;\n"
"MOVR R0.w, R1.w;\n"
"MADR R0.xyz, R1, 2, -1;\n"
"DP3R_SAT R1.x, R4, R0;\n"
"POWR R1.x, R1.x, 16;\n"
"MULR R1.x, R1.x, R0.w;\n"
"DP3R_SAT R0.x, R2, R0;\n"
"TEX R2, f[TEX0], TEX3, 2D;\n"
"TEX R3, f[TEX1], TEX0, 2D;\n"
"MADR R0, R2, R0.x, R1.x;\n"
"MULR o[COLR], R0, R3;\n"
"END";

static char delux_fragment_program_colored[] =
"!!FP1.0\n"
"TEX R0, f[TEX1], TEX1, 2D;\n"
"TEX R1, f[TEX0], TEX2, 2D;\n"
"MADR R0.xyz, R0, 2, -1;\n"
"ADDR R3.xyz, p[0], -f[TEX5];\n"
"DP3R R2.x, R0, f[TEX2];\n"
"DP3R R2.y, R0, f[TEX3];\n"
"DP3R R2.z, R0, f[TEX4];\n"
"DP3R R4.x, R3, f[TEX2];\n"
"DP3R R4.y, R3, f[TEX3];\n"
"DP3R R4.z, R3, f[TEX4];\n"
"DP3R R0.x, R2, R2;\n"
"RSQR R0.x, R0.x;\n"
"MULR R2.xyz, R0.x, R2;\n"
"DP3R R0.x, R4, R4;\n"
"RSQR R0.x, R0.x;\n"
"MADR R4.xyz, R0.x, R4, R2;\n"
"MULR R4.xyz, R4, 0.5;\n"
"MOVR R0.w, R1.w;\n"
"MADR R0.xyz, R1, 2, -1;\n"
"DP3R_SAT R1.x, R4, R0;\n"
"POWR R1.x, R1.x, 16;\n"
"TEX R0, f[TEX0], TEX4, 2D;\n"
"MULR R1, R1.x, R0;\n"
"DP3R_SAT R0.x, R2, R0;\n"
"TEX R2, f[TEX0], TEX3, 2D;\n"
"TEX R3, f[TEX1], TEX0, 2D;\n"
"MADR R0, R2, R0.x, R1;\n"
"MULR o[COLR], R0, R3;\n"
"END";


typedef enum
{
    V_BUMP_PROGRAM = 0,
    V_BUMP_PROGRAM2,
    V_DELUX_PROGRAM,
    MAX_V_PROGRAM
} v_programs;

typedef enum
{
    F_BUMP_PROGRAM = 0,
    F_BUMP_PROGRAM_COLOR,
    F_BUMP_PROGRAM2,
    F_BUMP_PROGRAM2_COLOR,
    F_DELUX_PROGRAM,
    F_DELUX_PROGRAM_COLOR,
    MAX_F_PROGRAM
} f_programs;

static char* vertex_progs[MAX_V_PROGRAM] = 
{
    bump_vertex_program,
    bump_vertex_program2,
    delux_vertex_program
};

static char* fragment_progs[MAX_F_PROGRAM] = 
{
    bump_fragment_program,
    bump_fragment_program_colored,
    bump_fragment_program2,
    bump_fragment_program2_colored,
    delux_fragment_program,
    delux_fragment_program_colored
};

static GLuint vertex_programs[MAX_V_PROGRAM];
static GLuint fragment_programs[MAX_F_PROGRAM];


//#define NV3xDEBUG

#if defined(NV3xDEBUG) && defined(_WIN32)
static void NV3x_checkerror()
{
    GLuint error = glGetError();
    if ( error != GL_NO_ERROR )
    {
        int line;
        const char* err;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_NV, &line);
        err = glGetString(GL_PROGRAM_ERROR_STRING_NV);

        _asm { int 3 };
    }
}
#else

#define NV3x_checkerror() do { } while(0)

#endif


void NV3x_CreateShaders()
{
    int i;

#if !defined(__APPLE__) && !defined (MACOSX)
    SAFE_GET_PROC(qglProgramLocalParameter4fARB,glProgramLocalParameter4fARBPROC,"glProgramLocalParameter4fARB");

#endif /* !__APPLE__ && !MACOSX */

    glEnable(GL_VERTEX_PROGRAM_NV);
    NV3x_checkerror();

    qglGenProgramsNV(MAX_V_PROGRAM, &vertex_programs[0]);
    NV3x_checkerror();
    for ( i = 0; i < MAX_V_PROGRAM; i++ )
    {
        qglLoadProgramNV( GL_VERTEX_PROGRAM_NV, vertex_programs[i],
    		          strlen(vertex_progs[i]), (const GLubyte *) vertex_progs[i]);
        NV3x_checkerror();
    }

    // Track the texture unit 2 matrix in registers 4-7
    qglTrackMatrixNV( GL_VERTEX_PROGRAM_NV, 4, GL_TEXTURE2_ARB, GL_IDENTITY_NV );

    // Track the texture unit 3 matrix in registers 8-11
    qglTrackMatrixNV( GL_VERTEX_PROGRAM_NV, 8, GL_TEXTURE3_ARB, GL_IDENTITY_NV );

    glDisable(GL_VERTEX_PROGRAM_NV);
    NV3x_checkerror();

    glEnable(GL_FRAGMENT_PROGRAM_NV);
    NV3x_checkerror();

    qglGenProgramsNV(MAX_F_PROGRAM, &fragment_programs[0]);
    NV3x_checkerror();
    for ( i = 0; i < MAX_F_PROGRAM; i++ )
    {
        qglLoadProgramNV( GL_FRAGMENT_PROGRAM_NV, fragment_programs[i],
    		          strlen(fragment_progs[i]), (const GLubyte *) fragment_progs[i]);
        NV3x_checkerror();
    }

    glDisable(GL_FRAGMENT_PROGRAM_NV);
    NV3x_checkerror();
}

void NV3x_DisableBumpShader(shader_t* shader)
{
    //tex 0 = normal map
    //tex 1 = color map
    //tex 2 = attenuation
    //tex 3 = normalization cubemap
    //tex 4 = (optional light filter, depends on light settings)
    //tex 4/5 = colored gloss map if used

    glDisable(GL_FRAGMENT_PROGRAM_NV);
    glDisable(GL_VERTEX_PROGRAM_NV);

    GL_SelectTexture(GL_TEXTURE2_ARB);
    glPopMatrix();

    if (currentshadowlight->filtercube)
    {        
	GL_SelectTexture(GL_TEXTURE3_ARB);
	glPopMatrix();
    }
    glMatrixMode(GL_MODELVIEW);

    GL_SelectTexture(GL_TEXTURE0_ARB);
}


void NV3x_EnableBumpShader(const transform_t *tr, const lightobject_t *lo,
                          qboolean alias, shader_t* shader)
{
    //tex 0 = normal map
    //tex 1 = color map
    //tex 2 = attenuation
    //tex 3 = normalization cubemap
    //tex 4 = (optional light filter, depends on light settings)
    //tex 4/5 = colored gloss map if used

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
    glEnable(GL_VERTEX_PROGRAM_NV);
    NV3x_checkerror();
    glEnable(GL_FRAGMENT_PROGRAM_NV);
    NV3x_checkerror();

    GL_SelectTexture(GL_TEXTURE3_ARB);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, normcube_texture_object);

    if (currentshadowlight->filtercube)
    {
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
        GL_SetupCubeMapMatrix(tr);

        GL_SelectTexture(GL_TEXTURE4_ARB);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, currentshadowlight->filtercube);

	qglBindProgramNV( GL_VERTEX_PROGRAM_NV, vertex_programs[V_BUMP_PROGRAM2] );
	NV3x_checkerror();

        if ( shader->glossstages[0].type == STAGE_GLOSS )
        {
            qglBindProgramNV( GL_FRAGMENT_PROGRAM_NV, fragment_programs[F_BUMP_PROGRAM2_COLOR] );
            GL_SelectTexture(GL_TEXTURE5_ARB);
            GL_BindAdvanced(shader->glossstages[0].texture[0]);
        }
        else
        {
            qglBindProgramNV( GL_FRAGMENT_PROGRAM_NV, fragment_programs[F_BUMP_PROGRAM2] );
        }
	NV3x_checkerror();
    }
    else
    {
	qglBindProgramNV( GL_VERTEX_PROGRAM_NV, vertex_programs[V_BUMP_PROGRAM] );
	NV3x_checkerror();
        if ( shader->glossstages[0].type == STAGE_GLOSS )
        {
	    qglBindProgramNV( GL_FRAGMENT_PROGRAM_NV, fragment_programs[F_BUMP_PROGRAM_COLOR] );
            GL_SelectTexture(GL_TEXTURE4_ARB);
            GL_BindAdvanced(shader->glossstages[0].texture[0]);
        }
        else
        {
	    qglBindProgramNV( GL_FRAGMENT_PROGRAM_NV, fragment_programs[F_BUMP_PROGRAM] );
        }
	NV3x_checkerror();
    }

    GL_SelectTexture(GL_TEXTURE0_ARB);
    qglProgramParameter4fNV( GL_VERTEX_PROGRAM_NV, 0, lo->objectorigin[0],
			     lo->objectorigin[1],  lo->objectorigin[2], 0.0f);
    NV3x_checkerror();
    qglProgramParameter4fNV( GL_VERTEX_PROGRAM_NV, 1, lo->objectvieworg[0],
    		             lo->objectvieworg[1],  lo->objectvieworg[2], 0.0f);
    NV3x_checkerror();
    qglProgramParameter4fNV( GL_VERTEX_PROGRAM_NV, 2, 0.5f, 0.0f, 0.0f, 0.0f);
				
    NV3x_checkerror();
}

void NV3x_EnableDeluxShader(shader_t* shader)
{
    glEnable(GL_VERTEX_PROGRAM_NV);
    NV3x_checkerror();
    glEnable(GL_FRAGMENT_PROGRAM_NV);
    NV3x_checkerror();
    qglBindProgramNV( GL_VERTEX_PROGRAM_NV, vertex_programs[V_DELUX_PROGRAM] );
    NV3x_checkerror();
    if ( shader->glossstages[0].type == STAGE_GLOSS )
    {
        qglBindProgramNV( GL_FRAGMENT_PROGRAM_NV, fragment_programs[F_DELUX_PROGRAM_COLOR] );
        GL_SelectTexture(GL_TEXTURE4_ARB);
        GL_BindAdvanced(shader->glossstages[0].texture[0]);
    }
    else
    {
        qglBindProgramNV( GL_FRAGMENT_PROGRAM_NV, fragment_programs[F_DELUX_PROGRAM] );
    }
    NV3x_checkerror();
    qglProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_NV, 0,
		 		   r_refdef.vieworg[0], r_refdef.vieworg[1],
				   r_refdef.vieworg[2], 0.0);
    NV3x_checkerror();
}

void NV3x_DisableDeluxShader(shader_t* shader)
{
    glDisable(GL_FRAGMENT_PROGRAM_NV);
    glDisable(GL_VERTEX_PROGRAM_NV);
}


/************************

Shader utility routines

*************************/

void NV3x_SetupTcMod(tcmod_t *tc)
{
    switch (tc->type)
    {
    case TCMOD_ROTATE:
	glTranslatef(0.5,0.5,0.0);
	glRotatef(realtime * tc->params[0],0,0,1);
	glTranslatef(-0.5, -0.5, 0.0);
	break;
    case TCMOD_SCROLL:
	glTranslatef(realtime * tc->params[0], realtime * tc->params[1], 0.0);
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


void NV3x_SetupSimpleStage(stage_t *s)
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
	NV3x_SetupTcMod(&s->tcmods[i]);	
    }

    if (s->src_blend > -1)
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
	GL_BindAdvanced(s->texture[0]);
}

/************************

Generic triangle list routines

*************************/

void FormatError(); // In gl_bumpgf.c

void NV3x_drawTriangleListBump (const vertexdef_t *verts, int *indecies,
			       int numIndecies, shader_t *shader,
			       const transform_t *tr, const lightobject_t *lo)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);

    GL_AddColor();
    glColor3fv(&currentshadowlight->color[0]);

    NV3x_EnableBumpShader(tr, lo, true, shader);

    //bind the correct textures
    GL_SelectTexture(GL_TEXTURE0_ARB);
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    if (shader->numbumpstages > 0)
	GL_BindAdvanced(shader->bumpstages[0].texture[0]);
    if (!verts->texcoords)
	FormatError();
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);

    GL_SelectTexture(GL_TEXTURE1_ARB);
    qglClientActiveTextureARB(GL_TEXTURE1_ARB);
    if (shader->numcolorstages > 0)
	GL_BindAdvanced(shader->colorstages[0].texture[0]);
    if (!verts->tangents)
	FormatError();
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(3, GL_FLOAT, verts->tangentstride, verts->tangents);

    if (!verts->binormals)
	FormatError();
    GL_SelectTexture(GL_TEXTURE2_ARB);
    qglClientActiveTextureARB(GL_TEXTURE2_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(3, GL_FLOAT, verts->binormalstride, verts->binormals);

    if (!verts->normals)
	FormatError();
    GL_SelectTexture(GL_TEXTURE3_ARB);
    qglClientActiveTextureARB(GL_TEXTURE3_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(3, GL_FLOAT, verts->normalstride, verts->normals);

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
    NV3x_DisableBumpShader(shader);
}

void NV3x_drawTriangleListBase (vertexdef_t *verts, int *indecies,
			       int numIndecies, shader_t *shader,
                               int lightmapIndex)
{
    int i;

    glGetError();
    glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    GL_SelectTexture(GL_TEXTURE0_ARB);
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    if (!shader->cull)
    { 
	glDisable(GL_CULL_FACE); 
    } 

	glColor3ub(255,255,255);

    for ( i = 0; i < shader->numstages; i++)
    {
	NV3x_SetupSimpleStage(&shader->stages[i]);
	glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
	glPopMatrix();
    }
    glMatrixMode(GL_MODELVIEW);

    if (verts->lightmapcoords && (lightmapIndex >= 0) && (shader->flags & SURF_PPLIGHT)) 
    {
	//Delux lightmapping
	qboolean usedelux = (sh_delux.value != 0);
	if (shader->numcolorstages)
	{
	    if (shader->colorstages[0].alphatresh > 0)
		usedelux = false;
	}

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
	    glEnable(GL_TEXTURE_2D);
            qglClientActiveTextureARB(GL_TEXTURE0_ARB);
	    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	    GL_Bind(lightmap_textures+lightmapIndex);

	    // Delux map
            GL_SelectTexture(GL_TEXTURE1_ARB);
	    glEnable(GL_TEXTURE_2D);
            qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	    glTexCoordPointer(2, GL_FLOAT, verts->lightmapstride,
			      verts->lightmapcoords);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	    GL_Bind(lightmap_textures+lightmapIndex+1);

	    // Setup normal map
            GL_SelectTexture(GL_TEXTURE2_ARB);
	    glEnable(GL_TEXTURE_2D);
	    if (shader->numbumpstages)
	    {
		if (shader->bumpstages[0].numtextures)
		    GL_BindAdvanced(shader->bumpstages[0].texture[0]);
	    }
            qglClientActiveTextureARB(GL_TEXTURE2_ARB);
	    glTexCoordPointer(3, GL_FLOAT, verts->tangentstride,
			      verts->tangents);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

            // Setup base texture
            GL_SelectTexture(GL_TEXTURE3_ARB);
	    glEnable(GL_TEXTURE_2D);
            qglClientActiveTextureARB(GL_TEXTURE3_ARB);
	    glTexCoordPointer(3, GL_FLOAT, verts->binormalstride,
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
	    glTexCoordPointer(3, GL_FLOAT, verts->normalstride,
			      verts->normals);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	    NV3x_EnableDeluxShader(shader);
	    glDrawElements(GL_TRIANGLES,numIndecies,GL_UNSIGNED_INT,indecies);
	    NV3x_DisableDeluxShader(shader);

            qglClientActiveTextureARB(GL_TEXTURE4_ARB);
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            GL_SelectTexture(GL_TEXTURE3_ARB);
	    glDisable(GL_TEXTURE_2D);
            qglClientActiveTextureARB(GL_TEXTURE3_ARB);
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            GL_SelectTexture(GL_TEXTURE2_ARB);
	    glDisable(GL_TEXTURE_2D);
            qglClientActiveTextureARB(GL_TEXTURE2_ARB);
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            GL_SelectTexture(GL_TEXTURE1_ARB);
	    glDisable(GL_TEXTURE_2D);
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
	    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	    GL_SelectTexture(GL_TEXTURE1_ARB);
	    glEnable(GL_TEXTURE_2D);
	    GL_Bind(lightmap_textures+lightmapIndex);
            qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	    glTexCoordPointer(2, GL_FLOAT, verts->lightmapstride, verts->lightmapcoords);
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
	if (verts->colors && (shader->flags & SURF_PPLIGHT))
	{
	    glColorPointer(3, GL_UNSIGNED_BYTE, verts->colorstride, verts->colors);
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

void NV3x_sendSurfacesBase(msurface_t **surfs, int numSurfaces,
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

void NV3x_sendSurfacesDeLux(msurface_t **surfs, int numSurfaces,
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

void NV3x_drawSurfaceListBase (vertexdef_t* verts, msurface_t** surfs,
			      int numSurfaces, shader_t* shader)
{
    int i;
    int usedelux;

        checkerror();
    glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    GL_SelectTexture(GL_TEXTURE0_ARB);
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);

    glColor3ub(255,255,255);

    if (!shader->cull)
    {
	glDisable(GL_CULL_FACE);
	//Con_Printf("Cullstuff %s\n",shader->name);
    }


    for (i = 0; i < shader->numstages; i++)
    {
	NV3x_SetupSimpleStage(&shader->stages[i]);
	NV3x_sendSurfacesBase(surfs, numSurfaces, false);
	glPopMatrix();
    }
    if (verts->lightmapcoords && (shader->flags & SURF_PPLIGHT))
    {
	GL_SelectTexture(GL_TEXTURE1_ARB);
        qglClientActiveTextureARB(GL_TEXTURE1_ARB);
	glTexCoordPointer(2, GL_FLOAT, verts->lightmapstride,
	                  verts->lightmapcoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	//Delux lightmapping
	usedelux = (sh_delux.value != 0);
        if (shader->colorstages[0].alphatresh > 0)
            usedelux = false;

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

            GL_SelectTexture(GL_TEXTURE0_ARB);
	    glEnable(GL_TEXTURE_2D);
    
            GL_SelectTexture(GL_TEXTURE1_ARB);
	    glEnable(GL_TEXTURE_2D);

	    // Setup normal map
            GL_SelectTexture(GL_TEXTURE2_ARB);
	    glEnable(GL_TEXTURE_2D);
	    if (shader->numbumpstages)
	    {
		if (shader->bumpstages[0].numtextures)
		    GL_BindAdvanced(shader->bumpstages[0].texture[0]);
	    }

            // Setup base texture
            GL_SelectTexture(GL_TEXTURE3_ARB);
	    glEnable(GL_TEXTURE_2D);
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

	    NV3x_EnableDeluxShader(shader);
	    NV3x_sendSurfacesDeLux(surfs, numSurfaces, true);
	    NV3x_DisableDeluxShader(shader);

            GL_SelectTexture(GL_TEXTURE3_ARB);
	    glDisable(GL_TEXTURE_2D);
            GL_SelectTexture(GL_TEXTURE2_ARB);
	    glDisable(GL_TEXTURE_2D);
            GL_SelectTexture(GL_TEXTURE1_ARB);
	    glDisable(GL_TEXTURE_2D);
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

	    NV3x_sendSurfacesBase(surfs, numSurfaces, true);

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

void NV3x_sendSurfacesTA(msurface_t** surfs, int numSurfaces, const transform_t *tr, const lightobject_t *lo)
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
                NV3x_DisableBumpShader(lastshader);
                NV3x_EnableBumpShader(tr, lo, true, shader);
            }
            else
            {
                if ( !lastshader )
                {
                    // Enable shader for the first surface
                    NV3x_EnableBumpShader(tr, lo, true, shader);
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
	    GL_SelectTexture(GL_TEXTURE1_ARB);
	    if (shader->numcolorstages > 0)
		GL_BindAdvanced(shader->colorstages[0].texture[0]);
            if ( shader->glossstages > 0 )
            {
                // Bind colored gloss
                if (currentshadowlight->filtercube)
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
        NV3x_DisableBumpShader(lastshader);
}



void NV3x_drawSurfaceListBump (vertexdef_t *verts, msurface_t **surfs,
			      int numSurfaces,const transform_t *tr, const lightobject_t *lo)
{
    glVertexPointer(3, GL_FLOAT, verts->vertexstride, verts->vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, verts->texcoordstride, verts->texcoords);

    GL_AddColor();
    glColor3fv(&currentshadowlight->color[0]);

    NV3x_sendSurfacesTA(surfs,numSurfaces, tr, lo);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


typedef struct allocchain_s
{
    struct allocchain_s* next;
    char data[1];//variable sized
} allocchain_t;

static allocchain_t* allocChain = NULL;

void* NV3x_getDriverMem(size_t size, drivermem_t hint)
{
    allocchain_t *r = (allocchain_t *)malloc(size+sizeof(void *));
    r->next = allocChain;
    allocChain = r;
    return &r->data[0];
}

void NV3x_freeAllDriverMem(void)
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

void NV3x_freeDriver(void)
{
    //nothing here...
}


void BUMP_InitNV3x(void)
{
    GLint errPos, errCode;
    const GLubyte *errString;

    if ( gl_cardtype != NV3x ) return;

    NV3x_CreateShaders();


    //bind the correct stuff to the bump mapping driver
    gl_bumpdriver.drawSurfaceListBase = NV3x_drawSurfaceListBase;
    gl_bumpdriver.drawSurfaceListBump = NV3x_drawSurfaceListBump;
    gl_bumpdriver.drawTriangleListBase = NV3x_drawTriangleListBase;
    gl_bumpdriver.drawTriangleListBump = NV3x_drawTriangleListBump;
    gl_bumpdriver.getDriverMem = NV3x_getDriverMem;
    gl_bumpdriver.freeAllDriverMem = NV3x_freeAllDriverMem;
    gl_bumpdriver.freeDriver = NV3x_freeDriver;
}
