/*
 * Picking.cpp
 *
 * Copyright (c) 2014 by Universitaet Stuttgart (VISUS).
 * All rights reserved.
 *
 * This file is part of OGL4Core.
 */

#include "stdafx.h"
#include "Picking.h"
#include "Constants.h"
#include "GL/gl3w.h"
#include "GLHelpers.h"
#include "glm/gtc/matrix_transform.hpp"

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

const int NUM_BOX_ELEMENTS = 12;
const int NUM_CUBE_ELEMENTS = 12;

const int NUM_SPHERE_RES_THETA = 64;
const int NUM_SPHERE_RES_PHI = 128;
const int NUM_SPHERE_ELEMENTS = NUM_SPHERE_RES_PHI * (NUM_SPHERE_RES_THETA * 6 + 2 * 3);
const float RADIUS_SPHERE = 0.5f;

const int NUM_TORUS_RES_P = 64;
const int NUM_TORUS_RES_T = 64;
const int NUM_TORUS_ELEMENTS = NUM_TORUS_RES_P * NUM_TORUS_RES_T * 6;
const float RADIUS_TORUS_IN = 0.16f;
const float RADIUS_TORUS_OUT = 0.34f;

const int SIZE_OF_SPOTLIGHT_FBO = 2048;

/**
 * @brief Picking constructor
 */
Picking::Picking(COGL4CoreAPI * Api) : RenderPlugin(Api),
wWidth(512), wHeight(512) {
	this->myName = "PCVC/Picking";
	this->myDescription = "Second assignment of PCVC";
	fovY = 45.0f;
	zNear = 0.01f;
	zFar = 10.0f;
}

/**
 * @brief Picking destructor
 */
Picking::~Picking() {
}

/*
API callbacks
*/
void Picking::transform_mode_func(EnumVar<Picking>& var) {
	printf("changing transform mode from %d to %d\n", transformation_mode, var.GetValue());
	transformation_mode = var.GetValue();
	return;
}

void Picking::show_fbo_att_func(EnumVar<Picking>& var) {
	printf("changing view mode from %d to %d\n", att_id, var.GetValue());
	att_id = var.GetValue();
	if (att_id == 3) depth_map = 1;
	else depth_map = 0;
	return;
}

void Picking::change_proj_mx(APIVar<Picking, FloatVarPolicy>& var) {
	std::cout << zNear << " " << zFar << " " << fovY << std::endl;
}

void Picking::change_light_pos(APIVar<Picking, Dir3FVarPolicy>& var) {
	glm::vec3 new_light = glm::vec3(var.GetValue().x, var.GetValue().y, var.GetValue().z) + glm::vec3(1.0f);
	printf("changing light position from %s to %s\n", glm::to_string(light_pos).c_str(), glm::to_string(new_light).c_str());
	light_pos = new_light;
	return;
}

void Picking::toggle_light_on_off(ButtonVar<Picking>& var) {
	printf("toggle light\n");
	show_light = !show_light;
}



/**
 * @brief Picking activate method
 */
