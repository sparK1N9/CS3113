/*
Morphing Mirror Force by Haotian Shen
Player Red: W A S D to move (relative to the spaceship), J K to rotate, space to shoot
Player Blue: arrow keys to move (relative to the spaceship), 7 8(numpad) to rotate, 0(numpad) to shoot
*/
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#include "stb_image.h"
#include "Entity.h"
#include "ParticleSys.h"
#include <algorithm>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif
// 60 FPS (1.0f/60.0f)
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
using std::vector;

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

bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector> &points1, const std::vector<Vector> &points2, Vector &penetration) {
	float normalX = -edgeY;
	float normalY = edgeX;
	float len = sqrtf(normalX*normalX + normalY*normalY);
	normalX /= len;
	normalY /= len;

	std::vector<float> e1Projected;
	std::vector<float> e2Projected;

	for (int i = 0; i < points1.size(); i++) {
		e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
	}
	for (int i = 0; i < points2.size(); i++) {
		e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
	}

	std::sort(e1Projected.begin(), e1Projected.end());
	std::sort(e2Projected.begin(), e2Projected.end());

	float e1Min = e1Projected[0];
	float e1Max = e1Projected[e1Projected.size() - 1];
	float e2Min = e2Projected[0];
	float e2Max = e2Projected[e2Projected.size() - 1];

	float e1Width = fabs(e1Max - e1Min);
	float e2Width = fabs(e2Max - e2Min);
	float e1Center = e1Min + (e1Width / 2.0);
	float e2Center = e2Min + (e2Width / 2.0);
	float dist = fabs(e1Center - e2Center);
	float p = dist - ((e1Width + e2Width) / 2.0);

	if (p >= 0) {
		return false;
	}

	float penetrationMin1 = e1Max - e2Min;
	float penetrationMin2 = e2Max - e1Min;

	float penetrationAmount = penetrationMin1;
	if (penetrationMin2 < penetrationAmount) {
		penetrationAmount = penetrationMin2;
	}

	penetration.x = normalX * penetrationAmount;
	penetration.y = normalY * penetrationAmount;

	return true;
}

bool penetrationSort(const Vector &p1, const Vector &p2) {
	return p1.length() < p2.length();
}

bool checkSATCollision(const std::vector<Vector> &e1Points, const std::vector<Vector> &e2Points, Vector &penetration) {
	std::vector<Vector> penetrations;
	for (int i = 0; i < e1Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e1Points.size() - 1) {
			edgeX = e1Points[0].x - e1Points[i].x;
			edgeY = e1Points[0].y - e1Points[i].y;
		}
		else {
			edgeX = e1Points[i + 1].x - e1Points[i].x;
			edgeY = e1Points[i + 1].y - e1Points[i].y;
		}
		Vector penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}
	for (int i = 0; i < e2Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e2Points.size() - 1) {
			edgeX = e2Points[0].x - e2Points[i].x;
			edgeY = e2Points[0].y - e2Points[i].y;
		}
		else {
			edgeX = e2Points[i + 1].x - e2Points[i].x;
			edgeY = e2Points[i + 1].y - e2Points[i].y;
		}
		Vector penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);

		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}

	std::sort(penetrations.begin(), penetrations.end(), penetrationSort);
	penetration = penetrations[0];

	Vector e1Center;
	for (int i = 0; i < e1Points.size(); i++) {
		e1Center.x += e1Points[i].x;
		e1Center.y += e1Points[i].y;
	}
	e1Center.x /= (float)e1Points.size();
	e1Center.y /= (float)e1Points.size();

	Vector e2Center;
	for (int i = 0; i < e2Points.size(); i++) {
		e2Center.x += e2Points[i].x;
		e2Center.y += e2Points[i].y;
	}
	e2Center.x /= (float)e2Points.size();
	e2Center.y /= (float)e2Points.size();

	Vector ba;
	ba.x = e1Center.x - e2Center.x;
	ba.y = e1Center.y - e2Center.y;

	if ((penetration.x * ba.x) + (penetration.y * ba.y) < 0.0f) {
		penetration.x *= -1.0f;
		penetration.y *= -1.0f;
	}

	return true;
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

void shoot(vector<Entity> &lasers, Entity &player, GLuint items) {
	Entity laser;
	laser.matrix = player.matrix;
	laser.matrix.Translate(0, 0.4f, 0);
	laser.sprite = SheetSprite(items, 856, 94, 9, 37, 1024, 1024, 0.25f);
	laser.velocity_y = 6;
	laser.friction_x = laser.friction_y = 0;
	laser.initVecs(4);
	laser.angle = player.angle + PI / 2;
	lasers.push_back(laser);
}

bool removable(Entity &bullet) { return (bullet.timer > 3.0f); }

