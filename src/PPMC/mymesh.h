/*****************************************************************************
* Copyright (C) 2011 Adrien Maglo and Clément Courbet
*
* This file is part of PPMC.
*
* PPMC is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* PPMC is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with PPMC.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef PROGRESSIVEPOLYGONS_MYMESH_H
#define PROGRESSIVEPOLYGONS_MYMESH_H

#ifndef CGAL_EIGEN3_ENABLED
#define CGAL_EIGEN3_ENABLED
#endif

#include <iostream>
#include <fstream>
#include <stdint.h>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/circulator.h>
#include <CGAL/bounding_box.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Polyhedron_incremental_builder_3.h>

#include <queue>
#include <assert.h>

#include "configuration.h"
#include "query_context.h"

#include "aab.h"
#include "util.h"

// definition for the CGAL library
//typedef CGAL::Exact_predicates_exact_constructions_kernel MyKernel;
typedef CGAL::Simple_cartesian<float> MyKernel;

typedef MyKernel::Point_3 Point;
typedef MyKernel::Vector_3 Vector;

typedef CGAL::Simple_cartesian<double> MyKernelDouble;
typedef MyKernelDouble::Vector_3 VectorDouble;

typedef CGAL::Simple_cartesian<int> MyKernelInt;
typedef MyKernelInt::Point_3 PointInt;
typedef MyKernelInt::Vector_3 VectorInt;

using namespace hispeed;


// the builder for reading the base mesh
template <class HDS> class MyMeshBaseBuilder : public CGAL::Modifier_base<HDS>
{
public:
    MyMeshBaseBuilder(std::deque<Point> *p_pointDeque, std::deque<uint32_t *> *p_faceDeque)
        : p_pointDeque(p_pointDeque), p_faceDeque(p_faceDeque) {}

    void operator()(HDS& hds)
    {
        CGAL::Polyhedron_incremental_builder_3<HDS> B(hds, true);

        size_t nbVertices = p_pointDeque->size();
        size_t nbFaces = p_faceDeque->size();

        B.begin_surface(nbVertices, nbFaces);

        for (unsigned i = 0; i < nbVertices; ++i)
            B.add_vertex(p_pointDeque->at(i));

        for (unsigned i = 0; i < nbFaces; ++i)
        {
            B.begin_facet();
            uint32_t *f = p_faceDeque->at(i);
            for (unsigned j = 1; j < f[0] + 1; ++j)
                B.add_vertex_to_facet(f[j]);
            B.end_facet();
        }

        B.end_surface();
    }

private:
    std::deque<Point> *p_pointDeque;
    std::deque<uint32_t *> *p_faceDeque;
};

// My vertex type has a isConquered flag
template <class Refs>
class MyVertex : public CGAL::HalfedgeDS_vertex_base<Refs,CGAL::Tag_true, Point>
{
    enum Flag {Unconquered=0, Conquered=1};

  public:
    MyVertex(): CGAL::HalfedgeDS_vertex_base<Refs,CGAL::Tag_true, Point>(), flag(Unconquered), id(0), i_quantCellId(0){}
	MyVertex(const Point &p): CGAL::HalfedgeDS_vertex_base<Refs,CGAL::Tag_true, Point>(p), flag(Unconquered), id(0), i_quantCellId(0){}

	inline void resetState()
	{
	  flag=Unconquered;
	}

	inline bool isConquered() const
	{
	  return flag==Conquered;
	}

	inline void setConquered()
	{
	  flag=Conquered;
	}

	inline size_t getId() const
	{
		return id;
	}

	inline void setId(size_t nId)
	{
		id = nId;
	}

	inline unsigned getQuantCellId() const
	{
		return i_quantCellId;
	}

	inline void setQuantCellId(unsigned nId)
	{
		i_quantCellId = nId;
	}

	inline void setRecessing(){
		is_protruding = false;
	}

	inline bool isProtruding(){
		return is_protruding;
	}

  private:
	Flag flag;
	size_t id;
	unsigned i_quantCellId;
	bool is_protruding = true;
};


// My vertex type has a isConquered flag
template <class Refs>
class MyHalfedge : public CGAL::HalfedgeDS_halfedge_base<Refs>
{
    enum Flag {NotYetInQueue=0, InQueue=1, InQueue2=2, NoLongerInQueue=3};
    enum Flag2 {Original, Added, New};
    enum ProcessedFlag {NotProcessed, Processed};

  public:
        MyHalfedge(): flag(NotYetInQueue), flag2(Original),
        processedFlag(NotProcessed){}

	inline void resetState()
	{
		flag = NotYetInQueue;
		flag2 = Original;
		processedFlag = NotProcessed;
	}

        /* Flag 1 */

	inline void setInQueue()
	{
	  flag=InQueue;
	}

	inline void setInProblematicQueue()
	{
	  assert(flag==InQueue);
	  flag=InQueue2;
	}

	inline void removeFromQueue()
	{
	  assert(flag==InQueue || flag==InQueue2);
	  flag=NoLongerInQueue;
	}

	inline bool isInNormalQueue() const
	{
	  return flag==InQueue;
	}

	inline bool isInProblematicQueue() const
	{
	  return flag==InQueue2;
	}

	/* Processed flag */

	inline void resetProcessedFlag()
	{
	  processedFlag = NotProcessed;
	}

	inline void setProcessed()
	{
		processedFlag = Processed;
	}

	inline bool isProcessed() const
	{
		return (processedFlag == Processed);
	}

	/* Flag 2 */

	inline void setAdded()
	{
	  assert(flag2 == Original);
	  flag2=Added;
	}

	inline void setNew()
	{
		assert(flag2 == Original);
		flag2 = New;
	}

	inline bool isAdded() const
	{
	  return flag2==Added;
	}

	inline bool isOriginal() const
	{
	  return flag2==Original;
	}

	inline bool isNew() const
	{
	  return flag2 == New;
	}

  private:
	Flag flag;
	Flag2 flag2;
    ProcessedFlag processedFlag;

};

