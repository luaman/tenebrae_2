/**
* Gives a definition of the vertices passed to the shedule* functions.
* Use NULL if the data is not available/applicable to the suff you send.
*/
typedef struct {

	float *vertices;
	int vertexstride;

	float *texcoords;
	int texcoordstride;

	float *lightmapcoords;
	int lightmapstride;

	float *tangents;
	int tangentstride;

	float *binormals;
	int binormalstride;

	float *normals;
	int normalstride;

	unsigned char *colors;
	int colorstride;

} vertexdef_t;

/**
* Defines driver managed memory types
*/
typedef enum {DM_SLOWREADWRITE, DM_SLOWREAD, DM_NORMAL} drivermem_t;
//DM_SLOWREADWRITE: the memory is slow in writing and reading, won't be updated outside of
//the driver much
//DM_SLOWREAD: the memory is slow in reading (uncached or worse...), won't be read outside
//of the driver much.  It supports decent writing speeds.  (This is probably what you'll want
//most of the time.)
//DM_NORMAL: Fast reading and writing

/**
* This is a generic bumpdriver struct, it contains al the driver routines
*/
typedef struct {
	//system code

	//initialize the driver, we don't call this twice currently but we may need this if
	//we want to do dynamic resolution changes => reinit of the whole opengl stuff.
	void (*initDriver) (void);
	void (*freeDriver) (void);

	//gets a pointer to driver memory, it can just return system memory too if the driver
	//doesn't support it.
	void *(*getDriverMem) (size_t size, drivermem_t hint);

	//frees all driver mem
	//FIXME: do we need real deallocation support
	void (*freeAllDriverMem) (void);
	
	//FIXME: Do we need fence like support?

	//drawing code

	void (*drawTriangleListBase) (vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader, int lightmapIndex);//-1 for no lightmap
	void (*drawTriangleListBump) (const vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader, const transform_t *tr);
	void (*drawTriangleListSys) (vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader);
	void (*drawSurfaceListBase) (vertexdef_t *verts, msurface_t **surfs, int numSurfaces, shader_t *shader);
	void (*drawSurfaceListBump) (vertexdef_t *verts, msurface_t **surfs, int numSurfaces, const transform_t *tr);

} bumpdriver_t;

extern bumpdriver_t gl_bumpdriver;