
DriverPtr GL_WrapUserPointer(void *p);
DriverPtr GL_OffsetDriverPtr(DriverPtr p, int offset);

/**
* Gives a definition of the vertices passed to the shedule* functions.
* Use NULL if the data is not available/applicable to the suff you send.
*/
typedef struct {

	DriverPtr vertices;
	int vertexstride;

	DriverPtr texcoords;
	int texcoordstride;

	DriverPtr lightmapcoords;
	int lightmapstride;

	DriverPtr tangents;
	int tangentstride;

	DriverPtr binormals;
	int binormalstride;

	DriverPtr normals;
	int normalstride;

	DriverPtr colors;
	int colorstride;

} vertexdef_t;


typedef struct {
	vec3_t	objectorigin;
	vec3_t	objectvieworg;
} lightobject_t;

/**
* This is a generic bumpdriver struct, it contains al the driver routines
*/
typedef struct {
	//system code

	//initialize the driver, we don't call this twice currently but we may need this if
	//we want to do dynamic resolution changes => reinit of the whole opengl stuff.
	void (*initDriver) (void);
	void (*freeDriver) (void);

	//drawing code
	void (*drawTriangleListBase) (vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader, int lightmapIndex);//-1 for no lightmap
	void (*drawTriangleListBump) (const vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader, const transform_t *tr, const lightobject_t *lo);
	void (*drawTriangleListSys) (vertexdef_t *verts, int *indecies, int numIndecies, shader_t *shader);
	void (*drawSurfaceListBase) (vertexdef_t *verts, msurface_t **surfs, int numSurfaces, shader_t *shader);
	void (*drawSurfaceListBump) (vertexdef_t *verts, msurface_t **surfs, int numSurfaces, const transform_t *tr, const lightobject_t *lo);

} bumpdriver_t;

extern bumpdriver_t gl_bumpdriver;
