/*
Copyright (C) 2001-2002 Charles Hollemeersch

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

"Shader" loading an management
*/
#include "quakedef.h"

#define SHADER_MAX_NAME 128
#define SHADER_MAX_STAGES 8
#define SHADER_MAX_TCMOD 8
#define SHADER_MAX_STATUSES 8

typedef enum {STAGE_SIMPLE, STAGE_COLOR, STAGE_BUMP, STAGE_GLOSS} stagetype_t;
typedef enum {TCMOD_ROTATE, TCMOD_SCROLL, TCMOD_SCALE, TCMOD_STRETCH} tcmodtype_t;
/**
* A single stage in the shader,
* a stage is one block { } in a shader definition
* this corresponds to a single pass for simple shaders
* or a single texture definition for bumpmapping.
*/
typedef struct tcmod_s {
	float params[7];
	tcmodtype_t type;
} tcmod_t;

typedef struct stage_s {
	stagetype_t type;
	int			numtcmods;
	tcmod_t		tcmods[SHADER_MAX_TCMOD];	
	int			numtextures;
	gltexture_t *texture[8];  //animations
	int			src_blend, dst_blend; //have special values for bumpmap passes
	int			alphatresh;
} stage_t;

#define	SURF_NOSHADOW		0x40000	//don't cast stencil shadows
#define	SURF_BUMP			0x80000	//do diffuse bumpmapping and gloss if gloss is enabled too
#define	SURF_GLOSS			0x100000//do gloss
#define	SURF_PPLIGHT		0x200000//do per pixel lighting...
									//if bump is unset it uses a flat bumpmap and no gloss
									//if bump is set and gloss unet it does only diffuse bumps

/**
* A shader, holds the stages and the general info for that shader.
*/
typedef struct shader_s {
	char		name[SHADER_MAX_NAME];
	int			flags;
	int			numstages;
	stage_t		stages[SHADER_MAX_STAGES];
	stage_t		colorStage;
	stage_t		bumpStage;
	stage_t		glossStage;
	vec3_t		fog_color;
	float		fog_dist;
	int			numstatus;
	struct shader_s	*status[SHADER_MAX_STATUSES]; //if the object the shader is on it's status is > 0 this shader will be used insead of the base shader...
	struct shader_s	*next;	//in the shader linked list
} shader_t;

static shader_t *shaderList;

/**
* Generates a clean stage before one is parsed over it
*/
void clearStage(stage_t *stage) {
	memset(stage,0,sizeof(stage_t));
	stage->type = STAGE_SIMPLE;
}

/**
* Generates a default shader from the given name.
*/
void initDefaultStage(char *texname, stage_t *stage, stagetype_t type) {
	memset(stage,0,sizeof(stage_t));
	stage->numtextures = 1;
	if (type == STAGE_BUMP) {
		stage->texture[0] = GL_CacheTexture(texname,true,TEXTURE_NORMAL);
	} else
		stage->texture[0] = GL_CacheTexture(texname,true,TEXTURE_RGB);
	stage->numtcmods = 0;
	stage->type = type;
}

/**
* Generates a default shader from the given name.
*/
void initDefaultShader(char *name, shader_t *shader) {
	char namebuff[256];

	strncpy(shader->name,name,sizeof(shader->name));
	shader->flags = 0;
	shader->numstages = 3;
	initDefaultStage(name, &shader->stages[0], STAGE_COLOR);
	sprintf(namebuff,"%s%s",name,"_normal");
	initDefaultStage(namebuff, &shader->stages[1], STAGE_BUMP);
	sprintf(namebuff,"%s%s",name,"_gloss");
	initDefaultStage(namebuff, &shader->stages[2], STAGE_GLOSS);
}

/*
================
GL_ShaderForName

Load a shader, searches for the shader in the shader list 
and returns a pointer to the shader
================
*/
shader_t *GL_ShaderForName(char *name) {

	shader_t *s;
	s = shaderList;

	while(s) {
		if (!strcmp(name,s->name)) {
			return s;
		}
		s = s->next;
	}

	s = malloc(sizeof(shader_t));
	if (!s) {
		Sys_Error("Malloc failed!");
	}

	//make a shader with the default filenames (_bump, _gloss, _normal)
	initDefaultShader(name,s);
	s->next = shaderList;
	shaderList = s;

	return s;
}

// parse some stuff but make sure you give some errors
#define GET_SAFE_TOKEN data = COM_Parse (data);\
if (!data)\
Sys_Error ("ED_ParseEntity: EOF without closing brace");\
if (com_token[0] == '}')\
  Sys_Error ("ED_ParseEntity: '}' not expected here")

