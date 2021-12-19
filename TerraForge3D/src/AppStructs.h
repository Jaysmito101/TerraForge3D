#pragma once

#include <string.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <time.h>
#include <json.hpp>

struct Vec2 {
	float x = 0;
	float y = 0;
};

struct Vec3 {
	float x = 0;
	float y = 0;
	float z = 0;
};

struct Vec4 {
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;
};

struct Stats
{
	float deltaTime = 1;
	float frameRate = 1;
	int triangles = 0;
	int vertCount = 0;
};


struct ActiveWindows {
	bool styleEditor = false;
	bool statsWindow = false;
	bool shaderEditorWindow = false;
	bool texturEditorWindow = false;
	bool seaEditor = false;
	bool elevationNodeEditorWindow = true;
	bool contribWindow = false;
	bool textureStore = false;
	bool osLisc = false;
	bool filtersManager = false;
	bool foliageManager = false;
	bool supportersTribute = false;
	bool skySettings = false;
	bool modulesManager = false;
};