#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#include "stb_image.h"
#include "Entity.h"
#include "tileMap.h"
#include <vector>
using namespace std;

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif
// 60 FPS (1.0f/60.0f)
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

SDL_Window* displayWindow;
Matrix modelMatrix;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	program->setModelMatrix(modelMatrix);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(-worldY / TILE_SIZE);
}

void XCollision(Entity& entity, unsigned char** levelData, vector<int>& solid) {
	float right = entity.x + entity.width / 2;
	float left = entity.x - entity.width / 2;
	int x, y;
	worldToTileCoordinates(right, entity.y, &x, &y);
	if (find(solid.begin(), solid.end(), levelData[y][x]) != solid.end() && right > TILE_SIZE*x) {
		entity.x -= right - TILE_SIZE*x + 0.00000000001;
		entity.velocity_x = 0;
		entity.collidedRight = true;
	}
	worldToTileCoordinates(left, entity.y, &x, &y);
	if (find(solid.begin(), solid.end(), levelData[y][x]) != solid.end() && left < TILE_SIZE*x + TILE_SIZE) {
		entity.x += TILE_SIZE*x + TILE_SIZE - left + 0.00000000001;
		entity.velocity_x = 0;
		entity.collidedLeft = true;
	}
}

void YCollision(Entity& entity, unsigned char** levelData, vector<int>& solid) {
	float top = entity.y + entity.height / 2;
	float bottom = entity.y - entity.height / 2;
	int x, y;
	worldToTileCoordinates(entity.x, top, &x, &y);
	if (find(solid.begin(), solid.end(), levelData[y][x]) != solid.end() && top > (-TILE_SIZE * y) - TILE_SIZE) {
		entity.y -= top - (-TILE_SIZE * y) + TILE_SIZE + 0.00000000001;
		entity.velocity_y = 0;
		entity.collidedTop = true;
	}
	worldToTileCoordinates(entity.x, bottom, &x, &y);
	if (find(solid.begin(), solid.end(), levelData[y][x]) != solid.end() && bottom < -TILE_SIZE*y) {
		entity.y += -TILE_SIZE*y - bottom + 0.00000000001;
		entity.velocity_y = 0;
		entity.collidedBottom = true;
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Mirror Force", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	SDL_Event event;
	bool done = false;
	float lastFrameTicks = 0.0f;
	glViewport(0, 0, 1280, 720);
	Matrix projectionMatrix;
	Matrix viewMatrix;
	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	glUseProgram(program.programID);
	int gems = 0;
	GLuint fonts = LoadTexture("font2.png");
	GLuint tiles = LoadTexture("arne_sprites.png");
	TileMap map;
	map.player.sprite = SheetSprite(tiles, 0.0f, 80.0f, 16.0f, 16.0f, 256.0f, 128.0f, 0.2f);
	map.levelFile = "map.txt";
	map.buildLevel();
	vector<int> solids = { 3, 4, 5, 6, 7, 8, 20 };
	for (Entity& gem : map.gems) {
		gem.sprite = SheetSprite(tiles, 96.0f, 16.0f, 16.0f, 16.0f, 256.0f, 128.0f, 0.2f);
		YCollision(gem, map.levelData, solids);
	}
	while (!done) {
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_SPACE && map.player.collidedBottom) {
				map.player.velocity_y = 9.0f;
			}
			else if (event.type == SDL_KEYUP && (event.key.keysym.scancode == SDL_SCANCODE_LEFT || event.key.keysym.scancode == SDL_SCANCODE_RIGHT)) {
				map.player.acceleration_x = 0.0f;
			}
		}
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		float fixedElapsed = elapsed;
		if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
			fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
		}
		while (fixedElapsed >= FIXED_TIMESTEP) {
			fixedElapsed -= FIXED_TIMESTEP;
			map.player.Update(FIXED_TIMESTEP);
			YCollision(map.player, map.levelData, solids);
			XCollision(map.player, map.levelData, solids);
		}
		map.player.Update(fixedElapsed);
		YCollision(map.player, map.levelData, solids);
		XCollision(map.player, map.levelData, solids);
		if (keys[SDL_SCANCODE_RIGHT]) {
			map.player.acceleration_x = 5;
		}
		else if (keys[SDL_SCANCODE_LEFT]) {
			map.player.acceleration_x = -5;
		}
		glClearColor(0, 0.5f, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		program.setProjectionMatrix(projectionMatrix);
		viewMatrix.identity();
		viewMatrix.Translate(-map.player.x, -map.player.y, 0);
		program.setViewMatrix(viewMatrix);
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		map.draw(&program, tiles);
		map.player.Render(&program);
		for (Entity& gem : map.gems) {
			if (map.player.collidesWith(&gem)) {
				gem.y = 100;
				gems += 1;
			}
			gem.Render(&program);
		}
		viewMatrix.identity();
		program.setViewMatrix(viewMatrix);
		modelMatrix.identity();
		modelMatrix.Translate(-2.6f, 1.8f, 0);
		DrawText(&program, fonts, "Gems: " + to_string(gems), 0.2f, 0.0f);
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