bool removableP(ParticleEmitter &pe) { return (pe.lifeTime >= pe.maxLifetime); }

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Morphing Mirror Force", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
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
	GLuint fonts = LoadTexture("font2.png");
	GLuint bg = LoadTexture("blue.png");
	SheetSprite background(bg, 0, 0, 256, 256, 256, 256, 8);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	Mix_Music *music;
	music = Mix_LoadMUS("Abandoned_Space.mp3");
	Mix_PlayMusic(music, -1);
	Mix_Chunk *explosion;
	explosion = Mix_LoadWAV("Blast.wav");
	Mix_Chunk *laser;
	laser = Mix_LoadWAV("Laser.wav");
	Mix_Chunk *ref;
	ref = Mix_LoadWAV("ref.wav");
	Mix_Chunk *ref1;
	ref1 = Mix_LoadWAV("ref1.wav");
	Mix_Chunk *trans;
	trans = Mix_LoadWAV("Mirror Breaking.wav");
	Mix_Chunk *win;
	win = Mix_LoadWAV("win.wav");
	Mix_Chunk *vicR;
	vicR = Mix_LoadWAV("vic1.wav");
	Mix_Chunk *vicB;
	vicB = Mix_LoadWAV("vic.wav");
	Mix_Chunk *draw;
	draw = Mix_LoadWAV("draw.wav");
	enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL_ONE, STATE_GAME_LEVEL_TWO, STATE_GAME_LEVEL_THREE, STATE_GAME_OVER };
	int state = STATE_MAIN_MENU;
	bool p1shoot, p2shoot, roundEnd;
	int p1score, p2score, p1hp, p2hp;
	p1shoot = true;
	p2shoot = true;
	roundEnd = false;
	p1score = p2score = 0;
	p1hp = p2hp = 9;
	GLuint items = LoadTexture("spritesheet_elements.png");
	GLuint players = LoadTexture("sheet.png");
	GLuint spark = LoadTexture("fire.png");
	vector<ParticleEmitter> parts;
	Entity player, player2, rock1, rock2, rock3, boarder1, boarder2, boarder3, boarder4, metal1, metal2, metal3, glass1, glass2, glass3;
	vector <Entity> lasers, staticObjs, nonStaticObjs, refObjs;
	player.sprite = SheetSprite(players, 325, 0, 98, 75, 1024, 1024, 0.5f);
	player2.sprite = SheetSprite(players, 325, 739, 98, 75, 1024, 1024, 0.5f);
	boarder1.sprite = SheetSprite(items, 220, 700, 220, 70, 2048, 2048, 2.2f);
	boarder2.sprite = SheetSprite(items, 220, 700, 220, 70, 2048, 2048, 2.2f);
	boarder3.sprite = SheetSprite(items, 1640, 280, 70, 220, 2048, 2048, 4);
	boarder4.sprite = SheetSprite(items, 1640, 280, 70, 220, 2048, 2048, 4);
	rock1.sprite = SheetSprite(items, 220, 630, 220, 70, 2048, 2048, 0.5f);
	rock2.sprite = SheetSprite(items, 440, 910, 220, 140, 2048, 2048, 0.75f);
	rock3.sprite = SheetSprite(items, 1560, 1430, 70, 70, 2048, 2048, 0.75f);
	metal1.sprite = SheetSprite(items, 1360, 140, 140, 140, 2048, 2048, 0.75f);
	metal2.sprite = SheetSprite(items, 220, 420, 220, 140, 2048, 2048, 0.75f);
	metal3.sprite = SheetSprite(items, 1630, 1720, 70, 220, 2048, 2048, 0.75f);
	glass1.sprite = SheetSprite(items, 0, 490, 220, 140, 2048, 2048, 0.75f);
	glass2.sprite = SheetSprite(items, 1220, 500, 140, 220, 2048, 2048, 0.75f);
	glass3.sprite = SheetSprite(items, 800, 790, 140, 140, 2048, 2048, 0.75f);
	glass1.matrix.Translate(-2, -0.8f, 0);
	glass2.matrix.Translate(2, 0.9f, 0);
	glass1.matrix.Rotate(0.15f);
	glass2.matrix.Rotate(-0.4f);
	glass1.angle = 0.15f;
	glass2.angle = -0.4f;
	metal1.matrix.Translate(-2, -0.8f, 0);
	metal2.matrix.Translate(2, 0.9f, 0);
	metal1.matrix.Rotate(0.15f);
	metal2.matrix.Rotate(-0.4f);
	metal1.angle = 0.15f;
	metal2.angle = -0.4f;
	boarder1.matrix.Translate(0, -2.9f, 0);
	boarder2.matrix.Translate(0, 2.9f, 0);
	boarder3.matrix.Translate(-4.0f, 0, 0);
	boarder4.matrix.Translate(4.0f, 0, 0);
	player.matrix.Translate(-2, 0, 0);
	player2.matrix.Translate(2, 0, 0);
	rock1.matrix.Translate(-2, 1, 0);
	rock2.matrix.Translate(2, -1, 0);
	rock1.matrix.Rotate(0.15f);
	rock2.matrix.Rotate(-0.4f);
	rock1.angle = 0.15f;
	rock2.angle = -0.4f;
	player.initVecs(3);
	player2.initVecs(3);
	player.matrix.Rotate(PI * 3 / 2);
	player.angle = PI * 3 / 2;
	player2.matrix.Rotate(PI / 2);
	player2.angle = PI / 2;
	staticObjs.push_back(boarder1);
	staticObjs.push_back(boarder2);
	staticObjs.push_back(boarder3);
	staticObjs.push_back(boarder4);
	staticObjs.push_back(rock1);
	staticObjs.push_back(rock2);
	staticObjs.push_back(rock3);
	refObjs.push_back(glass1);
	refObjs.push_back(glass2);
	refObjs.push_back(glass3);
	nonStaticObjs.push_back(metal1);
	nonStaticObjs.push_back(metal2);
	nonStaticObjs.push_back(metal3);
	for (Entity &e : staticObjs) {
		e.initVecs(4);
		e.toWorldCoord();
	}
	for (Entity &e : nonStaticObjs) {
		e.initVecs(4);
		e.toWorldCoord();
	}
	for (Entity &e : refObjs) {
		e.initVecs(4);
		e.toWorldCoord();
	}
	Vector penetration;
	while (!done) {
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYUP && (event.key.keysym.scancode == SDL_SCANCODE_A || event.key.keysym.scancode == SDL_SCANCODE_W || event.key.keysym.scancode == SDL_SCANCODE_S || event.key.keysym.scancode == SDL_SCANCODE_D || event.key.keysym.scancode == SDL_SCANCODE_J || event.key.keysym.scancode == SDL_SCANCODE_K)) {
				player.acceleration_x = player.acceleration_y = 0.0f;
				player.rotation = 0;
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN && state == STATE_MAIN_MENU && event.button.button == 1) {
				Mix_PlayChannel(-1, trans, 0);
				state = STATE_GAME_LEVEL_ONE;
			}
			else if (event.type == SDL_KEYDOWN && state == STATE_MAIN_MENU && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}
			else if ((event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_SPACE && p1shoot) && (state == STATE_GAME_LEVEL_ONE || state == STATE_GAME_LEVEL_TWO || state == STATE_GAME_LEVEL_THREE)) {
				Mix_PlayChannel(-1, laser, 0);
				shoot(lasers, player, players);
				player.timer = 0;
				p1shoot = false;
			}
			else if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_KP_0 && p2shoot && (state == STATE_GAME_LEVEL_ONE || state == STATE_GAME_LEVEL_TWO || state == STATE_GAME_LEVEL_THREE)) {
				Mix_PlayChannel(-1, laser, 0);
				shoot(lasers, player2, players);
				player2.timer = 0;
				p2shoot = false;
			}
			else if (event.type == SDL_KEYUP && (event.key.keysym.scancode == SDL_SCANCODE_UP || event.key.keysym.scancode == SDL_SCANCODE_DOWN || event.key.keysym.scancode == SDL_SCANCODE_LEFT || event.key.keysym.scancode == SDL_SCANCODE_RIGHT || event.key.keysym.scancode == SDL_SCANCODE_KP_7 || event.key.keysym.scancode == SDL_SCANCODE_KP_8)) {
				player2.acceleration_x = player2.acceleration_y = 0.0f;
				player2.rotation = 0;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);
		background.Draw(&program);
		switch (state) {
		case STATE_MAIN_MENU:
			modelMatrix.identity();
			modelMatrix.Translate(-3.0f, 0.5f, 0);
			DrawText(&program, fonts, "Morphing Mirror Force", 0.3f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-1.7f, -1.0f, 0);
			DrawText(&program, fonts, "Click anywhere to begin", 0.15f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-1.5f, -1.5f, 0);
			DrawText(&program, fonts, "Press Escape to quit the game", 0.1f, 0.0f);
			break;
		case STATE_GAME_LEVEL_ONE: {
			if (!roundEnd) {
				float ticks = (float)SDL_GetTicks() / 1000.0f;
				float elapsed = ticks - lastFrameTicks;
				lastFrameTicks = ticks;
				float fixedElapsed = elapsed;
				if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
					fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
				}
				while (fixedElapsed >= FIXED_TIMESTEP) {
					fixedElapsed -= FIXED_TIMESTEP;
					player.Update(FIXED_TIMESTEP);
					player2.Update(FIXED_TIMESTEP);
					for (Entity& e : lasers) {
						e.Update(FIXED_TIMESTEP);
						if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
							Mix_PlayChannel(-1, explosion, 0);
							ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
							parts.push_back(p);
							e.matrix.Translate(0, 15, 0);
							e.velocity_x = e.velocity_y = 0;
							e.timer = 4;
							p1hp--;
						}
						else if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
							Mix_PlayChannel(-1, explosion, 0);
							ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
							parts.push_back(p);
							e.matrix.Translate(0, 15, 0);
							e.velocity_x = e.velocity_y = 0;
							e.timer = 4;
							p2hp--;
						}
						for (Entity &f : staticObjs) {
							if (checkSATCollision(e.worldCoords, f.worldCoords, penetration)) {
								Mix_PlayChannel(-1, ref, 0);
								e.move(penetration.x, penetration.y);
								float tmp = e.angle;
								if (abs(penetration.y) > abs(penetration.x)) {
									if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
										e.angle = tmp + PI - 2 * (e.angle - f.angle);
									}
									else {
										e.angle = tmp + PI + 2 * (-e.angle + f.angle);
									}
								}
								else {
									if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
										e.angle = tmp - 2 * (e.angle - f.angle);
									}
									else {
										e.angle = tmp + 2 * (-e.angle + f.angle);
									}
								}
								if (e.angle > 2 * PI) e.angle -= (2 * PI);
								if (e.angle < 0) e.angle += (2 * PI);
								e.matrix.Rotate(e.angle - tmp);
								e.matrix.Rotate(PI);
							}
						}
					}
					for (Entity &e : staticObjs) {
						if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
							player.move(-penetration.x, -penetration.y);
						}
						if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
							player2.move(-penetration.x, -penetration.y);
						}
					}
					if (checkSATCollision(player.worldCoords, player2.worldCoords, penetration)) {
						player2.move(-penetration.x*0.5f, -penetration.y*0.5f);
						player.move(penetration.x*0.5f, penetration.y*0.5f);
					}
				}
				player.Update(fixedElapsed);
				player2.Update(fixedElapsed);
				for (Entity& e : lasers) {
					e.Update(fixedElapsed);
					e.timer += elapsed;
					if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
						Mix_PlayChannel(-1, explosion, 0);
						ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
						parts.push_back(p);
						e.matrix.Translate(0, 15, 0);
						e.velocity_x = e.velocity_y = 0;
						e.timer = 4;
						p1hp--;
					}
					else if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
						Mix_PlayChannel(-1, explosion, 0);
						ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
						parts.push_back(p);
						e.matrix.Translate(0, 15, 0);
						e.velocity_x = e.velocity_y = 0;
						e.timer = 4;
						p2hp--;
					}
					for (Entity &f : staticObjs) {
						if (checkSATCollision(e.worldCoords, f.worldCoords, penetration)) {
							Mix_PlayChannel(-1, ref, 0);
							e.move(penetration.x, penetration.y);
							float tmp = e.angle;
							if (abs(penetration.y) > abs(penetration.x)) {
								if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
									e.angle = tmp + PI - 2 * (e.angle - f.angle);
								}
								else {
									e.angle = tmp + PI + 2 * (-e.angle + f.angle);
								}
							}
							else {
								if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
									e.angle = tmp - 2 * (e.angle - f.angle);
								}
								else {
									e.angle = tmp + 2 * (-e.angle + f.angle);
								}
							}
							if (e.angle > 2 * PI) e.angle -= (2 * PI);
							if (e.angle < 0) e.angle += (2 * PI);
							e.matrix.Rotate(e.angle - tmp);
							e.matrix.Rotate(PI);
						}
					}
				}
				for (Entity &e : staticObjs) {
					if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
						player.move(-penetration.x, -penetration.y);
					}
					if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
						player2.move(-penetration.x, -penetration.y);
					}
				}
				if (checkSATCollision(player.worldCoords, player2.worldCoords, penetration)) {
					player2.move(-penetration.x*0.5f, -penetration.y*0.5f);
					player.move(penetration.x*0.5f, penetration.y*0.5f);
				}
				for (ParticleEmitter& e : parts) {
					e.Update(elapsed);
				}
				lasers.erase(std::remove_if(lasers.begin(), lasers.end(), removable), lasers.end());
				parts.erase(std::remove_if(parts.begin(), parts.end(), removableP), parts.end());
				if (!p1shoot) {
					player.timer += elapsed;
					if (player.timer >= 0.5f) {
						player.timer = 0;
						p1shoot = true;
					}
				}
				if (!p2shoot) {
					player2.timer += elapsed;
					if (player2.timer >= 0.5f) {
						player2.timer = 0;
						p2shoot = true;
					}
				}
				if (keys[SDL_SCANCODE_D]) {
					player.acceleration_x = 3;
				}
				if (keys[SDL_SCANCODE_A]) {
					player.acceleration_x = -3;
				}
				if (keys[SDL_SCANCODE_W]) {
					player.acceleration_y = 3;
				}
				if (keys[SDL_SCANCODE_S]) {
					player.acceleration_y = -3;
				}
				if (keys[SDL_SCANCODE_J]) {
					player.rotation = 0.003f;
				}
				if (keys[SDL_SCANCODE_K]) {
					player.rotation = -0.003f;
				}
				if (keys[SDL_SCANCODE_RIGHT]) {
					player2.acceleration_x = 3;
				}
				if (keys[SDL_SCANCODE_LEFT]) {
					player2.acceleration_x = -3;
				}
				if (keys[SDL_SCANCODE_UP]) {
					player2.acceleration_y = 3;
				}
				if (keys[SDL_SCANCODE_DOWN]) {
					player2.acceleration_y = -3;
				}
				if (keys[SDL_SCANCODE_KP_7]) {
					player2.rotation = 0.003f;
				}
				if (keys[SDL_SCANCODE_KP_8]) {
					player2.rotation = -0.003f;
				}
				player.Render(&program);
				player2.Render(&program);
				for (Entity& e : staticObjs) {
					e.Render(&program);
				}
				for (Entity& e : lasers) {
					e.Render(&program);
				}
				for (ParticleEmitter& e : parts) {
					e.Render(&program);
				}
				if (p1hp <= 0 && p2hp <= 0) {
					roundEnd = true;
					Mix_PlayChannel(-1, win, 0);
				}
				else if (p1hp <= 0) {
					Mix_PlayChannel(-1, win, 0);
					roundEnd = true;
					p2score++;
				}
				else if (p2hp <= 0) {
					Mix_PlayChannel(-1, win, 0);
					roundEnd = true;
					p1score++;
				}
			}
			else {
				p1shoot = p2shoot = false;
				if (p1hp <= 0 && p2hp <= 0) {
					modelMatrix.identity();
					modelMatrix.Translate(-0.6f, 0.5f, 0);
					DrawText(&program, fonts, "DRAW!", 0.3f, 0.0f);
					modelMatrix.identity();
					modelMatrix.Translate(-1.9f, -1.5f, 0);
					DrawText(&program, fonts, "Such draw, much skill, very rare, wow!", 0.1f, 0.0f);
				}
				else if (p1hp <= 0) {
					modelMatrix.identity();
					modelMatrix.Translate(-2.8f, 0.5f, 0);
					DrawText(&program, fonts, "Blue Player Victory!", 0.3f, 0.0f);
				}
				else if (p2hp <= 0) {
					modelMatrix.identity();
					modelMatrix.Translate(-2.7f, 0.5f, 0);
					DrawText(&program, fonts, "Red Player Victory!", 0.3f, 0.0f);
				}
				modelMatrix.identity();
				modelMatrix.Translate(-1.7f, -1.0f, 0);
				DrawText(&program, fonts, "Press Enter to continue", 0.15f, 0.0f);
				if (keys[SDL_SCANCODE_KP_ENTER] || keys[SDL_SCANCODE_RETURN]) {
					Mix_PlayChannel(-1, trans, 0);
					staticObjs.resize(4);
					p1hp = p2hp = 9;
					lasers.resize(0);
					roundEnd = false;
					player.matrix.identity();
					player2.matrix.identity();
					player.matrix.Translate(-2, 0, 0);
					player2.matrix.Translate(2, 0, 0);
					player.matrix.Rotate(PI * 3 / 2);
					player.angle = PI * 3 / 2;
					player2.matrix.Rotate(PI / 2);
					player2.angle = PI / 2;
					p1shoot = p2shoot = true;
					state = STATE_GAME_LEVEL_TWO;
				}
			}
			modelMatrix.identity();
			modelMatrix.Translate(-0.8f, 1.7f, 0);
			DrawText(&program, fonts, std::to_string(p1score) + "\t:\t" + std::to_string(p2score), 0.4f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-0.4f, 1.9f, 0);
			DrawText(&program, fonts, "SCORE", 0.2f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-3.2f, 1.9f, 0);
			DrawText(&program, fonts, "Red Player\t\t\t\t\t\t\t\t\t\t\t\tBlue Player", 0.2f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-3.2f, 1.7f, 0);
			DrawText(&program, fonts, "HP: " + std::to_string(p1hp) + "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" + "HP: " + std::to_string(p2hp), 0.2f, 0.0f);
			break;
		}
		case STATE_GAME_LEVEL_TWO: {
			if (!roundEnd) {
				float ticks = (float)SDL_GetTicks() / 1000.0f;
				float elapsed = ticks - lastFrameTicks;
				lastFrameTicks = ticks;
				float fixedElapsed = elapsed;
				if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
					fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
				}
				while (fixedElapsed >= FIXED_TIMESTEP) {
					fixedElapsed -= FIXED_TIMESTEP;
					player.Update(FIXED_TIMESTEP);
					player2.Update(FIXED_TIMESTEP);
					for (Entity& e : lasers) {
						e.Update(FIXED_TIMESTEP);
						if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
							Mix_PlayChannel(-1, explosion, 0);
							ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
							parts.push_back(p);
							e.matrix.Translate(0, 15, 0);
							e.velocity_x = e.velocity_y = 0;
							e.timer = 4;
							p1hp--;
						}
						else if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
							Mix_PlayChannel(-1, explosion, 0);
							ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
							parts.push_back(p);
							e.matrix.Translate(0, 15, 0);
							e.velocity_x = e.velocity_y = 0;
							e.timer = 4;
							p2hp--;
						}
						for (Entity &f : staticObjs) {
							if (checkSATCollision(e.worldCoords, f.worldCoords, penetration)) {
								Mix_PlayChannel(-1, ref, 0);
								e.move(penetration.x, penetration.y);
								float tmp = e.angle;
								if (abs(penetration.y) > abs(penetration.x)) {
									if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
										e.angle = tmp + PI - 2 * (e.angle - f.angle);
									}
									else {
										e.angle = tmp + PI + 2 * (-e.angle + f.angle);
									}
								}
								else {
									if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
										e.angle = tmp - 2 * (e.angle - f.angle);
									}
									else {
										e.angle = tmp + 2 * (-e.angle + f.angle);
									}
								}
								if (e.angle > 2 * PI) e.angle -= (2 * PI);
								if (e.angle < 0) e.angle += (2 * PI);
								e.matrix.Rotate(e.angle - tmp);
								e.matrix.Rotate(PI);
							}
						}
						for (Entity &f : nonStaticObjs) {
							if (checkSATCollision(e.worldCoords, f.worldCoords, penetration)) {
								e.move(penetration.x * 2 / 3, penetration.y * 2 / 3);
								f.move(-penetration.x / 3, -penetration.y / 3);
								float tmp = e.angle;
								if (abs(penetration.y) > abs(penetration.x)) {
									if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
										e.angle = tmp + PI - 2 * (e.angle - f.angle);
									}
									else {
										e.angle = tmp + PI + 2 * (-e.angle + f.angle);
									}
								}
								else {
									if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
										e.angle = tmp - 2 * (e.angle - f.angle);
									}
									else {
										e.angle = tmp + 2 * (-e.angle + f.angle);
									}
								}
								if (e.angle > 2 * PI) e.angle -= (2 * PI);
								if (e.angle < 0) e.angle += (2 * PI);
								e.matrix.Rotate(e.angle - tmp);
								e.matrix.Rotate(PI);
							}
						}
					}
					for (Entity &e : staticObjs) {
						if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
							player.move(-penetration.x, -penetration.y);
						}
						if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
							player2.move(-penetration.x, -penetration.y);
						}
					}
					for (Entity &e : nonStaticObjs) {
						e.toWorldCoord();
						if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
							player.move(-penetration.x / 2, -penetration.y / 2);
							e.move(penetration.x*0.5f, penetration.y*0.5f);
						}
						if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
							player2.move(-penetration.x / 2, -penetration.y / 2);
							e.move(penetration.x*0.5f, penetration.y*0.5f);
						}
						for (Entity &f : staticObjs) {
							if (checkSATCollision(f.worldCoords, e.worldCoords, penetration)) {
								e.move(-penetration.x, -penetration.y);
							}
						}
					}
					if (checkSATCollision(player.worldCoords, player2.worldCoords, penetration)) {
						player2.move(-penetration.x*0.5f, -penetration.y*0.5f);
						player.move(penetration.x*0.5f, penetration.y*0.5f);
					}
					if (checkSATCollision(nonStaticObjs[0].worldCoords, nonStaticObjs[1].worldCoords, penetration)) {
						nonStaticObjs[1].move(-penetration.x*0.5f, -penetration.y*0.5f);
						nonStaticObjs[0].move(penetration.x*0.5f, penetration.y*0.5f);
					}
					if (checkSATCollision(nonStaticObjs[2].worldCoords, nonStaticObjs[1].worldCoords, penetration)) {
						nonStaticObjs[1].move(-penetration.x*0.5f, -penetration.y*0.5f);
						nonStaticObjs[2].move(penetration.x*0.5f, penetration.y*0.5f);
					}
					if (checkSATCollision(nonStaticObjs[0].worldCoords, nonStaticObjs[2].worldCoords, penetration)) {
						nonStaticObjs[2].move(-penetration.x*0.5f, -penetration.y*0.5f);
						nonStaticObjs[0].move(penetration.x*0.5f, penetration.y*0.5f);
					}
				}
				player.Update(fixedElapsed);
				player2.Update(fixedElapsed);
				for (Entity& e : lasers) {
					e.Update(fixedElapsed);
					e.timer += elapsed;
					if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
						Mix_PlayChannel(-1, explosion, 0);
						ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
						parts.push_back(p);
						e.matrix.Translate(0, 15, 0);
						e.velocity_x = e.velocity_y = 0;
						e.timer = 4;
						p1hp--;
					}
					else if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
						Mix_PlayChannel(-1, explosion, 0);
						ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
						parts.push_back(p);
						e.matrix.Translate(0, 15, 0);
						e.velocity_x = e.velocity_y = 0;
						e.timer = 4;
						p2hp--;
					}
					for (Entity &f : staticObjs) {
						if (checkSATCollision(e.worldCoords, f.worldCoords, penetration)) {
							Mix_PlayChannel(-1, ref, 0);
							e.move(penetration.x, penetration.y);
							float tmp = e.angle;
							if (abs(penetration.y) > abs(penetration.x)) {
								if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
									e.angle = tmp + PI - 2 * (e.angle - f.angle);
								}
								else {
									e.angle = tmp + PI + 2 * (-e.angle + f.angle);
								}
							}
							else {
								if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
									e.angle = tmp - 2 * (e.angle - f.angle);
								}
								else {
									e.angle = tmp + 2 * (-e.angle + f.angle);
								}
							}
							if (e.angle > 2 * PI) e.angle -= (2 * PI);
							if (e.angle < 0) e.angle += (2 * PI);
							e.matrix.Rotate(e.angle - tmp);
							e.matrix.Rotate(PI);
						}
					}
					for (Entity &f : nonStaticObjs) {
						if (checkSATCollision(e.worldCoords, f.worldCoords, penetration)) {
							e.move(penetration.x * 2 / 3, penetration.y * 2 / 3);
							f.move(-penetration.x / 3, -penetration.y / 3);
							float tmp = e.angle;
							if (abs(penetration.y) > abs(penetration.x)) {
								if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
									e.angle = tmp + PI - 2 * (e.angle - f.angle);
								}
								else {
									e.angle = tmp + PI + 2 * (-e.angle + f.angle);
								}
							}
							else {
								if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
									e.angle = tmp - 2 * (e.angle - f.angle);
								}
								else {
									e.angle = tmp + 2 * (-e.angle + f.angle);
								}
							}
							if (e.angle > 2 * PI) e.angle -= (2 * PI);
							if (e.angle < 0) e.angle += (2 * PI);
							e.matrix.Rotate(e.angle - tmp);
							e.matrix.Rotate(PI);
						}
					}
				}
				for (Entity &e : staticObjs) {
					if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
						player.move(-penetration.x, -penetration.y);
					}
					if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
						player2.move(-penetration.x, -penetration.y);
					}
				}
				for (Entity &e : nonStaticObjs) {
					e.toWorldCoord();
					if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
						player.move(-penetration.x / 2, -penetration.y / 2);
						e.move(penetration.x*0.5f, penetration.y*0.5f);
					}
					if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
						player2.move(-penetration.x / 2, -penetration.y / 2);
						e.move(penetration.x*0.5f, penetration.y*0.5f);
					}
					for (Entity &f : staticObjs) {
						if (checkSATCollision(f.worldCoords, e.worldCoords, penetration)) {
							e.move(-penetration.x, -penetration.y);
						}
					}
				}
				if (checkSATCollision(player.worldCoords, player2.worldCoords, penetration)) {
					player2.move(-penetration.x*0.5f, -penetration.y*0.5f);
					player.move(penetration.x*0.5f, penetration.y*0.5f);
				}
				if (checkSATCollision(nonStaticObjs[0].worldCoords, nonStaticObjs[1].worldCoords, penetration)) {
					nonStaticObjs[1].move(-penetration.x*0.5f, -penetration.y*0.5f);
					nonStaticObjs[0].move(penetration.x*0.5f, penetration.y*0.5f);
				}
				if (checkSATCollision(nonStaticObjs[2].worldCoords, nonStaticObjs[1].worldCoords, penetration)) {
					nonStaticObjs[1].move(-penetration.x*0.5f, -penetration.y*0.5f);
					nonStaticObjs[2].move(penetration.x*0.5f, penetration.y*0.5f);
				}
				if (checkSATCollision(nonStaticObjs[0].worldCoords, nonStaticObjs[2].worldCoords, penetration)) {
					nonStaticObjs[2].move(-penetration.x*0.5f, -penetration.y*0.5f);
					nonStaticObjs[0].move(penetration.x*0.5f, penetration.y*0.5f);
				}
				for (ParticleEmitter& e : parts) {
					e.Update(elapsed);
				}
				lasers.erase(std::remove_if(lasers.begin(), lasers.end(), removable), lasers.end());
				parts.erase(std::remove_if(parts.begin(), parts.end(), removableP), parts.end());
				if (!p1shoot) {
					player.timer += elapsed;
					if (player.timer >= 0.5f) {
						player.timer = 0;
						p1shoot = true;
					}
				}
				if (!p2shoot) {
					player2.timer += elapsed;
					if (player2.timer >= 0.5f) {
						player2.timer = 0;
						p2shoot = true;
					}
				}
				if (keys[SDL_SCANCODE_D]) {
					player.acceleration_x = 3;
				}
				if (keys[SDL_SCANCODE_A]) {
					player.acceleration_x = -3;
				}
				if (keys[SDL_SCANCODE_W]) {
					player.acceleration_y = 3;
				}
				if (keys[SDL_SCANCODE_S]) {
					player.acceleration_y = -3;
				}
				if (keys[SDL_SCANCODE_J]) {
					player.rotation = 0.003f;
				}
				if (keys[SDL_SCANCODE_K]) {
					player.rotation = -0.003f;
				}
				if (keys[SDL_SCANCODE_RIGHT]) {
					player2.acceleration_x = 3;
				}
				if (keys[SDL_SCANCODE_LEFT]) {
					player2.acceleration_x = -3;
				}
				if (keys[SDL_SCANCODE_UP]) {
					player2.acceleration_y = 3;
				}
				if (keys[SDL_SCANCODE_DOWN]) {
					player2.acceleration_y = -3;
				}
				if (keys[SDL_SCANCODE_KP_7]) {
					player2.rotation = 0.003f;
				}
				if (keys[SDL_SCANCODE_KP_8]) {
					player2.rotation = -0.003f;
				}
				player.Render(&program);
				player2.Render(&program);
				for (Entity& e : staticObjs) {
					e.Render(&program);
				}
				for (Entity& e : nonStaticObjs) {
					e.Render(&program);
				}
				for (Entity& e : lasers) {
					e.Render(&program);
				}
				for (ParticleEmitter& e : parts) {
					e.Render(&program);
				}
				if (p1hp <= 0 && p2hp <= 0) {
					Mix_PlayChannel(-1, win, 0);
					roundEnd = true;
				}
				else if (p1hp <= 0) {
					roundEnd = true;
					p2score++;
					if (p2score != 2) Mix_PlayChannel(-1, win, 0);
				}
				else if (p2hp <= 0) {
					roundEnd = true;
					p1score++;
					if (p1score != 2) Mix_PlayChannel(-1, win, 0);
				}
			}
			else {
				p1shoot = p2shoot = false;
				if (p1score == 2 || p2score == 2) {
					if (p1score == 2) Mix_PlayChannel(-1, vicR, 0);
					else Mix_PlayChannel(-1, vicB, 0);
					state = STATE_GAME_OVER;
				}
				else {
					if (p1hp <= 0 && p2hp <= 0) {
						modelMatrix.identity();
						modelMatrix.Translate(-0.6f, 0.5f, 0);
						DrawText(&program, fonts, "DRAW!", 0.3f, 0.0f);
						modelMatrix.identity();
						modelMatrix.Translate(-1.9f, -1.5f, 0);
						DrawText(&program, fonts, "Such draw, much skill, very rare, wow!", 0.1f, 0.0f);
					}
					else if (p1hp <= 0) {
						modelMatrix.identity();
						modelMatrix.Translate(-2.8f, 0.5f, 0);
						DrawText(&program, fonts, "Blue Player Victory!", 0.3f, 0.0f);
					}
					else if (p2hp <= 0) {
						modelMatrix.identity();
						modelMatrix.Translate(-2.7f, 0.5f, 0);
						DrawText(&program, fonts, "Red Player Victory!", 0.3f, 0.0f);
					}
					modelMatrix.identity();
					modelMatrix.Translate(-1.6f, -1.0f, 0);
					DrawText(&program, fonts, "Press Enter to continue", 0.15f, 0.0f);
					if (keys[SDL_SCANCODE_KP_ENTER] || keys[SDL_SCANCODE_RETURN]) {
						Mix_PlayChannel(-1, trans, 0);
						p1shoot = p2shoot = true;
						p1hp = p2hp = 9;
						lasers.resize(0);
						player.matrix.identity();
						player2.matrix.identity();
						player.matrix.Translate(-2, 0, 0);
						player2.matrix.Translate(2, 0, 0);
						player.matrix.Rotate(PI * 3 / 2);
						player.angle = PI * 3 / 2;
						player2.matrix.Rotate(PI / 2);
						player2.angle = PI / 2;
						state++;
					}
				}
			}
			modelMatrix.identity();
			modelMatrix.Translate(-0.8f, 1.7f, 0);
			DrawText(&program, fonts, std::to_string(p1score) + "\t:\t" + std::to_string(p2score), 0.4f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-0.4f, 1.9f, 0);
			DrawText(&program, fonts, "SCORE", 0.2f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-3.2f, 1.9f, 0);
			DrawText(&program, fonts, "Red Player\t\t\t\t\t\t\t\t\t\t\t\tBlue Player", 0.2f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-3.2f, 1.7f, 0);
			DrawText(&program, fonts, "HP: " + std::to_string(p1hp) + "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" + "HP: " + std::to_string(p2hp), 0.2f, 0.0f);
			break;
		}
		case STATE_GAME_LEVEL_THREE: {
			float ticks = (float)SDL_GetTicks() / 1000.0f;
			float elapsed = ticks - lastFrameTicks;
			lastFrameTicks = ticks;
			float fixedElapsed = elapsed;
			if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
				fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
			}
			while (fixedElapsed >= FIXED_TIMESTEP) {
				fixedElapsed -= FIXED_TIMESTEP;
				player.Update(FIXED_TIMESTEP);
				player2.Update(FIXED_TIMESTEP);
				for (Entity& e : lasers) {
					e.Update(FIXED_TIMESTEP);
					if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
						Mix_PlayChannel(-1, explosion, 0);
						ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
						parts.push_back(p);
						e.matrix.Translate(0, 15, 0);
						e.velocity_x = e.velocity_y = 0;
						e.timer = 4;
						p1hp--;
					}
					else if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
						Mix_PlayChannel(-1, explosion, 0);
						ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
						parts.push_back(p);
						e.matrix.Translate(0, 15, 0);
						e.velocity_x = e.velocity_y = 0;
						e.timer = 4;
						p2hp--;
					}
					for (Entity &f : staticObjs) {
						if (checkSATCollision(e.worldCoords, f.worldCoords, penetration)) {
							Mix_PlayChannel(-1, ref, 0);
							e.move(penetration.x, penetration.y);
							float tmp = e.angle;
							if (abs(penetration.y) > abs(penetration.x)) {
								if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
									e.angle = tmp + PI - 2 * (e.angle - f.angle);
								}
								else {
									e.angle = tmp + PI + 2 * (-e.angle + f.angle);
								}
							}
							else {
								if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
									e.angle = tmp - 2 * (e.angle - f.angle);
								}
								else {
									e.angle = tmp + 2 * (-e.angle + f.angle);
								}
							}
							if (e.angle > 2 * PI) e.angle -= (2 * PI);
							if (e.angle < 0) e.angle += (2 * PI);
							e.matrix.Rotate(e.angle - tmp);
							e.matrix.Rotate(PI);
						}
					}
					for (Entity &f : refObjs) {
						if (checkSATCollision(e.worldCoords, f.worldCoords, penetration)) {
							Mix_PlayChannel(-1, ref1, 0);
							e.move(penetration.x, penetration.y);
							e.angle += PI;
							if (e.angle > 2 * PI) e.angle -= (2 * PI);
							e.matrix.Rotate(PI);
						}
					}
				}
				for (Entity &e : staticObjs) {
					if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
						player.move(-penetration.x, -penetration.y);
					}
					if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
						player2.move(-penetration.x, -penetration.y);
					}
				}
				for (Entity &e : refObjs) {
					if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
						player.move(-penetration.x, -penetration.y);
					}
					if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
						player2.move(-penetration.x, -penetration.y);
					}
				}
				if (checkSATCollision(player.worldCoords, player2.worldCoords, penetration)) {
					player2.move(-penetration.x*0.5f, -penetration.y*0.5f);
					player.move(penetration.x*0.5f, penetration.y*0.5f);
				}
			}
			player.Update(fixedElapsed);
			player2.Update(fixedElapsed);
			for (Entity& e : lasers) {
				e.Update(fixedElapsed);
				e.timer += elapsed;
				if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
					Mix_PlayChannel(-1, explosion, 0);
					ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
					parts.push_back(p);
					e.matrix.Translate(0, 15, 0);
					e.velocity_x = e.velocity_y = 0;
					e.timer = 4;
					p1hp--;
				}
				else if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
					Mix_PlayChannel(-1, explosion, 0);
					ParticleEmitter p = ParticleEmitter(20, spark, penetration, e.worldCoords[1]);
					parts.push_back(p);
					e.matrix.Translate(0, 15, 0);
					e.velocity_x = e.velocity_y = 0;
					e.timer = 4;
					p2hp--;
				}
				for (Entity &f : staticObjs) {
					if (checkSATCollision(e.worldCoords, f.worldCoords, penetration)) {
						Mix_PlayChannel(-1, ref, 0);
						e.move(penetration.x, penetration.y);
						float tmp = e.angle;
						if (abs(penetration.y) > abs(penetration.x)) {
							if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
								e.angle = tmp + PI - 2 * (e.angle - f.angle);
							}
							else {
								e.angle = tmp + PI + 2 * (-e.angle + f.angle);
							}
						}
						else {
							if (e.angle < PI / 2 || (e.angle > PI &&e.angle < PI * 3 / 2)) {
								e.angle = tmp - 2 * (e.angle - f.angle);
							}
							else {
								e.angle = tmp + 2 * (-e.angle + f.angle);
							}
						}
						if (e.angle > 2 * PI) e.angle -= (2 * PI);
						if (e.angle < 0) e.angle += (2 * PI);
						e.matrix.Rotate(e.angle - tmp);
						e.matrix.Rotate(PI);
					}
				}
				for (Entity &f : refObjs) {
					if (checkSATCollision(e.worldCoords, f.worldCoords, penetration)) {
						Mix_PlayChannel(-1, ref1, 0);
						e.move(penetration.x, penetration.y);
						e.angle += PI;
						if (e.angle > 2 * PI) e.angle -= (2 * PI);
						e.matrix.Rotate(PI);
					}
				}
			}
			for (Entity &e : staticObjs) {
				if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
					player.move(-penetration.x, -penetration.y);
				}
				if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
					player2.move(-penetration.x, -penetration.y);
				}
			}
			for (Entity &e : refObjs) {
				if (checkSATCollision(e.worldCoords, player.worldCoords, penetration)) {
					player.move(-penetration.x, -penetration.y);
				}
				if (checkSATCollision(e.worldCoords, player2.worldCoords, penetration)) {
					player2.move(-penetration.x, -penetration.y);
				}
			}
			if (checkSATCollision(player.worldCoords, player2.worldCoords, penetration)) {
				player2.move(-penetration.x*0.5f, -penetration.y*0.5f);
				player.move(penetration.x*0.5f, penetration.y*0.5f);
			}
			for (ParticleEmitter& e : parts) {
				e.Update(elapsed);
			}
			lasers.erase(std::remove_if(lasers.begin(), lasers.end(), removable), lasers.end());
			parts.erase(std::remove_if(parts.begin(), parts.end(), removableP), parts.end());
			if (!p1shoot) {
				player.timer += elapsed;
				if (player.timer >= 0.5f) {
					player.timer = 0;
					p1shoot = true;
				}
			}
			if (!p2shoot) {
				player2.timer += elapsed;
				if (player2.timer >= 0.5f) {
					player2.timer = 0;
					p2shoot = true;
				}
			}
			if (keys[SDL_SCANCODE_D]) {
				player.acceleration_x = 3;
			}
			if (keys[SDL_SCANCODE_A]) {
				player.acceleration_x = -3;
			}
			if (keys[SDL_SCANCODE_W]) {
				player.acceleration_y = 3;
			}
			if (keys[SDL_SCANCODE_S]) {
				player.acceleration_y = -3;
			}
			if (keys[SDL_SCANCODE_J]) {
				player.rotation = 0.003f;
			}
			if (keys[SDL_SCANCODE_K]) {
				player.rotation = -0.003f;
			}
			if (keys[SDL_SCANCODE_RIGHT]) {
				player2.acceleration_x = 3;
			}
			if (keys[SDL_SCANCODE_LEFT]) {
				player2.acceleration_x = -3;
			}
			if (keys[SDL_SCANCODE_UP]) {
				player2.acceleration_y = 3;
			}
			if (keys[SDL_SCANCODE_DOWN]) {
				player2.acceleration_y = -3;
			}
			if (keys[SDL_SCANCODE_KP_7]) {
				player2.rotation = 0.003f;
			}
			if (keys[SDL_SCANCODE_KP_8]) {
				player2.rotation = -0.003f;
			}
			player.Render(&program);
			player2.Render(&program);
			for (Entity& e : staticObjs) {
				e.Render(&program);
			}
			for (Entity& e : lasers) {
				e.Render(&program);
			}
			for (Entity &e : refObjs) {
				e.Render(&program);
			}
			for (ParticleEmitter& e : parts) {
				e.Render(&program);
			}
			if (p1hp <= 0 && p2hp <= 0) {
				if (p1score > p2score) Mix_PlayChannel(-1, vicR, 0);
				else if (p1score < p2score) Mix_PlayChannel(-1, vicB, 0);
				else Mix_PlayChannel(-1, draw, 0);
				state = STATE_GAME_OVER;
			}
			else if (p1hp <= 0) {
				Mix_PlayChannel(-1, vicB, 0);
				p2score++;
				state = STATE_GAME_OVER;
			}
			else if (p2hp <= 0) {
				Mix_PlayChannel(-1, vicR, 0);
				p1score++;
				state = STATE_GAME_OVER;
			}
			modelMatrix.identity();
			modelMatrix.Translate(-0.8f, 1.7f, 0);
			DrawText(&program, fonts, std::to_string(p1score) + "\t:\t" + std::to_string(p2score), 0.4f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-0.4f, 1.9f, 0);
			DrawText(&program, fonts, "SCORE", 0.2f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-3.2f, 1.9f, 0);
			DrawText(&program, fonts, "Red Player\t\t\t\t\t\t\t\t\t\t\t\tBlue Player", 0.2f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-3.2f, 1.7f, 0);
			DrawText(&program, fonts, "HP: " + std::to_string(p1hp) + "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" + "HP: " + std::to_string(p2hp), 0.2f, 0.0f);
			break;
		}
		case STATE_GAME_OVER:
			modelMatrix.identity();
			modelMatrix.Translate(-0.8f, 1.7f, 0);
			DrawText(&program, fonts, std::to_string(p1score) + "\t:\t" + std::to_string(p2score), 0.4f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-0.4f, 1.9f, 0);
			DrawText(&program, fonts, "SCORE", 0.2f, 0.0f);
			modelMatrix.identity();
			if (p1score == p2score) {
				modelMatrix.Translate(-1.0f, 0.5f, 0);
				DrawText(&program, fonts, "DRAW!", 0.5f, 0.0f);
				modelMatrix.identity();
				modelMatrix.Translate(-1.7f, -0.6f, 0);
				DrawText(&program, fonts, "A draw against all odds?", 0.15f, 0.0f);
				modelMatrix.identity();
				modelMatrix.Translate(-2.1f, -0.8f, 0);
				DrawText(&program, fonts, "It's a miracle! You two should get married!", 0.1f, 0.0f);
			}
			else if (p1score > p2score) {
				modelMatrix.Translate(-0.6f, 0.9f, 0);
				DrawText(&program, fonts, "Winner:", 0.2f, 0.0f);
				modelMatrix.identity();
				modelMatrix.Translate(-2.2f, 0.5f, 0);
				DrawText(&program, fonts, "Red Player", 0.5f, 0.0f);
				modelMatrix.identity();
				modelMatrix.Translate(-2.1f, -0.6f, 0);
				DrawText(&program, fonts, "\"Red superior, blue inferior!\"", 0.15f, 0.0f);
			}
			else {
				modelMatrix.Translate(-0.6f, 0.9f, 0);
				DrawText(&program, fonts, "Winner:", 0.2f, 0.0f);
				modelMatrix.identity();
				modelMatrix.Translate(-2.5f, 0.5f, 0);
				DrawText(&program, fonts, "Blue Player", 0.5f, 0.0f);
				modelMatrix.identity();
				modelMatrix.Translate(-1.4f, -0.6f, 0);
				DrawText(&program, fonts, "\"EZPZ Lemon squeezy!\"", 0.15f, 0.0f);
			}
			modelMatrix.identity();
			modelMatrix.Translate(-2.4f, -1.5f, 0);
			DrawText(&program, fonts, "Press Enter to return to main menu", 0.15f, 0.0f);
			if (keys[SDL_SCANCODE_KP_ENTER] || keys[SDL_SCANCODE_RETURN]) {
				Mix_FreeChunk(ref1);
				state = 0;
				p1shoot = true;
				p2shoot = true;
				roundEnd = false;
				p1score = p2score = 0;
				p1hp = p2hp = 9;
				for (Entity &e : nonStaticObjs) {
					e.matrix.identity();
				}
				nonStaticObjs[0].matrix.Translate(-2, -0.8f, 0);
				nonStaticObjs[1].matrix.Translate(2, 0.9f, 0);
				nonStaticObjs[0].matrix.Rotate(0.15f);
				nonStaticObjs[1].matrix.Rotate(-0.4f);
				nonStaticObjs[0].angle = 0.15f;
				nonStaticObjs[1].angle = -0.4f;
				Entity rock1, rock2, rock3;
				rock1.sprite = SheetSprite(items, 220, 630, 220, 70, 2048, 2048, 0.5f);
				rock2.sprite = SheetSprite(items, 440, 910, 220, 140, 2048, 2048, 0.75f);
				rock3.sprite = SheetSprite(items, 1560, 1430, 70, 70, 2048, 2048, 0.75f);
				rock1.matrix.Translate(-2, 1, 0);
				rock2.matrix.Translate(2, -1, 0);
				rock1.matrix.Rotate(0.15f);
				rock2.matrix.Rotate(-0.4f);
				rock1.angle = 0.15f;
				rock2.angle = -0.4f;
				staticObjs.push_back(rock1);
				staticObjs.push_back(rock2);
				staticObjs.push_back(rock3);
				player.matrix.identity();
				player2.matrix.identity();
				player.matrix.Translate(-2, 0, 0);
				player2.matrix.Translate(2, 0, 0);
				player.matrix.Rotate(PI * 3 / 2);
				player.angle = PI * 3 / 2;
				player2.matrix.Rotate(PI / 2);
				player2.angle = PI / 2;
				for (Entity &e : staticObjs) {
					e.initVecs(4);
					e.toWorldCoord();
				}
			}
			break;
		}
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SDL_GL_SwapWindow(displayWindow);
	}
	Mix_FreeMusic(music);
	Mix_FreeChunk(explosion);
	Mix_FreeChunk(laser);
	Mix_FreeChunk(ref);
	Mix_FreeChunk(ref1);
	Mix_FreeChunk(trans);
	Mix_FreeChunk(win);
	Mix_FreeChunk(vicR);
	Mix_FreeChunk(vicB);
	Mix_FreeChunk(draw);
	SDL_Quit();
	return 0;
}