/**
* Parse a tcmod command out of the shader file
*/
char *ParceTcMod(char *data, stage_t *stage) {

	GET_SAFE_TOKEN;

	if (stage->numtcmods >= SHADER_MAX_TCMOD) {
		Sys_Error("More than %i tcmods in stage\n",SHADER_MAX_TCMOD);
	}

	if (!strcmp(com_token, "rotate")) {

		stage->tcmods[stage->numtcmods].type = TCMOD_ROTATE;
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[0] = atoi(com_token);

	} else if (!strcmp(com_token, "scroll")) {

		stage->tcmods[stage->numtcmods].type = TCMOD_SCROLL;
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[0] = atoi(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[1] = atoi(com_token);

	} else if (!strcmp(com_token, "scale")) {
		
		stage->tcmods[stage->numtcmods].type = TCMOD_SCALE;
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[0] = atoi(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[1] = atoi(com_token);

	} else if (!strcmp(com_token, "stretch")) {
		
		stage->tcmods[stage->numtcmods].type = TCMOD_STRETCH;
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[0] = atoi(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[1] = atoi(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[2] = atoi(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[3] = atoi(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[4] = atoi(com_token);

	} else if (!strcmp(com_token, "turb")) {
		
		//parse it and give a warning that it's not supported
		stage->tcmods[stage->numtcmods].type = TCMOD_SCALE;
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[0] = atoi(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[1] = atoi(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[2] = atoi(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[3] = atoi(com_token);
		Con_Printf("Warning: turb not supported by Tenebrae.\n");

	} else {
		Con_Printf("Sorry not supported yet ... maybe never\n");
	}

	stage->numtcmods++;

	return data;
}

/**
* Parse a single shader stage out of the shader file
* returns the new position in the file
*/
char *ParseStage (char *data, shader_t *shader)
{
	char		command[256];
	char		texture[256];
	stage_t		stage;

	//Con_Printf("Parsing shader stage...\n");
	clearStage(&stage);

// go through all the dictionary pairs
	while (1)
	{	
		data = COM_Parse (data);

		//end of stage
		if (com_token[0] == '}') {
			//end of stage cleanup...

			//stage->texture = GL_CacheTexture(com_token);
			if (stage.type == STAGE_GLOSS) {
				shader->glossStage = stage;
				shader->flags = shader->flags | SURF_GLOSS;
			} else if (stage.type == STAGE_COLOR) {
				shader->colorStage = stage;
				shader->flags = shader->flags | SURF_PPLIGHT;	
			} else if (stage.type == STAGE_BUMP) {
				shader->bumpStage = stage;
				shader->flags = shader->flags | SURF_BUMP;
			} else {
				if (shader->numstages >= SHADER_MAX_STAGES)
					return NULL;
				shader->stages[shader->numstages] = stage;
				shader->numstages++;
			}
			break;
		}
		
		//ugh nasty
		if (!data)
			Sys_Error ("ParseStage: EOF without '}'");

		//see what command it is
		strncpy (command, com_token,sizeof(command));
		if (!strcmp(command, "stage")) {

			GET_SAFE_TOKEN;
			if (!strcmp(com_token, "diffusemap")) {
				stage.type = STAGE_COLOR;
			} else if (!strcmp(com_token, "bumpmap")) {
				stage.type = STAGE_BUMP;
			} else if (!strcmp(com_token, "specularmap")) {
				stage.type = STAGE_GLOSS;
			} else {
				Con_Printf("Unknown stage type %s\n",com_token);
			}

		} else if (!strcmp(command, "map")) {

			GET_SAFE_TOKEN;
			//add the textures to the known texture list
			//stage.texture = GL_CacheTexture(com_token);
			//Con_Printf("Cache texture %s\n",com_token);
			strncpy(texture,com_token,sizeof(texture));

		} else if (!strcmp(command, "tcMod")) {

			data = ParceTcMod(data,&stage);

		} else if (!strcmp(command, "blendfunc")) {

			GET_SAFE_TOKEN;
			stage.src_blend = SHADER_BlendModeForName(com_token);
			if (stage.src_blend < 0) {
				if (!strcmp(com_token,"blend")) {
					stage.src_blend = GL_SRC_ALPHA;
					stage.dst_blend = GL_ONE_MINUS_SRC_ALPHA;
				} else if (!strcmp(com_token,"add")) {
					stage.src_blend = GL_ONE;
					stage.dst_blend = GL_ONE;
				}
			} else {
				GET_SAFE_TOKEN;
				stage.dst_blend = SC_BlendModeForName(com_token);
			}

			if (stage.type != STAGE_SIMPLE) {
				Con_Printf("Warning: Blendfunc with nonsimpel stage type will be ignored.\n");
			}

		} else if (!strcmp(command, "alphafunc")) {

			GET_SAFE_TOKEN;
			if (!strcmp(com_token,"GE128")) {
				stage.alphatresh = 128;
			}

			if ((stage.type == STAGE_BUMP) || (stage.type == STAGE_GLOSS)){
				Con_Printf("Warning: Alphafunc with bump or normal stage type will be ignored.\n");
			}

		} else {

			Con_Printf("Unknown statement %s\n",command);
			data = COM_SkipLine(data);
			//return NULL; //stop parsing

		}
	}
	return data;
}

/**
* Automatically generate a bump stage with the bumpmap keyword
*/
void normalStage(shader_t *shader, char *name) {
	clearStage(&shader->bumpStage);
	shader->bumpStage.type = STAGE_BUMP;
	//shader->bumpstage->texture = GL_CacheTexture(com_token);	
	shader->flags = shader->flags | SURF_BUMP;
}

/**
* Automatically generate a specular stage with the specularmap keyword
*/
void specularStage(shader_t *shader, char *name) {
	clearStage(&shader->glossStage);
	shader->glossStage.type = STAGE_GLOSS;
	//shader->glossStage->texture = GL_CacheTexture(com_token);	
	shader->flags = shader->flags | SURF_GLOSS;	
}

/**
* Automatically generate a bump stage with the colormap keyword
*/
void diffuseStage(shader_t *shader, char *name) {
	clearStage(&shader->colorStage);
	shader->colorStage.type = STAGE_COLOR;
	//shader->colorStage->texture = GL_CacheTexture(com_token);	
	shader->flags = shader->flags | SURF_PPLIGHT;	
}

/**
* Parse a single shader out of the shader file
*/
char *ParseShader (char *data, shader_t *shader)
{
	char		command[256];

	while (1)
	{	
		data = COM_Parse (data);

		//end of shader
		if (com_token[0] == '}')
			break;
		
		//ugh nasty
		if (!data)
			Sys_Error ("ParseShader: EOF without '}'");

		//see what command it is
		strncpy (command, com_token,sizeof(command));
		if (command[0] == '{') {
			data = ParseStage(data, shader);
		} else if (!strcmp(command,"normalmap")) {
			GET_SAFE_TOKEN;
			normalStage(shader,com_token);
		} else if (!strcmp(command,"diffusemap")) {
			GET_SAFE_TOKEN;
			diffuseStage(shader,com_token);
		} else if (!strcmp(command,"specularmap")) {
			GET_SAFE_TOKEN;
			specularStage(shader,com_token);
		} else if (!strcmp(command,"surfaceparm")) {
			data = COM_SkipLine(data);
		} else {

			//ignore q3map and radiant commands
			if (strncmp("qer_",com_token,4) && strncmp(com_token,"q3map_",5))
				/*Con_Printf("Unknown statement %s\n",command)*/;
			data = COM_SkipLine(data);
		}
	}

	return data;
}

void LoadShadersFromString (char *data)
{	
	shader_t shader;
// parse shaders
	memset(&shader,0,sizeof(shader));
	while (1)
	{
		data = COM_Parse (data);
		if (!data)
			break;

		strncpy(shader.name,com_token,sizeof(shader.name));

		data = COM_Parse (data);
		if (!data)
			break;
		if (com_token[0] != '{')
			Sys_Error ("ED_LoadFromFile: found %s when expecting {",com_token);

		//Con_Printf("Parsing shader %s\n",shader.name);
		data = ParseShader (data, &shader);
	}
	
	if ((!fog_start.value) && (!fog_end.value)) {
		Cvar_SetValue ("gl_fog",0.0);
	}
}

void AddShaderFile(const char *filename) {
	char *buffer = COM_LoadTempFile (filename);
	Con_Printf("Parsing shaderscript: %s\n",filename);
	if (!buffer) return;
	LoadShadersFromString(buffer);
}

/**
* This should be called once during engine startup.
* 
*/
void R_InitShaders() {
	//clear list
	shaderList = NULL;
	//load all scripts
	Con_Printf("=================================\n");
	Con_Printf("Shader_Init: Initializing shaders\n");
	COM_FindAllExt("scripts","shader",AddShaderFile);
	Con_Printf("=================================\n");
}

#ifndef _WIN32
//warn non win32 devs they should free the memory...
#error Call this routine from vid shutdown!
#endif
void R_ShutdownShaders() {
	shader_t *s;

	while (shaderList) {
		s = shaderList;
		shaderList = shaderList->next;
		free(s);
	}
}