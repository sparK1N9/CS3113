#pragma once
#include "SheetSprite.h"

enum EntityType {
	ENTITY_PLAYER,
	ENTITY_GEM
};

class Entity {
public:
	Entity();
	void Update(float elapsed);
	void Render(ShaderProgram *program);
	bool collidesWith(Entity *entity);
	SheetSprite sprite;
	float x;
	float y;
	float width;
	float height;
	float velocity_x;
	float velocity_y;
	float acceleration_x;
	float acceleration_y;
	float friction_x;
	float friction_y;
	bool isStatic;
	EntityType entityType;
	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};