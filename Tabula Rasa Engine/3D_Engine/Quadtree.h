#ifndef __QUAD_TREE_H__
#define __QUAD_TREE_H__

#include "MathGeoLib/MathGeoLib.h"

#include <list>
#include <map>

class GameObject;

class QuadtreeNode {

	// Constructor
public:
	QuadtreeNode(AABB limit);
	~QuadtreeNode();

	void Insert(GameObject* go);
	bool IsLeaf() const;
	void GenerateChilds();
	void RedistributeObjects();

	// Intersections stuff
	void CollectsGOs(const Frustum& frustum, std::vector<GameObject*>& go_output)const;
	void CollectIntersectingGOs(const LineSegment& line_segment, std::map<float, GameObject*>& intersect_map) const;

	bool FrustumContainsAaBox(const AABB & ref_box, const Frustum& frustum)const;


public:
	AABB box;
	QuadtreeNode* parent = nullptr;
	QuadtreeNode* childs[4];
	std::list<GameObject*> objects_inside;
};

class Quadtree {

	// Constructor
public:
	Quadtree();
	~Quadtree();

	// Operations
public:

	void Create(AABB limits);
	void Insert(GameObject* go);

	void FillWithAABBs(std::vector<AABB>& vector);
	void IterateToFillAABBs(QuadtreeNode* node, std::vector<AABB>& vector);

	// Intersection stuff
	void CollectsGOs(const Frustum& frustum, std::vector<GameObject*>& go_output) const;
	void CollectIntersectingGOs(const LineSegment& line_segment, std::map<float, GameObject*>& intersect_map) const;

	void Clear();


public:
	QuadtreeNode* root_node = nullptr;

};


#endif // __QUAD_TREE_H__