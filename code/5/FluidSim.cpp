/*
 * FluidSim.cpp
 *
 * Copyright (c) 2014 by Universitaet Stuttgart (VISUS).
 * All rights reserved.
 *
 * This file is part of OGL4Core.
 */

#include "stdafx.h"
#include "FluidSim.h"
#include "Constants.h"
#include "DatRawLoader.h"
#include "GLHelpers.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Defs.h"

#include <climits>
#include <cfloat>


#ifndef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#endif

FluidSim::FluidSim(COGL4CoreAPI* Api) : RenderPlugin(Api),
wWidth(2048), wHeight(2048) {
	this->myName = "PCVC/VolumeVis";
	this->myDescription = "Third assignment of PCVC";
}

FluidSim::~FluidSim() {
	Deactivate();
}

// initial setup
bool FluidSim::Activate(void) {
	// --------------------------------------------------
	//  Get the path name of the plugin folder, e.g.
	//  "/path/to/oglcore/Plugins/PCVC/FluidSim"
	// --------------------------------------------------
	pathName = this->GetCurrentPluginPath();

	// Initialize manipulator for camera view
	camHandle = this->AddManipulator("View", &this->viewMX, Manipulator::MANIPULATOR_ORBIT_VIEW_3D);
	this->SelectCurrentManipulator(camHandle);
	this->SetManipulatorRotation(camHandle, glm::vec3(1, 0, 0), 0.0f);
	this->SetManipulatorDolly(camHandle, -2.0f);

	// --------------------------------------------------
	//  TODO: Initialize API variables here
	// --------------------------------------------------

	EnumPair ep[] = { {0,"raycasting"},{1,"rasterization"} };
	mode.Set(this, "Mode", ep, 2, &FluidSim::modeChanged);
	mode.Register();
	mode = 1;

	EnumPair ep2[] = { {0,"DamBreak"},{1,"Emitter"} };
	dataset.Set(this, "DataSet", ep2, 2, &FluidSim::datasetChanged);
	dataset.Register();
	dataset = 1;

	fovY.Set(this, "FoVy");
	fovY.Register();
	fovY.SetMinMax(5.0f, 90.0f);
	fovY.SetVisible(false);
	fovY = 45.0f;

	sphere_radius.Set(this, "Radius");
	sphere_radius.Register();
	sphere_radius.SetStep(0.001f);
	sphere_radius = 0.01f;

	theta_cam.Set(this, "Theta");
	theta_cam.Register();
	theta_cam.SetMinMax(0.0f, M_PI);
	theta_cam = 2.35f;
	theta_cam.SetStep(0.01f);

	phi_cam.Set(this, "Phi");
	phi_cam.Register();
	phi_cam.SetMinMax(0.0f, 2 * M_PI);
	phi_cam = 0.48f;
	phi_cam.SetStep(0.01f);

	visual_eff.Set(this, "VisualEffect");
	visual_eff.Register();
	visual_eff.SetMinMax(0.0f, 1.0f);
	visual_eff = 1.0f;
	visual_eff.SetStep(0.01f);

	res_x.Set(this, "ResX");
	res_x.Register();
	res_x.SetMinMax(8, 7200);
	res_x = 640;
	res_x.SetStep(8);

	res_y.Set(this, "ResY");
	res_y.Register();
	res_y.SetMinMax(8, 7200);
	res_y = 640;
	res_y.SetStep(8);

	auto_rotate.Set(this, "AutoRotate");
	auto_rotate.Register();
	auto_rotate = true;

	auto_change_ts.Set(this, "AutoChangeTimeStep");
	auto_change_ts.Register();
	auto_change_ts = false;

	show_skybox.Set(this, "ShowSkyBox");
	show_skybox.Register();
	show_skybox = false;

	show_unitbox.Set(this, "ShowUnitBox");
	show_unitbox.Register();
	show_unitbox = false;

	show_iso.Set(this, "ShowIsosurface");
	show_iso.Register();
	show_iso = false;

	show_spheres.Set(this, "ShowSpheres");
	show_spheres.Register();
	show_spheres = false;

	// box rendering
	boxVertShaderName = pathName + std::string("/resources/box.vert");
	boxFragShaderName = pathName + std::string("/resources/box.frag");
	shaderBox.CreateProgramFromFile(boxVertShaderName.c_str(), boxFragShaderName.c_str());

	// cube rendering (debugging)
	boxVertShaderName = pathName + std::string("/resources/cube.vert");
	boxFragShaderName = pathName + std::string("/resources/cube.frag");
	shaderCube.CreateProgramFromFile(boxVertShaderName.c_str(), boxFragShaderName.c_str());

	// particle rendering
	particleVertShaderName = pathName + std::string("/resources/particles.vert");
	particleFragShaderName = pathName + std::string("/resources/particles.frag");
	shaderParticle.CreateProgramFromFile(particleVertShaderName.c_str(), particleFragShaderName.c_str());

	// sphere rendering
	std::string vsSphere = pathName + std::string("/resources/sphere.vert");
	std::string fsSphere = pathName + std::string("/resources/sphere.frag");
	std::string gsSphere = pathName + std::string("/resources/sphere.geom");
	shaderSphere.CreateProgramFromFile(vsSphere.c_str(), gsSphere.c_str(), fsSphere.c_str());

	// grid rendering
	boxVertShaderName = pathName + std::string("/resources/grid.vert");
	boxFragShaderName = pathName + std::string("/resources/grid.frag");
	shaderGrid.CreateProgramFromFile(boxVertShaderName.c_str(), boxFragShaderName.c_str());

	// iso rendering
	boxVertShaderName = pathName + std::string("/resources/iso.vert");
	boxFragShaderName = pathName + std::string("/resources/iso.frag");
	shaderIsosurface.CreateProgramFromFile(boxVertShaderName.c_str(), boxFragShaderName.c_str());
	quadVertShaderName = pathName + std::string("/resources/quad.vert");
	quadFragShaderName = pathName + std::string("/resources/quad_iso.frag");
	shaderIsosurfaceRayTracing.CreateProgramFromFile(quadVertShaderName.c_str(), quadFragShaderName.c_str());

	// quad rendering
	quadVertShaderName = pathName + std::string("/resources/quad.vert");
	quadFragShaderName = pathName + std::string("/resources/quad.frag");
	shaderQuad.CreateProgramFromFile(quadVertShaderName.c_str(), quadFragShaderName.c_str());
	vaQuad.Create(4);
	vaQuad.SetArrayBuffer(0, GL_FLOAT, 2, ogl4_2dQuadVerts);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);

	glGenBuffers(1, &ssbo_particles_pos);
	glGenBuffers(1, &ssbo_grid_if_near_particles);
	glGenBuffers(1, &ssbo_triangles_pos);

	glGenBuffers(1, &vaIsosurfaceID);

	modeChanged(mode);
	datasetChanged(dataset);

	printf("done loading dataset.\n");

	generate_possible_cam_pos();
	printf("done generating camera positions.\n");

	createSphere();
	printf("done creating spheres.\n");

	loadSkybox();
	printf("done loading skybox.\n");
	return true;
}

