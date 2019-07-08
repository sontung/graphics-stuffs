/*
 * HelloCube.cpp
 *
 * Copyright (c) 2014 by Universitaet Stuttgart (VISUS).
 * All rights reserved.
 *
 * This file is part of OGL4Core.
 */

#include "stdafx.h"
#include "HelloCube.h"
#include "glm/gtc/matrix_transform.hpp"

const static float  INIT_CAMERA_DOLLY = 5.0f;
const static float  INIT_CAMERA_FOVY = 45.0f;

const static float  MIN_CAMERA_PITCH = -89.99f;
const static float  MAX_CAMERA_PITCH = 89.99f;

// Vertex and Fragment shader code

// --------------------------------------------------
//  These are only dummy shaders and have to be 
//  replaced by your own written shaders.
// --------------------------------------------------
const GLchar* minimalVertexShaderCode = R"glsl(
    #version 330 
	layout(location = 0) in vec4 in_position; 
	uniform mat4 mvp; 
	out vec4 tex; 
    void main() { 
	    gl_Position = mvp*in_position; 
	    tex = (in_position+vec4(1.0))/vec4(2.0);
    }
)glsl";

const GLchar* minimalFragmentShaderCode = R"glsl(
    #version 330
    out vec4 fragColor;
	in vec4 tex; 
	uniform float how_many_checkers;
    void main() { 
		float total;

		// to have checker pattern, we need to rescale tex coord from [0.25, 0.75] to [0, 3] with 3 is the number of tile for example
		// next we only need to use floor and mod to decide which black or white it should be.
		total = floor((tex.x-0.25)*2*how_many_checkers) + floor((tex.y-0.25)*2*how_many_checkers) + floor((tex.z-0.25)*2*how_many_checkers);
		if (mod(total, 2.0) == 0)  fragColor = vec4(0.0, 0.0, 0.0, 1.0);
		else fragColor = vec4(1.0, 1.0, 1.0, 1.0);
        fragColor *= tex;
    }
)glsl";

// --------------------------------------------------    
//  Number of cube vertices: numCubeVertices
//  Vertex data for cube:    cubeVertices
//
//  To render the cube faces use vertex indices!
// --------------------------------------------------
const static int numCubeVertices = 8;

const static GLfloat cubeVertices[] = {
	-0.5f,-0.5f,-0.5f,1.0f, // 0
	 0.5f,-0.5f,-0.5f,1.0f, // 1
	 0.5f, 0.5f,-0.5f,1.0f, // 2
	-0.5f, 0.5f,-0.5f,1.0f, // 3
	-0.5f,-0.5f, 0.5f,1.0f, // 4
	 0.5f,-0.5f, 0.5f,1.0f, // 5
	 0.5f, 0.5f, 0.5f,1.0f, // 6
	-0.5f, 0.5f, 0.5f,1.0f, // 7
};

const static GLuint cubeFaces[] = {
	3,1,0,
	2,3,1,
	0,7,4,
	7,3,0,
	7,5,6,
	7,5,4,
	2,1,5,
	2,6,5,
	7,3,2,
	6,2,7,
	4,1,0,
	4,1,5
};


/**
 * @brief HelloCube constructor
 */
HelloCube::HelloCube(COGL4CoreAPI* Api) : RenderPlugin(Api),
shaderProg(0),
aspectRatio(1.0f) {
	this->myName = "PCVC/HelloCube";
	this->myDescription = "First assignment of PCVC";
}

/**
 * @brief HelloCube destructor
 */
HelloCube::~HelloCube() {
}

/*
Callback for changing background menu
*/
void HelloCube::bg_color_func(APIVar<HelloCube, Color3FVarPolicy>& var) {
	printf("changing bg color \n");
	glClearColor(var.GetValue().r, var.GetValue().g, var.GetValue().b, 1.0f);
	return;
};

/*
Callback for transform menu
*/
void HelloCube::transform_mode_func(EnumVar<HelloCube>& var) {
	printf("changing transform mode from %d to %d\n", transformation_mode, var.GetValue());
	transformation_mode = var.GetValue();
	return;
}

