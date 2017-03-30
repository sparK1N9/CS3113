#pragma once
#include "ShaderProgram.h"

class SheetSprite {
public:
	SheetSprite() {}
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float sheetWidth, float sheetHeight, float size) : size(size), textureID(textureID), u(u / sheetWidth), v(v / sheetHeight), width(width / sheetWidth), height(height / sheetHeight), aspect(width / height) {}
	void Draw(ShaderProgram *program);
	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
	float aspect;
};