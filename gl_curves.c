/*
Copyright (C) 2002-2003 Charles Hollemeersch

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

PENTA:

Bezier curve code...
We evaluate curves at load time based on the user's precision preferences.
No dynamic lod...
*/
#include "quakedef.h"

int			numleafbrushes;

//these are just utility structures
typedef struct {
	int firstcontrol;
	int firstvertex;
	int controlwidth, controlheight;
	int width, height;
} curve_t;

typedef struct {
	vec3_t		position;
	float		texture[2];
	float		lightmap[2];
    byte		color[4];
	vec3_t		tangent;
	vec3_t		binormal;
	vec3_t		normal;
} meshvertex_t;


#define MAX_BIN 10
int binomials[MAX_BIN][MAX_BIN];

/*
We roll or own Bezier code...
Dunno how id is supposed to do it but we just evaluate the Bernstein polynomials....
It's not particulary efficient but we pre-evaluate them so it's not a problem...
*/

int fac(int n) {
	int i;
	int rez = 1;
	for (i=2;i<=n;i++) {
		rez*=i;
	}
	return rez;
}

int binomial(int n, int k) {
	return fac(n)/fac(k)/fac(n-k);
}

//Make a lookup table ...
void CS_FillBinomials(void) {
	int i,j;
	for (i=0; i<MAX_BIN; i++) {
		for (j=0; j<MAX_BIN; j++) {
			binomials[i][j] = binomial(i,j);
		}
	}
}

//Evaluates the bernstein polynomial
float Bernstein(int k, int n, float u) {
	return (float)binomials[n][k]*(float)pow(1.0-u,n-k)*(float)pow(u,k);
}

/*
=================
EvaluateBezier

Evaluates the bezier surface with given control points at the u,v parameters
=================
*/
void EvaluateBezier(mmvertex_t *controlpoints,int ofsw, int ofsh, int width, int height, float u, float v,meshvertex_t *result) {

	int i,j;
	float scale;
	float color[4];
	int	n=3;
	int	m=3;
	mmvertex_t *controlpoint, *controlpoint2;
	vec3_t temp;

	for (i=0; i<4; i++) {
		color[i] = 0.0f;
	}

	for (i=0; i<3; i++) {
		result->position[i] = 0.0;
		result->tangent[i] = 0.0;
		result->normal[i] = 0.0;
		result->binormal[i] = 0.0;
	}

	for (i=0; i<2; i++) {
		result->texture[i] = 0.0;
		result->lightmap[i] = 0.0;
	}

	//Calculate vertices & texture coords
	for (i=0; i<n; i++) {
		for (j=0; j<m; j++) {
			scale = Bernstein(i,n-1,u)*Bernstein(j,m-1,v);

			controlpoint = &controlpoints[(ofsw+i)+(ofsh+j)*width];
			result->position[0]+=(scale*controlpoint->position[0]);
			result->position[1]+=(scale*controlpoint->position[1]);
			result->position[2]+=(scale*controlpoint->position[2]);

			result->texture[0]+=(scale*controlpoint->texture[0]);
			result->texture[1]+=(scale*controlpoint->texture[1]);

			result->lightmap[0]+=(scale*controlpoint->lightmap[0]);
			result->lightmap[1]+=(scale*controlpoint->lightmap[1]);

			color[0]+=(scale*controlpoint->color[0]);
			color[1]+=(scale*controlpoint->color[1]);
			color[2]+=(scale*controlpoint->color[2]);
			color[3]+=(scale*controlpoint->color[3]);
		}
	}

	//Yeah parametric tangent space! (done by deriving the function to u or v)
/*	
	//tangent
	for (i=0; i<n; i++) {
		for (j=0; j<m-1; j++) {
			scale = Bernstein(i,n-1,u)*Bernstein(j,m-2,v);

			controlpoint = &controlpoints[(ofsw+i)+(ofsh+j+1)*width];
			controlpoint2 = &controlpoints[(ofsw+i)+(ofsh+j)*width];
			VectorSubtract(controlpoint->position,controlpoint2->position, temp);
			result->tangent[0] +=  scale*temp[0];
			result->tangent[1] +=  scale*temp[1];
			result->tangent[2] +=  scale*temp[2];
		}
	}
	VectorScale(result->tangent,m-1,result->tangent);
	VectorNormalize(result->tangent); //needed?

	//binormal
	for (i=0; i<n-1; i++) {
		for (j=0; j<m; j++) {
			scale = Bernstein(i,n-2,u)*Bernstein(j,m-1,v);

			controlpoint = &controlpoints[(ofsw+i+1)+(ofsh+j)*width];
			controlpoint2 = &controlpoints[(ofsw+i)+(ofsh+j)*width];
			VectorSubtract(controlpoint->position,controlpoint2->position, temp);
			result->binormal[0] +=  scale*temp[0];
			result->binormal[1] +=  scale*temp[1];
			result->binormal[2] +=  scale*temp[2];
		}
	}
	VectorScale(result->binormal,n-1,result->binormal);
	VectorNormalize(result->binormal); //needed?

	//normal
	CrossProduct(result->binormal, result->tangent, result->normal);
	VectorNormalize(result->normal); //needed?
*/	
	/*
	VectorCopy(result->binormal, temp);
	VectorCopy(result->tangent, result->binormal);
	VectorCopy(result->tangent, temp);
	*/
	//VectorScale(result->tangent,-1,result->tangent);

	for (i=0; i<4; i++) {
		result->color[i] = (byte)color[i];
	}
}

