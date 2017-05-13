#pragma once
#include "SheetSprite.h"
#include "Vector.h"
#include <vector>
#define PI 3.1415926f

class Entity {
public:
	Entity();
	void Update(float elapsed);
	void Render(ShaderProgram *program);
	void toWorldCoord();
	void initVecs(int n);
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
	Vector scale;
	float rotation;
	float angle;
	std::vector<Vector> vecs;
	float timer;
	std::vector<Vector> worldCoords;
	void move(float x, float y);
};

float lerp(float v0, float v1, float t);