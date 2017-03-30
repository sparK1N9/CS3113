#include "Entity.h"

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}

Entity::Entity() :velocity_x(0), velocity_y(0), width(0.2f), height(0.2f), friction_x(3), friction_y(3) {}

void Entity::Update(float elapsed) {
	collidedTop = false;
	collidedBottom = false;
	collidedLeft = false;
	collidedRight = false;
	velocity_x = lerp(velocity_x, 0.0f, elapsed * friction_x);
	velocity_y = lerp(velocity_y, 0.0f, elapsed * friction_y);
	velocity_x += acceleration_x * elapsed;
	velocity_y += -10 * elapsed;
	x += velocity_x * elapsed;
	y += velocity_y * elapsed;
}

void Entity::Render(ShaderProgram *program) {
	Matrix modelMatrix;
	modelMatrix.Translate(x, y, 0);
	program->setModelMatrix(modelMatrix);
	sprite.Draw(program);
}

bool Entity::collidesWith(Entity *entity) {
	return !(x + width / 2 <= entity->x - entity->width / 2 || x - width / 2 >= entity->x + entity->width / 2 || y + height / 2 <= entity->y - entity->height / 2 || y - height / 2 >= entity->y + entity->height / 2);
}