bool Picking::Activate(void) {
	// --------------------------------------------------
	//  Get the path name of the plugin folder, e.g.
	//  "/path/to/oglcore/Plugins/PCVC/Picking"
	// --------------------------------------------------
	std::string pathName = this->GetCurrentPluginPath();

	// --------------------------------------------------
	//  TODO: Add manipulator that controls the view
	//  matrix of the camera.
	// --------------------------------------------------
	int manipulator_id = this->AddManipulator("View mat", &viewMX, Manipulator::MANIPULATOR_ORBIT_VIEW_3D);
	this->EnableManipulator(manipulator_id);


	// --------------------------------------------------
	//  TODO: Initialize API variables here
	// --------------------------------------------------
	toggle_light.Set(this, "Toggle_light", &Picking::toggle_light_on_off);
	toggle_light.Register();
	manipulate_light_pos.Set(this, "Light_position", &Picking::change_light_pos);
	manipulate_light_pos.Register();
	EnumPair transform_choices[] = { {0, "translation"}, {1, "rotation"} };
	transform_mode.Set(this, "Mode", transform_choices, 2, &Picking::transform_mode_func);
	transform_mode.Register();
	EnumPair show_choices[] = { {0, "scene"}, {1, "color id"}, {2, "normal map"}, {3, "depth map"} };
	showFBOAtt.Set(this, "View", show_choices, 4, &Picking::show_fbo_att_func);
	showFBOAtt.Register();
	zNear.Set(this, "znear", &Picking::change_proj_mx);
	zNear.SetStep(0.1);
	zNear.Register();

	zFar.Set(this, "zfar", &Picking::change_proj_mx);
	zFar.SetStep(0.1);
	zFar.Register();

	fovY.Set(this, "fovy", &Picking::change_proj_mx);
	fovY.SetStep(10);
	fovY.Register();

	// --------------------------------------------------
	//  TODO: Initialize shaders for rendering a box
	//  a cube, a sphere, and a torus.
	// --------------------------------------------------
	vsQuad = pathName + std::string("/resources/quad.vert");
	fsQuad = pathName + std::string("/resources/quad.frag");
	shaderQuad.CreateProgramFromFile(vsQuad.c_str(), fsQuad.c_str());

	vsCube = pathName + std::string("/resources/cube.vert");
	fsCube = pathName + std::string("/resources/cube.frag");
	gsCube = pathName + std::string("/resources/cube.geom");
	shaderCube.CreateProgramFromFile(vsCube.c_str(), gsCube.c_str(), fsCube.c_str());

	vsBox = pathName + std::string("/resources/box.vert");
	fsBox = pathName + std::string("/resources/box.frag");
	shaderBox.CreateProgramFromFile(vsBox.c_str(), fsBox.c_str());

	vsSphere = pathName + std::string("/resources/sphere.vert");
	fsSphere = pathName + std::string("/resources/sphere.frag");
	gsSphere = pathName + std::string("/resources/sphere.geom");
	shaderSphere.CreateProgramFromFile(vsSphere.c_str(), gsSphere.c_str(), fsSphere.c_str());

	vsTorus = pathName + std::string("/resources/torus.vert");
	fsTorus = pathName + std::string("/resources/torus.frag");
	gsTorus = pathName + std::string("/resources/torus.geom");
	shaderTorus.CreateProgramFromFile(vsTorus.c_str(), gsTorus.c_str(), fsTorus.c_str());


	// --------------------------------------------------
	//  TODO: Load textures from the "resources/textures"
	//  folder. Use the "LoadPPMTexture" helper function 
	//  from GLHelpers.h.
	// --------------------------------------------------
	std::string earth_img = pathName + "/resources/textures/earth.ppm";
	texEarth = LoadPPMTexture(earth_img.c_str(), wWidth, wHeight);
	std::string cube_img = pathName + "/resources/textures/cube1.ppm";
	texDice = LoadPPMTexture(cube_img.c_str(), wWidth, wHeight);
	std::string board_img = pathName + "/resources/textures/board.ppm";
	texBoard = LoadPPMTexture(board_img.c_str(), wWidth, wHeight);

	// --------------------------------------------------
	//  Initialize FBO and color attachments to zero
	//  Initialize SpotLight stuff to zero.
	// --------------------------------------------------
	fboID = dboID = 1;
	colAttachID[0] = colAttachID[1] = colAttachID[2] = 0;


	lightFboID = lightDboID = 0;
	lightFboSize[0] = lightFboSize[1] = SIZE_OF_SPOTLIGHT_FBO;
	lightZnear = 0.1f;
	lightZfar = 50.0f;

	// --------------------------------------------------
	//  Initialize vertex arrays
	// --------------------------------------------------
	initVAs();

	// --------------------------------------------------
	//  TODO: 
	//  The 3D scene consists of four objects: a table, 
	//  a die, a sphere, and a torus.
	//  Setup four corresponding 'Objects' and assign
	//  a unique color to each object.
	// --------------------------------------------------
	glm::vec3 translate_cube = glm::vec3(0.4f, 1.1f, 0.0f);
	glm::vec3 translate_sphere = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 translate_torus = glm::vec3(-0.5f, -0.5f, -0.2f);
	translate_vectors.push_back(translate_cube);
	translate_vectors.push_back(translate_sphere);
	translate_vectors.push_back(translate_torus);

	scale_vectors.push_back(glm::vec3(1.0f));
	scale_vectors.push_back(glm::vec3(1.0f));
	scale_vectors.push_back(glm::vec3(1.5, 1.5, 1.5));

	rotation_angles.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
	rotation_angles.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
	rotation_angles.push_back(glm::vec3(0.0f, 0.0f, 0.0f));

	glm::mat4 model_mat_cube = glm::translate(glm::mat4(), translate_cube);
	Object o1 = Object(1, 500, model_mat_cube, &vaCube, (int)(NUM_CUBE_ELEMENTS * 3), &shaderCube, texDice); //cube
	mObjectList.push_back(o1);

	glm::mat4 model_mat_sphere = glm::translate(glm::mat4(), translate_sphere);
	Object o2 = Object(2, 600, model_mat_sphere, &vaSphere, sphereIBOsize, &shaderSphere, texEarth); //sphere
	mObjectList.push_back(o2);

	glm::mat4 model_mat_torus = glm::translate(glm::mat4(), translate_torus) * glm::scale(glm::mat4(), glm::vec3(1.5, 1.5, 1.5));
	Object o3 = Object(3, 700, model_mat_torus, &vaTorus, torusIBOsize, &shaderTorus, texEarth); //torus
	mObjectList.push_back(o3);

	// --------------------------------------------------
	//  Request some parameters
	// --------------------------------------------------
	GLint maxColAtt;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColAtt);
	fprintf(stderr, "Maximum number of color attachments: %d\n", maxColAtt);

	GLint maxGeomOuputVerts;
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &maxGeomOuputVerts);
	fprintf(stderr, "Maximum number of geometry output vertices: %d\n", maxGeomOuputVerts);

	// --------------------------------------------------
	//  Initialize clear color and enable depth testing
	// --------------------------------------------------
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);

	aspect = 1.0f;
	pickedObjNum = -1;
	picking_indices.push_back(0);
	picking_indices.push_back(0);
	picking_indices.push_back(0);

	return true;
}

