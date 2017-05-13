#include "Entity.h"

float lerp(float v0, float v1, float t) {
	return (1 - t)*v0 + t*v1;
}

Entity::Entity() :timer(0), velocity_x(0), velocity_y(0), acceleration_x(0), acceleration_y(0), friction_x(3), friction_y(3), rotation(0), angle(0) {}

void Entity::Update(float elapsed) {
	velocity_x = lerp(velocity_x, 0.0f, elapsed * friction_x);
	velocity_y = lerp(velocity_y, 0.0f, elapsed * friction_y);
	velocity_x += acceleration_x * elapsed;
	velocity_y += acceleration_y * elapsed;
	matrix.Translate(velocity_x * elapsed, velocity_y * elapsed, 0);
	matrix.Rotate(rotation);
	angle += rotation;
	if (angle > 2 * PI) {
		angle -= 2 * PI;
	}
	if (angle < 0) {
		angle += 2 * PI;
	}
	toWorldCoord();
}

void Entity::Render(ShaderProgram *program) {
	program->setModelMatrix(matrix);
	sprite.Draw(program);
}

void Entity::toWorldCoord() {
	worldCoords.resize(vecs.size());
	for (int i = 0; i < vecs.size(); i++) {
		float x = vecs[i].x;
		float y = vecs[i].y;
		worldCoords[i].x = matrix.m[0][0] * x + matrix.m[1][0] * y + matrix.m[3][0];
		worldCoords[i].y = matrix.m[0][1] * x + matrix.m[1][1] * y + matrix.m[3][1];
	}
}

void Entity::initVecs(int n) {
	Vector v1(-0.5f * sprite.size * sprite.aspect, -0.5f * sprite.size);
	Vector v4(0.5f * sprite.size * sprite.aspect, -0.5f * sprite.size);
	vecs.push_back(v1);
	if (n == 4) {
		Vector v2(-0.5f * sprite.size * sprite.aspect, 0.5f * sprite.size);
		Vector v3(0.5f * sprite.size * sprite.aspect, 0.5f * sprite.size);
		vecs.push_back(v2);
		vecs.push_back(v3);
	}
	else if (n == 3) {
		Vector v5(0, 0.5f * sprite.size);
		vecs.push_back(v5);
	}
	vecs.push_back(v4);
}

void Entity::move(float x, float y) {
	Matrix m = matrix.inverse();
	float x1 = m.m[0][0] * x + m.m[1][0] * y;
	float y1 = m.m[0][1] * x + m.m[1][1] * y;
	matrix.Translate(x1, y1, 0);
}