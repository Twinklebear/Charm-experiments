#pragma once

#include <memory>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include "ray.h"
#include "bbox.h"
#include "geometry.h"
#include "diff_geom.h"

// TODO: Rewrite to use Afra & Szirmay-Kalos's stackless MBVH2 traversal
// and clean up code to integrate into libpt

namespace pt {

/* A BVH2 built off of the one introduced in PBR and
 * modified to use Afta and Szirmay-Kalos's stackless traversal
 * algorithm.
 */
class BVH {
	// Information about some geometry being place in the BVH
	struct GeomInfo {
		// The index of the geometry in the BVH's geometry vector
		size_t geom_idx;
		// Center of the geometry's bounding box
		glm::vec3 center;
		BBox bounds;

		GeomInfo(size_t i, const BBox &b);
	};

	// Node used when constructing the BVH tree
	struct BuildNode {
		// Node's children, null if a leaf node. TODO: Also track sibling and parent
		std::array<std::unique_ptr<BuildNode>, 2> children;
		/* Offset to the first geometry in this node and # of geometry in it
		 * ngeom = 0 if this is an interior node
		 */
		size_t geom_offset, ngeom;
		BBox bounds;
		// Which axis was split to form this node's children, 0 = X, 1 = Y, 2 = Z
		int split;

		/* Construct a leaf node
		 */
		BuildNode(size_t geom_offset, size_t ngeom, const BBox &bounds);
		/* Construct an interior node, where a & b are the node's children
		 */
		BuildNode(int split, std::unique_ptr<BuildNode> a, std::unique_ptr<BuildNode> b);
	};

	/* Nodes used to store the final flattened BVH structure, the first child
	 * is right after the parent but the second child is at some offset further down
	 */
	struct FlatNode {
		BBox bounds;
		// TODO: Store sibling and parent info as well
		union {
			// Used for leaves to locate geometry
			size_t geom_offset;
			// Used for interiors to locate second child
			size_t second_child;
		};
		// Number of geometry stored in this node, 0 if interior
		uint16_t ngeom;
		uint16_t axis;
	};

	// Max amount of geometry per leaf node
	size_t max_geom;
	// The geometry being stored in this BVH
	std::vector<Geometry*> geometry;
	// The final flatted BVH structure
	std::vector<FlatNode> flat_nodes;

public:
	/* Construct the BVH to create a hierarchy of the refined geometry passed in
	 * using the desird split method. max_geom specifies the maximum geometry that
	 * can be stored per node, default is 128, max is 256
	 * The defaults for the empty constructor will build an empty BVH
	 */
	BVH(const std::vector<Geometry*> &geom = std::vector<Geometry*>{},
		size_t max_geom = 128);
	// Get the bounds for the BVH
	BBox bounds() const;
	// Perform an intersection test on the geometry stored in the BVH
	bool intersect(Ray &ray, DifferentialGeometry &diff_geom) const;

private:
	/* Construct a subtree of the BVH for the build_geom from [start, end)
	 * and return the root of this subtree. The geometry is placed in the child
	 * nodes is partitioned into ordered geom for easier look up later
	 * Also returns the total nodes in this subtree, for use later when flattening
	 * the BVH
	 */
	std::unique_ptr<BuildNode> build(std::vector<GeomInfo> &build_geom, std::vector<Geometry*> &ordered_geom,
		size_t start, size_t end, size_t &total_nodes);
	/* Build a leaf node in the tree using the geometry passed and push the ordered geometry for the
	 * leaf into ordered_geom
	 */
	std::unique_ptr<BuildNode> build_leaf(std::vector<GeomInfo> &build_geom, std::vector<Geometry*> &ordered_geom,
		size_t start, size_t end, const BBox &box);
	/* Recursively flatten the BVH tree into the flat nodes vector
	 * offset tracks the current offset into the flat nodes vector
	 */
	size_t flatten_tree(const std::unique_ptr<BuildNode> &node, size_t &offset);
};

}

