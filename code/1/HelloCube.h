/*
 * HelloCube.h
 *
 * Copyright (c) 2014 by Universitaet Stuttgart (VISUS).
 * All rights reserved.
 *
 * Created by Thomas Mueller  <Thomas.Mueller@vis.uni-stuttgart.de>
 *
 * Description:
 *    Practical Course in Visual Computing
 *    First assignment
 *
 * This file is part of OGL4Core.
 */

#include <cassert>
#include "RenderPlugin.h"
#include "glm/glm.hpp"
#include "GL/gl3w.h"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"

class OGL4COREPLUGIN_API HelloCube : public RenderPlugin {
public:
	HelloCube(COGL4CoreAPI* Api);
	~HelloCube(void);
	GLuint* get_vbo(); // return pointer to vbo
	GLuint* get_ibo(); // return pointer to ibo
	GLint* get_checkers(); // return pointer to checkers
	virtual bool Activate(void);
	virtual bool Deactivate(void);
	virtual bool Init(void);
	virtual bool Keyboard(int key, int action, int mods, int x, int y);
	virtual bool Motion(int x, int y);
	virtual bool PassiveMotion(int x, int y);
	virtual bool Mouse(int button, int state, int mods, int x, int y);
	virtual bool Render(void);
	virtual bool Resize(int w, int h);

protected:
	// Enter methods to create shader program
	bool   setShaderProgram();

	// Write callback functions for the API variables
	void bg_color_func(APIVar<HelloCube, Color3FVarPolicy>& var);
	void transform_mode_func(EnumVar<HelloCube>& var);
	void reset_mode_func(ButtonVar<HelloCube>& var);
	void change_number_of_checkers_func(APIVar<HelloCube, UInt16VarPolicy>& var);

	// Calculate model matrix
	void calculate_model_matrix();

	// Set view matrix depending on camera pitch and yaw
	void   setCameraView();

	// Set projection matrix 
	void   setProjMatrix();


private:
	// API variables
	APIVar<HelloCube, Color3FVarPolicy> bgcolor;
	EnumVar<HelloCube> transform_mode;
	ButtonVar<HelloCube> reset_mode;
	APIVar<HelloCube, UInt16VarPolicy> change_number_of_checkers;

	int transformation_mode = 0; // which control mode
	GLfloat number_of_checkers[2] = { 2.0f, 0.0f }; // specify how many checkers

	GLint checkers; // to pass how many checkers user wants to shader
	GLuint va, vbo, ibo;
	GLuint shaderProg, vertexShader, fragmentShader;
	GLuint MatrixID; // to pass MVP matrix to shader

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	glm::mat4 modelMatrix;
	glm::mat4 objectTranslateMatrix;
	glm::mat4 objectRotateMatrix;
	glm::mat4 objectScaleMatrix;

	glm::vec3 translate_vec = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 scale_vec = glm::vec3(1.0f, 1.0f, 1.0f);
	float angleX = 0;
	float angleY = 0;
	float angleZ = 0;

	float pitch = 180.0f;
	float yaw = 90.0f;
	float fovy = 45.0f;
	float dolly = -3.0f;

	float aspectRatio;
};

extern "C" OGL4COREPLUGIN_API RenderPlugin* OGL4COREPLUGIN_CALL CreateInstance(COGL4CoreAPI* Api) {
	return new HelloCube(Api);
}