/*
Callback for reset button
*/
void HelloCube::reset_mode_func(ButtonVar<HelloCube>& var) {
	printf("reset orientation\n");
	translate_vec = glm::vec3(0.0f, 0.0f, 0.0f);
	scale_vec = glm::vec3(1.0f, 1.0f, 1.0f);
	angleX = 0;
	angleY = 0;
	angleZ = 0;
	pitch = 180.0f;
	yaw = 90.0f;
	fovy = 45.0f;
	dolly = -3.0f;
	return;
}

/*
Callback for changing how many checkers
*/
void HelloCube::change_number_of_checkers_func(APIVar<HelloCube, UInt16VarPolicy>& var) {
	printf("changing number of checkers from %f to %d\n", number_of_checkers[0], var.GetValue());
	number_of_checkers[0] = (float)var.GetValue();
	return;
}


GLuint* HelloCube::get_vbo() {
	return &vbo;
}

GLuint* HelloCube::get_ibo() {
	return &ibo;
}

GLint* HelloCube::get_checkers() {
	return &checkers;
}

/**
 * @brief HelloCube activate method
 */
bool HelloCube::Activate(void) {
	// --------------------------------------------------
	//  Initialize API variables here
	// --------------------------------------------------
	bgcolor.Set(this, "Background Color", &HelloCube::bg_color_func);
	glm::vec3 initial_color = glm::vec3(255, 255, 255);
	bgcolor.SetValue(initial_color);
	bgcolor.Register();
	EnumPair transform_choices[] = { {0, "translation"}, {1, "scale"}, {2, "rotation"}, {3, "camera"} };
	transform_mode.Set(this, "Mode", transform_choices, 4, &HelloCube::transform_mode_func);
	transform_mode.Register();
	reset_mode.Set(this, "Reset", &HelloCube::reset_mode_func);
	reset_mode.Register();
	change_number_of_checkers.Set(this, "N.o checkers", &HelloCube::change_number_of_checkers_func);
	change_number_of_checkers.SetValue((int)number_of_checkers[0]);
	change_number_of_checkers.Register();


	// --------------------------------------------------
	//  Generate vertex array and vertex buffer objects
	//  for the cube rendering.
	// --------------------------------------------------
	GLuint* ptr_to_vbo = HelloCube::get_vbo();
	glGenBuffers(1, ptr_to_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, HelloCube::vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	GLuint* ptr_to_ibo = HelloCube::get_ibo();
	glGenBuffers(1, ptr_to_ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, HelloCube::ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeFaces), cubeFaces, GL_STATIC_DRAW);


	// --------------------------------------------------
	//  Initialize shader program
	// --------------------------------------------------
	HelloCube::vertexShader = glCreateShader(GL_VERTEX_SHADER);
	HelloCube::fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(HelloCube::vertexShader, 1, &minimalVertexShaderCode, NULL);
	glCompileShader(HelloCube::vertexShader);

	// Check vertex shader
	GLint Result = GL_FALSE;
	int InfoLogLength;
	glGetShaderiv(HelloCube::vertexShader, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(HelloCube::vertexShader, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(HelloCube::vertexShader, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}
	else printf("Vertex shader compiled.\n");

	glShaderSource(HelloCube::fragmentShader, 1, &minimalFragmentShaderCode, NULL);
	glCompileShader(HelloCube::fragmentShader);

	// Check Fragment Shader
	glGetShaderiv(HelloCube::fragmentShader, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(HelloCube::fragmentShader, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(HelloCube::fragmentShader, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}
	else printf("Fragment shader compiled.\n");

	printf("Linking program\n");
	HelloCube::shaderProg = glCreateProgram();
	glAttachShader(HelloCube::shaderProg, HelloCube::vertexShader);
	glAttachShader(HelloCube::shaderProg, HelloCube::fragmentShader);
	glLinkProgram(HelloCube::shaderProg);

	// Check the program
	glGetProgramiv(HelloCube::shaderProg, GL_LINK_STATUS, &Result);
	glGetProgramiv(HelloCube::shaderProg, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(HelloCube::shaderProg, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}
	else printf("Program compiled.\n");
	glUseProgram(HelloCube::shaderProg);


	// --------------------------------------------------
	//  Set other stuff if necessary
	// --------------------------------------------------   
	MatrixID = glGetUniformLocation(shaderProg, "mvp");
	checkers = glGetUniformLocation(shaderProg, "how_many_checkers");

	glEnable(GL_DEPTH_TEST);
	return true;
}

/*
Check if two matrices are equal, for validating results against glm built ins
*/
bool compare_glm_mat4(glm::mat4 mat1, glm::mat4 mat2) {
	for (int i = 0; i < 4; i++) {
		if (!glm::all(glm::equal(mat1[i], mat2[i]))) {
			printf("%d: %s %s\n", i,
				glm::to_string(mat1[i]).c_str(),
				glm::to_string(mat2[i]).c_str());
			return false;
		}
	}
	return true;
}

/*
Calculate translation matrix
*/
glm::mat4 calculate_trans_mat(glm::vec3 trans_vec) {
	glm::mat4 res = glm::mat4(1.0f);
	res[3] = glm::vec4(trans_vec, 1.0f);

	// checking results against glm::translate
	glm::mat4 ref_res = glm::translate(glm::mat4(1.0f), trans_vec);
	assert(compare_glm_mat4(res, ref_res));
	return res;
}

/*
Calculate scaling matrix
*/
glm::mat4 calculate_scale_mat(glm::vec3 sca_vec) {
	glm::mat4 res = glm::mat4(1.0f);
	res[0] = glm::vec4(sca_vec[0], 0.0f, 0.0f, 0.0f);
	res[1] = glm::vec4(0.0f, sca_vec[1], 0.0f, 0.0f);
	res[2] = glm::vec4(0.0f, 0.0f, sca_vec[2], 0.0f);

	// checking results against glm::scale
	glm::mat4 ref_res = glm::scale(sca_vec);
	if (compare_glm_mat4(res, ref_res)) printf("scaling matrix is correct\n");
	else {
		printf("scaling matrix is incorrect\n");
		printf("%s\n %s\n",
			glm::to_string(res).c_str(),
			glm::to_string(ref_res).c_str());
	}
	assert(compare_glm_mat4(res, ref_res));
	return res;
}

/**
Calculate rotation matrix from this wiki
https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
*/
glm::mat4 calculate_rot_mat(float ang, glm::vec3 rot_axis) {
	glm::mat4 res = glm::mat4(1.0f);
	glm::mat4 ref_res = glm::rotate(ang, rot_axis); // for checking results
	float ux = rot_axis[0];
	float uy = rot_axis[1];
	float uz = rot_axis[2];
	float c = glm::cos(ang);
	float s = glm::sin(ang);
	res[0] = glm::vec4(c + ux * ux * (1 - c), uy * ux * (1 - c) + uz * s, uz * ux * (1 - c) - uy * s, 0.0f);
	res[1] = glm::vec4(ux * uy * (1 - c) - uz * s, c + uy * uy * (1 - c), uz * uy * (1 - c) + ux * s, 0.0f);
	res[2] = glm::vec4(ux * uz * (1 - c) + uy * s, uy * uz * (1 - c) - ux * s, c + uz * uz * (1 - c), 0.0f);

	// checking result against glm::rotate
	if (compare_glm_mat4(res, ref_res)) printf("rotation matrix is correct\n");
	else {
		printf("rotation matrix is incorrect\n");
		printf("%s\n %s\n",
			glm::to_string(res).c_str(),
			glm::to_string(ref_res).c_str());
	}
	assert(compare_glm_mat4(res, ref_res));
	return res;
}

/*
Calculate model matrix
*/
void HelloCube::calculate_model_matrix() {
	if (transformation_mode == 0) {
		objectTranslateMatrix = calculate_trans_mat(translate_vec);
	}
	else if (transformation_mode == 1) {
		objectScaleMatrix = calculate_scale_mat(scale_vec);
	}
	else if (transformation_mode == 2) {
		objectRotateMatrix = calculate_rot_mat(angleZ, glm::vec3(0.0f, 0.0f, 1.0f)) *
			calculate_rot_mat(angleY, glm::vec3(0.0f, 1.0f, 0.0f)) *
			calculate_rot_mat(angleX, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	modelMatrix = objectTranslateMatrix * objectRotateMatrix * objectScaleMatrix;
}

/**
 * @brief HelloCube render method
 */
bool HelloCube::Render(void) {
	printf("Rendering with %f checkers ...\n", number_of_checkers[0]);
	glClear(GL_COLOR_BUFFER_BIT ^ GL_DEPTH_BUFFER_BIT);
	calculate_model_matrix();
	setProjMatrix();
	setCameraView();
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(projectionMatrix * viewMatrix * modelMatrix));

	glUniform1f(checkers, number_of_checkers[0]);


	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, HelloCube::vbo);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		4,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, HelloCube::ibo);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
	glDisableVertexAttribArray(0);
	return false;
}

/**
 * @brief HellCube mouse motion callback function
 * @param x
 * @param y
 */
bool HelloCube::Motion(int x, int y) {
	// --------------------------------------------------
	//  Camera and object control
	// --------------------------------------------------
	int xpos, ypos;
	int dx, dy;
	GetLastMousePos(xpos, ypos);
	dx = xpos - x;
	dy = ypos - y;
	if (IsLeftButtonPressed()) {
		if (transformation_mode == 0) {
			translate_vec.x += dx * 0.001;
			translate_vec.y += dy * 0.001;
		}
		else if (transformation_mode == 1) {
			scale_vec.x += dx * 0.001;
			scale_vec.y += dy * 0.001;
		}
		else if (transformation_mode == 2) {
			angleX += dy * 0.01;
			angleY -= dx * 0.01;

		}
		else if (transformation_mode == 3) {
			pitch += dy * 0.1;
			yaw -= dx * 0.1;
		}
	}
	else if (IsRightButtonPressed()) {
		if (transformation_mode == 0) {
			translate_vec.y -= dx * 0.001;
			translate_vec.z += dy * 0.001;
		}
		else if (transformation_mode == 1) {
			scale_vec.y += dx * 0.001;
			scale_vec.z += dy * 0.001;
		}
		else if (transformation_mode == 2) {
			angleZ += dy * 0.01;
			angleY += dx * 0.01;
		}
		else if (transformation_mode == 3) {
			if (dy < 0 && dolly > -10) dolly += dy * 0.1;
			else if (dy > 0 && -2 > dolly) dolly += dy * 0.1;
		}
	}
	else if (IsMidButtonPressed()) {
		if (transformation_mode == 0) {
			translate_vec.x -= dx * 0.001;
			translate_vec.z += dy * 0.001;
		}
		else if (transformation_mode == 1) {
			scale_vec.x -= dx * 0.001;
			scale_vec.z += dy * 0.001;
		}
		else if (transformation_mode == 2) {
			angleX += dx * 0.01;
			angleZ -= dy * 0.01;
		}
		else if (transformation_mode == 3) {
			fovy += dy * 0.1;
		}
	}
	printf("from mouse motion %d %d %f %f\n", xpos, ypos, HelloCube::translate_vec.x, HelloCube::translate_vec.y);

	// --------------------------------------------------
	//  After a mouse interaction, the OpenGL window
	//  has to be updated.
	// --------------------------------------------------   
	PostRedisplay();
	return false;
}

/**
 * @brief HelloCube deactive method
 */
bool HelloCube::Deactivate(void) {
	glDisable(GL_DEPTH_TEST);

	// --------------------------------------------------
	// Do not forget to cleanup the plugin!
	// --------------------------------------------------
	glDetachShader(HelloCube::shaderProg, HelloCube::vertexShader);
	glDetachShader(HelloCube::shaderProg, HelloCube::fragmentShader);

	glDeleteShader(HelloCube::vertexShader);
	glDeleteShader(HelloCube::fragmentShader);
	glDeleteBuffers(1, HelloCube::get_vbo());
	glDeleteBuffers(1, HelloCube::get_ibo());
	glDeleteProgram(HelloCube::shaderProg);
	return true;
}

/**
 * @brief HelloCube initialization method
 */
bool HelloCube::Init(void) {
	if (gl3wInit()) {
		fprintf(stderr, "Error: Failed to initialize gl3w.\n");
		return false;
	}
	return true;
}

/**
 * @brief HelloCube keyboard callback function
 * @param key
 * @param x
 * @param y
 */
bool HelloCube::Keyboard(int key, int action, int mods, int x, int y) {
	return false;
}


/**
 * @brief HelloCube mouse callback function
 * @param button
 * @param state
 * @param x
 * @param y
 */
bool HelloCube::Mouse(int button, int state, int mods, int x, int y) {
	int xpos, ypos;
	GetLastMousePos(xpos, ypos);
	return false;
}

bool HelloCube::PassiveMotion(int x, int y) {
	return false;
}

/**
 * @brief HelloCube resize method
 * @param width
 * @param height
 */
bool HelloCube::Resize(int width, int height) {
	return true;
}

bool HelloCube::setShaderProgram() {
	return false;
}

float dot(glm::vec3 & u, glm::vec3 & v) {
	float sum = 0.0f;
	for (int i = 0; i < 3; i++) sum += u[i] * v[i];
	return sum;
}

/*
Normalize a vector
*/
glm::vec3 normalize(glm::vec3 & u) {
	float sum = dot(u, u);
	sum = sqrt(sum);
	glm::vec3 res;
	glm::vec3 ref_res = glm::normalize(u);
	for (int i = 0; i < 3; i++) res[i] = u[i] / sum;
	return res;
}

/*
Negate a vector
*/
glm::vec3 negate(glm::vec3 & u) {
	glm::vec3 res;
	for (int i = 0; i < 3; i++) res[i] = -u[i];
	return res;
}

/*
Cross product between 2 vectors
*/
glm::vec3 cross(glm::vec3 & u, glm::vec3 & v) {
	glm::vec3 result = glm::vec3(
		u[1] * v[2] - u[2] * v[1],
		u[2] * v[0] - u[0] * v[2],
		u[0] * v[1] - u[1] * v[0]
	);
	return result;
}

/*
Calculate lookat matrix
reference: https://learnopengl.com/Getting-started/Camerahttps://learnopengl.com/Getting-started/Camera
*/
glm::mat4 lookat(glm::vec3 & eye, glm::vec3 & at, glm::vec3 & up) {
	glm::vec3 f = normalize(at - eye);
	glm::vec3 u = normalize(up);
	glm::vec3 s = normalize(cross(f, u));
	u = cross(s, f);

	glm::mat4 res = glm::mat4(1.0f);
	res[0][0] = s.x;
	res[1][0] = s.y;
	res[2][0] = s.z;
	res[0][1] = u.x;
	res[1][1] = u.y;
	res[2][1] = u.z;
	res[0][2] = -f.x;
	res[1][2] = -f.y;
	res[2][2] = -f.z;
	res[3][0] = -dot(s, eye);
	res[3][1] = -dot(u, eye);
	res[3][2] = dot(f, eye);

	// validating result
	glm::mat4 ref_res = glm::lookAt(eye, at, up);
	if (compare_glm_mat4(res, ref_res)) printf("view matrix is correct\n");
	else {
		printf("view matrix is incorrect\n");
		printf("%s\n%s\n",
			glm::to_string(res).c_str(),
			glm::to_string(ref_res).c_str());
	}
	assert(compare_glm_mat4(res, ref_res));
	return res;
}

void HelloCube::setCameraView() {
	viewMatrix = glm::mat4(1.0f);

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, dolly);
	glm::vec3 cameraFront;
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = normalize(front);
	viewMatrix = lookat(cameraPos, cameraFront, cameraUp);
	printf("dolly=%f pitch=%f yaw=%f\n", dolly, pitch, yaw);
	printf("camera front %s\n", glm::to_string(cameraFront).c_str());
}

/*
Calculate perspective projection matrix
*/
glm::mat4 perspective(float fovy, float aspect, float near_plane, float far_plane) {
	float f = 1.0 / glm::tan(fovy / 2);
	float d = far_plane - near_plane;

	glm::mat4 res = glm::mat4(1.0f);
	res[0][0] = f / aspect;
	res[1][1] = f;
	res[2][2] = -(near_plane + far_plane) / d;
	res[2][3] = -2 * near_plane * far_plane / d;
	res[3][2] = -1;
	res[3][3] = 0.0;
	res = glm::transpose(res);
	glm::mat4 ref_res = glm::perspective(fovy, aspect, near_plane, far_plane);

	// validating result
	if (compare_glm_mat4(res, ref_res)) printf("proj matrix is correct\n");
	else {
		printf("proj matrix is incorrect\n");
		printf("%s\n%s\n",
			glm::to_string(res).c_str(),
			glm::to_string(ref_res).c_str());
	}
	assert(compare_glm_mat4(res, ref_res));
	return res;
}

void HelloCube::setProjMatrix() {
	// Set the projection matrix using camera field-of-view,
	// the aspect ratio of the window, and the near- and far-
	// clipping distances.
	printf("aspect ratio = %f, fovy=%f\n", aspectRatio, fovy);
	projectionMatrix = perspective(glm::radians(fovy), aspectRatio, 0.00001f, 100.0f);

}
