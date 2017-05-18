#include <cassert>
#include <algorithm>
#include <memory>
#include <cmath>
#include <vector>
#include <array>
#include "linalg/ray.h"
#include "linalg/util.h"
#include "geometry/geometry.h"
#include "geometry/bbox.h"
#include "accelerators/bvh.h"

namespace pt {

BVH::GeomInfo::GeomInfo(size_t i, const BBox &b) : geom_idx(i), center(b.lerp(0.5, 0.5, 0.5)), bounds(b){}

BVH::BuildNode::BuildNode(size_t geom_offset, size_t ngeom, const BBox &bounds)
	: children({nullptr, nullptr}), geom_offset(geom_offset), ngeom(ngeom), bounds(bounds)
{}
BVH::BuildNode::BuildNode(AXIS split, std::unique_ptr<BuildNode> a, std::unique_ptr<BuildNode> b)
	: children({std::move(a), std::move(b)}), geom_offset(0), ngeom(0),
	bounds(children[0]->bounds.box_union(children[1]->bounds)), split(split)
{}

// Bucket used for SAH split method
struct SAHBucket {
	size_t count;
	BBox bounds;

	SAHBucket() : count(0) {}
};

BVH::BVH(const std::vector<Geometry*> &geom, unsigned max_geom)
	: max_geom(std::min(256u, max_geom))
{
	for (Geometry *g : geom){
		g->refine(geometry);
	}
	//If we didn't get any geometry to partition then we don't have to do anything
	if (geometry.empty()){
		return;
	}
	//Get bounds and index info together for the geometry we're storing
	std::vector<GeomInfo> build_geom;
	build_geom.reserve(geometry.size());
	for (size_t i = 0; i < geometry.size(); ++i){
		build_geom.emplace_back(i, geometry[i]->bound());
	}

	std::vector<Geometry*> ordered_geom;
	ordered_geom.reserve(geometry.size());
	size_t total_nodes = 0;
	std::unique_ptr<BuildNode> root = build(build_geom, ordered_geom, 0, geometry.size(), total_nodes);
	//Our BVH structure refers to the ordered geometry so swap out our unordered list for
	//the correctly ordered one
	geometry.swap(ordered_geom);

	//Recursively flatten the tree for faster traversal
	flat_nodes.resize(total_nodes);
	size_t offset = 0;
	flatten_tree(root, offset);
}
BBox BVH::bounds() const {
	return !flat_nodes.empty() ? flat_nodes[0].bounds : BBox{};
}
bool BVH::intersect(Ray &r, DifferentialGeometry &diff_geom) const {
	if (flat_nodes.empty()){
		return false;
	}
	bool hit = false;
	Vector inv_dir{1 / r.d.x, 1 / r.d.y, 1 / r.d.z};
	std::array<int, 3> neg_dir = {inv_dir.x < 0, inv_dir.y < 0, inv_dir.z < 0};
	//Stack of nodes to be visited and the current node being visited. todo_offset is the top of stack
	// TODO: Replace with Afra and Szirmay-Kalos stackless bitstring traversal
	// make possible to get this string out so we can traverse regions, not necessarily
	// geometry we're interesting. On a local node we'd use Embree, at the data distribution
	// level (and maybe 1 or 2 down to local data) we'd use this BVH
	std::array<int, 64> todo;
	int todo_offset = 0, current = 0;
	//Step through the BVH visiting the current node and pushing on nodes that need to be visited
	while (true){
		const FlatNode &fnode = flat_nodes[current];
		//Check if we hit this node, fast_box_interesect is a faster specialized intersection
		//for this traversal
		if (fast_box_intersect(fnode.bounds, r, inv_dir, neg_dir)){
			//If it's a leaf node check the geometry
			if (fnode.ngeom > 0){
				for (uint32_t i = 0; i < fnode.ngeom; ++i){
					if (geometry[fnode.geom_offset + i]->intersect(r, diff_geom)){
						hit = true;
					}
				}
				if (todo_offset == 0){
					break;
				}
				current = todo[--todo_offset];
			}
			//If it's an interior node we move to the near one and push the far one on the stack
			else {
				//Figure out which node is further along the ray and push it onto the stack
				//and traverse the nearer one
				if (neg_dir[fnode.axis]){
					todo[todo_offset++] = current + 1;
					current = fnode.second_child;
				}
				else {
					todo[todo_offset++] = fnode.second_child;
					++current;
				}
			}
		}
		else {
			//If we've checked everything on our list we're done
			if (todo_offset == 0){
				break;
			}
			current = todo[--todo_offset];
		}
	}
	return hit;
}
std::unique_ptr<BVH::BuildNode> BVH::build(std::vector<GeomInfo> &build_geom,
		std::vector<Geometry*> &ordered_geom, size_t start, size_t end, size_t &total_nodes)
{
	++total_nodes;
	//Find total bounds for the geometry we're trying to store
	BBox box;
	for (int i = start; i < end; ++i){
		box = box.box_union(build_geom[i].bounds);
	}
	const size_t ngeom = end - start;
	//Build and return a leaf node for the geometry
	if (ngeom == 1){
		return build_leaf(build_geom, ordered_geom, start, end, box);
	}
	//Need to build an interior node
	//Figure out which axis to split on by bounding the various geometry's centroids
	//and picking the one with the most variation
	BBox centroids;
	for (size_t i = start; i < end; ++i){
		centroids = centroids.box_union(build_geom[i].center);
	}
	AXIS axis = centroids.max_extent();
	//We can now partition the primitives along this axis
	size_t mid = (start + end) / 2;
	//If all the geometry's centers are on the same point we can't partition
	if (centroids.max[axis] == centroids.min[axis]){
		//Check that we can fit all the geometry into a single leaf node, if not we need
		//to force a split
		if (ngeom < max_geom){
			return build_leaf(build_geom, ordered_geom, start, end, box);
		}
		else {
			return std::make_unique<BuildNode>(axis,
				build(build_geom, ordered_geom, start, mid, total_nodes),
				build(build_geom, ordered_geom, mid, end, total_nodes));
		}
	}
	// Partition the primitives based on the surface area heuristic
	// If there's only a few primitives just use EQUAL and break
	if (ngeom < 5){
		mid = (start + end) / 2;
		std::nth_element(build_geom.begin() + start, build_geom.begin() + mid,
				build_geom.begin() + end,
				[axis](const GeomInfo &a, const GeomInfo &b){
				return a.center[axis] < b.center[axis];
				});
		break;
	}
	//We're just going to consider the possibilities of splitting along 12 bucket boundaries
	std::array<SAHBucket, 12> buckets;
	//Place the various geometry we're partitioning into the appropriate bucket, scale
	//the position along the axis into a bucket index
	for (size_t i = start; i < end; ++i){
		size_t b = (build_geom[i].center[axis] - centroids.min[axis])
			/ (centroids.max[axis] - centroids.min[axis]) * buckets.size();
		b = b == buckets.size() ? b - 1 : b;
		++buckets[b].count;
		buckets[b].bounds = buckets[b].bounds.box_union(build_geom[i].bounds);
	}
	//Use the SAH to compute the costs of splitting at each bucket except the last
	std::array<float, 11> cost;
	for (size_t i = 0; i < cost.size(); ++i){
		SAHBucket left, right;
		for (size_t j = 0; j <= i; ++j){
			left.bounds = left.bounds.box_union(buckets[j].bounds);
			left.count += buckets[j].count;
		}
		for (size_t j = i + 1; j < buckets.size(); ++j){
			right.bounds = right.bounds.box_union(buckets[j].bounds);
			right.count += buckets[j].count;
		}
		//cost: cost of traversel + (cost of hitting in left + cost of hitting in right) / total area of node
		cost[i] = .125f + (left.count * left.bounds.surface_area()
				+ right.count * right.bounds.surface_area()) / box.surface_area();
	}
	//Find the lowest cost split we can make here
	const auto min_cost = std::min_element(cost.begin(), cost.end());
	const size_t min_cost_idx = std::distance(cost.begin(), min_cost);
	//If we're forced to split by the amount of geometry here or it's cheaper to split then do so
	if (ngeom > max_geom || *min_cost < ngeom){
		//Partition the geometry about the splitting bucket
		auto mid_ptr = std::partition(build_geom.begin() + start, build_geom.begin() + end,
				[min_cost_idx, axis, centroids](const GeomInfo &g){
				size_t b = (g.center[axis] - centroids.min[axis])
				/ (centroids.max[axis] - centroids.min[axis]) * 12;
				b = b == 12 ? b - 1 : b;
				return b <= min_cost_idx;
				});
		mid = std::distance(build_geom.begin(), mid_ptr);
	} else {
		return build_leaf(build_geom, ordered_geom, start, end, box);
	}

	assert(start != mid && mid != end);
	return std::make_unique<BuildNode>(axis,
		build(build_geom, ordered_geom, start, mid, total_nodes),
		build(build_geom, ordered_geom, mid, end, total_nodes));
}
std::unique_ptr<BVH::BuildNode> BVH::build_leaf(std::vector<GeomInfo> &build_geom,
		std::vector<Geometry*> &ordered_geom, size_t start, size_t end, const BBox &box)
{
	const size_t ngeom = end - start;
	// Store the offset to this leaf's geometry then push it on
	const size_t geom_offset = ordered_geom.size();
	for (size_t i = start; i < end; ++i){
		ordered_geom.push_back(geometry[build_geom[i].geom_idx]);
	}
	return std::make_unique<BuildNode>(geom_offset, ngeom, box);
}
size_t BVH::flatten_tree(const std::unique_ptr<BuildNode> &node, size_t &offset){
	FlatNode &fnode = flat_nodes[offset];
	fnode.bounds = node->bounds;
	const size_t node_offset = offset++;
	//If the node has geometry we're creating a leaf
	if (node->ngeom > 0){
		fnode.geom_offset = node->geom_offset;
		fnode.ngeom = node->ngeom;
	}
	//Otherwise we're making an interior node
	else {
		fnode.axis = node->split;
		fnode.ngeom = 0;
		flatten_tree(node->children[0], offset);
		fnode.second_child = flatten_tree(node->children[1], offset);
	}
	return node_offset;
}

}

