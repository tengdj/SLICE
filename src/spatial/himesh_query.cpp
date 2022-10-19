/*
 * himesh_query.cpp
 *
 *  Created on: Sep 24, 2022
 *      Author: teng
 */

#include "himesh.h"
#include "geometry.h"

namespace hispeed{

void print_triangle(float *triangle, size_t size){
	printf("OFF\n%ld %ld 0\n\n",size*3,size);
	for(size_t i=0;i<size*3;i++){
		printf("%f %f %f\n", *(triangle+3*i), *(triangle+3*i+1), *(triangle+3*i+2));
	}
	for(size_t i=0;i<size;i++){
		printf("3\t%ld %ld %ld\n",i*3,i*3+1,i*3+2);
	}
	printf("\n");
}

bool HiMesh::intersect(HiMesh *target){
	float *tri1, *tri2;
	size_t s1 = fill_triangles(tri1);
	size_t s2 = target->fill_triangles(tri2);
//	print_triangle(tri1, s1);
//	print_triangle(tri2, s2);

	bool inter = TriInt_single(tri1, tri2, s1, s2);
	delete []tri1;
	delete []tri2;
	return inter;
}

float HiMesh::distance(HiMesh *target){
	float dist;
	if(global_ctx.etype == DT_Triangle){
		float *tri1, *tri2;
		size_t s1 = fill_triangles(tri1);
		size_t s2 = target->fill_triangles(tri2);
		dist = TriDist_single(tri1, tri2, s1, s2);
		delete []tri1;
		delete []tri2;
	}else{
		float *seg1,*seg2;
		size_t s1 = fill_segments(seg1);
		size_t s2 = target->fill_segments(seg2);
		dist = SegDist_single(seg1, seg2, s1, s2);
		delete []seg1;
		delete []seg2;
	}
	return dist;
}

bool HiMesh::intersect_tree(HiMesh *target){
	assert(segments.size()>0);
    for(Segment &s:segments){
    	if(target->get_aabb_tree_triangle()->do_intersect(s)){
    		return true;
    	}
    }
    return false;
}

float HiMesh::distance_tree(HiMesh *target){
	double min_dist = DBL_MAX;
	{
		list<Point> vertices = get_vertices();
		for(Point &p:vertices){
			FT sqd;
			if(global_ctx.etype == DT_Segment){
				sqd = target->get_aabb_tree_segment()->squared_distance(p);
			}else{
				sqd = target->get_aabb_tree_triangle()->squared_distance(p);
			}
			double dist = (double)CGAL::to_double(sqd);
			if(min_dist>dist){
				min_dist = dist;
			}
		}
	}
	{
		list<Point> vertices = target->get_vertices();
		for(Point &p:vertices){
			FT sqd;
			if(global_ctx.etype == DT_Segment){
				sqd = get_aabb_tree_segment()->squared_distance(p);
			}else{
				sqd = get_aabb_tree_triangle()->squared_distance(p);
			}
			double dist = (double)CGAL::to_double(sqd);
			if(min_dist>dist){
				min_dist = dist;
			}
		}
	}

	return sqrt(min_dist);
}

range HiMesh::distance_range(HiMesh *target){
	range dist;
	dist.maxdist = distance(target);
	dist.mindist = dist.maxdist - getmaximumCut() - target->getmaximumCut();
	return dist;
}

}