/**
* Quake3 beziers, are made up out of one or more 3x3 bezier patches
*/
void EvaluateBiquadraticBeziers(mmvertex_t *controlpoints, int width, int height, float u, float v,meshvertex_t *result) {


//	EvaluateBezier(controlpoints,0,0,width,height,u,v,result);

	//calculate number of patches in curve
	int numpatchx = (width- 1) / 2;
	int	numpatchy = (height- 1) / 2;

	float invx = 1.0f / numpatchx;
	float invy = 1.0f / numpatchy;

	//caclucate patch given u/v is on
	int ofsx = floor(u*numpatchx)*2;
	int ofsy = floor(v*numpatchy)*2;

	if (ofsx >= (width-1)) ofsx-=2;
	if (ofsy >= (height-1)) ofsy-=2;

	//calculate u/v relative to patch
	u = (u-(ofsx/2)*invx)*numpatchx;
	v = (v-(ofsy/2)*invy)*numpatchy;

	EvaluateBezier(controlpoints,ofsx,ofsy,width,height,u,v,result);
}

/**
* "Evaluates the controlpoints"
*/
void PutMeshOnCurve(curve_t in, mmvertex_t *verts) {
	int		i, j, l, w, h;
	float	prev, next;
	float	du, dv, u ,v;
	meshvertex_t results[128*128];
	du = 1.0f/(in.width-1);
	dv = 1.0f/(in.height-1);

	for (i=0, u=0; i<in.width; i++, u+=du) {
		for (j=0, v=0; j<in.height; j++, v+=dv) {
			EvaluateBiquadraticBeziers(verts,in.width,in.height,u,v,&results[i+j*in.width]);
		}
	}

	for (i=0; i<in.width*in.height; i++) {
		VectorCopy(results[i].position,verts[i].position);
	}
}

#define MAX_EXPANDED_AXIS 128

