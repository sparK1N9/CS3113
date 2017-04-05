#pragma once
#include "SheetSprite.h"
#include "Vector.h"
#include <vector>

class Entity {
public:
	Entity();
	void Update(float elapsed);
	void Render(ShaderProgram *program);
	void toWorldCoord(std::vector<Vector> &worldCoords);
	void initVecs();
	SheetSprite sprite;
	float width;
	float height;
	float velocity_x;
	float velocity_y;
	float acceleration_x;
	float acceleration_y;
	float friction_x;
	float friction_y;
	Matrix matrix;
	Vector position;
	Vector scale;
	float rotation;
	std::vector<Vector> vecs;
};