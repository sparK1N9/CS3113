#pragma once

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "ShaderProgram.h"
#include "Entity.h"
using namespace std;

#define SPRITE_COUNT_X 16
#define SPRITE_COUNT_Y 8
#define TILE_SIZE 0.2f
#define LEVEL_WIDTH 128
#define LEVEL_HEIGHT 32

class TileMap {
public:
	void buildLevel();
	void readLevelFile();
	bool readHeader(ifstream &stream);
	bool readLayerData(ifstream &stream);
	bool readEntityData(ifstream &stream);
	void placeEntity(string &type, float x, float y);
	void draw(ShaderProgram* program, int texture);
	int mapWidth;
	int mapHeight;
	string levelFile;
	unsigned char** levelData;
	vector<float> vertexData;
	vector<float> texCoordData;
	Entity player;
	vector<Entity> gems;
};
