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
// r_misc.c

#include "quakedef.h"

byte	dottexture[8][8] =
{
	{0,1,1,0,0,0,0,0},
	{1,1,1,1,0,0,0,0},
	{1,1,1,1,0,0,0,0},
	{0,1,1,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};

void R_InitParticleTexture (void)
{
	int		x,y;
	byte	data[8][8][4];

	//
	// particle texture
	//
	particletexture = texture_extension_number++;
    GL_Bind(particletexture);

	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}
	glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	particletexture_blood = EasyTgaLoad("textures/particles/blood.tga");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	particletexture_dirblood = EasyTgaLoad("textures/particles/blood2.tga");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

/*
===============
R_Envmap_f

Grab six views for environment mapping tests
===============
*/
void R_Envmap_f (void)
{
	byte	buffer[256*256*4];

	glDrawBuffer  (GL_FRONT);
	glReadBuffer  (GL_FRONT);
	envmap = true;

	r_refdef.vrect.x = 0;
	r_refdef.vrect.y = 0;
	r_refdef.vrect.width = 256;
	r_refdef.vrect.height = 256;

	r_refdef.viewangles[0] = 0;
	r_refdef.viewangles[1] = 0;
	r_refdef.viewangles[2] = 0;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	COM_WriteFile ("env0.rgb", buffer, sizeof(buffer));		

	r_refdef.viewangles[1] = 90;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	COM_WriteFile ("env1.rgb", buffer, sizeof(buffer));		

	r_refdef.viewangles[1] = 180;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	COM_WriteFile ("env2.rgb", buffer, sizeof(buffer));		

	r_refdef.viewangles[1] = 270;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	COM_WriteFile ("env3.rgb", buffer, sizeof(buffer));		

	r_refdef.viewangles[0] = -90;
	r_refdef.viewangles[1] = 0;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	COM_WriteFile ("env4.rgb", buffer, sizeof(buffer));		

	r_refdef.viewangles[0] = 90;
	r_refdef.viewangles[1] = 0;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	R_RenderView ();
	glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	COM_WriteFile ("env5.rgb", buffer, sizeof(buffer));		

	envmap = false;
	glDrawBuffer  (GL_BACK);
	glReadBuffer  (GL_BACK);
	GL_EndRendering ();
}

/*
===============
R_Init
===============
*/
void R_Init (void)
{	
	extern cvar_t gl_finish;

	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	
	Cmd_AddCommand ("envmap", R_Envmap_f);	
	Cmd_AddCommand ("pointfile", R_ReadPointFile_f);	

	Cvar_RegisterVariable (&r_norefresh);
	Cvar_RegisterVariable (&r_lightmap);
	Cvar_RegisterVariable (&r_fullbright);
	Cvar_RegisterVariable (&cg_showentities);
	Cvar_RegisterVariable (&cg_showviewmodel);
//	Cvar_RegisterVariable (&r_mirroralpha);
	Cvar_RegisterVariable (&r_wateralpha);
	Cvar_RegisterVariable (&r_novis);
	Cvar_RegisterVariable (&r_noareaportal);

	Cvar_RegisterVariable (&gl_finish);
	Cvar_RegisterVariable (&gl_clear);
/*	Cvar_RegisterVariable (&gl_texsort);

 	if (gl_mtexable)
		Cvar_SetValue ("gl_texsort", 0.0);
*/
	Cvar_RegisterVariable (&gl_polyblend);

	Cvar_RegisterVariable (&gl_calcdepth);
	Cvar_RegisterVariable (&sh_lightmapbright);
	Cvar_RegisterVariable (&sh_visiblevolumes);
	Cvar_RegisterVariable (&sh_entityshadows);
	Cvar_RegisterVariable (&sh_meshshadows);
	Cvar_RegisterVariable (&sh_worldshadows);
	Cvar_RegisterVariable (&sh_showlightsciss);
	Cvar_RegisterVariable (&sh_occlusiontest);
	Cvar_RegisterVariable (&sh_showlightvolume);
	Cvar_RegisterVariable (&sh_glows);
	Cvar_RegisterVariable (&cg_showfps); // muff
	Cvar_RegisterVariable (&sh_debuginfo);
	Cvar_RegisterVariable (&sh_norevis);
	Cvar_RegisterVariable (&sh_nosvbsp);
	Cvar_RegisterVariable (&sh_noeclip);
	Cvar_RegisterVariable (&sh_infinitevolumes);
	Cvar_RegisterVariable (&sh_noscissor);
	Cvar_RegisterVariable (&sh_nocleversave);
	Cvar_RegisterVariable (&sh_bumpmaps);
	Cvar_RegisterVariable (&sh_playershadow);
	Cvar_RegisterVariable (&sh_nocache);
	Cvar_RegisterVariable (&sh_glares);
	Cvar_RegisterVariable (&sh_noefrags);
	Cvar_RegisterVariable (&sh_showtangent);
	Cvar_RegisterVariable (&sh_noshadowpopping);

	Cvar_RegisterVariable (&mir_detail);
	Cvar_RegisterVariable (&mir_frameskip);
	Cvar_RegisterVariable (&mir_forcewater);
	Cvar_RegisterVariable (&mir_distance);
	Cvar_RegisterVariable (&gl_wireframe);
	Cvar_RegisterVariable (&gl_mesherror);

	Cvar_RegisterVariable (&fog_r);
	Cvar_RegisterVariable (&fog_g);
	Cvar_RegisterVariable (&fog_b);
	Cvar_RegisterVariable (&fog_start);
	Cvar_RegisterVariable (&fog_end);
	Cvar_RegisterVariable (&gl_fog);

	Cvar_RegisterVariable (&fog_waterfog);
	Cvar_RegisterVariable (&gl_caustics);
	Cvar_RegisterVariable (&gl_truform);
	Cvar_RegisterVariable (&gl_truform_tesselation);
	Cvar_RegisterVariable (&r_tangentscale);
	Cvar_RegisterVariable (&sh_delux);
	Cvar_RegisterVariable (&sh_rtlights);
	Cvar_RegisterVariable (&gl_clipboth);
	Cvar_RegisterVariable (&gl_displacement);

	R_InitParticleEffects();
	R_InitParticles ();
	R_InitDecals ();
	R_InitParticleTexture ();

	CS_FillBinomials();

#ifdef GLTEST
	Test_Init ();
#endif

	playertextures = texture_extension_number;
	texture_extension_number += 16;
}

/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	int		i;
	
	//Exec a config file in maps dir with same name as the map - Eradicator
	//Cbuf_AddText(va("exec maps/%s.cfg\n", sv.name));
	//Cbuf_Execute();
	
	for (i=0 ; i<256 ; i++)
		d_lightstylevalue[i] = 264;		// normal light value

	memset (&r_worldentity, 0, sizeof(r_worldentity));
	r_worldentity.model = cl.worldmodel;

// clear out efrags in case the level hasn't been reloaded
// FIXME: is this one short?
	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
		cl.worldmodel->leafs[i].efrags = NULL;
		 	
	r_viewleaf = NULL;
	R_ClearParticles ();
	R_ClearDecals ();

	GL_BuildLightmaps ();

	R_CopyVerticesToHunk();

	// identify sky texture
	skyshadernum = -1;
	mirrortexturenum = -1;
	for (i=0 ; i<cl.worldmodel->nummapshaders ; i++)
	{
		if (!Q_strncmp(cl.worldmodel->mapshaders[i].shader->name,"sky",3) )
			skyshadernum = i;
 		cl.worldmodel->mapshaders[i].texturechain = NULL;
	}

	if (skyshadernum < 0) {
		//Con_Printf("No sky texture found");
	}


	strcpy(skybox_name,"default");
	skybox_cloudspeed = 1.0;
//#ifdef QUAKE2

//#endif

	//PENTA: Clear lights
	// PENTA: Delete all static lights
	numStaticShadowLights = 0;
	numShadowLights = 0;

	R_ClearInstantCaches();
	R_ClearBrushInstantCaches();
	R_NewMirrorChains();
	causticschain = NULL;
}


/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f (void)
{
	int		i;
	float		start, stop, time;

	glDrawBuffer  (GL_FRONT);
	glFinish ();

	start = Sys_FloatTime ();
	for (i=0 ; i<128 ; i++)
	{
		r_refdef.viewangles[1] = i/128.0*360.0;
		R_RenderView ();
	}

	glFinish ();
	stop = Sys_FloatTime ();
	time = stop-start;
	Con_Printf ("%f seconds (%f fps)\n", time, 128/time);

	glDrawBuffer  (GL_BACK);
	GL_EndRendering ();
}

void D_FlushCaches (void)
{
}


