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

#ifndef __MODEL__
#define __MODEL__

#include "modelgen.h"
#include "spritegn.h"
#include "render.h"

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

// entity effects

#define	EF_BRIGHTFIELD			1
#define	EF_MUZZLEFLASH 			2
#define	EF_BRIGHTLIGHT 			4
#define	EF_DIMLIGHT 			8
#define EF_BLUE					64
#define EF_RED					128
#define	EF_GREEN				32
#define EF_FULLDYNAMIC			16


//PENTA: moved here, we needed it
typedef struct plane_s
{
	vec3_t	normal;
	float	dist;
} plane_t;


/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vec3_t		position;
} mvertex_t;


//PENTA: "modern" vertices
//WARNING: if you change the size of this also change VERTEXSIZE
typedef struct
{
	vec3_t		position;
	float		texture[2];
	float		lightmap[2];
    byte		color[4];
} mmvertex_t;


#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


// plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mplane_s
{
	vec3_t	normal;
	float	dist;
	byte	type;			// for texture axis selection and fast side tests
	byte	signbits;		// signx + signy<<1 + signz<<1
	byte	pad[2];
} mplane_t;


//Textures
#define TEXTURE_RGB 1
#define TEXTURE_NORMAL 2
#define TEXTURE_CUBEMAP 3

typedef struct
{
	int		texnum;
	char	identifier[MAX_OSPATH*2+1];
	int		width, height;
	qboolean	mipmap;
	int		loadtype;
	int		gltype;
	void	*dynamic; //if this is set we have a texture with changing pixels (currently only roq's)
} gltexture_t;

#define SHADER_MAX_NAME 128
#define SHADER_MAX_STAGES 8
#define SHADER_MAX_BUMP_STAGES 4
#define SHADER_MAX_TCMOD 8
#define SHADER_MAX_STATUSES 8

typedef enum {STAGE_SIMPLE, STAGE_COLOR, STAGE_BUMP, STAGE_GLOSS, STAGE_GRAYGLOSS} stagetype_t;
typedef enum {TCMOD_ROTATE, TCMOD_SCROLL, TCMOD_SCALE, TCMOD_STRETCH} tcmodtype_t;

#define STAGE_CUBEMAP 1

typedef struct tcmod_s {
	float params[7];
	tcmodtype_t type;
} tcmod_t;

/**
* A single stage in the shader,
* a stage is one block { } in a shader definition
* this corresponds to a single pass for simple shaders
* or a single texture definition for bumpmapping.
*/
typedef struct stage_s {
	stagetype_t type;
	int			numtcmods;
	tcmod_t		tcmods[SHADER_MAX_TCMOD];	
	int			numtextures;
	gltexture_t *texture[8];  //animations
	int			src_blend, dst_blend; //have special values for bumpmap passes
	int			alphatresh;
	char		filename[MAX_QPATH*3+2];
	int			flags;
} stage_t;

#define	SURF_NOSHADOW		0x40000	//don't cast stencil shadows
#define	SURF_BUMP			0x80000	//do diffuse bumpmapping and gloss if gloss is enabled too
#define	SURF_GLOSS			0x100000//do gloss
#define	SURF_PPLIGHT		0x200000//do per pixel lighting...
									//if bump is unset it uses a flat bumpmap and no gloss
									//if bump is set and gloss unet it does only diffuse bumps
#define	SURF_TRANSLUCENT	0x400000//surface is translucent

/**
* A shader, holds the stages and the general info for that shader.
*/
typedef struct shader_s {
	char		name[SHADER_MAX_NAME];
	int			flags;
	int			contents;
	int			numstages;
	stage_t		stages[SHADER_MAX_STAGES];
	int			numcolorstages;
	stage_t		colorstages[SHADER_MAX_BUMP_STAGES];//these ase a bit special, so separate them
	int			numbumpstages;
	stage_t		bumpstages[SHADER_MAX_BUMP_STAGES];
	int			numglossstages;
	stage_t		glossstages[SHADER_MAX_BUMP_STAGES];
	vec3_t		fog_color;
	float		fog_dist;
	float		displaceScale;
	float		displaceBias;
	char		displacementname[MAX_QPATH];
	gltexture_t *displacement;
	int			numstatus;
	struct shader_s	*status[SHADER_MAX_STATUSES]; //if the object the shader is on it's status is > 0 this shader will be used insead of the base shader...
	struct shader_s	*next;	//in the shader linked list
	qboolean	mipmap;
	qboolean	cull;
	qboolean	texturesAreLoaded;	//The textures of the different shader stages are loaded
} shader_t;

//2048 triangles for shader
#define MAX_SHADER_INCECIES 6144

typedef struct mapshader_s
{
	shader_t	*shader;
	struct msurface_s	*texturechain;	// for gl_texsort drawing
	struct mesh_s *meshchain;
	int numindecies;
	int	indecies[MAX_SHADER_INCECIES];
} mapshader_t;