// helper for loading sky box
GLuint loadCubemap(std::vector<std::string> faces) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrComponents;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

// load skybox
void FluidSim::loadSkybox() {
	float skyboxVertices[] = {
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};
	vaSkybox.Create(sizeof(skyboxVertices) / sizeof(float));
	vaSkybox.SetArrayBuffer(0, GL_FLOAT, 3, skyboxVertices);

	// load skybox tex
	std::string pathName = this->GetCurrentPluginPath();
	std::vector<std::string> faces
	{
		pathName + "resources/skybox/sea/right.jpg",
		pathName + "resources/skybox/sea/left.jpg",
		pathName + "resources/skybox/sea/top.jpg",
		pathName + "resources/skybox/sea/bottom.jpg",
		pathName + "resources/skybox/sea/front.jpg",
		pathName + "resources/skybox/sea/back.jpg"
	};
	cube_map_tex = loadCubemap(faces);

	// a cube for debugging
	float cubeVertices[] = {
		// positions          // normals
		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,

		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,

		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,

		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,
	};
	vaCube.Create(36);
	vaCube.SetArrayBuffer(0, GL_FLOAT, 3, cubeVertices);

}

bool FluidSim::Deactivate(void) {
	// --------------------------------------------------
	//  TODO: Do not forget to clear all allocated sources.
	// --------------------------------------------------
	glDisable(GL_PROGRAM_POINT_SIZE);
	glDisable(GL_DEPTH_TEST);
	vaBox.Delete();
	vaParticle.Delete();
	vaSphere.Delete();
	vaSkybox.Delete();
	vaCube.Delete();
	vaGrid.Delete();

	glDeleteBuffers(1, &ssbo_particles_pos);
	glDeleteBuffers(1, &ssbo_grid_if_near_particles);
	glDeleteBuffers(1, &ssbo_triangles_pos);

	glDeleteBuffers(1, &vaIsosurfaceID);

	glDeleteTextures(1, &cube_map_tex);
	glDeleteTextures(1, &fboID);
	glDeleteTextures(3, colAttachID);


	shaderCube.RemoveAllShaders();
	shaderQuad.RemoveAllShaders();
	shaderSphere.RemoveAllShaders();
	shaderBox.RemoveAllShaders();
	shaderParticle.RemoveAllShaders();
	shaderGrid.RemoveAllShaders();
	shaderIsosurface.RemoveAllShaders();

	particles_position.clear();
	particles_position.shrink_to_fit();
	grid_positions.clear();
	grid_positions.shrink_to_fit();
	grid_if_near_particles.clear();
	grid_if_near_particles.shrink_to_fit();
	grid3dpos2isoval.clear();

	all_triangles.clear();
	triangle_size_each_time_step.clear();
	all_cubes.clear();
	all_triangles.shrink_to_fit();
	triangle_size_each_time_step.shrink_to_fit();
	all_cubes.shrink_to_fit();

	for (int i = 0; i < VA_timesteps.size(); i++) {
		VA_timesteps[i].Delete();
	}
	VA_timesteps.clear();
	VA_timesteps.shrink_to_fit();
	return true;
}

bool FluidSim::Init(void) {
	if (gl3wInit()) {
		fprintf(stderr, "Error: Failed to initialize gl3w.\n");
		return false;
	}
	return true;
}

bool FluidSim::Keyboard(int key, int action, int mods, int x, int y) {
	std::string pathName = this->GetCurrentPluginPath();
	PostRedisplay();
	return false;
}

bool FluidSim::Motion(int x, int y) {
	return false;
}

bool FluidSim::Mouse(int button, int state, int mods, int x, int y) {
	PostRedisplay();
	return false;
}

