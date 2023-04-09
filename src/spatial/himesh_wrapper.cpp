/*
 * himesh_wrapper.cpp
 *
 *  Created on: Jun 24, 2022
 *      Author: teng
 */


#include "himesh.h"

namespace hispeed{

/*
 * himesh wrapper functions
 * */

HiMesh_Wrapper::HiMesh_Wrapper(){
	pthread_mutex_init(&lock, NULL);
}
HiMesh_Wrapper::~HiMesh_Wrapper(){
	for(Voxel *v:voxels){
		delete v;
	}
	voxels.clear();
	if(mesh){
		delete mesh;
	}
	results.clear();
}

void HiMesh_Wrapper::advance_to(uint lod){
	assert(mesh);
	mesh->decode(lod);
	cur_lod = lod;
}

void HiMesh_Wrapper::disable_innerpart(){
	if(voxels.size()>1){
		for(Voxel *v:voxels){
			delete v;
		}
		voxels.clear();
		Voxel *v = new Voxel();
		v->set_box(box);
		for(int i=0;i<3;i++){
			v->core[i] = (v->high[i]-v->low[i])/2+v->low[i];
		}
		voxels.push_back(v);
	}
}

// fill the triangles into voxels
size_t HiMesh_Wrapper::fill_voxels(){
	assert(mesh);
	size_t sz = mesh->fill_voxels(voxels);
	return sz;
}

size_t HiMesh_Wrapper::num_vertices(){
	return mesh->size_of_vertices();
}

void HiMesh_Wrapper::reset(){
	pthread_mutex_lock(&lock);
	for(Voxel *v:voxels){
		v->reset();
	}
	results.clear();
	pthread_mutex_unlock(&lock);
}

void HiMesh_Wrapper::report_result(HiMesh_Wrapper *result){
	pthread_mutex_lock(&lock);
	results.push_back(result);
	pthread_mutex_unlock(&lock);
	//log("%d find %d", id, result->id);
}

}