#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
#define SURF_DRAWTURB		0x10
#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80
//PENTA: Mirror surface
#define SURF_MIRROR			0x100
#define SURF_GLASS			0x200
#define SURF_CAULK			0x400


// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
} medge_t;


//size in floats...
#define	VERTEXSIZE	8

//Penta
//I changed this the poly only stores the first index for the vertex in the global vertex list...
typedef struct glpoly_s
{
	struct	glpoly_s	*next;
	struct	glpoly_s	*chain;
	int		numverts;
	int		flags;			// for SURF_UNDERWATER
	int			lightTimestamp;	//PENTA: timestamp of last light that
								//this polygon was visible to
	struct	mneighbour_s *neighbours;
	int		firstvertex;
	int		numindecies;	//number of polygons indexes
	int		numneighbours;
	int		indecies[1];	//polygon indexes... variable sized
} glpoly_t;


typedef struct mneighbour_s {
	int		p1, p2;
	glpoly_t *n;
} mneighbour_t;

typedef struct msurface_s
{
	int			visframe;		// should be drawn when node is crossed

	mplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges
	
	//short		texturemins[2];
	//short		extents[2];

	//int			light_s, light_t;	// gl lightmap coordinates

	glpoly_t	*polys;				// multiple if warped
	struct	msurface_s	*texturechain;
	struct	msurface_s	*shadowchain;
	struct	msurface_s	*lightmapchain;

	mapshader_t	*shader;
	vec3_t		tangent;
	vec3_t		binormal;
	
	int			lightmaptexturenum;

	//used still?
	byte		styles[MAXLIGHTMAPS];
	int			cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	qboolean	cached_dlight;				// true if dynamic light in cache
	byte		*samples;		// [numstyles*surfsize]
} msurface_t;

//FIXME: use a matrix instead??
typedef struct {
	vec3_t angles;
	vec3_t origin;
	vec3_t scale;
} transform_t;

typedef struct {
	int segment;	//segment 0 is the system memory, higher segments are driver mem
	long offset;		//offset into segment, maximum size of offset varies
} DriverPtr;

typedef struct mesh_s
{
	vec3_t mins; //axis aligned bounding box
	vec3_t maxs;

	//User space: For shadow volumes and decail generation
	vec3_t *userVerts; //Vertices in user memory
	plane_t *triplanes; //per triangle plane eq's (for shadow volumes)

	//Driver space: For render passes
	DriverPtr vertices;
	DriverPtr tangents;
	DriverPtr binormals;
	DriverPtr normals;

	int	numvertices;

	int *indecies;
	int numindecies;

	int numtriangles;
	int *neighbours; //numtriangles*3 neighbour triangle indexes

	int *unexplodedIndecies; //only during load time an when isExploded == true

	transform_t trans;

	int	visframe;
	int lightTimestamp;
	mapshader_t	*shader;
	int lightmapIndex;

	qboolean isExploded; //Every triangle has unique vertices, no shared ones

	struct mesh_s *next; //for the texture chains
	struct mesh_s *shadowchain;

} mesh_t;

typedef struct mnode_s
{
// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	
	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// node specific
	mplane_t	*plane;
	struct mnode_s	*children[2];	

	//PFIX: Remove these 2 
	unsigned short		firstsurface;
	unsigned short		numsurfaces;

	int			ichildren[2]; //children as indexes into the array instead of pointers...
} mnode_t;

typedef struct mleaf_s
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// leaf specific
	byte		*compressed_vis;
	efrag_t		*efrags;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;

	int			firstmesh;
	int			nummeshes;
	
	int			firstbrush;
	int			numbrushes;
	int			area;
	int			cluster;

	int			key;			// BSP sequence number for leaf's contents
	int			index;
	byte		ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct
{
	dclipnode_t	*clipnodes;
	mplane_t	*planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
} hull_t;

typedef struct
{
	mplane_t	*plane;
	mapshader_t	*shader;
} mbrushside_t;

typedef struct
{
	int			contents;
	int			numsides;
	int			firstbrushside;
	int			checkcount;		// to avoid repeated testings
} mbrush_t;

typedef struct
{
	int			numareaportals[MAX_MAP_AREAS];
} marea_t;

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/

typedef struct mspriteframe_s
{
	int		width;
	int		height;
	float	up, down, left, right;
	shader_t	*shader;
} mspriteframe_t;

typedef struct
{
	int				maxwidth;
	int				maxheight;
	int				numframes;
	mspriteframe_t	frames[1];
} msprite_t;


/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct
{
	int					firstpose;
	int					numposes;
	float				interval;
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
	char				name[16];
} maliasframedesc_t;

typedef struct
{
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
} maliasgroupframedesc_t;

typedef struct
{
	int						numframes;
	int						intervals;
	maliasgroupframedesc_t	frames[1];
} maliasgroup_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
//PENTA: no it's not we only modify opengl

