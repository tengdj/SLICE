#ifndef HISPEED_GEOMETRY_H
#define HISPEED_GEOMETRY_H

#include <math.h>
#include <stdio.h>
#include <iostream>
#include <float.h>
#include "mygpu.h"
#include "util.h"
#include "pthread.h"
using namespace std;

namespace hispeed{

// useful functions

// copy
inline
void
VcV(float Vr[3], const float V[3])
{
  Vr[0] = V[0];  Vr[1] = V[1];  Vr[2] = V[2];
}

// minus
inline
void
VmV(float Vr[3], const float V1[3], const float V2[3])
{
  Vr[0] = V1[0] - V2[0];
  Vr[1] = V1[1] - V2[1];
  Vr[2] = V1[2] - V2[2];
}

// plus
inline
void
VpV(float Vr[3], const float V1[3], const float V2[3])
{
  Vr[0] = V1[0] + V2[0];
  Vr[1] = V1[1] + V2[1];
  Vr[2] = V1[2] + V2[2];
}

// plus after product
inline
void
VpVxS(float Vr[3], const float V1[3], const float V2[3], float s)
{
  Vr[0] = V1[0] + V2[0] * s;
  Vr[1] = V1[1] + V2[1] * s;
  Vr[2] = V1[2] + V2[2] * s;
}

inline
void
VcrossV(float Vr[3], const float V1[3], const float V2[3])
{
  Vr[0] = V1[1]*V2[2] - V1[2]*V2[1];
  Vr[1] = V1[2]*V2[0] - V1[0]*V2[2];
  Vr[2] = V1[0]*V2[1] - V1[1]*V2[0];
}

// dot product
inline
float
VdotV(const float V1[3], const float V2[3])
{
  return (V1[0]*V2[0] + V1[1]*V2[1] + V1[2]*V2[2]);
}

// Euclid distance
inline
float
VdistV2(const float V1[3], const float V2[3])
{
  return ( (V1[0]-V2[0]) * (V1[0]-V2[0]) +
	   (V1[1]-V2[1]) * (V1[1]-V2[1]) +
	   (V1[2]-V2[2]) * (V1[2]-V2[2]));
}


// multiple each value in V with constant s
inline
void
VxS(float Vr[3], const float V[3], float s)
{
  Vr[0] = V[0] * s;
  Vr[1] = V[1] * s;
  Vr[2] = V[2] * s;
}

inline
void
VdS(float Vr[3], const float V[3], float s)
{
	assert(s>0);
  Vr[0] = V[0] / s;
  Vr[1] = V[1] / s;
  Vr[2] = V[2] / s;
}

inline float distance(const float *p1, const float *p2){
	float cur_dist = 0;
	for(int t=0;t<3;t++){
		cur_dist += (p1[t]-p2[t])*(p1[t]-p2[t]);
	}
	return cur_dist;
}

typedef struct result_container_{
	uint p1;
	uint p2;
	bool intersected;
	float distance;
} result_container;

class geometry_param{
public:
	geometry_param(){

	}
	~geometry_param(){
		if(data!=NULL){
			delete data;
			data = NULL;
		}
		if(hausdorff!=NULL){
			delete hausdorff;
			hausdorff = NULL;
		}
		if(offset_size!=NULL){
			delete offset_size;
			offset_size = NULL;
		}
	}
	int id = 0;
	uint pair_num = 0;
	uint element_num = 0;
	size_t element_pair_num = 0;
	float *data = NULL;
	float *hausdorff = NULL;
	// the offset and size of the computing pairs
	uint *offset_size = NULL;
	result_container *results = NULL;
};


void compute_normal(float *Norm, const float *triangle);
bool PointInTriangleCylinder(const float *point, const float *triangle);
void project_points_to_triangle_plane(const float *point, const float *triangle, float projected_point[3]);
float PointTriangleDist(const float *point, const float *triangle);
float TriDist(const float *S, const float *T);
result_container TriDist_single(const float *data1, const float *data2, size_t size1, size_t size2);
void TriDist_batch_gpu(gpu_info *gpu, const float *data, const uint *offset_size,
		               result_container *result, const uint pair_num, const uint element_num);

result_container TriInt_single(const float *data1, const float *data2, size_t size1, size_t size2, const float *hausdorff1 = NULL, const float *hausdorff2 = NULL);
void TriInt_batch_gpu(gpu_info *gpu, const float *data, const uint *offset_size,
		result_container *result, const uint batch_num, const uint triangle_num);

class geometry_computer{
	pthread_mutex_t gpu_lock;
	pthread_mutex_t cpu_lock;
	int max_thread_num = hispeed::get_num_threads();
	bool cpu_busy = false;
	bool gpu_busy = false;
	bool request_cpu();
	void release_cpu();
	gpu_info *request_gpu(int min_size, bool force=false);
	void release_gpu(gpu_info *info);

	char *d_cuda = NULL;
	vector<gpu_info *> gpus;

public:
	~geometry_computer();
	geometry_computer(){
		pthread_mutex_init(&cpu_lock, NULL);
		pthread_mutex_init(&gpu_lock, NULL);
	}

	bool init_gpus();
	void get_distance_gpu(geometry_param &param);
	void get_distance_cpu(geometry_param &param);
	void get_distance(geometry_param &param);

	void get_intersect_gpu(geometry_param &param);
	void get_intersect_cpu(geometry_param &param);
	void get_intersect(geometry_param &param);
	void set_thread_num(uint num){
		max_thread_num = num;
	}
};


}
#endif