/**
* Evaluate the mesh, subdivide the control grid amount times.
* Copies the resulting vertices to the out mesh.
*/
void SubdivideCurve(curve_t *in, mesh_t *out, mmvertex_t *verts, int amount) {
	int		i, j, l, w, h, newwidth, newheight;
	float	prev, next;
	float	du, dv, u ,v;
	meshvertex_t *expand;

	newwidth = in->controlwidth*amount;
	newheight = in->controlheight*amount;
	
	//only a temporaly buffer
	expand = malloc(sizeof(meshvertex_t)*newwidth*newheight);

	if (!expand) Sys_Error("No more memory\n");

	du = 1.0f/(newwidth-1);
	dv = 1.0f/(newheight-1);

	for (i=0, u=0; i<newwidth; i++, u+=du) {
		for (j=0, v=0; j<newheight; j++, v+=dv) {
			EvaluateBiquadraticBeziers(verts,in->controlwidth,in->controlheight,u,v,&expand[i+j*newwidth]);
		}
	}

	out->numvertices = newwidth*newheight;
	in->width = newwidth;
	in->height = newheight;
/*
	out->tangents = Hunk_Alloc(sizeof(vec3_t)*out->numvertices);
	out->binormals = Hunk_Alloc(sizeof(vec3_t)*out->numvertices);
	out->normals = Hunk_Alloc(sizeof(vec3_t)*out->numvertices);
*/	
	for (i=0; i<newwidth*newheight; i++) {

		//put the vertices in the global vertex table
		if (i==0)
			out->firstvertex = R_AllocateVertexInTemp(expand[i].position, expand[i].texture, expand[i].lightmap, expand[i].color);
		else
			R_AllocateVertexInTemp(expand[i].position, expand[i].texture, expand[i].lightmap, expand[i].color);
/*
		VectorCopy(expand[i].binormal, out->binormals[i]);
		VectorCopy(expand[i].normal, out->normals[i]);
		VectorCopy(expand[i].tangent, out->tangents[i]);
*/
	}

	free(expand);
}

void CreateIndecies(curve_t *curve, mesh_t *mesh)
{
	int i,j, i1, i2, li1, li2;
	int w,h, index;

	h = curve->width;
	w = curve->height;

	mesh->numtriangles = (curve->width-1)*(curve->height-1)*2;
	mesh->numindecies = mesh->numtriangles*3;
	mesh->indecies = (int *)Hunk_Alloc(sizeof(int)*mesh->numindecies);

	li1 = h;
	li2 = 0;
	index = 0;
	for (i=0; i<w-1; i++) {

		li1 = (i+1)*h;
		li2 = i*h;
		for (j=1; j<h; j++) {
			i1 = j+(i+1)*h;
			i2 = j+i*h;
			
			mesh->indecies[index++] = li2;
			mesh->indecies[index++] = li1;
			mesh->indecies[index++] = i2;

			mesh->indecies[index++] = i2;
			mesh->indecies[index++] = li1;
			mesh->indecies[index++] = i1;

			li1 = i1;
			li2 = i2;
		}
	}
}

void TangentForPoly(int *index, mmvertex_t *vertices,vec3_t Tangent, vec3_t Binormal);
void NormalForPoly(int *index, mmvertex_t *vertices,vec3_t Normal);

void CreateTangentSpace(mesh_t *mesh) {
	
	int i,j;
	int *num = malloc(sizeof(int)*mesh->numvertices);
	vec3_t tang, bin, v1, v2, norm;

	Q_memset(num,0,sizeof(int)*mesh->numvertices);
	mesh->tangents = Hunk_Alloc(sizeof(vec3_t)*mesh->numvertices);
	mesh->binormals = Hunk_Alloc(sizeof(vec3_t)*mesh->numvertices);
	mesh->normals = Hunk_Alloc(sizeof(vec3_t)*mesh->numvertices);

	//average for every triangle
	for (i=0; i<mesh->numtriangles; i++) {
		TangentForPoly(&mesh->indecies[i*3],&tempVertices[mesh->firstvertex],tang,bin);
		NormalForPoly(&mesh->indecies[i*3],&tempVertices[mesh->firstvertex],norm);


		for (j=0; j<3; j++) {
			VectorAdd(mesh->tangents[mesh->indecies[i*3+j]],tang,mesh->tangents[mesh->indecies[i*3+j]]);
			VectorAdd(mesh->binormals[mesh->indecies[i*3+j]],bin,mesh->binormals[mesh->indecies[i*3+j]]);
			VectorAdd(mesh->normals[mesh->indecies[i*3+j]],norm,mesh->normals[mesh->indecies[i*3+j]]);
			num[mesh->indecies[i*3+j]]++;
		}
	}

	for (i=0; i<mesh->numvertices; i++) {
		if (num[i] != 0) {
			VectorScale(mesh->tangents[i],1.0f/num[i],mesh->tangents[i]);
			VectorNormalize(mesh->tangents[i]);

			VectorScale(mesh->binormals[i],1.0f/num[i],mesh->binormals[i]);
			VectorNormalize(mesh->binormals[i]);

			VectorScale(mesh->normals[i],1.0f/num[i],mesh->normals[i]);
			VectorNormalize(mesh->normals[i]);

			//CrossProduct(mesh->binormals[i], mesh->tangents[i], mesh->normals[i]);
		} else Con_Printf("num == 0\n");
	}

	free(num);
}