/**
 * @brief Picking render method
 */
bool Picking::Render(void) {
	drawToFBO();

	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, wWidth, wHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shaderQuad.Bind();
	vaQuad.Bind();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, dboID);
	glUniform1i(glGetUniformLocation(shaderQuad.GetProgHandle(), "depth_texture"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, colAttachID[att_id]);
	glUniform1i(glGetUniformLocation(shaderQuad.GetProgHandle(), "screen_texture"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, colAttachID[2]);
	glUniform1i(glGetUniformLocation(shaderQuad.GetProgHandle(), "normal_texture"), 2);

	glUniform1i(glGetUniformLocation(shaderQuad.GetProgHandle(), "depth_rendered"), depth_map);
	glUniform1f(glGetUniformLocation(shaderQuad.GetProgHandle(), "near"), zNear);
	glUniform1f(glGetUniformLocation(shaderQuad.GetProgHandle(), "far"), zFar);
	glUniform1f(glGetUniformLocation(shaderQuad.GetProgHandle(), "light_on"), (float)show_light);

	glm::mat4 inverse_view = glm::inverse(viewMX);
	glm::mat4 inverse_proj = glm::inverse(projMX);

	glUniformMatrix4fv(glGetUniformLocation(shaderQuad.GetProgHandle(), "inverse_view"), 1, GL_FALSE, glm::value_ptr(inverse_view));
	glUniformMatrix4fv(glGetUniformLocation(shaderQuad.GetProgHandle(), "inverse_proj"), 1, GL_FALSE, glm::value_ptr(inverse_proj));

	glUniform3f(glGetUniformLocation(shaderQuad.GetProgHandle(), "light_pos"), light_pos.x, light_pos.y, light_pos.z);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	vaQuad.Release();
	shaderQuad.Release();

	return false;
}

/**
 * Initialize all vertex arrays
 */
void Picking::initVAs() {
	// --------------------------------------------------
	//  Create vertex arrays for box and cube.
	//  Create vertex arrays for sphere and torus.
	// --------------------------------------------------
	createBoxAndCube(); //nothing
	createSphere(); //nothing
	createTorus(); //nothing

	// --------------------------------------------------
	//  Create a vertex array for the window filling quad.
	// --------------------------------------------------
	float quadVertices[] = {   0.0f, 0.0f,  0.0f,1.0f, 1.0f, 0.0f, 1.0f,1.0f };
	float quadPositions[] = { -1.0f,-1.0f, -1.0f,1.0f, 1.0f,-1.0f, 1.0f,1.0f };
	vaQuad.Create(4);
	vaQuad.SetArrayBuffer(0, GL_FLOAT, 2, quadVertices);	
	vaQuad.SetArrayBuffer(1, GL_FLOAT, 2, quadPositions);

	printf("All VAs are init\n");
}

/**
 *  @brief Create vertex array object for box and cube.
 */
void Picking::createBoxAndCube() {
	// --------------------------------------------------
	//  The box is used to indicate the selected object.
	//  It is made up of the eight corners of a unit
	//  cube that are connected by lines.
	// --------------------------------------------------
	float boxVertices[] = {
		-0.5f, -0.5f, -0.5f, 1.0f,
		 0.5f, -0.5f, -0.5f, 1.0f,
		 0.5f,  0.5f, -0.5f, 1.0f,
		-0.5f,  0.5f, -0.5f, 1.0f,

		-0.5f, -0.5f,  0.5f, 1.0f,
		 0.5f, -0.5f,  0.5f, 1.0f,
		 0.5f,  0.5f,  0.5f, 1.0f,
		-0.5f,  0.5f,  0.5f, 1.0f };

	GLuint boxEdges[] = {
		0,1, 1,2, 2,3, 3,0,
		4,5, 5,6, 6,7, 7,4,
		0,4, 1,5, 2,6, 3,7 };

	// --------------------------------------------------
	//  In order to texturize a unit cube we have to 
	//  define the corners more than once. Here, we use
	//  the following index scheme:
	//
	//      12 -13
	//       | \ |
	//   7 - 8 - 9 -10 -11 
	//   | \ | \ | \ | \ |
	//   2 - 3 - 4 - 5 - 6
	//       | \ |
	//       0 - 1
	//
	//  Please note that the texture has to be in 
	//  cube map format and of pixel size "res x res".
	// --------------------------------------------------

	float cubeVertices[] = {
		 -0.5f, -0.5f, -0.5f, 1.0f, //0.
		  0.5f, -0.5f, -0.5f, 1.0f, //1.
		 -0.5f, -0.5f, -0.5f, 1.0f,//2.
		 -0.5f, -0.5f,  0.5f, 1.0f,//3

		  0.5f, -0.5f,  0.5f, 1.0f,//4
		  0.5f, -0.5f, -0.5f, 1.0f,//5.
		 -0.5f, -0.5f, -0.5f, 1.0f,//6.
		 -0.5f,  0.5f, -0.5f, 1.0f,//7.

		 -0.5f,  0.5f,  0.5f, 1.0f,//8
		  0.5f,  0.5f,  0.5f, 1.0f,//9
		  0.5f,  0.5f, -0.5f, 1.0f,//10
		 -0.5f,  0.5f, -0.5f, 1.0f,//11.

		 -0.5f,  0.5f, -0.5f, 1.0f,//12.
		  0.5f,  0.5f, -0.5f, 1.0f//13
	};

	float cubeTexCoords[] = {
		0.25f, 0.0f,
		0.5f,  0.0f,
		0.0f,  0.25f,
		0.25f, 0.25f,

		0.5f,  0.25f,
		0.75f, 0.25f,
		1.0f,  0.25f,
		0.0f,  0.5f,

		0.25f, 0.5f,
		0.5f,  0.5f,
		0.75f, 0.5f,
		1.0f,  0.5f,

		0.25f, 0.75f,
		0.5f,  0.75f
	};

	GLuint cubeFaces[] = {
		0,1,3, 3,1,4,
		2,3,7, 7,3,8,
		3,4,8, 8,4,9,

		4,5,9, 9,5,10,
		5,6,10, 10,6,11,
		8,9,12, 12,9,13 };

	// --------------------------------------------------
	//  TODO: Create the vertex arrays for box and cube.
	// --------------------------------------------------
	vaCube.Create(14);
	vaCube.SetArrayBuffer(0, GL_FLOAT, 4, cubeVertices);

	glGenBuffers(1, &cubeIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeFaces), cubeFaces, GL_STATIC_DRAW);
	glGenBuffers(1, &vaCubeTex);
	glBindBuffer(GL_ARRAY_BUFFER, vaCubeTex);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeTexCoords), cubeTexCoords, GL_STATIC_DRAW);


	vaBox.Create(8);
	vaBox.SetArrayBuffer(0, GL_FLOAT, 4, boxVertices);
	vaBox.SetElementBuffer(0, 24, boxEdges);
}