// My face type has a vertex flag
template <class Refs>
class MyFace : public CGAL::HalfedgeDS_face_base<Refs>
{
    enum Flag {Unknown=0, Splittable=1, Unsplittable=2};
    enum ProcessedFlag {NotProcessed, Processed};

  public:
    MyFace(): flag(Unknown), processedFlag(NotProcessed){}

	inline void resetState()
	{
          flag = Unknown;
          processedFlag = NotProcessed;
	}

	inline void resetProcessedFlag()
	{
	  processedFlag = NotProcessed;
	}

	inline bool isConquered() const
	{
	  return (flag==Splittable ||flag==Unsplittable) ;
	}

	inline bool isSplittable() const
	{
	  return (flag==Splittable) ;
	}

	inline bool isUnsplittable() const
	{
	  return (flag==Unsplittable) ;
	}

	inline void setSplittable()
	{
	  assert(flag == Unknown);
	  flag=Splittable;
	}

	inline void setUnsplittable()
	{
	  assert(flag == Unknown);
	  flag=Unsplittable;
	}

	inline void setProcessedFlag()
	{
		processedFlag = Processed;
	}

	inline bool isProcessed() const
	{
		return (processedFlag == Processed);
	}

	inline Point getRemovedVertexPos() const
	{
		return removedVertexPos;
	}

	inline void setRemovedVertexPos(Point p)
	{
		removedVertexPos = p;
	}

	inline void addImpactPoint(Point p){
		for(Point &ep:impact_points){
			if(ep==p){
				return;
			}
		}
		impact_points.push_back(p);
	}

	inline void addImpactPoints(vector<Point> ps){
		for(Point p:ps){
			addImpactPoint(p);
		}
	}

	inline vector<Point> getImpactPoints(){
		return impact_points;
	}

	inline void resetImpactPoints(){
		impact_points.clear();
	}

  private:
	Flag flag;
	ProcessedFlag processedFlag;

	Point removedVertexPos;
	VectorInt residual;

	vector<Point> impact_points;
};


struct MyItems : public CGAL::Polyhedron_items_3
{
    template <class Refs, class Traits>
    struct Face_wrapper {
        typedef MyFace<Refs> Face;
    };

	template <class Refs, class Traits>
    struct Vertex_wrapper {
        typedef MyVertex<Refs> Vertex;
    };

	template <class Refs, class Traits>
    struct Halfedge_wrapper {
        typedef MyHalfedge<Refs> Halfedge;
    };
};

// Operation list.
enum Operation {Idle = 0,
                DecimationConquest, RemovedVertexCoding, InsertedEdgeCoding, // Compression.
                UndecimationConquest, InsertedEdgeDecoding // Decompression.
                };

const static char *operation_str[6] = {"Idle","DecimationConquest", "RemovedVertexCoding", "InsertedEdgeCoding", // Compression.
        "UndecimationConquest", "InsertedEdgeDecoding"};

class MyMesh: public CGAL::Polyhedron_3< MyKernel, MyItems >
{

  typedef CGAL::Polyhedron_3< MyKernel, MyItems > PolyhedronT;
public:
	MyMesh(unsigned i_decompPercentage,
		   const int i_mode,
		   const char* data,
		   long length);

	~MyMesh();

	void stepOperation();
	void batchOperation();
	void completeOperation();

	Vector computeNormal(Facet_const_handle f) const;
	Vector computeVertexNormal(Halfedge_const_handle heh) const;

	Point barycenter(Facet_const_handle f) const;

	float getBBoxDiagonal() const;
	Vector getBBoxCenter() const;

	// General
	void computeBoundingBox();