bool FluidSim::Render(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (auto_change_ts) time_step += step_size;
	int index = (int)std::floor(time_step);

	if (time_step > max_time_step + 1) {
		time_step = (float)min_time_step;
		light_pos = glm::vec3(((float)rand() / (RAND_MAX)), ((float)rand() / (RAND_MAX)), ((float)rand() / (RAND_MAX)));
		return false;
	}

	glViewport(0, 0, wWidth, wHeight);
	viewAspect = wWidth / static_cast<float>(wHeight);

	projMX = glm::perspective(glm::radians(static_cast<float>(fovY)), viewAspect, 0.01f, 100.0f);
	
	// another cam system
	glm::vec3 lookat = glm::vec3(0, 0, -2);
	glm::vec3 eye = lookat + compute_cam_pos();
	glm::vec3 n = glm::normalize(eye - lookat);
	glm::vec3 v_up = glm::vec3(0, 1, 0);
	glm::vec3 u = glm::normalize(glm::cross(n, v_up));
	glm::vec3 v = glm::cross(n, u);

	// image plane ref: (http://web.cse.ohio-state.edu/~shen.94/681/Site/Slides_files/basic_algo.pdf)
	float distace_from_cam_to_imageplane = 1.0f;
	float H = 2 * distace_from_cam_to_imageplane * glm::tan(glm::radians(fovY / 2.0f));
	float W = H * viewAspect;
	glm::vec3 C_pos = eye - n * distace_from_cam_to_imageplane;
	glm::vec3 L_pos = C_pos - u * W / 2.0f - v * H / 2.0f;
	int plane_res_x = static_cast<int>(res_x);
	int plane_res_y = static_cast<int>(res_y);
	float pixel_width = W / (float)plane_res_x;
	float pixel_height = H / (float)plane_res_y;

	// particles' positions at this time step
	int first_ind2 = 0;
	int last_ind2;
	for (int k = 0; k < index - 1; k++) {
		first_ind2 += nb_particles[k]*3;
	}
	last_ind2 = first_ind2 + nb_particles[index - 1]*3;
	printf("time step %d: particles size = %d\n", index - 1, last_ind2-first_ind2);

	std::vector<float>::const_iterator first = particles_position.begin() + first_ind2;
	std::vector<float>::const_iterator last = particles_position.begin() + last_ind2;
	std::vector<float> sub_vec(first, last);

	// isosurface triangles' positions at this time step
	int first_ind = 0;
	int last_ind;
	for (int k = 0; k < index - 1; k++) {
		first_ind += triangle_size_each_time_step[k];
	}
	last_ind = first_ind + triangle_size_each_time_step[index - 1];
	std::vector<float>::const_iterator first1 = all_triangles.begin() + first_ind;
	std::vector<float>::const_iterator last1 = all_triangles.begin() + last_ind;
	std::vector<float> triangles_this_time_step(first1, last1);
	printf("           : triangles size = %d\n", index - 1, triangles_this_time_step.size());

	// unitbox
	if (show_unitbox) {
		shaderCube.Bind();
		vaBox.Bind();
		glUniformMatrix4fv(shaderSphere.GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(viewMX));
		glUniformMatrix4fv(shaderSphere.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));
		glDrawElements(GL_LINES, ogl4_numBoxEdges * 2, GL_UNSIGNED_INT, 0);
		vaBox.Release();
		shaderCube.Release();
	}

	   /* shaderGrid.Bind();
		vaGrid.Bind();
		glUniform1i(shaderGrid.GetUniformLocation("time_step"), index - 1);
		glUniform1i(shaderGrid.GetUniformLocation("nb_time_steps"), max_time_step);
		glUniformMatrix4fv(shaderGrid.GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(viewMX));
		glUniformMatrix4fv(shaderGrid.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_grid_if_near_particles);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * grid_if_near_particles.size(), grid_if_near_particles.data(), GL_STATIC_READ);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_grid_if_near_particles);
		glDrawArrays(GL_POINTS, 0, vaGrid.GetNumVertices() / 4);
		vaGrid.Release();
		shaderGrid.Release();*/

	// skybox
	if (show_skybox) {
		glDepthFunc(GL_LEQUAL);
		shaderBox.Bind();
		vaSkybox.Bind();

		glUniformMatrix4fv(shaderBox.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));
		glUniformMatrix4fv(shaderBox.GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(glm::mat4(glm::mat3(viewMX))));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map_tex);
		glUniform1i(glGetUniformLocation(shaderBox.GetProgHandle(), "skybox"), 0);

		glDrawArrays(GL_TRIANGLES, 0, 36);

		vaSkybox.Release();
		shaderBox.Release();
		glDepthFunc(GL_LESS);
	}

	// ray casting
	if (mode == 0) {
		if (show_iso) {

			shaderIsosurfaceRayTracing.Bind();
			vaQuad.Bind();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, colAttachID[0]);
			glUniform1i(glGetUniformLocation(shaderIsosurfaceRayTracing.GetProgHandle(), "skybox"), 0);

			glUniform1i(shaderIsosurfaceRayTracing.GetUniformLocation("nb_triangles"), triangles_this_time_step.size());
			glUniform1i(shaderIsosurfaceRayTracing.GetUniformLocation("res_x"), plane_res_x);
			glUniform1i(shaderIsosurfaceRayTracing.GetUniformLocation("res_y"), plane_res_y);
			glUniform1f(shaderIsosurfaceRayTracing.GetUniformLocation("pixel_width"), pixel_width);
			glUniform1f(shaderIsosurfaceRayTracing.GetUniformLocation("pixel_height"), pixel_height);
			glUniform1f(shaderIsosurfaceRayTracing.GetUniformLocation("radius"), static_cast<float>(sphere_radius));
			glUniform1f(shaderIsosurfaceRayTracing.GetUniformLocation("visual_eff"), static_cast<float>(visual_eff));

			glUniform3f(shaderIsosurfaceRayTracing.GetUniformLocation("L_pos"), L_pos.x, L_pos.y, L_pos.z);
			glUniform3f(shaderIsosurfaceRayTracing.GetUniformLocation("u"), u.x, u.y, u.z);
			glUniform3f(shaderIsosurfaceRayTracing.GetUniformLocation("v"), v.x, v.y, v.z);
			glUniform3f(shaderIsosurfaceRayTracing.GetUniformLocation("cam_pos"), eye.x, eye.y, eye.z);
			glUniform3f(shaderIsosurfaceRayTracing.GetUniformLocation("light_pos"), light_pos.x, light_pos.y, light_pos.z);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_triangles_pos);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * triangles_this_time_step.size(), triangles_this_time_step.data(), GL_STATIC_READ);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_triangles_pos);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			vaQuad.Release();
			shaderIsosurfaceRayTracing.Release();
		}
		if (show_spheres) {
			shaderQuad.Bind();
			vaQuad.Bind();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, colAttachID[0]);
			glUniform1i(glGetUniformLocation(shaderQuad.GetProgHandle(), "skybox"), 0);

			glUniform1i(shaderQuad.GetUniformLocation("nb_particles"), sub_vec.size());
			glUniform1i(shaderQuad.GetUniformLocation("res_x"), plane_res_x);
			glUniform1i(shaderQuad.GetUniformLocation("res_y"), plane_res_y);
			glUniform1f(shaderQuad.GetUniformLocation("pixel_width"), pixel_width);
			glUniform1f(shaderQuad.GetUniformLocation("pixel_height"), pixel_height);
			glUniform1f(shaderQuad.GetUniformLocation("radius"), static_cast<float>(sphere_radius));


			glUniform3f(shaderQuad.GetUniformLocation("L_pos"), L_pos.x, L_pos.y, L_pos.z);
			glUniform3f(shaderQuad.GetUniformLocation("u"), u.x, u.y, u.z);
			glUniform3f(shaderQuad.GetUniformLocation("v"), v.x, v.y, v.z);
			glUniform3f(shaderQuad.GetUniformLocation("cam_pos"), eye.x, eye.y, eye.z);
			glUniform3f(shaderQuad.GetUniformLocation("light_pos"), light_pos.x, light_pos.y, light_pos.z);


			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles_pos);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * sub_vec.size(), sub_vec.data(), GL_STATIC_READ);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_particles_pos);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			vaQuad.Release();
			shaderQuad.Release();
		}
	}
	else if (mode == 1) {
		if (show_spheres) {
			shaderSphere.Bind();
			vaSphere.Bind();
			glUniformMatrix4fv(shaderSphere.GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(viewMX));
			glUniformMatrix4fv(shaderSphere.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));
			for (int i = 0; i < sub_vec.size(); i += 3) {
				glm::mat4 model_mx = glm::translate(glm::mat4(1.0f),
					glm::vec3(
						sub_vec[i],
						sub_vec[i + 1],
						sub_vec[i + 2]));
				glUniformMatrix4fv(shaderSphere.GetUniformLocation("modelMX"), 1, GL_FALSE, glm::value_ptr(model_mx));
				glDrawElements(GL_TRIANGLES, sphere_ibo_size, GL_UNSIGNED_INT, nullptr);
			}
			vaSphere.Release();
			shaderSphere.Release();
		} 
		if (show_iso) {
			// splitting into chunks
			std::vector<float>::iterator from = triangles_this_time_step.begin();
			std::vector<float> subList;
			int blockSize = 999;
			while (!triangles_this_time_step.empty()) {
				if (triangles_this_time_step.end() - from > blockSize) {
					subList.assign(from, from + blockSize);
					from += blockSize;

					glBindBuffer(GL_ARRAY_BUFFER, vaIsosurfaceID);
					glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT) * subList.size(), subList.data(), GL_STATIC_DRAW);
					shaderIsosurface.Bind();
					glEnableVertexAttribArray(0);
					glBindBuffer(GL_ARRAY_BUFFER, vaIsosurfaceID);
					glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
					glUniformMatrix4fv(shaderIsosurface.GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(viewMX));
					glUniformMatrix4fv(shaderIsosurface.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));
					glDrawArrays(GL_TRIANGLES, 0, subList.size() / 3);
					glDisableVertexAttribArray(0);
					shaderIsosurface.Release();

				}
				else {
					subList.assign(from, triangles_this_time_step.end());

					glBindBuffer(GL_ARRAY_BUFFER, vaIsosurfaceID);
					glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT) * subList.size(), subList.data(), GL_STATIC_DRAW);
					shaderIsosurface.Bind();
					glEnableVertexAttribArray(0);
					glBindBuffer(GL_ARRAY_BUFFER, vaIsosurfaceID);
					glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
					glUniformMatrix4fv(shaderIsosurface.GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(viewMX));
					glUniformMatrix4fv(shaderIsosurface.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));
					glDrawArrays(GL_TRIANGLES, 0, subList.size() / 3);
					glDisableVertexAttribArray(0);
					shaderIsosurface.Release();

					triangles_this_time_step.clear();
					subList.clear();
				}
			}
		}

	}
	
	// Some UI visibility
	if (show_iso) {
		sphere_radius.SetVisible(false);
	}
	else {
		sphere_radius.SetVisible(true);
	}
	return false;
}