/**
 * @brief Create vertex array object for a sphere.
 */
void Picking::createSphere() {
	// clear memory of prev arrays
	std::vector<float> vertices;
	std::vector<float> texCoords;

	float radius = 0.3f;
	float x, y, z, xy;                              // vertex position
	float s, t;                                     // vertex texCoord

	float min_x, max_x, min_y, max_y, min_z, max_z; // for box rendering

	float sectorCount = NUM_SPHERE_RES_PHI;
	float stackCount = NUM_SPHERE_RES_THETA;

	float phi_step = 2 * M_PI / NUM_SPHERE_RES_PHI;
	float theta_step = M_PI / NUM_SPHERE_RES_THETA;
	float phi, theta;

	for (int i = 0; i <= NUM_SPHERE_RES_THETA; ++i)
	{
		theta = i * theta_step;                 // starting from 0 to pi
		xy = radius * sinf(theta);             // r * sin(theta)
		z = radius * cosf(theta);              // r * cos(theta)

		for (int j = 0; j <= NUM_SPHERE_RES_PHI; ++j)
		{
			phi = j * phi_step;           // starting from 0 to 2pi

			// vertex position (x, y, z)
			x = xy * cosf(phi);             // r * sin(theta) * cos(phi)
			y = xy * sinf(phi);             // r * sin(theta) * sin(phi)
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);

			if (i == 0 && j == 0) {
				min_x = max_x = x;
				min_y = max_y = y;
				min_z = max_z = z;
			}

			if (x < min_x) min_x = x;
			if (x > max_x) max_x = x;
			if (y < min_y) min_y = y;
			if (y > max_y) max_y = y;
			if (z < min_z) min_z = z;
			if (z > max_z) max_z = z;


			// vertex tex coord (s, t) range between [0, 1]
			s = (float)j / NUM_SPHERE_RES_PHI;
			t = (float)i / NUM_SPHERE_RES_THETA;
			texCoords.push_back(s);
			texCoords.push_back(t);
		}
	}

	// generate CCW index list of sphere triangles
	std::vector<int> indices;
	int k1, k2;
	for (int i = 0; i < NUM_SPHERE_RES_THETA; ++i)
	{
		k1 = i * (NUM_SPHERE_RES_PHI + 1);     // beginning of current stack
		k2 = k1 + NUM_SPHERE_RES_PHI + 1;      // beginning of next stack

		for (int j = 0; j < NUM_SPHERE_RES_PHI; ++j, ++k1, ++k2)
		{
			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			if (i != 0)
			{
				indices.push_back(k1);
				indices.push_back(k2);
				indices.push_back(k1 + 1);
			}

			// k1+1 => k2 => k2+1
			if (i != (stackCount - 1))
			{
				indices.push_back(k1 + 1);
				indices.push_back(k2);
				indices.push_back(k2 + 1);
			}
		}
	}

	vaSphere.Create(vertices.size());
	vaSphere.SetArrayBuffer(0, GL_FLOAT, 3, vertices.data());

	scaleSphere = glm::vec3(max_x - min_x + 0.05, max_y - min_y + 0.05, max_z - min_z + 0.05);


	glGenBuffers(1, &vaSphereTex);
	glBindBuffer(GL_ARRAY_BUFFER, vaSphereTex);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*texCoords.size(), texCoords.data(), GL_STATIC_DRAW);

	sphereIBOsize = indices.size();
	glGenBuffers(1, &sphereIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)* sphereIBOsize, &indices[0], GL_STATIC_DRAW);

	printf("Done generating sphere: %d vertices, %d/%d indices\n", vertices.size(), indices.size(), NUM_SPHERE_ELEMENTS);
}

