#ifndef __tr_PRIMITIVE_H__
#define __tr_PRIMITIVE_H__

#include "Color.h"

#include "trDefs.h"

#include "MathGeoLib\MathGeoLib.h"
#include "Glew\include\GL\glew.h"


enum PrimitiveTypes
{
	Primitive_Point,
	Primitive_Arrow,
	Primitive_Plane,
	Primitive_Cube,
	Primitive_Sphere,
	Primitive_Cylinder,
	Primitive_Ray,
	Primitive_Frustum,
	Primitive_Grid,
	Primitive_Gizmo
};

class trPrimitive
{
public:

	trPrimitive();

	virtual void	Render() const;
	virtual void	InnerRender() const = 0;
	void			SetPos(float x, float y, float z);
	void			SetRotation(float angle, const math::float3 &u);
	void			Scale(float x, float y, float z);
	PrimitiveTypes	GetType() const;

public:

	Color color;
	math::float4x4 transform;
	bool axis = false;

protected:
	PrimitiveTypes type;

	uint indices_index = 0u;
	uint vertices_index = 0u;

	uint num_index = 0u;
	uint num_vertices = 0u;

	// TODO: implement this
	float* indices = nullptr;
	float* vertices = nullptr;
	float* normals = nullptr;
};

#endif // __PRIMITIVE_H__