// samples all possible camera coordinates as spherical coordinates for auto rotation mode
void FluidSim::generate_possible_cam_pos() {
	float radius = 4.0f;
	float x, y, z, xy;                              // vertex position

	const int NUM_SPHERE_RES_THETA = 64;
	const int NUM_SPHERE_RES_PHI = 64;

	float sectorCount = NUM_SPHERE_RES_PHI;
	float stackCount = NUM_SPHERE_RES_THETA;

	float phi_step = 2 * M_PI / NUM_SPHERE_RES_PHI;
	float theta_step = M_PI / NUM_SPHERE_RES_THETA;
	float phi, theta;

	for (int i = 1; i <= NUM_SPHERE_RES_THETA; ++i)
	{
		theta = i * theta_step;                 // starting from 0 to pi
		xy = radius * sinf(theta);             // r * sin(theta)
		z = radius * cosf(theta);              // r * cos(theta)

		for (int j = 0; j <= NUM_SPHERE_RES_PHI; ++j)
		{
			phi = j * phi_step;           // starting from 0 to 2pi
			x = xy * cosf(phi);             // r * sin(theta) * cos(phi)
			y = xy * sinf(phi);             // r * sin(theta) * sin(phi)
			possible_cam_pos.push_back(x);
			possible_cam_pos.push_back(y);
			possible_cam_pos.push_back(z);
			possible_cam_pos.push_back(phi);
			possible_cam_pos.push_back(theta);
		}
	}
}

