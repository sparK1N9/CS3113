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

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

class Entity {
public:
	float x;
	float y;
	float width;
	float height;
	float speed;
	float angle;
};

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	SDL_Event event;
	bool done = false;
	glViewport(0, 0, 640, 360);
	ShaderProgram program(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	Matrix projectionMatrix;
	Matrix modelMatrix; // field
	Matrix viewMatrix;
	Matrix modelMatrix2; // left paddle
	modelMatrix2.Translate(-3.35f, 0, 0);
	Matrix modelMatrix3; // right paddle
	modelMatrix3.Translate(3.35f, 0, 0);
	Matrix modelMatrix4; // ball
	Entity left;
	Entity right;
	Entity ball;
	left.y = right.y = 0;
	left.width = right.width = 0.1f;
	left.height = right.height = 0.5;
	ball.width = ball.height = 0.1f;
	ball.x = 0;
	ball.y = 1.849;
	modelMatrix4.Translate(0.0f, ball.y, 0);
	ball.speed = 4;
	ball.angle = -3.1415926 / 3;
	float timer = 0;
	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	glUseProgram(program.programID);
	float lastFrameTicks = 0.0f;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		// field
		float vertices[] = { -0.05f, 1.7f, -0.05f, 1.2f, 0.05f, 1.7f, -0.05f, 1.2f, 0.05f, 1.2f, 0.05f, 1.7f, -0.05f, 0.7f, -0.05f, 0.2f, 0.05f, 0.7f, -0.05f, 0.2f, 0.05f, 0.2f, 0.05f, 0.7f, -0.05f, -0.3f, -0.05f, -0.8f, 0.05f, -0.3f, -0.05f, -0.8f, 0.05f, -0.8f, 0.05f, -0.3f, -0.05f, -1.3f, -0.05f, -1.8f, 0.05f, -1.3f, -0.05f, -1.8f, 0.05f, -1.8f, 0.05f, -1.3f, -3.4f, 2.0f, -3.4f, 1.9f, 3.4f, 2.0f, -3.4f, 1.9f, 3.4f, 1.9f, 3.4f, 2.0f, -3.4f, -2.0f, 3.4f, -2.0f, 3.4f, -1.9f, -3.4f, -1.9f, -3.4f, -2.0f, 3.4f, -1.9f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// left paddle. Controled with 'w' and 's' keys
		program.setModelMatrix(modelMatrix2);
		float vertices2[] = { -0.05, 0.25, -0.05, -0.25, 0.05, 0.25, 0.05, 0.25, -0.05, -0.25, 0.05, -0.25 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_W] && left.y <= 1.65) {
			left.y += 3 * elapsed;
			modelMatrix2.Translate(0.0f, 3 * elapsed, 0.0f);
		}
		else if (keys[SDL_SCANCODE_S] && left.y >= -1.65) {
			left.y -= 3 * elapsed;
			modelMatrix2.Translate(0.0f, -3 * elapsed, 0.0f);
		}

		// right paddle.  Controled with up arrow and down arrow
		program.setModelMatrix(modelMatrix3);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		if (keys[SDL_SCANCODE_UP] && right.y <= 1.65) {
			right.y += 3 * elapsed;
			modelMatrix3.Translate(0.0f, 3 * elapsed, 0.0f);
		}
		else if (keys[SDL_SCANCODE_DOWN] && right.y >= -1.65) {
			right.y -= 3 * elapsed;
			modelMatrix3.Translate(0.0f, -3 * elapsed, 0.0f);
		}

		// ball
		program.setModelMatrix(modelMatrix4);
		float vertices4[] = { -0.05f, 0.05f, -0.05f, -0.05f, 0.05f, 0.05f, -0.05f, -0.05f, 0.05f, -0.05f, 0.05f, 0.05f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices4);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		if (ball.y >= 1.85) {
			ball.y = 1.85;
			modelMatrix4.Translate(0.0f, -ball.y + 1.85, 0.0f);
			ball.angle *= -1;
		}
		else if (ball.y <= -1.85) {
			ball.y = -1.85;
			modelMatrix4.Translate(0.0f, -ball.y - 1.85, 0.0f);
			ball.angle *= -1;
		}
		if (ball.x <= -3.25 && ball.x >= -3.45 && ball.y - 0.05 < left.y + 0.25 && ball.y + 0.05 > left.y - 0.25) {
			ball.x = -3.25;
			//ball.angle = 3.1415926 - ball.angle;
			ball.angle = 3.1415926 * (ball.y - left.y);
		}
		else if (ball.x >= 3.25 && ball.x <= 3.45 && ball.y - 0.05 < right.y + 0.25 && ball.y + 0.05 > right.y - 0.25) {
			ball.x = 3.25;
			//ball.angle = 3.1415926 - ball.angle;
			ball.angle = 3.1415926 * (1 - ball.y + right.y);
		}
		else if ((ball.x <= -3.25 || ball.x >= 3.25) && timer <= 1) {
			ball.x += 10;
			modelMatrix4.Translate(10.0f, 0.0f, 0.0f);
			timer += elapsed;
			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		else if ((ball.x <= -3.25 || ball.x >= 3.25) && timer > 1) {
			timer = 0;
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			modelMatrix4.Translate(-ball.x, 0.0f, 0.0f);
			modelMatrix2.Translate(0.0f, -left.y, 0.0f);
			modelMatrix3.Translate(0.0f, -right.y, 0.0f);
			ball.x = left.y = right.y = 0;
		}
		ball.x += cos(ball.angle) * elapsed * ball.speed;
		modelMatrix4.Translate(cos(ball.angle) * elapsed * ball.speed, 0.0f, 0.0f);
		ball.y += sin(ball.angle) * elapsed * ball.speed;
		modelMatrix4.Translate(0.0f, sin(ball.angle) * elapsed * ball.speed, 0.0f);

		glDisableVertexAttribArray(program.positionAttribute);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