/**
 * @brief Create vertex array object for a torus.
 */
void Picking::createTorus() {
	// clear memory of prev arrays
	std::vector<float> vertices;
	std::vector<float> angles;   // for calculating normals
	std::vector<float> texCoords;
	std::vector<float> colors;   // for checkerboard pattern


	float radius = 0.1f;
	float Radius = 0.2f;
	float x, y, z, xy;                              // vertex position
	float s, t;                                     // vertex texCoord

	float min_x, max_x, min_y, max_y, min_z, max_z; // for box rendering

	float t_step = 2 * M_PI / NUM_TORUS_RES_T;
	float p_step = 2 * M_PI / NUM_TORUS_RES_P;
	float t_angle, p_angle;

	for (int i = 0; i <= NUM_TORUS_RES_P; ++i) {
		p_angle = i * p_step;                                         // starting from 0 to 2*pi
		xy = RADIUS_TORUS_OUT + RADIUS_TORUS_IN * cosf(p_angle);      // R + r * cos(p)
		z = RADIUS_TORUS_IN * sinf(p_angle);                          // r * sin(p)

		for (int j = 0; j <= NUM_TORUS_RES_T; ++j) {
			t_angle = j * t_step;               // starting from 0 to 2*pi

			// vertex position (x, y, z)
			x = xy * cosf(t_angle);             // (R + r * cos(p)) * cos(t)
			y = xy * sinf(t_angle);             // (R + r * cos(p)) * sin(t)
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);
			
			if (i == 0 && j == 0) {
				min_x = max_x = x;
				min_y = max_y = y;
				min_z = max_z = z;
			}


			if (x < min_x) min_x = x;
			if (x > max_x) max_x = x;
			if (y < min_y) min_y = y;
			if (y > max_y) max_y = y;
			if (z < min_z) min_z = z;
			if (z > max_z) max_z = z;

			angles.push_back(p_angle);
			angles.push_back(t_angle);

			// vertex tex coord (s, t) range between [0, 1]
			s = (float)j / NUM_TORUS_RES_T;
			t = (float)i / NUM_TORUS_RES_P;
			texCoords.push_back(s);
			texCoords.push_back(t);

			float decide_color = floor(s*10) + floor(t*10);
			if (fmod(decide_color, 2.0) > 0.5) colors.push_back(0.0f);
			else colors.push_back(1.0f);
		}
	}

	// generate CCW index list of torus triangles
	std::vector<int> indices;
	int k1, k2;
	for (int i = 0; i < NUM_TORUS_RES_P; ++i) {
		k1 = i * (NUM_TORUS_RES_T + 1);     // beginning of current stack
		k2 = k1 + NUM_TORUS_RES_T + 1;      // beginning of next stack

		for (int j = 0; j < NUM_TORUS_RES_T; ++j, ++k1, ++k2) {
			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			indices.push_back(k1);
			indices.push_back(k2);
			indices.push_back(k1 + 1);

			// k1+1 => k2 => k2+1
			indices.push_back(k1 + 1);
			indices.push_back(k2);
			indices.push_back(k2 + 1);
		}
	}

	vaTorus.Create(vertices.size());
	vaTorus.SetArrayBuffer(0, GL_FLOAT, 3, vertices.data());
	vaTorus.SetArrayBuffer(1, GL_FLOAT, 1, colors.data());
	vaTorus.SetArrayBuffer(2, GL_FLOAT, 2, angles.data());

	scaleTorus = glm::vec3(max_x - min_x + 0.05, max_y - min_y + 0.05, max_z - min_z + 0.05);

	torusIBOsize = indices.size();
	glGenBuffers(1, &torusIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, torusIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * torusIBOsize, &indices[0], GL_STATIC_DRAW);

	printf("Done generating torus: %d vertices, %d/%d indices\n", vertices.size(), indices.size(), NUM_TORUS_ELEMENTS);

}