// computes the current camera coordinate based on theta and phi
glm::vec3 FluidSim::compute_cam_pos() {
	float radius = 4.0f;
	float x, y, z, xy;   

	if (auto_rotate) {
		printf("Auto rotate index = %d/%d\n", cam_pos_index, possible_cam_pos.size()/5);
		glm::vec3 res = glm::vec3(possible_cam_pos[cam_pos_index], possible_cam_pos[cam_pos_index + 1], possible_cam_pos[cam_pos_index + 2]);
		phi_cam = possible_cam_pos[cam_pos_index + 3];
		theta_cam = possible_cam_pos[cam_pos_index + 4];
		cam_pos_index += 5;
		if (cam_pos_index >= possible_cam_pos.size()) cam_pos_index = 0;
		return res;
	}

	xy = radius * sinf(theta_cam);             // r * sin(theta)
	z = radius * cosf(theta_cam);
	x = xy * cosf(phi_cam);             // r * sin(theta) * cos(phi)
	y = xy * sinf(phi_cam);
	return glm::vec3(x, y, z);
}

bool FluidSim::Resize(int width, int height) {
	wWidth = width;
	wHeight = height;
	return true;
}

void FluidSim::modeChanged(EnumVar<FluidSim>& var) {
	int m = var;
	auto_rotate.SetVisible(false);
	theta_cam.SetVisible(false);
	phi_cam.SetVisible(false);
	show_unitbox.SetVisible(false);
	visual_eff.SetVisible(false);
	res_x.SetVisible(false);
	res_y.SetVisible(false);
	if (m == 0) {
		printf("change to ray casting mode\n");
		theta_cam.SetVisible(true);
		phi_cam.SetVisible(true);
		visual_eff.SetVisible(true);
		res_x.SetVisible(true);
		res_y.SetVisible(true);
		auto_rotate.SetVisible(true);
		show_unitbox = false;
	}
	else if (m == 1) {
		printf("change to rasterization mode\n");
		show_unitbox.SetVisible(true);
	}
}

void FluidSim::datasetChanged(EnumVar<FluidSim>& var) {
	printf("changing dataset\n");
	int m = var;
	grid3dpos2isoval.clear();
	particles_position.clear();
	grid_positions.clear();
	grid_if_near_particles.clear();
	all_triangles.clear();
	triangle_size_each_time_step.clear();
	all_cubes.clear();
	cube_size = 0.1;
	if (m == 0) {
		path_to_marching_cube_file = "/resources/sph/dam-25-0.1-0.1.txt";
		path_to_time_step_folder = "/resources/sph/dam25/";
		path_to_time_step_file = "/resources/sph/dam25/ParticleData_Fluid_%d.bgeo.txt";
	}
	else if (m == 1) {
		path_to_marching_cube_file = "/resources/sph/em-50-0.10-0.10.txt";
		path_to_time_step_folder = "/resources/sph/em50/";
		path_to_time_step_file = "/resources/sph/em50/ParticleData_Fluid_%d.bgeo.txt";
	}
	load_grid3d2iso();
	read_sph_data();
	return;
}

void FluidSim::createSphere() {
	// clear memory of prev arrays
	std::vector<float> vertices;
	std::vector<float> texCoords;

	float radius = 0.01f;
	float x, y, z, xy;                              // vertex position
	float s, t;                                     // vertex texCoord

	float min_x, max_x, min_y, max_y, min_z, max_z; // for box rendering

	const int NUM_SPHERE_RES_THETA = 8;
	const int NUM_SPHERE_RES_PHI = 8;

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
	vaSphere.SetElementBuffer(0, indices.size(), indices.data());
	sphere_ibo_size = indices.size();
	printf("va sphere size=%d, ibo size=%d\n", vertices.size(), indices.size());

	vertices.clear();
	texCoords.clear();
	indices.clear();
	vertices.shrink_to_fit();
	texCoords.shrink_to_fit();
	indices.shrink_to_fit();
}

// split a line into vector of floats
std::vector<float> split(std::string s) {
	std::string delimiter = " ";
	std::vector<float> res;
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		res.push_back(std::stof(token));
		s.erase(0, pos + delimiter.length());
	}
	res.push_back(std::stof(s));
	return res;
}