	inline size_t count_triangle(Facet_const_iterator f){
		size_t size = 0;
		Halfedge_const_handle e1 = f->halfedge();
		Halfedge_const_handle e3 = f->halfedge()->next()->next();
		while(e3!=e1){
			size++;
			e3 = e3->next();
		}
		return size;
	}
	size_t true_triangle_size();

	// Compression
	void startNextCompresssionOp();
	void beginDecimationConquest();
	void beginInsertedEdgeCoding();
	void decimationStep();
	void RemovedVertexCodingStep();
	void InsertedEdgeCodingStep();
	Halfedge_handle vertexCut(Halfedge_handle startH);
	void determineResiduals();
	void encodeInsertedEdges(unsigned i_operationId);
	void encodeRemovedVertices(unsigned i_operationId);
	void lift();

	// Compression geometry and connectivity tests.
	bool isRemovable(Vertex_handle v) const;
	bool isConvex(const std::vector<Vertex_const_handle> & polygon) const;
	bool isPlanar(const std::vector<Vertex_const_handle> &polygon, float epsilon) const;
	bool willViolateManifold(const std::vector<Halfedge_const_handle> &polygon) const;
	float removalError(Vertex_const_handle v,
					   const std::vector<Vertex_const_handle> &polygon) const;

	// Decompression
	void startNextDecompresssionOp();
	void beginUndecimationConquest();
	void beginInsertedEdgeDecoding();
	void undecimationStep();
	void InsertedEdgeDecodingStep();
	void insertRemovedVertices();
	void removeInsertedEdges();
	void decodeGeometrySym(Face_handle fh);
	void beginRemovedVertexCodingConquest();
	void determineGeometrySym(Halfedge_handle heh_gate, Face_handle fh);

	// Utils
	Vector computeNormal(Halfedge_const_handle heh_gate) const;
	Vector computeNormal(const std::vector<Vertex_const_handle> & polygon) const;
	Vector computeNormal(Point p[3]) const;
	Point barycenter(Halfedge_handle heh_gate) const;
	Point barycenter(const std::vector<Vertex_const_handle> &polygon) const;
	unsigned vertexDegreeNotNew(Vertex_const_handle vh) const;
	float triangleSurface(const Point p[]) const;
	float edgeLen(Halfedge_const_handle heh) const;
	float facePerimeter(const Face_handle fh) const;
	float faceSurface(Halfedge_handle heh) const;
	void pushHehInit();

	// IOs
	void writeCompressedData();
	void readCompressedData();
	void writeFloat(float f);
	float readFloat();
	void writeInt16(int16_t i);
	int16_t readInt16();
	void writeInt(int i);
	int readInt();
	char readChar();
	void writeChar(char ch);

	void writeBaseMesh();
	void readBaseMesh();
	void writeMeshOff(const char psz_filePath[]) const;
	void writeCurrentOperationMesh(std::string pathPrefix, unsigned i_id) const;

	//3dpro
	void computeImpactedFactors();
	bool isProtruding(Vertex_const_handle v) const;
	bool isProtruding(const std::vector<Halfedge_const_handle> &polygon) const;
	void profileProtruding();

	// Variables.

	// Gate queues
	std::queue<Halfedge_handle> gateQueue;
	std::queue<Halfedge_handle> problematicGateQueue;

	// Processing mode: 0 for compression and 1 for decompression.
	int i_mode;
	bool b_jobCompleted; // True if the job has been completed.

	Operation operation;
	unsigned i_curDecimationId;
	unsigned i_nbDecimations;

	// The vertices of the edge that is the departure of the coding and decoding conquests.
	Vertex_handle vh_departureConquest[2];
	// Geometry symbol list.
	std::deque<std::deque<Point> > geometrySym;

	// Connectivity symbol list.
	std::deque<std::deque<unsigned> > connectFaceSym;
	std::deque<std::deque<unsigned> > connectEdgeSym;

	// Number of vertices removed during current conquest.
	unsigned i_nbRemovedVertices;

	Point bbMin;
	Point bbMax;
	float f_bbVolume;

	// Initial number of vertices and faces.
	size_t i_nbVerticesInit;
	size_t i_nbFacetsInit;

	// The compressed data;
	char *p_data;
	size_t dataOffset; // the offset to read and write.
	size_t d_capacity;

	unsigned i_decompPercentage;

	// Store the maximum size we cutted in each round of compression
	vector<float> maximumCut;
	float getmaximumCut(){
		return i_nbDecimations>i_curDecimationId?maximumCut[i_nbDecimations - i_curDecimationId - 1]:0;
	}
	int processCount = 0;
};

// get the Euclid distance of two points
inline float get_distance(const Point &p1, const Point &p2){
	float dist = 0;
	for(int i=0;i<3;i++){
		dist += (p2[i]-p1[i])*(p2[i]-p1[i]);
	}
	return dist;
}


#endif
