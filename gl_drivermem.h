
extern DriverPtr nullDriver;
#define DRVNULL nullDriver

#define IsNullDriver(d) (((d).segment == nullDriver.segment) && ((d).offset == nullDriver.offset))


void *GL_MapToUserSpace(DriverPtr p);
void GL_UnmapFromUserSpace(DriverPtr p);
void drivermemcpy(DriverPtr dest, void *src, size_t size);
DriverPtr GL_StaticAlloc(size_t size, void *data);
DriverPtr GL_DynamicAlloc(size_t size, void *data);
void GL_FreeAll(void);
void GL_InitDriverMem(void);
void GL_FreeDriverMem(void);

void GL_InitVertexCache(void);
void GL_FreeVertexCache(void);
void GL_AllocVertexCache(const size_t size, DriverPtr *owner);
void GL_FlushVertexCache(void);