// compare a number to a max and min number for finding min max
void compare(float& min, float& max, float number) {
	if (number < min) min = number;
	else if (number > max) max = number;
}

// search for numbers in a string
int extract_number_from_string(std::string inp) {
	std::smatch match;

	if (std::regex_search(inp, match, std::regex("[^0-9]*([0-9]+).*"))) {
		return std::stoi(match[1]);
	}
	else {
		return -1;
	}
}

// list all files in a folder
std::vector<int> list_all_files(std::string path) {
	DIR* dir;
	struct dirent* ent;
	int min_time_step = -1;
	int max_time_step = -1;
	std::vector<int> res;
	if ((dir = opendir(path.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			//printf("%s\n", ent->d_name);
			int time_step = extract_number_from_string(ent->d_name);
			if (time_step < 0) continue;
			else {
				if (min_time_step < 0) {
					min_time_step = time_step;
					max_time_step = time_step;
				}
				else {
					if (time_step < min_time_step) min_time_step = time_step;
					else if (time_step > max_time_step) max_time_step = time_step;
				}
			}
		}
		closedir(dir);
	}
	printf("Smallest time step=%d, largest time step=%d\n", min_time_step, max_time_step);
	res.push_back(min_time_step);
	res.push_back(max_time_step);
	return res;
}

// load precomputed neighbor search
void FluidSim::load_grid3d2iso() {
	std::string pathName = this->GetCurrentPluginPath();
	std::string filePathName;
	std::string line;

	filePathName = pathName + path_to_marching_cube_file;
	std::ifstream myfile(filePathName);

	printf("reading marching cube file\n");
	// reading text file
	if (myfile.is_open()) {
		float max_a = 0;
		float max_b = 0;
		float max_c = 0;
		float min_d = 0;
		while (getline(myfile, line)) {
			std::vector<float> numbers = split(line);
			std::tuple<int, int, int> key = { (int)numbers[0], (int)numbers[1], (int)numbers[2] };
			std::vector<float>::const_iterator first = numbers.begin() + 3;
			std::vector<float>::const_iterator last = numbers.begin() + numbers.size();
			std::vector<float> val(first, last);
			std::vector<int> val_int(val.begin(), val.end());
			grid3dpos2isoval[key] = val_int;
			compare(min_d, max_a, numbers[0]);
			compare(min_d, max_b, numbers[1]);
			compare(min_d, max_c, numbers[2]);
		}
		printf("max grid index: %f %f %f\n", max_a, max_b, max_c);
		std::map<std::tuple<int, int, int>, std::vector<int>>::iterator it;

		for (it = grid3dpos2isoval.begin(); it != grid3dpos2isoval.end(); it++) {
			std::tuple<int, int, int> k = it->first;
			std::vector<int> v = it->second;
			int t1 = std::get<0>(k);
			int t2 = std::get<1>(k);
			int t3 = std::get<2>(k);
			if (t1 == 0 || t2 == 0 || t3 == 0 || t1 == (int)max_a || t2 == (int)max_b || t3 == (int)max_c) {
				std::vector<int> new_v;
				for (int i = 0; i < v.size(); i++) new_v.push_back(0);
				grid3dpos2isoval[k] = new_v;
			}

		}
		myfile.close();
	}
	else {
		std::cout << "loading failed: unable to open file" << std::endl;
		return;
	}
	
}

// read in sph data
void FluidSim::read_sph_data() {
	std::string pathName = this->GetCurrentPluginPath();

	printf("loading particles...\n");
	std::vector<int> info = list_all_files(pathName + path_to_time_step_folder);

	int first_time_step = info[0];
	int last_time_step = info[1];
	//last_time_step = 100;

	min_time_step = first_time_step;
	max_time_step = last_time_step;
	time_step = (float)first_time_step;

	std::string filePathName;
	std::string line;

	float max_pos_x = -10.0f;
	float min_pos_x = 10.0f;
	float max_pos_y = -10.0f;
	float min_pos_y = 10.0f;
	float max_pos_z = -10.0f;
	float min_pos_z = 10.0f;

	std::vector<float> avg_pos = { 0.0f, 0.0f, 0.0f };
	int last_particle_size = 0;
	for (int time_step = first_time_step; time_step <= last_time_step; time_step++) {
		char buffer[100];
		int n;
		int count = 0;
		n = sprintf(buffer, path_to_time_step_file.c_str(), time_step);
		std::string a_file(buffer);
		filePathName = pathName + a_file;
		std::ifstream myfile(filePathName);
		std::cout << a_file << std::endl;
		// reading text file
		if (myfile.is_open()) {
			while (getline(myfile, line)) {
				count++;
				std::vector<float> numbers = split(line);
				particles_position.push_back(numbers[0]);
				particles_position.push_back(numbers[1]);
				particles_position.push_back(numbers[2]);

				avg_pos[0] += numbers[0];
				avg_pos[1] += numbers[1];
				avg_pos[2] += numbers[2];
			}
			myfile.close();
		}
		else {
			std::cout << "loading failed: unable to open file" << std::endl;
			return;
		}

		nb_particles.push_back(particles_position.size() / 3 - last_particle_size);
		last_particle_size = particles_position.size() / 3;
	}

	// rescale to fit in unit box
	avg_pos[0] = avg_pos[0] / (particles_position.size() / 3.0f);
	avg_pos[1] = avg_pos[1] / (particles_position.size() / 3.0f);
	avg_pos[2] = avg_pos[2] / (particles_position.size() / 3.0f);
	glm::vec3 avg_pos2 = glm::vec3(0.0f);
	printf("avg pos = (%f, %f, %f)\n", avg_pos[0], avg_pos[1], avg_pos[2]);
	for (int i = 0; i < particles_position.size(); i = i + 3) {

		/*particles_position[i] -= avg_pos[0];
		particles_position[i + 1] -= avg_pos[1];
		particles_position[i + 2] -= avg_pos[2];*/

		compare(min_pos_x, max_pos_x, particles_position[i]);
		compare(min_pos_y, max_pos_y, particles_position[i + 1]);
		compare(min_pos_z, max_pos_z, particles_position[i + 2]);
		avg_pos2 += glm::vec3(particles_position[i], particles_position[i + 1], particles_position[i + 2]);
	}
	avg_pos2 = avg_pos2 / glm::vec3(particles_position.size() / 3.0f);
	printf("new avg %s\n", glm::to_string(avg_pos2).c_str());
	printf("new max pos = (%f, %f, %f), min pos = (%f, %f, %f)\n", max_pos_x, max_pos_y, max_pos_z, min_pos_x, min_pos_y, min_pos_z);

	float box[] = {
		min_pos_x,min_pos_y,min_pos_z,1.0f,
		max_pos_x,min_pos_y,min_pos_z,1.0f,
		max_pos_x,max_pos_y,min_pos_z,1.0f,
		min_pos_x,max_pos_y,min_pos_z,1.0f,

		min_pos_x,min_pos_y,max_pos_z,1.0f,
		max_pos_x,min_pos_y,max_pos_z,1.0f,
		max_pos_x,max_pos_y,max_pos_z,1.0f,
		min_pos_x,max_pos_y,max_pos_z,1.0f,
	};
	vaBox.Create(ogl4_num4dBoxVerts);
	vaBox.SetArrayBuffer(0, GL_FLOAT, 4, box);
	vaBox.SetElementBuffer(0, ogl4_numBoxEdges * 2, ogl4_BoxEdges);

	// create scalar field
	int x_steps = (int)((max_pos_x - min_pos_x) / cube_size);
	int y_steps = (int)((max_pos_y - min_pos_y) / cube_size);
	int z_steps = (int)((max_pos_z - min_pos_z) / cube_size);
	float id = 0.0f;

	// fast neighbor search
	id = 0.0f;
	double start_time = omp_get_wtime();
	for (int a = 0; a < x_steps; a++) {
		for (int b = 0; b < y_steps; b++) {
			for (int c = 0; c < z_steps; c++) {
				glm::vec3 grid_pos = glm::vec3(min_pos_x + a * cube_size, min_pos_y + b * cube_size, min_pos_z + c * cube_size);
				grid_positions.push_back(grid_pos.x);
				grid_positions.push_back(grid_pos.y);
				grid_positions.push_back(grid_pos.z);
				grid_positions.push_back(id);
				id = id + 1.0f;

				std::tuple<int, int, int> key = { a, b, c };
				std::vector<int> iso_val = grid3dpos2isoval[key];
				for (int idx = 0; idx < iso_val.size(); idx++) {
					grid_if_near_particles.push_back(iso_val[idx]);
				}
			}
		}
	}
	cube_origin = glm::vec3(min_pos_x, min_pos_y, min_pos_z);
	double end_time = omp_get_wtime();
	printf("scalar field generated in %f\n", end_time - start_time);

	// sample cubes from scalar field
	for (int a = 0; a < x_steps - 1; a++) {
		for (int b = 0; b < y_steps - 1; b++) {
			for (int c = 0; c < z_steps - 1; c++) {
				std::vector<int> a_cube = {
					a, b, c,
					a + 1, b, c,
					a + 1, b + 1, c,
					a, b + 1, c,
					a, b, c + 1,
					a + 1, b, c + 1,
					a + 1, b + 1, c + 1,
					a, b + 1, c + 1,
				};
				all_cubes.push_back(a_cube);
			}
		}
	}

	// extract iso surface
	for (int time_step = first_time_step; time_step <= last_time_step; time_step++) {
		extract_isosurface(time_step - 1);
	}
	printf("iso surface generated\n");

	/*vaGrid.Create(grid_positions.size());
	vaGrid.SetArrayBuffer(0, GL_FLOAT, 4, grid_positions.data());
	printf("grid positions = %d %d\n", grid_positions.size(), vaGrid.GetNumVertices());
	printf("grid status = %d, max id = %f\n", grid_if_near_particles.size(), id);*/

	printf("done loading particles\n");
	return;
}

// extract isosurface for one time step
void FluidSim::extract_isosurface(int which_time) {
	int edgeTable[256] = {
0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0 };
	int triTable[256][16] =
	{ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
	{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
	{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
	{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
	{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
	{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
	{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
	{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
	{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
	{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
	{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
	{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
	{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
	{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
	{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
	{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
	{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
	{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
	{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
	{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
	{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
	{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
	{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
	{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
	{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
	{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
	{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
	{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
	{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
	{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
	{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
	{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
	{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
	{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
	{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
	{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
	{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
	{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
	{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
	{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
	{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
	{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
	{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
	{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
	{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
	{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
	{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
	{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
	{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
	{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
	{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
	{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
	{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
	{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
	{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
	{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
	{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
	{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
	{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
	{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
	{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
	{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
	{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
	{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
	{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
	{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
	{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
	{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
	{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
	{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
	{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
	{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
	{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
	{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
	{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
	{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
	{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
	{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
	{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
	{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
	{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
	{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
	{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
	{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
	{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
	{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
	{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
	{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
	{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
	{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
	{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
	{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
	{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
	{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
	{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
	{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
	{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1} };

	int ntriang = 0;
	for (int i = 0; i < all_cubes.size(); i++) {
		std::vector<int> iso;
		std::vector<float> cube = compute_cube_position(all_cubes[i], iso, which_time);

		int cubeindex = 0;
		if (iso[0] == 1) cubeindex |= 1;
		if (iso[1] == 1) cubeindex |= 2;
		if (iso[2] == 1) cubeindex |= 4;
		if (iso[3] == 1) cubeindex |= 8;
		if (iso[4] == 1) cubeindex |= 16;
		if (iso[5] == 1) cubeindex |= 32;
		if (iso[6] == 1) cubeindex |= 64;
		if (iso[7] == 1) cubeindex |= 128;

		if (edgeTable[cubeindex] == 0) continue;
		glm::vec3 vertlist[12];

		if (edgeTable[cubeindex] & 1)
			vertlist[0] = (glm::vec3(cube[0], cube[1], cube[2]) + glm::vec3(cube[3], cube[4], cube[5])) / 2.0f;
		if (edgeTable[cubeindex] & 2)
			vertlist[1] =
			(glm::vec3(cube[6], cube[7], cube[8]) + glm::vec3(cube[3], cube[4], cube[5])) / 2.0f;
		if (edgeTable[cubeindex] & 4)
			vertlist[2] =
			(glm::vec3(cube[6], cube[7], cube[8]) + glm::vec3(cube[9], cube[10], cube[11])) / 2.0f;
		if (edgeTable[cubeindex] & 8)
			vertlist[3] =
			(glm::vec3(cube[0], cube[1], cube[2]) + glm::vec3(cube[9], cube[10], cube[11])) / 2.0f;
		if (edgeTable[cubeindex] & 16)
			vertlist[4] =
			(glm::vec3(cube[12], cube[13], cube[14]) + glm::vec3(cube[15], cube[16], cube[17])) / 2.0f;
		if (edgeTable[cubeindex] & 32)
			vertlist[5] =
			(glm::vec3(cube[18], cube[19], cube[20]) + glm::vec3(cube[15], cube[16], cube[17])) / 2.0f;
		if (edgeTable[cubeindex] & 64)
			vertlist[6] =
			(glm::vec3(cube[18], cube[19], cube[20]) + glm::vec3(cube[21], cube[22], cube[23])) / 2.0f;
		if (edgeTable[cubeindex] & 128)
			vertlist[7] =
			(glm::vec3(cube[12], cube[13], cube[14]) + glm::vec3(cube[21], cube[22], cube[23])) / 2.0f;
		if (edgeTable[cubeindex] & 256)
			vertlist[8] =
			(glm::vec3(cube[12], cube[13], cube[14]) + glm::vec3(cube[0], cube[1], cube[2])) / 2.0f;
		if (edgeTable[cubeindex] & 512)
			vertlist[9] =
			(glm::vec3(cube[3], cube[4], cube[5]) + glm::vec3(cube[15], cube[16], cube[17])) / 2.0f;
		if (edgeTable[cubeindex] & 1024)
			vertlist[10] =
			(glm::vec3(cube[6], cube[7], cube[8]) + glm::vec3(cube[18], cube[19], cube[20])) / 2.0f;
		if (edgeTable[cubeindex] & 2048)
			vertlist[11] =
			(glm::vec3(cube[9], cube[10], cube[11]) + glm::vec3(cube[21], cube[22], cube[23])) / 2.0f;

		/* Create the triangle */
		for (int j = 0; triTable[cubeindex][j] != -1; j += 3) {
			all_triangles.push_back(vertlist[triTable[cubeindex][j]].x);
			all_triangles.push_back(vertlist[triTable[cubeindex][j]].y);
			all_triangles.push_back(vertlist[triTable[cubeindex][j]].z);
			all_triangles.push_back(vertlist[triTable[cubeindex][j + 1]].x);
			all_triangles.push_back(vertlist[triTable[cubeindex][j + 1]].y);
			all_triangles.push_back(vertlist[triTable[cubeindex][j + 1]].z);
			all_triangles.push_back(vertlist[triTable[cubeindex][j + 2]].x);
			all_triangles.push_back(vertlist[triTable[cubeindex][j + 2]].y);
			all_triangles.push_back(vertlist[triTable[cubeindex][j + 2]].z);
			ntriang++;
		}
	}

	triangle_size_each_time_step.push_back(ntriang * 9);
	printf("time step %d: %d triangles extracted\n", which_time, ntriang);
}

// helper in above function
std::vector<float> FluidSim::compute_cube_position(std::vector<int> a_cube, std::vector<int>& iso_info, int a_time) {
	std::vector<float> res;
	for (int i = 0; i < a_cube.size(); i += 3) {
		float x = cube_origin[0] + a_cube[i] * cube_size;
		float y = cube_origin[1] + a_cube[i + 1] * cube_size;
		float z = cube_origin[2] + a_cube[i + 2] * cube_size;
		res.push_back(x);
		res.push_back(y);
		res.push_back(z);

		std::tuple<int, int, int> key = { a_cube[i], a_cube[i + 1], a_cube[i + 2] };

		std::vector<int> iso_info_at_this_vert = grid3dpos2isoval[key];
		iso_info.push_back(iso_info_at_this_vert[a_time]);
	}
	return res;
}