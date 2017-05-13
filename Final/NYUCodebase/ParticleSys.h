#pragma once
#include <vector>
#include "Vector.h"
#include "Entity.h"
#include "ShaderProgram.h"
#include "SheetSprite.h"
#include "Matrix.h"

struct Particle {
	Vector position;
	Vector velocity;
	float lifetime;
	float sizeDeviation;
};

class ParticleEmitter {
public:
	ParticleEmitter(unsigned int particleCount, GLuint sprite, Vector v, Vector pos) {
		textureID = sprite;
		v.normalize();
		position = pos;
		startSize = 0.25f;
		endSize = 0;
		sizeDeviation = 0.1f;
		maxLifetime = 0.5f;
		for (int i = 0; i < particleCount; i++) {
			Particle p;
			p.lifetime = 0;
			p.position = position;
			p.velocity.x = ((rand() % 10) - 5) / 10.0f + v.x;
			p.velocity.y = ((rand() % 10) - 5) / 10.0f + v.y;
			p.lifetime = (rand() % 50) / 100.0f;
			p.sizeDeviation = (rand() % 100) / 1000;
			particles.push_back(p);
		}
	}

	void Update(float elapsed) {
		for (Particle &p : particles) {
			p.position.x += p.velocity.x * elapsed;
			p.position.y += p.velocity.y * elapsed;
			p.lifetime += elapsed;
		}
		lifeTime += elapsed;
	}

	void Render(ShaderProgram *program) {
		Matrix matrix; 
		program->setModelMatrix(matrix);
		std::vector<float> vertices;
		std::vector<float> texCoords;
		for (int i = 0; i < particles.size(); i++) {
			float m = (particles[i].lifetime / maxLifetime);
			float size = lerp(startSize, endSize, m) + particles[i].sizeDeviation;
			if (size > 0) {
				vertices.insert(vertices.end(), {
					particles[i].position.x - size, particles[i].position.y + size,
					particles[i].position.x - size, particles[i].position.y - size,
					particles[i].position.x + size, particles[i].position.y + size,
					particles[i].position.x + size, particles[i].position.y + size,
					particles[i].position.x - size, particles[i].position.y - size,
					particles[i].position.x + size, particles[i].position.y - size
				});
				texCoords.insert(texCoords.end(), {
					0.0f, 0.0f,
					0.0f, 1.0f,
					1.0f, 0.0f,
					1.0f, 0.0f,
					0.0f, 1.0f,
					1.0f, 1.0f
				});
			}
		}
		glBindTexture(GL_TEXTURE_2D, textureID);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
		glEnableVertexAttribArray(program->texCoordAttribute);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 2);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	GLuint textureID;
	Vector position;
	float maxLifetime;
	std::vector<Particle> particles;
	Vector velocity;
	float startSize;
	float endSize;
	float sizeDeviation;
	float lifeTime;
};