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
// r_light.c - PENTA: almost obsolete now

#include "quakedef.h"

int	r_dlightframecount;
int		d_lightstylevalue[256];	// 8.8 fraction of base light value

//PENTA: Math utitity's
void ProjectVector(const vec3_t b,const vec3_t a,vec3_t c) {

   float dpa,dpab;

   dpa = DotProduct(a,a);
   dpab = DotProduct(a,b)/dpa;
   c[0] = a[0] * dpab;
   c[1] = a[1] * dpab;
   c[2] = a[2] * dpab;
}


void ProjectPlane(const vec3_t src,const vec3_t v1,const vec3_t v2,vec3_t dst) {

	vec3_t t1,t2;

	ProjectVector(src, v1, t1);
	ProjectVector(src, v2, t2);

	VectorAdd(t1,t2,dst);
}

/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight (void)
{
	int			i,j,k;
	
//
// light animations
// 'm' is normal light, 'a' is no light, 'z' is double bright
	i = (int)(cl.time*10);
	for (j=0 ; j<MAX_LIGHTSTYLES ; j++)
	{
		if (!cl_lightstyle[j].length)
		{
			d_lightstylevalue[j] = 256;
			continue;
		}
		k = i % cl_lightstyle[j].length;
		k = cl_lightstyle[j].map[k] - 'a';
		k = k*22;
		d_lightstylevalue[j] = k;
	}	
}

int R_LightPoint (vec3_t p)
{
	//PENTA: use the q3 lightgrid for this
	return 255;
}