/**
 * @brief Initialize all frame buffer objects (FBOs).
 */
void Picking::initFBOs() {
	if (wWidth <= 0 || wHeight <= 0) {
		return;
	}

	// --------------------------------------------------
	//  TODO: Create a frame buffer object (FBO) for 
	//  multiple render targets. Use the createFBOTexture
	//  method to initialize an empty texture.
	// --------------------------------------------------
	glGenFramebuffers(1, &fboID);
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glGenTextures(3, colAttachID);
	glGenTextures(1, &dboID);
	createFBOTexture(colAttachID[0], GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, wWidth, wHeight);
	createFBOTexture(colAttachID[1], GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, wWidth, wHeight);
	createFBOTexture(colAttachID[2], GL_RGB32F, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR, wWidth, wHeight);
	createFBOTexture(dboID, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, GL_LINEAR, wWidth, wHeight);

	GLenum DrawBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, DrawBuffers);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colAttachID[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, colAttachID[1], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, colAttachID[2], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dboID, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) { printf("frame buffer is complete\n"); }

	// --------------------------------------------------
	//  BONUS TASK:
	//  Create a frame buffer object (FBO) for the
	//  view from the spot light.
	// --------------------------------------------------
}

/**
 * @brief  Create a texture for use in the framebuffer object.
 * @param outID            reference to the texture handle ID
 * @param internalFormat   internal format of the texture
 * @param format           format of the data: GL_RGB,...
 * @param type             data type: GL_UNSIGNED_BYTE, GL_FLOAT,...
 * @param filter           texture filter: GL_LINEAR or GL_NEAREST
 * @param width            texture width
 * @param height           texture height
 */
void Picking::createFBOTexture(GLuint & outID, const GLenum internalFormat,
	const GLenum format, const GLenum type,
	GLint filter, int width, int height) {
	// --------------------------------------------------
	//  TODO: 
	//   Generate an empty 2D texture.
	//   Set min/mag filters.
	//   Set wrap mode in (s,t).
	// --------------------------------------------------
	glBindTexture(GL_TEXTURE_2D, outID);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
}

/**
 * Draw to framebuffer object
 */
