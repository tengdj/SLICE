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
}
void HiMesh_Wrapper::writeMeshOff(){
	assert(mesh);
	stringstream ss;
	ss<<id<<".off";
	mesh->writeMeshOff(ss.str().c_str());
}
void HiMesh_Wrapper::advance_to(int lod){
	assert(mesh);
	//pthread_mutex_lock(&lock);
	mesh->advance_to(lod);
	//pthread_mutex_unlock(&lock);
}
// fill the segments into voxels
// seg_tri: 0 for segments, 1 for triangle
size_t HiMesh_Wrapper::fill_voxels(enum data_type seg_tri){
	if(mesh){
		return mesh->fill_voxels(voxels, seg_tri);
	}
	return 0;
}
size_t HiMesh_Wrapper::num_vertices(){
	return mesh->size_of_vertices();
}

void HiMesh_Wrapper::reset(){
	pthread_mutex_lock(&lock);
	for(Voxel *v:voxels){
		v->reset();
	}
	pthread_mutex_unlock(&lock);
}

}
