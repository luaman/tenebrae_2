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

"Shader" loading an management.

All .shader files are parsed on engine startup.  We don't load textures for these of course,
textures are only loaded per level.
*/
#include "quakedef.h"

static shader_t *shaderList;

/*=====================================================

	Generic shader routines

======================================================*/

void GL_ShaderLoadTextures(shader_t *shader);

/**
* Generates a clean stage before one is parsed over it
*/
void clearStage(stage_t *stage) {
	memset(stage,0,sizeof(stage_t));
	stage->texture[0] = NULL;
	stage->type = STAGE_SIMPLE;
	stage->src_blend = -1;
	stage->dst_blend = -1;
}

/**
* Generates a default shader from the given name.
*/
void initDefaultStage(char *texname, stage_t *stage, stagetype_t type) {
	//memset(stage,0,sizeof(stage_t));
	clearStage(stage);
	stage->numtextures = 1;
	stage->numtcmods = 0;
	stage->type = type;
	strncpy(stage->filename,texname,2*MAX_QPATH+1);
}

/**
* Generates a default shader from the given name.
*/
void initDefaultShader(char *name, shader_t *shader) {
	char namebuff[256];

	memset(shader,0,sizeof(shader));
	strncpy(shader->name,name,sizeof(shader->name));
	shader->flags = 0;
	shader->numstages = 0;
	shader->mipmap = true;
	shader->cull = true;
	sprintf(namebuff,"%s.tga",name);
	initDefaultStage(namebuff, &shader->colorstages[0], STAGE_COLOR);
	sprintf(namebuff,"%s_norm.tga|%s_gloss.tga",name,name);
	initDefaultStage(namebuff, &shader->bumpstages[0], STAGE_BUMP);
	initDefaultStage(namebuff, &shader->glossstages[0], STAGE_GRAYGLOSS);

	shader->numbumpstages = 1;
	shader->numglossstages = 1;
	shader->numcolorstages = 1;
	shader->flags |= (SURF_GLOSS | SURF_PPLIGHT | SURF_BUMP);
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

	//Con_Printf("ShaderForName %s\n",name);
	while(s) {
		if (!strcmp(name,s->name)) {
			GL_ShaderLoadTextures(s);
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
	GL_ShaderLoadTextures(s);
	return s;
}

/*
================
StageLoadTextures

load all the textures for the given stage
================
*/
void StageLoadTextures(stage_t *stage, shader_t *shader) {
	
	stage->numtextures = 1;

	switch (stage->type) {
	case STAGE_COLOR:
	case STAGE_GLOSS:
	case STAGE_SIMPLE:
		stage->texture[0] = GL_CacheTexture(stage->filename, shader->mipmap, TEXTURE_RGB);
		break;
	case STAGE_BUMP:
		stage->texture[0] = GL_CacheTexture(stage->filename, shader->mipmap, TEXTURE_NORMAL);
		break;
	}
}

/*
================
GL_ShaderLoadTextures

load all the textures for the given shader
================
*/
void GL_ShaderLoadTextures(shader_t *shader) {
	int i, j;

	for (i=0; i<shader->numstages; i++) {
		StageLoadTextures(&shader->stages[i],shader);
	}

	for (i=0; i<shader->numbumpstages; i++) {
		StageLoadTextures(&shader->bumpstages[i],shader);
	}

	for (i=0; i<shader->numglossstages; i++) {
		StageLoadTextures(&shader->glossstages[i],shader);
	}

	for (i=0; i<shader->numcolorstages; i++) {
		StageLoadTextures(&shader->colorstages[i],shader);
	}

	for (i=0; i<shader->numglossstages; i++) {
		if (shader->glossstages[i].type == STAGE_GRAYGLOSS) {
			for (j=0; j< shader->glossstages[i].numtextures; j++)
				shader->glossstages[i].texture[j] = shader->bumpstages[i].texture[j];
		}
	}
}

qboolean IsShaderBlended(shader_t *s) {

	if (s->numstages)
		return (s->stages[0].src_blend >= 0);

	if (s->numcolorstages)
		return (s->colorstages[0].src_blend >= 0);

	return false;
}

/*=====================================================

	Shader script parsing

======================================================*/

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
		stage->tcmods[stage->numtcmods].params[0] = atof(com_token);

	} else if (!strcmp(com_token, "scroll")) {

		stage->tcmods[stage->numtcmods].type = TCMOD_SCROLL;
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[0] = atof(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[1] = atof(com_token);

	} else if (!strcmp(com_token, "scale")) {
		
		stage->tcmods[stage->numtcmods].type = TCMOD_SCALE;
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[0] = atof(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[1] = atof(com_token);

	} else if (!strcmp(com_token, "stretch")) {
		
		stage->tcmods[stage->numtcmods].type = TCMOD_STRETCH;
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[0] = atof(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[1] = atof(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[2] = atof(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[3] = atof(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[4] = atof(com_token);

	} else if (!strcmp(com_token, "turb")) {
		
		//parse it and give a warning that it's not supported
		stage->tcmods[stage->numtcmods].type = TCMOD_SCALE;
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[0] = atof(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[1] = atof(com_token);
		GET_SAFE_TOKEN;
		stage->tcmods[stage->numtcmods].params[2] = atof(com_token);
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
	qboolean	isgraygloss;
	//Con_Printf("Parsing shader stage...\n");
	clearStage(&stage);
	isgraygloss  = false;

// go through all the dictionary pairs
	while (1)
	{	
		data = COM_Parse (data);

		//end of stage
		if (com_token[0] == '}') {
			//end of stage is parsed now set everything up correctly
			strncpy(stage.filename,texture,MAX_QPATH);
			stage.numtextures = 1;

			if (stage.type == STAGE_GLOSS) {

				//if it has a gray gloss map we change the bumpmaps filename so it loads the gray
				//glossmap too, we won't bother with the glossmap in the graygloss shaders anymore
				//as we just bind the bumpmap.
				if (isgraygloss) {
					stage_t *bumpstage;

					//setup the filenames correctly
					stage.type = STAGE_GRAYGLOSS;
					if (shader->numbumpstages < 1) {
						Con_Printf("Gray gloss defined before bumpmap\n");
						return NULL;
					}
					bumpstage = &shader->bumpstages[shader->numbumpstages-1];
					sprintf(stage.filename,"%s|%s",texture,bumpstage->filename);
					strcpy(bumpstage->filename, stage.filename);
				}
				shader->flags = shader->flags | SURF_GLOSS;
				if (shader->numglossstages >= SHADER_MAX_BUMP_STAGES)
					return NULL;
				shader->glossstages[shader->numglossstages] = stage;
				shader->numglossstages++;
			} else if (stage.type == STAGE_COLOR) {
				shader->flags = shader->flags | SURF_PPLIGHT;
				if (shader->numcolorstages >= SHADER_MAX_BUMP_STAGES)
					return NULL;
				shader->colorstages[shader->numcolorstages] = stage;
				shader->numcolorstages++;				
			} else if (stage.type == STAGE_BUMP) {
				shader->flags = shader->flags | SURF_BUMP;
				if (shader->numbumpstages >= SHADER_MAX_BUMP_STAGES)
					return NULL;
				shader->bumpstages[shader->numbumpstages] = stage;
				shader->numbumpstages++;

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

			//If it has a gray modifier it means the glossmap has to be stored in the bumpmap alpha
			if (!strcmp(com_token, "gray")) {
				GET_SAFE_TOKEN;
				strncpy(texture,com_token,MAX_QPATH);
				isgraygloss = true;
			} else
				strncpy(texture,com_token,MAX_QPATH);

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
				} else {
					Con_Printf("Unknown blend func %s\n",com_token);
					stage.src_blend = -1;
					stage.dst_blend = -1;
				}
			} else {
				GET_SAFE_TOKEN;
				stage.dst_blend = SHADER_BlendModeForName(com_token);
				if (stage.dst_blend < 0)
					Con_Printf("Unknown blend func %s\n",com_token);
			}
		} else if (!strcmp(command, "alphafunc")) {

			GET_SAFE_TOKEN;
			if (!strcmp(com_token,"GE128")) {
				Con_Printf("alpha test\n");
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
	stage_t *stage;
	if (shader->numbumpstages >= SHADER_MAX_BUMP_STAGES)
			return;
	stage = &shader->bumpstages[shader->numbumpstages];
	shader->numbumpstages++;

	clearStage(stage);
	stage->type = STAGE_BUMP;
	strncpy(stage->filename,name,MAX_QPATH);
	shader->flags = shader->flags | SURF_BUMP;
}

/**
* Automatically generate a specular stage with the specularmap keyword
*/
void specularStage(shader_t *shader, char *name) {
	stage_t *stage;
	if (shader->numglossstages >= SHADER_MAX_BUMP_STAGES)
			return;
	stage = &shader->glossstages[shader->numglossstages];
	shader->numglossstages++;

	clearStage(stage);
	stage->type = STAGE_GLOSS;
	strncpy(stage->filename,name,MAX_QPATH);
	shader->flags = shader->flags | SURF_GLOSS;
}

/**
* Automatically generate a specular stage with the normalspecular keyword
* This is a shader with a grayscale gloss map .
*/
void normalSpecularStage(shader_t *shader, char *name, char *name2) {
	stage_t *stage, *stage2;
	char buff[MAX_QPATH*2+1];

	if (shader->numglossstages >= SHADER_MAX_BUMP_STAGES)
			return;

	if (shader->numbumpstages >= SHADER_MAX_BUMP_STAGES)
			return;

	stage = &shader->bumpstages[shader->numbumpstages];
	shader->numbumpstages++;

	clearStage(stage);
	stage->type = STAGE_BUMP;
	strncpy(stage->filename,name,MAX_QPATH);
	shader->flags = shader->flags | SURF_BUMP;

	stage2 = &shader->glossstages[shader->numglossstages];
	shader->numglossstages++;

	clearStage(stage2);
	stage2->type = STAGE_GRAYGLOSS;
	strncpy(stage2->filename,name2,MAX_QPATH);
	shader->flags = shader->flags | SURF_GLOSS;
	
	sprintf(buff,"%s|%s",stage->filename, stage2->filename);
	strcpy(stage->filename, buff);
	strcpy(stage2->filename, buff);
}

/**
* Automatically generate a bump stage with the colormap keyword
*/
void diffuseStage(shader_t *shader, char *name) {
	stage_t *stage;
	if (shader->numcolorstages >= SHADER_MAX_BUMP_STAGES)
			return;
	stage = &shader->colorstages[shader->numcolorstages];
	shader->numcolorstages++;

	clearStage(stage);
	stage->type = STAGE_COLOR;
	strncpy(stage->filename,name,MAX_QPATH);
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
		} else if (!strcmp(command,"normalspecular")) {
			GET_SAFE_TOKEN;
			strcpy(command,com_token);
			GET_SAFE_TOKEN;
			normalSpecularStage(shader,command, com_token);
		} else if (!strcmp(command,"surfaceparm")) {
			data = COM_SkipLine(data);
		} else if (!strcmp(command,"nomipmaps")) {
			shader->mipmap = false;
		} else if (!strcmp(command,"cull")) {
			GET_SAFE_TOKEN;
			if (!strcmp(com_token,"disable")) {
				shader->cull = false;
			} else
				shader->cull = true;
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
	shader_t shader, *s;
// parse shaders
	while (1)
	{
		data = COM_Parse (data);
		if (!data)
			break;

		//check if shader already exists
		s = shaderList;
		while(s) {
			if (!strcmp(com_token,s->name)) {
				Con_Printf("Error: Shader redefined %s\n",com_token);
				return;
			}
			s = s->next;
		}

		//make a new one
		s = malloc(sizeof(shader_t));
		if (!s) Sys_Error("Not enough mem");
		memset(s,0,sizeof(shader_t));
		s->next = shaderList;
		shaderList = s;
		strncpy(s->name,com_token,sizeof(s->name));
		s->mipmap = true;
		s->cull = true;

		//parse it from the file
		data = COM_Parse (data);
		if (!data)
			break;
		if (com_token[0] != '{')
			Sys_Error ("ED_LoadFromFile: found %s when expecting {",com_token);

		Con_Printf("Parsing %s...\n",s->name);
		data = ParseShader (data, s);
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

#if !defined(_WIN32) && !defined(SDL) && !defined(__glx__)
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