void Picking::drawToFBO() {
	if (!glIsFramebuffer(fboID)) {
		return;
	}

	glEnable(GL_DEPTH_TEST);

	glViewport(0, 0, wWidth, wHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	projMX = glm::perspective(glm::radians(static_cast<float>(fovY)), aspect, static_cast<float>(zNear), static_cast<float>(zFar));

	for (int i = 0; i < mObjectList.size(); i++) {
		Object * obj = &mObjectList[i];
		obj->shader->Bind();
		obj->shader->PrintInfo();
		obj->va->Bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, obj->texID);
		glUniform1i(glGetUniformLocation(obj->shader->GetProgHandle(), "texture_sampler"), 0);

		glUniformMatrix4fv(obj->shader->GetUniformLocation("modelMX"), 1, GL_FALSE, glm::value_ptr(obj->modelMX));
		glUniformMatrix4fv(obj->shader->GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));
		glUniformMatrix4fv(obj->shader->GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(viewMX));

		glm::vec3 color = idToColor(obj->id*100);
		glUniform3f(obj->shader->GetUniformLocation("pickIdCol"), color.x, color.y, color.z);

		if (obj->id == 1) {
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, vaCubeTex);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIBO);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
			glDisableVertexAttribArray(1);
		}
		else if (obj->id == 2) {
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, vaSphereTex);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO);
			glDrawElements(GL_TRIANGLES, sphereIBOsize, GL_UNSIGNED_INT, nullptr);
			glDisableVertexAttribArray(1);
		}
		else if (obj->id == 3) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, torusIBO);
			glDrawElements(GL_TRIANGLES, torusIBOsize, GL_UNSIGNED_INT, nullptr);
		}
		obj->shader->Release();
		obj->va->Release();

		shaderBox.Bind();
		vaBox.Bind();

		if (picking_indices[obj->id - 1]) {
			if (obj->id == 1) glUniform3f(shaderBox.GetUniformLocation("scale"), scaleCube.x, scaleCube.y, scaleCube.z);
			if (obj->id == 2) glUniform3f(shaderBox.GetUniformLocation("scale"), scaleSphere.x, scaleSphere.y, scaleSphere.z);
			if (obj->id == 3) glUniform3f(shaderBox.GetUniformLocation("scale"), scaleTorus.x, scaleTorus.y, scaleTorus.z);

			glUniformMatrix4fv(shaderBox.GetUniformLocation("modelMX"), 1, GL_FALSE, glm::value_ptr(obj->modelMX));
			glUniformMatrix4fv(shaderBox.GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(viewMX));
			glUniformMatrix4fv(shaderBox.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));
			glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
		}

		vaBox.Release();
		shaderBox.Release();
	}
}


/**
 * Delete all framebuffer objects.
 */
void Picking::deleteFBOs() {
	glDeleteFramebuffers(1, &fboID);
}

/**
 * @brief Check status of bound framebuffer object (FBO).
 */
void Picking::checkFBOStatus() {
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (status) {
	case GL_FRAMEBUFFER_UNDEFINED: {
		fprintf(stderr, "FBO: undefined.\n");
		break;
	}
	case GL_FRAMEBUFFER_COMPLETE: {
		fprintf(stderr, "FBO: complete.\n");
		break;
	}
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: {
		fprintf(stderr, "FBO: incomplete attachment.\n");
		break;
	}
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: {
		fprintf(stderr, "FBO: no buffers are attached to the FBO.\n");
		break;
	}
	case GL_FRAMEBUFFER_UNSUPPORTED: {
		fprintf(stderr, "FBO: combination of internal buffer formats is not supported.\n");
		break;
	}
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: {
		fprintf(stderr, "FBO: number of samples or the value for ... does not match.\n");
		break;
	}
	}
}

/**
 *  Convert object ID to unique color
 * @param id object ID.
 * @return color
 */
glm::vec3 Picking::idToColor(unsigned int id) {
	int r = (id & 0x000000FF) >> 0;
	int g = (id & 0x0000FF00) >> 8;
	int b = (id & 0x00FF0000) >> 16;
	glm::vec3 col = glm::vec3(r / 1.0f, g / 1.0f, b / 1.0f);

	return col;
}

/**
 *  Convert color to object ID.
 * @param buf color defined as 3-array [red,green,blue]
 * @return object ID
 */
unsigned int Picking::colorToId(unsigned char buf[3]) {
	unsigned int num;
	//buf *= glm::vec3(255.0f);
	num = buf[0] + buf[1] * 256 + buf[2] * 256 * 256;
	return num/100;
}

/**
 * @brief Picking deactive method
 */
bool Picking::Deactivate(void) {
	// --------------------------------------------------
	//  TODO: Do not forget to clear all allocated sources.
	// --------------------------------------------------
	shaderQuad.RemoveAllShaders();
	shaderCube.RemoveAllShaders();
	shaderSphere.RemoveAllShaders();
	shaderTorus.RemoveAllShaders();
	shaderBox.RemoveAllShaders();

	vaBox.Delete();
	vaCube.Delete();
	vaSphere.Delete();
	vaTorus.Delete();
	vaQuad.Delete();

	glDeleteTextures(1, &texBoard);
	glDeleteTextures(1, &texDice);
	glDeleteTextures(1, &texEarth);
	glDeleteTextures(1, &texCrate);
	glDeleteTextures(1, &texScreen);
	glDeleteTextures(1, &dboID);

	glDeleteBuffers(1, &torusIBO);
	glDeleteBuffers(1, &sphereIBO);
	glDeleteBuffers(1, &vaSphereTex);
	glDeleteBuffers(1, &cubeIBO);
	glDeleteBuffers(1, &vaCubeTex);



	glDisable(GL_DEPTH_TEST);
	return true;
}

