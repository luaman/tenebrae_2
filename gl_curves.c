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

mcurve_t	*curvechain = NULL;

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
void EvaluateBezier(mmvertex_t *controlpoints,int ofsw, int ofsh, int width, int height, float u, float v,mmvertex_t *result) {

	int i,j;
	float scale;
	float color[4];
	int	n=3;
	int	m=3;
	mmvertex_t *controlpoint;

	for (i=0; i<4; i++) {
		color[i] = 0.0f;
	}

	for (i=0; i<3; i++) {
		result->position[i] = 0.0;
	}

	for (i=0; i<2; i++) {
		result->texture[i] = 0.0;
		result->lightmap[i] = 0.0;
	}

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

	for (i=0; i<4; i++) {
		result->color[i] = (byte)color[i];
	}
}

void EvaluateBiquadraticBeziers(mmvertex_t *controlpoints, int width, int height, float u, float v,mmvertex_t *result) {


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


void MeanVert( mmvertex_t *a, mmvertex_t *b, mmvertex_t *out ) {
	out->position[0] = 0.5 * (a->position[0] + b->position[0]);
	out->position[1] = 0.5 * (a->position[1] + b->position[1]);
	out->position[2] = 0.5 * (a->position[2] + b->position[2]);

	out->texture[0] = 0.5 * (a->texture[0] + b->texture[0]);
	out->texture[1] = 0.5 * (a->texture[1] + b->texture[1]);

	out->lightmap[0] = 0.5 * (a->lightmap[0] + b->lightmap[0]);
	out->lightmap[1] = 0.5 * (a->lightmap[1] + b->lightmap[1]);

	out->color[0] = (a->color[0] + b->color[0]) >> 1;
	out->color[1] = (a->color[1] + b->color[1]) >> 1;
	out->color[2] = (a->color[2] + b->color[2]) >> 1;
	out->color[3] = (a->color[3] + b->color[3]) >> 1;
	
}


/*
=================
PutMeshOnCurve

Drops the aproximating points onto the curve
=================
*/
void PutMeshOnCurve(mcurve_t in, mmvertex_t *verts) {
	int		i, j, l, w, h;
	float	prev, next;
	float	du, dv, u ,v;
	mmvertex_t results[128*128];
	du = 1.0f/(in.width-1);
	dv = 1.0f/(in.height-1);

	for (i=0, u=0; i<in.width; i++, u+=du) {
		for (j=0, v=0; j<in.height; j++, v+=dv) {
			EvaluateBiquadraticBeziers(verts,in.width,in.height,u,v,&results[i+j*in.width]);
		}
	}

	for (i=0; i<in.width*in.height; i++) {
		verts[i] = results[i];
	}

/*
	// put all the aproximating points on the curve
	for (i=0; i<w; i++) {
		for (j=1; j<h; j+=2) {
			for (l=0; l<3; l++) {
				prev = ( verts[j*in.width+i].position[l] + verts[(j+1)*in.width+i].position[l] ) * 0.5;
				next = ( verts[j*in.width+i].position[l] + verts[(j-1)*in.width+i].position[l] ) * 0.5;
				verts[j*in.width+i].position[l] = ( prev + next ) * 0.5;
			}
		}
	}

	for (j=0; j<h; j++) {
		for (i=1; i<w; i+=2) {
			for (l=0; l<3; l++) {
				prev = ( verts[j*in.width+i].position[l] + verts[j*in.width+i+1].position[l] ) * 0.5;
				next = ( verts[j*in.width+i].position[l] + verts[j*in.width+i-1].position[l] ) * 0.5;
				verts[j*in.width+i].position[l] = ( prev + next ) * 0.5;
			}
		}
	}
*/
}

void SubdivideCurve(mcurve_t *in, mmvertex_t *verts, int amount) {
	int		i, j, l, w, h, newwidth, newheight;
	float	prev, next;
	float	du, dv, u ,v;
	mmvertex_t expand[128*128];

	newwidth = in->controlwidth*amount;
	newheight = in->controlheight*amount;

	du = 1.0f/(newwidth-1);
	dv = 1.0f/(newheight-1);

	for (i=0, u=0; i<newwidth; i++, u+=du) {
		for (j=0, v=0; j<newheight; j++, v+=dv) {
			EvaluateBiquadraticBeziers(verts,in->controlwidth,in->controlheight,u,v,&expand[i+j*newwidth]);
		}
	}

	for (i=0; i<newwidth*newheight; i++) {
		if (i==0)
			in->firstvertex = R_AllocateVertexInTemp(expand[i].position,
						expand[i].texture, expand[i].lightmap, expand[i].color);
		else
			R_AllocateVertexInTemp(expand[i].position,
					expand[i].texture, expand[i].lightmap, expand[i].color);
	}

	in->width = newwidth;
	in->height = newheight;
}

#define MAX_EXPANDED_AXIS 128

int	originalWidths[MAX_EXPANDED_AXIS];
int	originalHeights[MAX_EXPANDED_AXIS];

/*
=================
SubdivideMesh

=================
*/
/*
void SubdivideMesh(mcurve_t *in, float maxError, float minLength, mmvertex_t *verts) {
	int			i, j, k, l;
	mmvertex_t	prev, next, mid;
	vec3_t		prevposition, nextposition, midposition;
	vec3_t		delta;
	float		len;
	mmvertex_t	expand[MAX_EXPANDED_AXIS][MAX_EXPANDED_AXIS];

	for ( i = 0 ; i < in->width ; i++ ) {
		for ( j = 0 ; j < in->height ; j++ ) {
			expand[j][i] = verts[j*in->width+i];
		}
	}

	for ( i = 0 ; i < in->height ; i++ ) {
		originalHeights[i] = i;
	}
	for ( i = 0 ; i < in->width ; i++ ) {
		originalWidths[i] = i;
	}

	// horizontal subdivisions
	for ( j = 0 ; j + 2 < in->width ; j += 2 ) {
		// check subdivided midpoints against control points
		for ( i = 0 ; i < in->height ; i++ ) {
			for ( l = 0 ; l < 3 ; l++ ) {
				prevposition[l] = expand[i][j+1].position[l] - expand[i][j].position[l]; 
				nextposition[l] = expand[i][j+2].position[l] - expand[i][j+1].position[l]; 
				midposition[l] = (expand[i][j].position[l] + expand[i][j+1].position[l] * 2
						+ expand[i][j+2].position[l] ) * 0.25;
			}

			// if the span length is too long, force a subdivision
			if ( Length( prevposition ) > minLength 
				|| Length( nextposition ) > minLength ) {
				break;
			}

			// see if this midpoint is off far enough to subdivide
			VectorSubtract( expand[i][j+1].position, midposition, delta );
			len = Length( delta );
			if ( len > maxError ) {
				break;
			}
		}

		if ( in->width + 2 >= MAX_EXPANDED_AXIS ) {
			break;	// can't subdivide any more
		}

		if ( i == in->height ) {
			continue;	// didn't need subdivision
		}

		// insert two columns and replace the peak
		in->width += 2;

		for ( k = in->width - 1 ; k > j + 3 ; k-- ) {
			originalWidths[k] = originalWidths[k-2];
		}
		originalWidths[j+3] = originalWidths[j+1];
		originalWidths[j+2] = originalWidths[j+1];
		originalWidths[j+1] = originalWidths[j];

		for ( i = 0 ; i < in->height ; i++ ) {
			MeanVert( &expand[i][j], &expand[i][j+1], &prev );
			MeanVert( &expand[i][j+1], &expand[i][j+2], &next );
			MeanVert( &prev, &next, &mid );

			for ( k = in->width - 1 ; k > j + 3 ; k-- ) {
				expand[i][k] = expand[i][k-2];
			}
			expand[i][j + 1] = prev;
			expand[i][j + 2] = mid;
			expand[i][j + 3] = next;
		}

		// back up and recheck this set again, it may need more subdivision
		j -= 2;

	}

	// vertical subdivisions
	for ( j = 0 ; j + 2 < in->height ; j += 2 ) {
		// check subdivided midpoints against control points
		for ( i = 0 ; i < in->width ; i++ ) {
			for ( l = 0 ; l < 3 ; l++ ) {
				prevposition[l] = expand[j+1][i].position[l] - expand[j][i].position[l]; 
				nextposition[l] = expand[j+2][i].position[l] - expand[j+1][i].position[l]; 
				midposition[l] = (expand[j][i].position[l] + expand[j+1][i].position[l] * 2
						+ expand[j+2][i].position[l] ) * 0.25;
			}

			// if the span length is too long, force a subdivision
			if ( Length( prevposition ) > minLength 
				|| Length( nextposition ) > minLength ) {
				break;
			}
			// see if this midpoint is off far enough to subdivide
			VectorSubtract( expand[j+1][i].position, midposition, delta );
			len = Length( delta );
			if ( len > maxError ) {
				break;
			}
		}

		if ( in->height + 2 >= MAX_EXPANDED_AXIS ) {
			break;	// can't subdivide any more
		}

		if ( i == in->width ) {
			continue;	// didn't need subdivision
		}

		// insert two columns and replace the peak
		in->height += 2;

		for ( k = in->height - 1 ; k > j + 3 ; k-- ) {
			originalHeights[k] = originalHeights[k-2];
		}
		originalHeights[j+3] = originalHeights[j+1];
		originalHeights[j+2] = originalHeights[j+1];
		originalHeights[j+1] = originalHeights[j];

		for ( i = 0 ; i < in->width ; i++ ) {
			MeanVert( &expand[j][i], &expand[j+1][i], &prev );
			MeanVert( &expand[j+1][i], &expand[j+2][i], &next );
			MeanVert( &prev, &next, &mid );

			for ( k = in->height - 1 ; k > j + 3 ; k-- ) {
				expand[k][i] = expand[k-2][i];
			}
			expand[j+1][i] = prev;
			expand[j+2][i] = mid;
			expand[j+3][i] = next;
		}

		// back up and recheck this set again, it may need more subdivision
		j -= 2;

	}

	// collapse the verts

	verts = &expand[0][0];
	for ( i = 1 ; i < in->height ; i++ ) {
		memmove( &verts[i*in->width], expand[i], in->width * sizeof(mmvertex_t) );
	}

	for (i=0; i<in->width; i++) {
		for (j=0; j<in->height; j++) {
				if ((i==0) && (j==0))
					in->firstcontrol = R_AllocateVertexInTemp(expand[j][i].position,
										expand[j][i].texture, expand[j][i].lightmap, expand[j][i].color);
				else
					R_AllocateVertexInTemp(expand[j][i].position,
						expand[j][i].texture, expand[j][i].lightmap, expand[j][i].color);
		}
	}
*/
	/*
	out.verts = &expand[0][0];
	for ( i = 1 ; i < out.height ; i++ ) {
		memmove( &out.verts[i*out.width], expand[i], out.width * sizeof(drawVert_t) );
	}

	return CopyMesh(&out);
	*/
//}

/*
=================
CurveCreate

Creates a curve from the given surface

=================
*/
void CS_Create(dq3face_t *in, mcurve_t *curve, texture_t *texture)
{
	curve->controlwidth = in->patchOrder[0];
	curve->controlheight = in->patchOrder[1];
	curve->firstcontrol = in->firstvertex;
	
	//just use the control points as vertices
	curve->firstvertex = in->firstmeshvertex;
	curve->width = curve->controlwidth;
	curve->height = curve->controlheight;

	curve->next = NULL;
	curve->texture = texture;

	if (gl_mesherror.value > 0)
		SubdivideCurve(curve,&tempVertices[curve->firstcontrol],gl_mesherror.value);

	//PutMeshOnCurve(*curve,&tempVertices[curve->firstcontrol]);
	//SubdivideMesh(curve,gl_mesherror.value,1000,&tempVertices[curve->firstcontrol]);
//	Con_Printf("MeshCurve %i %i %i\n",curve->firstcontrol,curve->controlwidth,curve->controlheight);
}

void CS_DrawAmbient(mcurve_t *curve)
{
	int i,j, i1, i2;
	int w,h;

	GL_Bind(curve->texture->gl_texturenum);
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