/*
=================
CurveCreate

Creates a curve from the given surface

=================
*/
void CS_Create(dq3face_t *in, mesh_t *mesh, mapshader_t *shader)
{
	curve_t curve;

	curve.controlwidth = in->patchOrder[0];
	curve.controlheight = in->patchOrder[1];
	curve.firstcontrol = in->firstvertex;
	
	//just use the control points as vertices
	curve.firstvertex = in->firstmeshvertex;

	//evaluate the mesh vertices
	if (gl_mesherror.value > 0)
		SubdivideCurve(&curve, mesh, &tempVertices[curve.firstcontrol], gl_mesherror.value);

	//setup rest of the mesh
	mesh->shader = shader;

	CreateIndecies(&curve, mesh);
	CreateTangentSpace(mesh);

	mesh->trans.origin[0] = mesh->trans.origin[1] = mesh->trans.origin[2] = 0.0f;
	mesh->trans.angles[0] = mesh->trans.angles[1] = mesh->trans.angles[2] = 0.0f;
	mesh->trans.scale[0] = mesh->trans.scale[1] = mesh->trans.scale[2] = 1.0f;

	//PutMeshOnCurve(*curve,&tempVertices[curve->firstcontrol]);
	//SubdivideMesh(curve,gl_mesherror.value,1000,&tempVertices[curve->firstcontrol]);
//	Con_Printf("MeshCurve %i %i %i\n",curve->firstcontrol,curve->controlwidth,curve->controlheight);
}

/**
*  Multiplies the curve's color with the current lightmap brightness.
*/
void CS_SetupMeshColors(mesh_t *mesh)
{
	int i;

	for (i=0; i<mesh->numvertices; i++) {
		globalVertexTable[i+mesh->firstvertex].color[0] = (int)(globalVertexTable[i+mesh->firstvertex].color[0]*sh_lightmapbright.value);
		globalVertexTable[i+mesh->firstvertex].color[1] = (int)(globalVertexTable[i+mesh->firstvertex].color[1]*sh_lightmapbright.value);
		globalVertexTable[i+mesh->firstvertex].color[2] = (int)(globalVertexTable[i+mesh->firstvertex].color[2]*sh_lightmapbright.value);
	}
}

/*
void CS_DrawAmbient(mcurve_t *curve)
{
	int i,j, i1, i2;
	int w,h;

	//GL_Bind(curve->texture->gl_texturenum);
	glShadeModel (GL_SMOOTH);
	//Con_Printf("Drawcurve %i %i %i\n",curve->firstvertex,curve->width,curve->height);
	h = curve->width;
	w = curve->height;
	for (i=0; i<w-1; i++) {
		c_brush_polys+= 2*(h-1);
		glBegin(GL_TRIANGLE_STRIP);
		for (j=0; j<h; j++) {
			i1 = curve->firstvertex+j+(i+1)*h;
			i2 = curve->firstvertex+j+i*h;

			glColor3ubv((byte *)&((float *)&globalVertexTable[i2])[7]);
			glTexCoord2fv(&((float *)&globalVertexTable[i2])[3]);
			glVertex3fv((float *)&globalVertexTable[i2]);

			glColor3ubv((byte *)&((float *)&globalVertexTable[i1])[7]);
			glTexCoord2fv(&((float *)&globalVertexTable[i1])[3]);
			glVertex3fv((float *)&globalVertexTable[i1]);
		}
		glEnd();
	}
}
*/