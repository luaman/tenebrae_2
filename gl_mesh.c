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
// gl_mesh.c: triangle model functions

#include "quakedef.h"

model_t		*aliasmodel;
aliashdr_t	*paliashdr;

/*
Yet another hack.  Some models seem to have edges shared between three triangles, this is obviously
a strange thing to have, we resolve it simply by throwing away that shared egde and giving all
triangles a "-1" neighbour for that edge.  This will give some unneeded fins for some edges of some models
but this number is generally verry low (< 3 edges per model) and only on a few models.
*/
int findneighbour(int index, int edgei, int numtris) {
	int i, j, v1, v0, found,foundj = 0;
	mtriangle_t *current = &triangles[index];
	mtriangle_t *t;
	qboolean	dup;

	v0 = current->vertindex[edgei];
	v1 = current->vertindex[(edgei+1)%3];

	//XYZ
	found = -1;
	dup = false;
	for (i=0; i<numtris; i++) {
		if (i == index) continue;
		t = &triangles[i];

		for (j=0; j<3; j++) {      
			if (((current->vertindex[edgei] == triangles[i].vertindex[j]) 
			    && (current->vertindex[(edgei+1)%3] == triangles[i].vertindex[(j+1)%3]))
				||
			   ((current->vertindex[edgei] == triangles[i].vertindex[(j+1)%3]) 
			    && (current->vertindex[(edgei+1)%3] == triangles[i].vertindex[j])))
			{
				//no edge for this model found yet?
				if (found == -1) {
					found = i;
					foundj = j;
				}
				//the three edges story
				else
					dup = true;
			}
  
		}
	}

	//normal edge, setup neighbour pointers
	if (!dup) {
		triangles[found].neighbours[foundj] = index;
		return found;
	}
	//naughty egde let no-one have the neighbour
	//Con_Printf("%s: warning: open edge added\n",loadname);
	return -1;
}

int	numNormals[MAXALIASTRIS];
int	dupIndex[MAXALIASTRIS];

void TangentForTri(mtriangle_t *tri, vec3_t norm, ftrivertx_t *verts, fstvert_t *texcos, vec3_t res) {

	vec3_t	vec1, vec2, dirv, tz;
	float	delta1, delta2, t;
	vec3_t	*v[3];
	float	st[3][2];
	int		j;

	for (j=0; j<3; j++) {
		v[j] = &verts[tri->vertindex[j]].v;

		st[j][0] = texcos[tri->vertindex[j]].s;
		st[j][1] = texcos[tri->vertindex[j]].t;
	}

	vec1[0] = *v[1][0] - *v[0][0];
	vec1[1] = *v[1][1] - *v[0][1];
	vec1[2] = *v[1][2] - *v[0][2];
	delta1 = st[1][0] - st[0][0];
      
	vec2[0] = *v[2][0] - *v[0][0];
	vec2[1] = *v[2][1] - *v[0][1];
	vec2[2] = *v[2][2] - *v[0][2];
	delta2 = st[2][0] - st[0][0];

	dirv[0] = (delta1 * vec2[0] - vec1[0] * delta2);
	dirv[1] = (delta1 * vec2[1] - vec1[1] * delta2);
	dirv[2] = (delta1 * vec2[2] - vec1[2] * delta2);

	VectorNormalize(dirv);

	VectorCopy(norm,tz);
	t = dirv[0]*tz[0]+dirv[1]*tz[1]+dirv[2]*tz[2];
	res[0] = dirv[0]-t*tz[0];
	res[1] = dirv[1]-t*tz[1];
	res[2] = dirv[2]-t*tz[2];
}

int NormalToLatLong( const vec3_t normal) {

	unsigned short r;
	byte *bytes = (byte *)&r;
	// check for singularities
	if ( normal[0] == 0 && normal[1] == 0 ) {
		if ( normal[2] > 0 ) {
			r = 0;
			bytes[0] = 0;
			bytes[1] = 0;		// lat = 0, long = 0
		} else {
			bytes[0] = 128;
			bytes[1] = 0;		// lat = 0, long = 128
		}
	} else {
		int	a, b;

		a = RAD2DEG( atan2( normal[1], normal[0] ) ) * (255.0f / 360.0f );
		a &= 0xff;

		b = RAD2DEG( acos( normal[2] ) ) * ( 255.0f / 360.0f );
		b &= 0xff;

		bytes[0] = b;	// longitude
		bytes[1] = a;	// lattitude
	}

	return r;
}
			
