#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#include <algorithm>
#include <random>
using namespace std;
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

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

class Vector3 {
public:
	Vector3() {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
	float x;
	float y;
	float z;
};

class SheetSprite {
public:
	SheetSprite() {}
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size) : size(size), textureID(textureID), u(u), v(v), width(width), height(height) {}
	void Draw(ShaderProgram *program);
	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

void SheetSprite::Draw(ShaderProgram *program) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};
	float aspect = width / height;
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size ,
		0.5f * size * aspect, -0.5f * size };
	program->setModelMatrix(modelMatrix);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

class Entity {
public:
	Vector3 position;
	Vector3 velocity;
	SheetSprite sprite;
	int points;
	bool lastRow;
	void Draw(ShaderProgram *program) {
		modelMatrix.identity();
		modelMatrix.Translate(position.x, position.y, 0);
		sprite.Draw(program);
	}
};

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

void ProcessMainMenu(ShaderProgram *program, int fonts) {
	modelMatrix.identity();
	modelMatrix.Translate(-2.6f, 0.5f, 0);
	DrawText(program, fonts, "Neo New Space Invaders", 0.25f, 0.0f);
	modelMatrix.identity();
	modelMatrix.Translate(-1.6f, -1.0f, 0);
	DrawText(program, fonts, "Click anywhere to begin", 0.15f, 0.0f);
}

void reset(Entity &player, Entity &playerBullet, vector<Entity> &enemies, int &shootGap, float &a, float &b, float &ev) {
	a = 0;
	b = 10;
	ev = -0.25f;
	player.position = Vector3(-3.1f, -1.5f, 0);
	playerBullet.position = Vector3(0, 3, 0);
	shootGap = 30;
	int x = 0;
	float y = 0;
	for (int i = 0; i < 55; i++) {
		if (i % 11 == 0 && i != 0) {
			y += 0.4f;
			x = 0;
		}
		enemies[i].position = Vector3(-2.5f + x * 0.5f, 1.4f - y, 0);
		x++;
		enemies[i].lastRow = (i >= 44) ? true : false;
	}
}

void enemyShoot(vector<Entity> &enBullets, Entity &enemy, GLuint items) {
	Entity enBullet;
	enBullet.position.x = enemy.position.x;
	enBullet.position.y = enemy.position.y - 0.25f;
	enBullet.sprite = SheetSprite(items, 856.0f / 1024.0f, 421.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, -0.25f);
	enBullet.velocity = Vector3(0, -3, 0);
	enBullets.push_back(enBullet);
}

bool removable(Entity &bullet) { return (bullet.position.x < -2.2f); }

bool hit(Entity &a, Entity &b) {
	float aAspect = a.sprite.width / a.sprite.height;
	float bAspect = b.sprite.width / b.sprite.height;
	float aWidth = aAspect * a.sprite.size / 2;
	float aHeight = a.sprite.size / 2;
	float bWidth = bAspect * b.sprite.size / 2;
	float bHeight = b.sprite.size / 2;
	float ax = a.position.x;
	float ay = a.position.y;
	float bx = b.position.x;
	float by = b.position.y;
	return ((ax + aWidth) > (bx - bWidth) && (ax - aWidth) < (bx + bWidth) && (ay + aHeight) > (by - bHeight) && (ay - aHeight) < (by + bHeight));
}

bool columnEmpty(vector<Entity> &enemies, int index) {
	return !(enemies[index].lastRow || enemies[index+11].lastRow || enemies[index+22].lastRow || enemies[index+33].lastRow || enemies[index+44].lastRow);
}

