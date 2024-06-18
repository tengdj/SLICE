/*
 * himesh_hausdorff.cpp
 *
 *  Created on: Mar 8, 2024
 *      Author: teng
 */
#include <map>
#include "himesh.h"

using namespace std;

namespace tdbase{

float HiMesh::getHausdorffDistance(){
	assert(i_nbDecimations>=i_curDecimationId);
	return i_nbDecimations>i_curDecimationId?globalHausdorfDistance[i_curDecimationId].second:0;
}

float HiMesh::getProxyHausdorffDistance(){
	assert(i_nbDecimations>=i_curDecimationId);
	return i_nbDecimations>i_curDecimationId?globalHausdorfDistance[i_curDecimationId].first:0;
}

pair<float, float> HiMesh::collectGlobalHausdorff(STAT_TYPE type){
	pair<float, float> current_hausdorf = pair<float, float>(0.0, 0.0);
	// some collected statistics for presentation
	if(type == MIN){
		current_hausdorf.first = DBL_MAX;
		current_hausdorf.second = DBL_MAX;
	}
	for(HiMesh::Face_iterator fit = facets_begin(); fit!=facets_end(); ++fit){
		const float fit_hdist = fit->getHausdorff();
		const float fit_proxy_hdist = fit->getProxyHausdorff();

		if(type == MAX){
			current_hausdorf.second = max(current_hausdorf.second, fit_hdist);
			current_hausdorf.first = max(current_hausdorf.first, fit_proxy_hdist);
		}else if(type == MIN){
			current_hausdorf.second = min(current_hausdorf.second, fit_hdist);
			current_hausdorf.first = min(current_hausdorf.first, fit_proxy_hdist);
		}else if(type == AVG){
			current_hausdorf.second += fit_hdist;
			current_hausdorf.first += fit_proxy_hdist;
		}

	}
	if(type == AVG){
		current_hausdorf.second /= size_of_facets();
		current_hausdorf.first /= size_of_facets();
	}

	return current_hausdorf;
}


static float point_to_face_distance(const Point &p, const HiMesh::Face_iterator &fit){
	const float point[3] = {p.x(), p.y(), p.z()};
	HiMesh::Halfedge_const_handle hd = fit->halfedge();
	HiMesh::Halfedge_const_handle h = hd->next();
	float mindist = DBL_MAX;
	while(h->next()!=hd){
		Point p1 = hd->vertex()->point();
		Point p2 = h->vertex()->point();
		Point p3 = h->next()->vertex()->point();
		h = h->next();
		const float triangle[9] = {p1.x(), p1.y(), p1.z(),
								   p2.x(), p2.y(), p2.z(),
								   p3.x(), p3.y(), p3.z()};
		float dist = PointTriangleDist(point, triangle);
		mindist = min(mindist, dist);
	}
	return mindist;
}

uint32_t HiMesh::sampling_rate = 30;
Hausdorff_Computing_Type HiMesh::calculate_method = HCT_BVHTREE;
bool HiMesh::use_byte_coding = true;

void sample_points_triangle(const Triangle &tri, unordered_set<Point> &points, int num_points){
	const Point &p1 = tri[0];
	const Point &p2 = tri[1];
	const Point &p3 = tri[2];
	points.emplace(p1);
	points.emplace(p2);
	points.emplace(p3);
	if(num_points>3){
		assert(num_points>3);
		int dimx = sqrt(num_points-3);
		int dimy = dimx==0?0:(num_points-3+dimx-1)/dimx;

		Point v1 = p1;
		Point v2(p2.x()-p1.x(), p2.y()-p1.y(), p2.z()-p1.z());
		Point v3(p3.x()-p1.x(), p3.y()-p1.y(), p3.z()-p1.z());

		float step_x = 1.0/(dimx+1);
		float step_y = 1.0/(dimy+1);
		for(float u = 0;u<1;u += step_x){
			for(float v = 0;v<1-u;v += step_y){
				if(!((u==0&&v==0)||(u==1&&v==1))){
					points.emplace(Point(v1.x()+u*v2.x()+v*v3.x(), v1.y()+u*v2.y()+v*v3.y(), v1.z()+u*v2.z()+v*v3.z()));
				}
			}
		}
	}
}

void HiMesh::sample_points(const Triangle &tri, unordered_set<Point> &points, float area_unit){
	int num_points = triangle_area(tri)/area_unit;
	sample_points_triangle(tri, points, num_points);
}

void HiMesh::sample_points(const HiMesh::Face_iterator &fit, unordered_set<Point> &points, float area_unit){

	const auto hd = fit->halfedge();
	auto h = hd->next();
	while(h->next()!=hd){
		Point p1 = hd->vertex()->point();
		Point p2 = h->vertex()->point();
		Point p3 = h->next()->vertex()->point();
		Triangle tri(p1, p2, p3);
		int num_points = triangle_area(tri)/area_unit+1;
		int ori = points.size();
		sample_points_triangle(tri, points, num_points);
		h = h->next();
	}
}

void HiMesh::sample_points(unordered_set<Point> &points){
	const float area_unit = sampling_gap();

	for(HiMesh::Face_iterator fit = facets_begin(); fit!=facets_end(); ++fit){
		sample_points(fit, points, area_unit);
	}
	log("%.5f %.10f (%d*%d)=%d %d", area(), area_unit, size_of_triangles(), sampling_rate, size_of_triangles()*sampling_rate, points.size());
}

Triangle expand(Triangle &tri){
	const Point &p1 = tri[0];
	const Point &p2 = tri[1];
	const Point &p3 = tri[2];
	return Triangle(Point(2*p1.x()-p2.x()/2-p3.x()/2, 2*p1.y()-p2.y()/2-p3.y()/2, 2*p1.z()-p2.z()/2-p3.z()/2),
					Point(2*p2.x()-p1.x()/2-p3.x()/2, 2*p2.y()-p1.y()/2-p3.y()/2, 2*p2.z()-p1.z()/2-p3.z()/2),
					Point(2*p3.x()-p1.x()/2-p2.x()/2, 2*p3.y()-p1.y()/2-p2.y()/2, 2*p3.z()-p1.z()/2-p2.z()/2));
}

vector<Triangle> triangulate(HiMesh::Face_iterator fit){
	vector<Triangle> ret;
	const auto hd = fit->halfedge();
	auto h = hd->next();
	while(h->next()!=hd){
		Point p1 = hd->vertex()->point();
		Point p2 = h->vertex()->point();
		Point p3 = h->next()->vertex()->point();
		ret.push_back(Triangle(p1,p2,p3));
		h = h->next();
	}
	return ret;
}

float encode_triangle2(Triangle &tri){
	const float *t = (const float *)&tri;
	float ret = 0.0;
	for(int i=0;i<9;i++){
		ret += i*(*(t+i));
	}
	return ret;
}

// TODO: a critical function, need to be further optimized
void HiMesh::computeHausdorfDistance(){
	// do not compute
	if(calculate_method == HCT_NULL ){
		globalHausdorfDistance.push_back({0, 0});
		return;
	}

	float dist = DBL_MAX;

	struct timeval start = get_cur_time();

	double smp_tm = 0;
	double usetree_tm = 0;
	double collect_triangle_tm = 0;
	double caldist_tm = 0;
	double ph_caldist_tm = 0.0;

	const float area_unit = sampling_gap();

	// associate each compressed facet with a list of original triangles, vice versa
	for(HiMesh::Face_iterator fit = facets_begin(); fit!=facets_end(); ++fit){

		struct timeval start = get_cur_time();
		if(fit->rg==NULL || !fit->isSplittable()){
			continue;
		}
		fit->resetHausdorff();
		float fit_hdist = 0.0;
		fit->triangles = triangulate(fit);

		for(Triangle &cur_tri:fit->triangles) {
			float curhdist = 0.0;

			/* sampling the current triangle */
			unordered_set<Point> points;
			sample_points(cur_tri, points, area_unit);
			smp_tm += get_time_elapsed(start, true);

			// simply calculate against the BVH-tree built on the original mesh
			if(HiMesh::calculate_method == HCT_BVHTREE)
			{
				for(auto p:points){
					float dist = distance_tree(p);
					curhdist = max(curhdist, dist);
				}
				caldist_tm += get_time_elapsed(start, true);
			}

			// to collect the triangles associated
			// todo: this part of the latency is involved by an earlier
			// version of the implementation, will be removed in a future release
			// which will record all the removed facets instead of vertices
			vector<MyTriangle *> triangles;
			if(HiMesh::calculate_method != HCT_BVHTREE){
				for(const Point &p:fit->rg->removed_vertices){
					for(MyTriangle *t:VFmap[p]){
						if(!t->processed){
							t->processed = true;
							triangles.push_back(t);
						}
					}
				}
				for(const Point &p:fit->rg->removed_vertices){
					for(MyTriangle *t:VFmap[p]){
						t->processed = false;
					}
				}
				assert(triangles.size()>0);
				collect_triangle_tm += get_time_elapsed(start, true);
			}

			// use the triangles associated for calculating the Hasudorff distance
			if(HiMesh::calculate_method == HCT_ASSOCIATE)
			{
				// brute-forcely calculate
				for(auto p:points){
					float dist = DBL_MAX;
					for(MyTriangle *t:triangles){
						dist = min(dist, tdbase::PointTriangleDist((const float *)&p, (const float *)&t->tri));
					}
					curhdist = max(curhdist, dist);
				}
				caldist_tm += get_time_elapsed(start, true);
			}

			uint32_t original_num = triangles.size();
			// further filter with the Triangle Cylinder
			if(HiMesh::calculate_method == HCT_ASSOCIATE_CYLINDER){
				for(vector<MyTriangle *>::iterator iter = triangles.begin();iter!=triangles.end();){
					bool keep = false;
					for(const Point &p:(*iter)->sampled_points){
						if(tdbase::PointInTriangleCylinder((const float *)&p, (const float *)&cur_tri)){
							keep = true;
							break;
						}
					}
					if(!keep){
						triangles.erase(iter);
					}else{
						iter++;
					}
				}
				collect_triangle_tm += get_time_elapsed(start, true);
			}

			for(MyTriangle *t:triangles){
				t->add_facet(fit);
			}

			if(HiMesh::calculate_method == HCT_ASSOCIATE_CYLINDER) {
				// brute-forcely calculate
				for(auto p:points){
					float dist = DBL_MAX;
					for(MyTriangle *t:triangles){
						dist = min(dist, tdbase::PointTriangleDist((const float *)&p, (const float *)&t->tri));
					}
					if(triangles.size() == 0){
						dist = 0;
					}
					curhdist = max(curhdist, dist);
				}
				caldist_tm += get_time_elapsed(start, true);
			}

			//log("#triangles: %ld-%d hdist: %.3f-%.3f-%.3f", original_num, triangles.size(), curhdist[0], curhdist[1], curhdist[2]);
			// collect results
			fit_hdist = max(fit_hdist, curhdist);
			triangles.clear();
		}
		// update the hausdorff distance
		fit->setHausdorff(fit_hdist + sqrt(area_unit/2.0));
	}
	start = get_cur_time();
	/*
	 *
	 * for computing the proxy hausdorf distance
	 *
	 * */
	if(HiMesh::calculate_method == HCT_BVHTREE){
		list<Triangle> triangles = get_triangles();
		map<float, HiMesh::Face_iterator> fits = get_fits();
		TriangleTree *tree = new TriangleTree(triangles.begin(), triangles.end());
		tree->build();
		tree->accelerate_distance_queries();
		struct timeval ss = get_cur_time();
		for(MyTriangle *t:original_facets){
			// for each sampled point, find the closest facet to it
			// and it will be proxy facet of that point
			for(const Point &p:t->sampled_points){
				TriangleTree::Point_and_primitive_id ppid = tree->closest_point_and_primitive(p);
				float dist = tdbase::distance((const float *)&p, (const float *)&ppid.first);
				Triangle tri = *ppid.second;
				float fs = encode_triangle2(tri);
				assert(fits.find(fs)!=fits.end());
				fits[fs]->updateProxyHausdorff(sqrt(dist)+sqrt(area_unit/2.0));

//				assert(fits.find(*ppid.second) != fits.end());
//				fits[*ppid.second]->updateProxyHausdorff(sqrt(dist));
			}
		}
		//logt("BVH %f", ss, sqrt(phdist));
	}

	if(HiMesh::calculate_method == HCT_ASSOCIATE || HiMesh::calculate_method == HCT_ASSOCIATE_CYLINDER){
		for(MyTriangle *t:original_facets){
			//log("%d", t->facets.size());
			if(t->facets.size()>0){
				Face_iterator cur_fit;
				Point cur_p;
				// calculate the Hausdorf distance
				float hdist = 0.0;
				// for each sampled point, find the closest facet to it
				// and it will be proxy facet of that point
				for(const Point &p:t->sampled_points){
					float dist = DBL_MAX;
					Face_iterator fit;
					for(Face_iterator &ft:t->facets){
						for(Triangle &tt:ft->triangles){
							float d = tdbase::PointTriangleDist((const float *)&p, (const float *)&tt);
							if(d<dist){
								dist = d;
								fit = ft;
							}
						}
					}
					fit->updateProxyHausdorff(dist+sqrt(area_unit/2.0));
					// get the maximum
					if(hdist < dist){
						hdist = dist;
						cur_p = p;
						cur_fit = fit;
					}
				}
			}
			t->reset();
		}
	}
	ph_caldist_tm += get_time_elapsed(start, true);

	pair<float, float> current_hausdorf = collectGlobalHausdorff();

	globalHausdorfDistance.push_back(current_hausdorf);

	if(global_ctx.verbose>=3)
	{
		if(i_curDecimationId%2!=0){
			log("step: %2d smp: %.3f tri: %.3f h_cal: %.3f ph_cal:%.3f avg_hdist#vertices: %ld #facets: %ld",
					i_curDecimationId,
					smp_tm, collect_triangle_tm, caldist_tm,ph_caldist_tm,
					size_of_vertices(), size_of_triangles());
			log("encode %d:\t[%.2f %.2f]\t%ld", i_curDecimationId, current_hausdorf.first, current_hausdorf.second, size_of_vertices());

			printf("%2d\t%.3f\t%.3f\t%.3f\n",
					i_curDecimationId,
					caldist_tm,ph_caldist_tm,caldist_tm+ph_caldist_tm);

			printf("%.3f\t%.3f\t",
					caldist_tm,ph_caldist_tm);
		}
	}
}

// TODO: a critical function, need to be further optimized
pair<float, float> HiMesh::computeHausdorfDistance(HiMesh *original){

	float dist = DBL_MAX;
	const float area_unit = sampling_gap();

	// associate each compressed facet with a list of original triangles, vice versa
	for(HiMesh::Face_iterator fit = facets_begin(); fit!=facets_end(); ++fit){

		struct timeval start = get_cur_time();

		fit->resetHausdorff();
		float fit_hdist = 0.0;
		fit->triangles = triangulate(fit);

		for(Triangle &cur_tri:fit->triangles) {
			float curhdist = 0.0;

			/* sampling the current triangle */
			unordered_set<Point> points;
			sample_points(cur_tri, points, area_unit);

			// simply calculate against the BVH-tree built on the original mesh
			for(auto p:points){
				float dist = original->distance_tree(p);
				curhdist = max(curhdist, dist);
			}
			//log("#triangles: %ld-%d hdist: %.3f-%.3f-%.3f", original_num, triangles.size(), curhdist[0], curhdist[1], curhdist[2]);
			// collect results
			fit_hdist = max(fit_hdist, curhdist);
		}
		// update the hausdorff distance
		fit->setHausdorff(fit_hdist + sqrt(area_unit/2.0));
	}

	/*
	 *
	 * for computing the proxy hausdorf distance
	 *
	 * */
	list<Triangle> triangles = get_triangles();
	map<float, HiMesh::Face_iterator> fits = get_fits();
	TriangleTree *tree = new TriangleTree(triangles.begin(), triangles.end());
	tree->build();
	tree->accelerate_distance_queries();
	struct timeval ss = get_cur_time();

	for(MyTriangle *t:original->original_facets){
		// for each sampled point, find the closest facet to it
		// and it will be proxy facet of that point
		for(const Point &p:t->sampled_points){

			TriangleTree::Point_and_primitive_id ppid = tree->closest_point_and_primitive(p);
			float dist = tdbase::distance((const float *)&p, (const float *)&ppid.first);
			Triangle tri = *ppid.second;
			float fs = encode_triangle2(tri);
			assert(fits.find(fs)!=fits.end());
			fits[fs]->updateProxyHausdorff(sqrt(dist)+sqrt(area_unit/2.0));
		}
	}

	return collectGlobalHausdorff();
	//logt("BVH %f", ss, sqrt(phdist));
}


}


