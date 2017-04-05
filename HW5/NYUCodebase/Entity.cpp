#include "Entity.h"

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}

Entity::Entity() :velocity_x(0), velocity_y(0), acceleration_x(0), acceleration_y(0), friction_x(3), friction_y(3), rotation(0) {}

void Entity::Update(float elapsed) {
	velocity_x = lerp(velocity_x, 0.0f, elapsed * friction_x);
	velocity_y = lerp(velocity_y, 0.0f, elapsed * friction_y);
	velocity_x += acceleration_x * elapsed;
	velocity_y += acceleration_y * elapsed;
	matrix.Translate(velocity_x * elapsed, velocity_y * elapsed, 0);
	matrix.Rotate(rotation);
}

void Entity::Render(ShaderProgram *program) {
	program->setModelMatrix(matrix);
	sprite.Draw(program);
}

void Entity::toWorldCoord(std::vector<Vector> &worldCoords) {
	worldCoords.resize(vecs.size());
	for (int i = 0; i < vecs.size(); i++) {
		float x = vecs[i].x;
		float y = vecs[i].y;
		worldCoords[i].x = matrix.m[0][0] * x + matrix.m[1][0] * y + matrix.m[3][0];
		worldCoords[i].y = matrix.m[0][1] * x + matrix.m[1][1] * y + matrix.m[3][1];
	}
}

void Entity::initVecs() {
	Vector v1(-0.5f * sprite.size * sprite.aspect, -0.5f * sprite.size);
	Vector v3(-0.5f * sprite.size * sprite.aspect, 0.5f * sprite.size);
	Vector v2(0.5f * sprite.size * sprite.aspect, 0.5f * sprite.size);
	Vector v4(0.5f * sprite.size * sprite.aspect, -0.5f * sprite.size);
	vecs.push_back(v1);
	vecs.push_back(v3);
	vecs.push_back(v2);
	vecs.push_back(v4);
}