int main(int argc, char *argv[])
{
	// initalization
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif
	float lastFrameTicks = 0.0f;
	enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL };
	int state = STATE_MAIN_MENU;
	SDL_Event event;
	bool done = false;
	glViewport(0, 0, 1280, 720);
	Matrix projectionMatrix;
	Matrix viewMatrix;
	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	glUseProgram(program.programID);
	GLuint fonts = LoadTexture("font2.png");
	GLuint bg = LoadTexture("blue.png");
	GLuint items = LoadTexture("sheet.png");
	GLuint base1 = LoadTexture("base.png"); 
	Entity base;
	base.sprite = SheetSprite(base1, 0.0f / 1024.0f, 0.0f / 1024.0f, 9.0f / 1024.0f, .05f / 1024.0f, 0.1f);
	base.position = Vector3(-3.55f, -1.75f, 0);
	Entity player;
	player.sprite = SheetSprite(items, 224.0f / 1024.0f, 832.0f / 1024.0f, 99.0f / 1024.0f, 75.0f / 1024.0f, 0.35f);
	player.velocity = Vector3(2, 0, 0);
	Entity playerBullet;
	playerBullet.sprite = SheetSprite(items, 858.0f / 1024.0f, 230.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, 0.25f);
	playerBullet.velocity = Vector3(0, 3, 0);
	vector <Entity> enBullets;
	vector<Entity> enemies;
	for (int i = 0; i < 11; i++) {
		Entity enemy1;
		enemy1.sprite = SheetSprite(items, 518.0f / 1024.0f, 325.0f / 1024.0f, 82.0f / 1024.0f, 84.0f / 1024.0f, 0.3f);
		enemy1.points = 30;
		enemies.push_back(enemy1);
	}
	for (int i = 0; i < 22; i++) {
		Entity enemy2;
		enemy2.sprite = SheetSprite(items, 518.0f / 1024.0f, 409.0f / 1024.0f, 82.0f / 1024.0f, 84.0f / 1024.0f, 0.3f);
		enemy2.points = 20;
		enemies.push_back(enemy2);
	}
	for (int i = 0; i < 22; i++) {
		Entity enemy3;
		enemy3.sprite = SheetSprite(items, 518.0f / 1024.0f, 493.0f / 1024.0f, 82.0f / 1024.0f, 84.0f / 1024.0f, 0.3f);
		enemy3.points = 10;
		enemies.push_back(enemy3);
	}
	int lives = 6;
	float ev;
	int shootGap;
	float a, b;
	float c = 0.0f;
	bool flag = true;
	bool loseFlag = false;
	int score = 0;

	// main game loop
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN && state == STATE_MAIN_MENU && event.button.button == 1) {
				// click anywhere to start game
				state = STATE_GAME_LEVEL;
				loseFlag = false;
				lives = 6;
				score = 0;
				reset(player, playerBullet, enemies, shootGap, a, b, ev);
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN && state == STATE_GAME_LEVEL && event.button.button == 1 && loseFlag) {
				// after game is over, click to return to main menu
				state = STATE_MAIN_MENU;
			}
		}
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		glClear(GL_COLOR_BUFFER_BIT);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		SheetSprite background(bg, 0, 0, 1, 1, 8);
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);
		background.Draw(&program);
		switch (state) {
		case STATE_MAIN_MENU:
			ProcessMainMenu(&program, fonts);
			break;
		case STATE_GAME_LEVEL:
			if (loseFlag) {
				modelMatrix.identity();
				modelMatrix.Translate(-2.6f, 1.8f, 0);
				DrawText(&program, fonts, "Score: " + to_string(score), 0.2f, 0.0f);
				modelMatrix.identity();
				modelMatrix.Translate(-2.6f, 1.4f, 0);
				DrawText(&program, fonts, "Game Over!", 0.2f, 0.0f);
				modelMatrix.identity();
				modelMatrix.Translate(-2.6f, 1.0f, 0);
				DrawText(&program, fonts, "Click to return to main menu.", 0.2f, 0.0f);
			}
			else {
				if (columnEmpty(enemies, a) && a < 11) {
					a++;
				}
				if (columnEmpty(enemies, b) && b >= 0) {
					b--;
				}
				const Uint8 *keys = SDL_GetKeyboardState(NULL);// left and right arrow to move, space to shoot
				if (keys[SDL_SCANCODE_LEFT] && player.position.x > -3.2) {
					player.position.x -= player.velocity.x * elapsed;
				}
				else if (keys[SDL_SCANCODE_RIGHT] && player.position.x < 3.2) {
					player.position.x += player.velocity.x * elapsed;
				}
				if (keys[SDL_SCANCODE_SPACE] && playerBullet.position.y > 2.2) {
					playerBullet.position.x = player.position.x;
					playerBullet.position.y = player.position.y + 0.25f;
				}
				playerBullet.position.y += playerBullet.velocity.y * elapsed;
				if (enemies[0].position.x<-3.35 - a * 0.5f || enemies[10].position.x>3.35 + (10.0f-b) * 0.5f) {
					ev *= -1;
					if (ev > 0) ev += 0.1f;
					else ev -= 0.1f;
				}
				float x1 = enemies[0].position.x;
				float x2 = enemies[10].position.x;
				if ((x1 < -3.35 - a * 0.5f || x2 > 3.35 + (10.0f - b) * 0.5f) && shootGap > 10) shootGap--;
				for (int i = 0; i < 55; i++) {
					if (x1 < -3.35 - a * 0.5f) {
						enemies[i].position.x += -3.35f - a * 0.5f - x1;
						enemies[i].position.y -= 0.1f;
					}
					else if (x2 > 3.35 + (10.0f - b) * 0.5f) {
						enemies[i].position.x -= x2 - (3.35f + (10.0f - b) * 0.5f);
						enemies[i].position.y -= 0.1f;
					}
					if (hit(playerBullet, enemies[i])) {
						enemies[i].position.y = -3;
						playerBullet.position = Vector3(0, 3, 0);
						score += enemies[i].points;
						flag = true;
						if (i > 10 && enemies[i].lastRow) { 
							if (enemies[i - 11].position.y > -3) {
								enemies[i - 11].lastRow = true;
							}
							else if (i > 21 && enemies[i - 22].position.y > -3) {
								enemies[i - 22].lastRow = true;
							}
							else if (i > 32 && enemies[i - 33].position.y > -3) {
								enemies[i - 33].lastRow = true;
							}
							else if (i > 43 && enemies[i - 44].position.y > -3) {
								enemies[i - 44].lastRow = true;
							}
						}
						enemies[i].lastRow = false;
					}
					else if (hit(player, enemies[i])) {
						enemies[i].position.y = -3;
						lives--;
						if (i > 10 && enemies[i].lastRow) { 
							if (enemies[i - 11].position.y > -3) {
								enemies[i - 11].lastRow = true;
							}
							else if (i > 21 && enemies[i - 22].position.y > -3) {
								enemies[i - 22].lastRow = true;
							}
							else if (i > 32 && enemies[i - 33].position.y > -3) {
								enemies[i - 33].lastRow = true;
							}
							else if (i > 43 && enemies[i - 44].position.y > -3) {
								enemies[i - 44].lastRow = true;
							}
						}
						enemies[i].lastRow = false;
						score += enemies[i].points;
						flag = true;
					}
					else if (hit(base, enemies[i])) {
						loseFlag = true;
						break;
					}
					enemies[i].position.x -= ev * elapsed;
					enemies[i].Draw(&program);
					if (enemies[i].lastRow) {
						int x = rand() % (int)(shootGap / elapsed);
						if (x == 0) enemyShoot(enBullets, enemies[i], items);
					}
				}
				enBullets.erase(remove_if(enBullets.begin(), enBullets.end(), removable), enBullets.end());
				for (Entity &b : enBullets) {
					b.position.y += b.velocity.y * elapsed;
					b.Draw(&program);
					if (hit(player, b)) {
						b.position.y = -2.2f;
						lives--;
					}
					else if (hit(playerBullet, b)) {
						b.position.y = -2.2f;
						playerBullet.position = Vector3(0, 3, 0);
					}
				}
				base.Draw(&program);
				player.Draw(&program);
				playerBullet.Draw(&program);
				modelMatrix.identity();
				modelMatrix.Translate(-2.6f, 1.8f, 0);
				DrawText(&program, fonts, "Score: " + to_string(score), 0.2f, 0.0f);
				modelMatrix.identity();
				modelMatrix.Translate(-2.6f, -1.9f, 0);
				DrawText(&program, fonts, "Lives:" + to_string(lives), 0.2f, 0.0f);
				if (lives <= 0) loseFlag = true;
				if (score > 1 && score % 990 == 0 && flag) {
					reset(player, playerBullet, enemies, shootGap, a, b, ev);
					c++;
					ev -= 0.05f * c;
					flag = false;
				}
			}
			break;
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