/**
 * @brief Picking initialization method
 */
bool Picking::Init(void) {
	if (gl3wInit()) {
		fprintf(stderr, "Error: Failed to initialize gl3w.\n");
		return false;
	}
	return true;
}

/**
 * @brief Picking keyboard callback function
 * @param key
 * @param x
 * @param y
 */
bool Picking::Keyboard(int key, int action, int mods, int x, int y) {
	// --------------------------------------------------
	//  TODO: Add keyboard functionaliy
	//    Reload shaders when pressing 'r'.
	//    Select between the different render outputs
	//    using [1,2,3,...].
	// --------------------------------------------------
	PostRedisplay();
	return false;
}

/**
 * @brief Picking mouse motion callback function
 * @param x
 * @param y
 */
bool Picking::Motion(int x, int y) {
	// --------------------------------------------------
	//  TODO: Add mouse functionality
	// --------------------------------------------------
	int xpos, ypos;
	int dx, dy;
	GetLastMousePos(xpos, ypos);
	dx = xpos - x;
	dy = ypos - y;
	if (IsRightButtonPressed() && IsShiftPressed()) {
		for (int i = 0; i < mObjectList.size(); i++) {
			if (picking_indices[i]) {
				if (transformation_mode == 0) translate_vectors[i] += glm::vec3(-dx * 0.001, dy * 0.001, 0);
				//else rotation_angles[i] += glm::vec3(-dy * 0.01, - dx * 0.01, 0);

				mObjectList[i].modelMX =
					glm::translate(glm::mat4(1.0f), translate_vectors[i]) *
					glm::rotate(rotation_angles[i].z, glm::vec3(0, 0, 1)) *
					glm::rotate(rotation_angles[i].y, glm::vec3(0, 1, 0)) *
					glm::rotate(rotation_angles[i].x, glm::vec3(1, 0, 0)) *
					glm::scale(glm::mat4(1.0f), scale_vectors[i]);
			}
		}
	}
	else if (IsMidButtonPressed() && IsShiftPressed()) {
		for (int i = 0; i < mObjectList.size(); i++) {
			if (picking_indices[i]) {
				if (transformation_mode == 0)  translate_vectors[i] += glm::vec3(0, 0, dx * 0.001);

				mObjectList[i].modelMX =
					glm::translate(glm::mat4(1.0f), translate_vectors[i]) *
					glm::mat4(1.0f) *
					glm::scale(glm::mat4(1.0f), scale_vectors[i]);
			}
		}
	}
	return false;
}

/**
 * @brief Picking mouse callback function
 * @param button
 * @param state
 * @param x
 * @param y
 */
bool Picking::Mouse(int button, int state, int mods, int x, int y) {
	// --------------------------------------------------
	//  TODO: Add mouse functionality
	// --------------------------------------------------
	int xpos, ypos;
	int dx, dy;
	GetLastMousePos(xpos, ypos);
	dx = xpos - x;
	dy = ypos - y;
	if (IsLeftButtonPressed() && IsShiftPressed()) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID);
		glReadBuffer(GL_COLOR_ATTACHMENT1);

		printf("Reading pixel at %d %d:", x, wHeight-y);
		unsigned char pixel[3];
		glReadPixels(x, wHeight - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixel);

		if (colorToId(pixel) == 1) {
			printf(" Picked Cube\n");
			picking_indices[0] = 1;
		}
		else if (colorToId(pixel) == 2) {
			printf(" Picked Sphere\n");
			picking_indices[1] = 1;
		}
		else if (colorToId(pixel) == 3) {
			printf(" Picked Torus\n");
			picking_indices[2] = 1;
		}
		else {
			printf(" Picked Nothing\n");
			picking_indices[0] = 0;
			picking_indices[1] = 0;
			picking_indices[2] = 0;
		}

		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}
	else if (IsMidButtonPressed()) {
	}

	// --------------------------------------------------
	//  After a mouse interaction, the OpenGL window
	//  has to be updated.
	// --------------------------------------------------   
	PostRedisplay();
	return false;
}

/**
 * @brief Picking resize method
 * @param width
 * @param height
 */
bool Picking::Resize(int width, int height) {
	wWidth = width;
	wHeight = height;
	aspect = wWidth / static_cast<float>(wHeight);

	// Set the projection matrix of the camera.
	projMX = glm::perspective(glm::radians(static_cast<float>(fovY)), aspect, static_cast<float>(zNear), static_cast<float>(zFar));

	// --------------------------------------------------
	//  Every time the window size changes, the size
	//  of the framebuffer object(s) have to be adapted.
	// --------------------------------------------------
	initFBOs();
	return true;
}