/*
================
GL_MakeAliasModelDisplayLists

PENTA: For shadow volume generation & bumpmapping we need extra data so we generate/save
it here. This is very memory consuming at the moment (I had to increase quake's default 
heap size for it) but since everything still fits well withing 32MB of RAM I don't have high
priority for fixing it.
================
*/

void GL_MakeAliasModelDisplayLists (model_t *m, aliashdr_t *hdr)
{
	int		i, j, k, l;
	ftrivertx_t	*verts, *v, *oldverts;
	mtriangle_t *tris;
	fstvert_t	*texcoords;
	char	cache[MAX_QPATH], fullpath[MAX_OSPATH];
	FILE	*f;
	plane_t	*norms;
	vec3_t	v1, v2, normal;
	vec3_t	triangle[3];
	vec3_t	*tangents;
	float	s,t;
	int		*indecies;
	int		newcount;


	aliasmodel = m;
	paliashdr = hdr;	// (aliashdr_t *)Mod_Extradata (m);
	//
	// look for a cached version
	//

/*
	strcpy (cache, "glquake/");
	COM_StripExtension (m->name+strlen("progs/"), cache+strlen("glquake/"));
	strcat (cache, ".ms2");

	COM_FOpenFile (cache, &f);	
	if (f)
	{
		fread (&numcommands, 4, 1, f);
		fread (&numorder, 4, 1, f);
		fread (&commands, numcommands * sizeof(commands[0]), 1, f);
		fread (&vertexorder, numorder * sizeof(vertexorder[0]), 1, f);
		fclose (f);
	}
	else
	{
		//
		// build it from scratch
		//
		Con_Printf ("meshing %s...\n",m->name);

		BuildTris ();		// trifans or strips

		//
		// save out the cached version
		//
		sprintf (fullpath, "%s/%s", com_gamedir, cache);
		f = fopen (fullpath, "wb");
		if (f)
		{
			fwrite (&numcommands, 4, 1, f);
			fwrite (&numorder, 4, 1, f);
			fwrite (&commands, numcommands * sizeof(commands[0]), 1, f);
			fwrite (&vertexorder, numorder * sizeof(vertexorder[0]), 1, f);
			fclose (f);
		}
	}

*/
	// save the data out
	paliashdr->poseverts = paliashdr->numverts;//numorder;
/*
	//cmds = Hunk_Alloc (numcommands * 4);
	//paliashdr->commands = (byte *)cmds - (byte *)paliashdr;
	//memcpy (cmds, commands, numcommands * 4);
*/
	//backup verts in oldverts since we use verts as an loop pointer
	//& convert vertices back to floating point

	//Set neighbours to NULL
	for (i=0 ; i<paliashdr->numtris ; i++)
		for (j=0 ; j<3 ; j++) {
			triangles[i].neighbours[j] = -1;
		}
	

	//PENTA: Generate edges information (for shadow volumes)
	//Note: We do this with the original vertices not the reordered onces since reordening them
	//duplicates vertices and we only compare indices
	for (i=0 ; i<paliashdr->numtris ; i++)
		for (j=0 ; j<3 ; j++) {
			//none found yet 
			if (triangles[i].neighbours[j] == -1) {
				triangles[i].neighbours[j] = findneighbour(i, j, paliashdr->numtris);
			}
		}

	//PENTA: Calculate texcoords for triangles (this duplicates some vertices that have different
	//texcoords for the sames verts)
	for (i=0; i<paliashdr->poseverts; i++) {
		dupIndex[i] = 0;
	}

	newcount = paliashdr->poseverts;
	for (i=0; i<paliashdr->numtris ; i++) {
		for (j=0; j<3; j++) {
			if (!triangles[i].facesfront && stverts[triangles[i].vertindex[j]].onseam) {

				if (dupIndex[triangles[i].vertindex[j]] != 0) {
					continue;
				}
				dupIndex[triangles[i].vertindex[j]] = newcount;
				newcount++;
			}
		}
	}

	for (i=0; i<newcount; i++) {
		dupIndex[i] = 0;
	}

	oldverts = verts = Hunk_Alloc (paliashdr->numposes * newcount * sizeof(ftrivertx_t) );
	paliashdr->posedata = (byte *)verts - (byte *)paliashdr;

	for (i=0; i<paliashdr->numtris ; i++) {
		for (j=0; j<3; j++) {
			s = stverts[triangles[i].vertindex[j]].s;
			t = stverts[triangles[i].vertindex[j]].t;

			if (!triangles[i].facesfront && stverts[triangles[i].vertindex[j]].onseam) {
				int newindex;

				if (dupIndex[triangles[i].vertindex[j]] != 0) {
					triangles[i].vertindex[j] = dupIndex[triangles[i].vertindex[j]];
					continue;
				}

				newindex = paliashdr->poseverts;

				if (paliashdr->poseverts >= MAXALIASVERTS) {
					Con_Printf("To many verts");
				}

				//Duplicate it in all poses
				for (k=0; k<paliashdr->numposes; k++) {
					verts[k*newcount+newindex].v[0] = poseverts[k][triangles[i].vertindex[j]].v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
					verts[k*newcount+newindex].v[1] = poseverts[k][triangles[i].vertindex[j]].v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
					verts[k*newcount+newindex].v[2] = poseverts[k][triangles[i].vertindex[j]].v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];
					verts[k*newcount+newindex].lightnormalindex = NormalToLatLong(r_avertexnormals[poseverts[k][triangles[i].vertindex[j]].lightnormalindex]); //XYZ
				}
				
				//Create a new stvert
				s += pheader->skinwidth / 2;//yet another crappy way to save some space
				stverts[newindex].s = (int)s;
				stverts[newindex].t = (int)t;

				//Adapt index pointer
				dupIndex[triangles[i].vertindex[j]] = newindex;
				triangles[i].vertindex[j] = newindex;

				//Next free
				paliashdr->poseverts++;
			} else {
				//Move it in all poses
				int newindex = triangles[i].vertindex[j];
				for (k=0; k<paliashdr->numposes; k++) {
					verts[k*newcount+newindex].v[0] = poseverts[k][newindex].v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
					verts[k*newcount+newindex].v[1] = poseverts[k][newindex].v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
					verts[k*newcount+newindex].v[2] = poseverts[k][newindex].v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];
					//verts[k*newcount+newindex].lightnormalindex = poseverts[k][newindex].lightnormalindex;
					verts[k*newcount+newindex].lightnormalindex = NormalToLatLong(r_avertexnormals[poseverts[k][triangles[i].vertindex[j]].lightnormalindex]); //XYZ
				}
			}
		}
	}
	if (paliashdr->poseverts != newcount) {
		Con_Printf("Not equal %i %i\n",paliashdr->poseverts,newcount);
	}

	//oldverts = verts = Hunk_Alloc (paliashdr->numposes * paliashdr->poseverts * sizeof(ftrivertx_t) );
	//paliashdr->posedata = (byte *)verts - (byte *)paliashdr;
	//for (i=0 ; i<paliashdr->numposes ; i++)
	//	for (j=0 ; j</*numorder*/paliashdr->poseverts ; j++) {
	//		vert = verts++;
	//		vert->v[0] = poseverts[i][/*vertexorder[j]*/j].v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
	//		vert->v[1] = poseverts[i][/*vertexorder[j]*/j].v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
	//		vert->v[2] = poseverts[i][/*vertexorder[j]*/j].v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];
	//		vert->lightnormalindex = poseverts[i][vertexorder[j]].lightnormalindex;
	//	}

	//verts = oldverts;

	//PENTA: Calculate texcoords for triangles (bump mapping)
	texcoords = Hunk_Alloc (paliashdr->poseverts * sizeof(fstvert_t));
	paliashdr->texcoords = (byte *)texcoords - (byte *)paliashdr;
	for (i=0; i<paliashdr->poseverts ; i++) {
		s = stverts[i].s;
		t = stverts[i].t;
		//if (!triangles[i].facesfront && stverts[triangles[i].vertindex[j]].onseam)
		//	s += pheader->skinwidth / 2;//yet another crappy way to save some space
		s = (s-0.5)  / pheader->skinwidth;
		t = (t-0.5)  / pheader->skinheight;
		texcoords[i].s = s;
		texcoords[i].t = t;
	}

	//PENTA: Save triangles (for shadow volumes)
	tris = Hunk_Alloc (paliashdr->numtris * sizeof(mtriangle_t));
	paliashdr->triangles = (byte *)tris - (byte *)paliashdr;
	Q_memcpy(tris, &triangles, paliashdr->numtris * sizeof(mtriangle_t));

	//PENTA: Calculate plane eq's for triangles (shadow volumes)
	norms = Hunk_Alloc (paliashdr->numtris * paliashdr->numposes * sizeof(plane_t));
	paliashdr->planes = (byte *)norms - (byte *)paliashdr;
	for (i=0; i<paliashdr->numposes; i++) {
		for (j=0; j<paliashdr->numtris ; j++) {

			//make 3 vec3_t's of this triangle's vertices
			for (k=0; k<3; k++) {
				v = &verts[i*paliashdr->poseverts + tris[j].vertindex[k]];
				for (l=0; l<3; l++)
					triangle[k][l] = v->v[l];		
			}

			//calculate their normal
			VectorSubtract(triangle[0], triangle[1], v1);
			VectorSubtract(triangle[2], triangle[1], v2);
			CrossProduct(v2,v1, normal);		
			VectorScale(normal, 1/Length(normal), norms[i*paliashdr->numtris+j].normal);
			
			//distance of plane eq
			norms[i*paliashdr->numtris+j].dist = DotProduct(triangle[0],norms[i*paliashdr->numtris+j].normal);
		}
	}
	

	//PENTA: Create index lists
	indecies = Hunk_Alloc (paliashdr->numtris * sizeof(int) * 3);
	paliashdr->indecies = (byte *)indecies - (byte *)paliashdr;
	for (i=0 ; i<paliashdr->numtris ; i++) {
		for (j=0 ; j<3 ; j++) {
			//Throw vertex index into our index array
			(*indecies) = triangles[i].vertindex[j];
			indecies++;
		}
	}

	
	//PENTA: Calculate tangents for vertices (bump mapping)
	tangents = Hunk_Alloc (paliashdr->poseverts * paliashdr->numposes * sizeof(vec3_t));
	paliashdr->tangents = (byte *)tangents - (byte *)paliashdr;
	paliashdr->binormals = 0;
	//for all frames
	for (i=0; i<paliashdr->numposes; i++) {

		//set temp to zero
		for (j=0; j<paliashdr->poseverts; j++) {
			tangents[i*paliashdr->poseverts+j][0] = 0;
			tangents[i*paliashdr->poseverts+j][1] = 0;
			tangents[i*paliashdr->poseverts+j][2] = 0;
			numNormals[j] = 0;
		}

		
		//for all tris
		for (j=0; j<paliashdr->numtris; j++) {
			vec3_t tangent;
			TangentForTri(&tris[j],norms[i*paliashdr->numtris+j].normal,
						  &verts[i*paliashdr->poseverts],texcoords,tangent);
			//for all vertices in the tri
			for (k=0; k<3; k++) {
				l = tris[j].vertindex[k];
				VectorAdd(tangents[i*paliashdr->poseverts+l],tangent,
							tangents[i*paliashdr->poseverts+l]); 
				numNormals[l]++;
			}
		}
		
		//calculate average
		for (j=0; j<paliashdr->poseverts; j++) {
			if (!numNormals[j]) continue;
			tangents[i*paliashdr->poseverts+j][0] = tangents[i*paliashdr->poseverts+j][0]/numNormals[j];
			tangents[i*paliashdr->poseverts+j][1] = tangents[i*paliashdr->poseverts+j][1]/numNormals[j];
			tangents[i*paliashdr->poseverts+j][2] = tangents[i*paliashdr->poseverts+j][2]/numNormals[j];
			VectorNormalize(tangents[i*paliashdr->poseverts+j]);
		}
	}
}