typedef struct mtriangle_s {
	int					facesfront;
	int					vertindex[3];
	int					neighbours[3];
} mtriangle_t;


#define	MAX_SKINS	32
typedef struct {
	int			ident;
	int			version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int			numskins;
	int			skinwidth;
	int			skinheight;
	int			numverts;
	int			numtris;
	int			numframes;
	synctype_t	synctype;
	int			flags;
	float		size;

     vec3_t					mins,maxs;	// bounding box
	int					numposes;
	int					poseverts;
	int					posedata;	// numposes*poseverts trivert_t
	int					commands;	// gl command list with embedded s/t
	int					triangles;	//PENTA: We need tris for shadow volumes
	int					planes;		//PENTA: Plane eq's for every triangle for every frame
	int					tangents;	//PENTA: Tangent for every vertex for every frame
	int					binormals;	//PENTA: Tangent for every vertex for every frame
	int					texcoords;	//PENTA: For every triangle the 3 texture coords
	int					indecies;	//PENTA: indecies for gl vertex arrays
	shader_t			*shader;
	int					texels[MAX_SKINS];	// only for player skins
	maliasframedesc_t	frames[1];	// variable sized
} aliashdr_t;

#define	MAXALIASVERTS	2048
#define	MAXALIASFRAMES	256
#define	MAXALIASTRIS	2048
extern	aliashdr_t	*pheader;
extern	stvert_t	stverts[MAXALIASVERTS];
extern	mtriangle_t	triangles[MAXALIASTRIS];
extern	trivertx_t	*poseverts[MAXALIASFRAMES];

//===================================================================

//
// Whole model
//

typedef enum {mod_brush, mod_sprite, mod_alias, mod_alias3} modtype_t;

#define	EF_ROCKET	1			// leave a trail
#define	EF_GRENADE	2			// leave a trail
#define	EF_GIB		4			// leave a trail
#define	EF_ROTATE	8			// rotate (bonus items)
#define	EF_TRACER	16			// green split trail
#define	EF_ZOMGIB	32			// small blood trail
#define	EF_TRACER2	64			// orange split trail + rotate
#define	EF_TRACER3	128			// purple trail
#define	EF_FASTVOLUME 256		// PENTA: fast shadow volume generation
#define	EF_FULLBRIGHT 512		// PENTA: draw model fullbright (lavaballs, torches, ...)
#define	EF_NOSHADOW 1024		//PENTA: don't draw any shadow

typedef struct model_s
{
	char		name[MAX_QPATH];
	qboolean	needload;		// bmodels and sprites don't cache normally

	modtype_t	type;
	int			numframes;
	synctype_t	synctype;
	
	int			flags;

//
// volume occupied by the model graphics
//		
	vec3_t		mins, maxs;
	float		radius;

//
// solid volume for clipping 
//
	qboolean	clipbox;
	vec3_t		clipmins, clipmaxs;

//
// brush model
//
	int			firstmodelsurface, nummodelsurfaces;
	int			firstmodelbrush, nummodelbrushes; //added for q3

	int			numsubmodels;
	dmodel_t	*submodels;

	int			numplanes;
	mplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numvertexes;
	vec3_t		*userVerts;	//Vertex positions (the rest is stored in the graphics driver)

	int			numedges;
	medge_t		*edges;

	int			numnodes;
	mnode_t		*nodes;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			numclipnodes;
	dclipnode_t	*clipnodes;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	int			numbrushes;
	mbrush_t	*brushes;

	int			numbrushsides;
	mbrushside_t *brushsides;

	int			numleafbrushes;
	int			*leafbrushes;

	int			numleafmeshes;
	int			*leafmeshes;

	int			numareas;
	marea_t		*areas;

	int			nummeshes;
	mesh_t		*meshes;

	hull_t		hulls[MAX_MAP_HULLS];

	int			nummapshaders;
	mapshader_t	*mapshaders;

	int			numclusters;
	int			clusterlen;
	byte		*visdata;


	byte		*lightdata;
	int			numlightmaps;
	char		*entities;

//
// additional model data
//
	cache_user_t	cache;		// only access through Mod_Extradata

} model_t;

//============================================================================

void	Mod_Init (void);
void	Mod_ClearAll (void);
model_t *Mod_ForName (char *name, qboolean crash);
void	*Mod_Extradata (model_t *mod);	// handles caching
void	Mod_TouchModel (char *name);
float	Mod_RadiusFromBounds (vec3_t mins, vec3_t maxs);

mleaf_t *Mod_PointInLeaf (float *p, model_t *model);
byte	*Mod_LeafPVS (mleaf_t *leaf, model_t *model);

#define	MAX_MOD_KNOWN	512
model_t	mod_known[MAX_MOD_KNOWN];
int		mod_numknown;

model_t *Mod_FindName (char *name);


#endif	// __MODEL__
