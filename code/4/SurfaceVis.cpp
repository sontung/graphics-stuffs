/*
 * SurfaceVis.cpp
 *
 * Copyright (c) 2014 by Universitaet Stuttgart (VISUS).
 * All rights reserved.
 *
 * This file is part of OGL4Core.
 */

#include "stdafx.h"
#include "SurfaceVis.h"
#include "GLHelpers.h"
#include "Constants.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Defs.h"
#include <functional>

const int IDX_OFFSET = 10;

/**
 * @brief SurfaceVis constructor
 */
SurfaceVis::SurfaceVis(COGL4CoreAPI *Api) : RenderPlugin(Api)
{
	this->myName = "PCVC/SurfaceVis";
	this->myDescription = "Fourth assignment of PCVC";
}

/**
 * @brief SurfaceVis destructor
 */
SurfaceVis::~SurfaceVis() {
}

/**
 * @brief SurfaceVis activate method
 */
bool SurfaceVis::Activate(void) {
	// --------------------------------------------------
	//  Get the path name of the plugin folder, e.g.
	//  "/path/to/oglcore/Plugins/PCVC/SurfaceVis"
	// --------------------------------------------------
	std::string pathName = this->GetCurrentPluginPath();

	// --------------------------------------------------
	//  Initialize manipulator for camera view
	// --------------------------------------------------
	int camHandle = this->AddManipulator("View", &this->viewMX, Manipulator::MANIPULATOR_ORBIT_VIEW_3D);
	this->SelectCurrentManipulator(camHandle);
	this->SetManipulatorRotation(camHandle, glm::vec3(1, 0, 0), 50.0f);
	this->SetManipulatorDolly(camHandle, -1.5f);

	// --------------------------------------------------
	//  Initialize API variables here
	// --------------------------------------------------
	EnumPair ep[] = { {0,"cup.txt"},{1,"scuril1.txt"},{2,"scuril2.txt"},{3,"simple1,txt"}, {4,"recent.txt"} };
	which_file_to_load.Set(this, "FileNames ", ep, 5, &SurfaceVis::LoadControlPointCb);
	which_file_to_load.Register();
	which_file_to_load = 4;



	nb_control_points_m.Set(this, "m");
	nb_control_points_m.Register();
	nb_control_points_m.SetMinMax(4, 10);
	nb_control_points_m = 6;
	prev_m = static_cast<int>(nb_control_points_m);

	nb_control_points_n.Set(this, "n");
	nb_control_points_n.Register();
	nb_control_points_n.SetMinMax(4, 10);
	nb_control_points_n = 7;
	prev_n = static_cast<int>(nb_control_points_n);

	fovY.Set(this, "FoVy");
	fovY.Register();
	fovY.SetMinMax(5.0f, 90.0f);
	fovY = 45.0f;

	pointSize.Set(this, "PointSize");
	pointSize.Register();
	pointSize.SetMinMax(0, 50);
	pointSize = 10;

	// --------------------------------------------------
	//  TODO: Initialize API variables here...
	//    - horizontal size of control point net
	//    - vertical size of control point net
	//    - index of picked control point
	//    - x-coordinate of picked control point
	//    - y-coordinate of picked control point
	//    - z-coordinate of picked control point
	// --------------------------------------------------

	dataFilename.Set(this, "Filename");
	dataFilename.Register();
	dataFilename = "recent.txt";

	show_control_points.Set(this, "ShowControlPoints");
	show_control_points.Register();
	show_control_points = true;

	show_normal_vectors.Set(this, "ShowNormals");
	show_normal_vectors.Register();
	show_normal_vectors = false;

	grid_lines.Set(this, "ShowGridLines");
	grid_lines.Register();
	grid_lines = true;

	doWireframe.Set(this, "Wireframe");
	doWireframe.Register();
	doWireframe.SetKeyShortcut("w");
	doWireframe = true;

	depth_test.Set(this, "WithDepthTest");
	depth_test.Register();
	depth_test = true;

	render_b_surface.Set(this, "RenderSurface");
	render_b_surface.Register();
	render_b_surface = true;

	save_control_points.Set(this, "SaveControlPoints", &SurfaceVis::SaveCtrlPoints);
	save_control_points.Register();

	// --------------------------------------------------
	//  TODO: Initialize API variables here...
	//    - inner and outer tessellation levels
	// --------------------------------------------------

	tess_lvl_in.Set(this, "TessLevelIn");
	tess_lvl_in.Register();
	tess_lvl_in.SetMinMax(4, 20);
	tess_lvl_in = 8;

	tess_lvl_out.Set(this, "TessLevelOut");
	tess_lvl_out.Register();
	tess_lvl_out.SetMinMax(4, 20);
	tess_lvl_out = 8;

	ambientColor.Set(this, "Ambient");
	ambientColor.Register();
	ambientColor = glm::vec3(1.0f);

	diffuseColor.Set(this, "Diffuse");
	diffuseColor.Register();
	diffuseColor = glm::vec3(1.0f);

	specularColor.Set(this, "Specular");
	specularColor.Register();
	specularColor = glm::vec3(1.0f);

	k_ambient.Set(this, "k_amb");
	k_ambient.Register();
	k_ambient.SetMinMax(0.0f, 1.0f);
	k_ambient.SetStep(0.001f);
	k_ambient = 0.2f;

	k_diffuse.Set(this, "k_diff");
	k_diffuse.Register();
	k_diffuse.SetMinMax(0.0f, 1.0f);
	k_diffuse.SetStep(0.001f);
	k_diffuse = 0.7f;

	k_specular.Set(this, "k_spec");
	k_specular.Register();
	k_specular.SetMinMax(0.0f, 1.0f);
	k_specular.SetStep(0.001f);
	k_specular = 0.0f;

	k_exp.Set(this, "k_exp");
	k_exp.Register();
	k_exp.SetMinMax(0.0f, 5000.0f);
	k_exp = 120.0f;

	freq.Set(this, "freq");
	freq.Register();
	freq.SetMinMax(0, 100);
	freq = 4;

	// --------------------------------------------------
	//  Initialize shader and VA for box rendering
	// --------------------------------------------------
	boxVertShaderName = pathName + std::string("/resources/box.vert");
	boxFragShaderName = pathName + std::string("/resources/box.frag");
	shaderBox.CreateProgramFromFile(boxVertShaderName.c_str(), boxFragShaderName.c_str());

	vaBox.Create(ogl4_num4dBoxVerts);
	vaBox.SetArrayBuffer(0, GL_FLOAT, 4, ogl4_4dBoxVerts);
	vaBox.SetElementBuffer(0, ogl4_numBoxEdges * 2, ogl4_BoxEdges);

	// --------------------------------------------------
	//  Initialize shader and VA for b-spline surface
	// --------------------------------------------------
	bsVertShaderName = pathName + std::string("/resources/surface.vs");
	bsTCShaderName = pathName + std::string("/resources/surface.tcs");
	bsTEShaderName = pathName + std::string("/resources/surface.tes");
	bsFragShaderName = pathName + std::string("/resources/surface.fs");
	shaderBSplineSurface.CreateProgramFromFile(
		bsVertShaderName.c_str(),
		bsTCShaderName.c_str(),
		bsTEShaderName.c_str(),
		bsFragShaderName.c_str());
	shaderBSplineSurface.PrintInfo();

	std::vector<float> v = {
		0.0f, 0.0f,
		0.0f, 0.25f,
		0.33f, 0.25f,
		0.33f, 0.0f
	};
	vaBSplineSurface.Create(4);
	vaBSplineSurface.SetArrayBuffer(0, GL_FLOAT, 2, v.data());


	glGenBuffers(1, &ssbo_u);
	glGenBuffers(1, &ssbo_v);
	glGenBuffers(1, &ssbo_control_points);


	// --------------------------------------------------
	//  Initialize shader and VA for rendering fbo content
	// --------------------------------------------------
	quadVertShaderName = pathName + std::string("/resources/quad.vert");
	quadFragShaderName = pathName + std::string("/resources/quad.frag");
	shaderQuad.CreateProgramFromFile(quadVertShaderName.c_str(), quadFragShaderName.c_str());
	vaQuad.Create(4);
	vaQuad.SetArrayBuffer(0, GL_FLOAT, 2, ogl4_2dQuadVerts);


	// --------------------------------------------------
	//  Initialize shader for control point rendering
	// --------------------------------------------------
	ctrlPointVertShaderName = pathName + std::string("/resources/ctrlPoints.vert");
	ctrlPointFragShaderName = pathName + std::string("/resources/ctrlPoints.frag");
	shaderCtrlPoints.CreateProgramFromFile(ctrlPointVertShaderName.c_str(), ctrlPointFragShaderName.c_str());
	createVA_control_points();

	// --------------------------------------------------
	//  Initialize a flat/rnd b-spline surface
	// --------------------------------------------------

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);


	return true;
}

/**
 * @brief SurfaceVis deactive method
 */
bool SurfaceVis::Deactivate(void) {
	// --------------------------------------------------
	//  TODO: Do not forget to clear all allocated sources.
	// --------------------------------------------------
	shaderBox.RemoveAllShaders();
	shaderCtrlPoints.RemoveAllShaders();
	shaderQuad.RemoveAllShaders();
	shaderBSplineSurface.RemoveAllShaders();

	vaBox.Delete();
	vaCtrlPoints.Delete();
	vaBSplineSurface.Delete();
	vaQuad.Delete();

	control_point_positions.clear();
	control_point_positions.shrink_to_fit();
	list_of_control_points.clear();
	list_of_control_points.shrink_to_fit();
	control_points_index.clear();
	control_points_index.shrink_to_fit();
	control_points_color.clear();
	control_points_color.shrink_to_fit();
	knot_vec_u.clear();
	knot_vec_u.shrink_to_fit();
	knot_vec_v.clear();
	knot_vec_v.shrink_to_fit();

	glDeleteBuffers(1, &ssbo_control_points);
	glDeleteBuffers(1, &ssbo_u);
	glDeleteBuffers(1, &ssbo_v);
	glDeleteFramebuffers(1, &fboID);

	glDeleteTextures(3, colAttachID);
	glDeleteTextures(1, &dboID);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_PROGRAM_POINT_SIZE);
	return true;
}

/**
 * @brief SurfaceVis initialization method
 */
bool SurfaceVis::Init(void) {
	if (gl3wInit()) {
		fprintf(stderr, "Error: Failed to initialize gl3w.\n");
		return false;
	}

	// --------------------------------------------------
	//  TODO: Test if tessellation shader is available
	// --------------------------------------------------
	return true;
}

/**
 * @brief SurfaceVis keyboard callback function
 * @param key
 * @param x
 * @param y
 */
bool SurfaceVis::Keyboard(int key, int action, int mods, int x, int y) {
	// --------------------------------------------------
	//  TODO: Add keyboard functionaliy
	//    Reload shaders when pressing 'r'.
	// --------------------------------------------------
	PostRedisplay();
	//printf("key pressed=%d\n", key);

	if (key == 88) {
		// deselect all control points
		for (int i = 0; i < list_of_control_points.size(); i++) {
			list_of_control_points[i].selected = 0;
		}
		updateVA_control_points();
	}
	else if (key == 48) {
		for (int i = 0; i < list_of_control_points.size(); i++) {
			list_of_control_points[i].x = list_of_control_points[i].x_ori;
			list_of_control_points[i].y = list_of_control_points[i].y_ori;
			list_of_control_points[i].z = list_of_control_points[i].z_ori;

		}
		updateVA_control_points();
	}
	return false;
}

/**
 * @brief SurfaceVis mouse motion callback function
 * @param x
 * @param y
 */
bool SurfaceVis::Motion(int x, int y) {
	int xpos, ypos;
	int dx, dy;
	GetLastMousePos(xpos, ypos);
	dx = xpos - x;
	dy = ypos - y;
	if (IsMidButtonPressed() && IsShiftPressed()) {
		for (int i = 0; i < list_of_control_points.size(); i++) {
			if (list_of_control_points[i].selected == 1) {
				list_of_control_points[i].x -= dx / (float)wWidth;
				list_of_control_points[i].y += dy / (float)wHeight;
			}
		}
		updateVA_control_points();
	}
	else if (IsLeftButtonPressed() && IsShiftPressed()) {
		for (int i = 0; i < list_of_control_points.size(); i++) {
			if (list_of_control_points[i].selected == 1) {
				list_of_control_points[i].z += dy / (float)wHeight;
			}
		}
		updateVA_control_points();
	}
	return false;
}

/**
 * @brief SurfaceVis mouse callback function
 * @param button
 * @param state
 * @param x
 * @param y
 */
bool SurfaceVis::Mouse(int button, int state, int mods, int x, int y) {
	if (IsLeftButtonPressed() && IsShiftPressed()) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID);
		glReadBuffer(GL_COLOR_ATTACHMENT1);

		unsigned char pixel[3];
		glReadPixels(x, wHeight - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixel);

		if (colorToId(pixel) > 0) {
			list_of_control_points[colorToId(pixel) - 1].selected = 1;
			updateVA_control_points(); // transfer select information to gpu
		}

		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

	return false;
}

bool SurfaceVis::Render(void) {
	drawToFBO();

	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, wWidth, wHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 pmx = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);
	shaderQuad.Bind();
	vaQuad.Bind();


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colAttachID[0]);
	glUniform1i(glGetUniformLocation(shaderQuad.GetProgHandle(), "tex"), 0);

	glUniformMatrix4fv(shaderQuad.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(pmx));


	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	vaQuad.Release();
	shaderQuad.Release();


	return false;
}

// compute neighboring vertices of a control point to draw grid lines
std::vector<int> neighbor_vertices(int i, int j, int m, int n) {
	std::vector<int> res;

	if (i < n - 1) {
		int n1 = (i + 1) * m + j;
		res.push_back(n1);
	}
	if (j < m - 1) {
		int n2 = i * m + j + 1;
		res.push_back(n2);
	}
	return res;
}

// compute vertex position of control point based on its index in the net
std::vector<float> calculate_vertex_coord(int i, int j, float start, float step_x, float step_y) {
	float x = -start + j * step_x;
	float y = start - i * step_y;
	std::vector<float> res = { x, y };
	return res;
}

// create knot vector
std::vector<float> create_knot_vector(int nb_control_points, int order = 4) {
	std::vector<float> knots;

	for (int i = 0; i < order; i++)
		knots.push_back(0);

	int middle = nb_control_points - order;
	for (int i = 0; i < middle; i++)
		knots.push_back(float(i + 1) / (middle + 1));

	for (int i = 0; i < order; i++)
		knots.push_back(1);
	return knots;
}

// create VA for control points
void SurfaceVis::createVA_control_points() {
	int curr_m = static_cast<int>(nb_control_points_m);
	int curr_n = static_cast<int>(nb_control_points_n);
	std::vector<int> selected;

	control_point_positions.clear();
	control_points_index.clear();
	control_points_color.clear();
	list_of_control_points.clear();

	float start_corner = 0.5f;
	float step_x = start_corner * 2.0f / (curr_m - 1);
	float step_y = start_corner * 2.0f / (curr_n - 1);

	for (int i = 0; i < curr_n; i++) {
		for (int j = 0; j < curr_m; j++) {
			// vertex position
			int curr_idx = i * curr_m + j;
			std::vector<float> coord = calculate_vertex_coord(i, j, start_corner, step_x, step_y);
			control_point_positions.push_back(coord[0]);
			control_point_positions.push_back(coord[1]);
			control_point_positions.push_back(0);

			// color information for picking
			Object o = Object(curr_idx + 1, coord[0], coord[1]);
			list_of_control_points.push_back(o);
			glm::vec3 uniq_color = idToColor(curr_idx + 1);
			control_points_color.push_back(uniq_color.x);
			control_points_color.push_back(uniq_color.y);
			control_points_color.push_back(uniq_color.z);

			// selection information for changing color when selected
			selected.push_back(0);

			// ibo for drawing lines
			std::vector<int> neighbors = neighbor_vertices(i, j, curr_m, curr_n);
			for (int u = 0; u < neighbors.size(); u++) {
				control_points_index.push_back(curr_idx);
				control_points_index.push_back(neighbors[u]);
			}
		}
	}

	// vertex array for control points
	vaCtrlPoints.Create(control_point_positions.size());
	vaCtrlPoints.SetArrayBuffer(0, GL_FLOAT, 3, control_point_positions.data());
	vaCtrlPoints.SetArrayBuffer(1, GL_FLOAT, 3, control_points_color.data());
	vaCtrlPoints.SetArrayBuffer(2, GL_INT, 1, selected.data());
	vaCtrlPoints.SetElementBuffer(0, control_points_index.size(), control_points_index.data());

	// knot vectors
	knot_vec_u = create_knot_vector(curr_n);
	knot_vec_v = create_knot_vector(curr_m);
	printf("knot vectors u for n: ");
	for (int i = 0; i < knot_vec_u.size(); i++) printf("%f ", knot_vec_u[i]);
	printf(" nb=%d, ns=%d \n", curr_n, curr_n - 3);
	printf("knot vectors v for m: ");
	for (int i = 0; i < knot_vec_v.size(); i++) printf("%f ", knot_vec_v[i]);
	printf(" nb=%d, ns=%d \n", curr_m, curr_m - 3);
	nb_patches = (curr_m - 3) * (curr_n - 3);

	// deallocate
	selected.clear();
	selected.shrink_to_fit();
}

// update VA control points when user changes
void SurfaceVis::updateVA_control_points() {
	std::vector<int> selected;
	control_point_positions.clear();


	for (int i = 0; i < list_of_control_points.size(); i++) {
		// vertex position
		control_point_positions.push_back(list_of_control_points[i].x);
		control_point_positions.push_back(list_of_control_points[i].y);
		control_point_positions.push_back(list_of_control_points[i].z);


		// selection information for changing color when selected
		selected.push_back(list_of_control_points[i].selected);
	}

	vaCtrlPoints.Create(control_point_positions.size());
	vaCtrlPoints.SetArrayBuffer(0, GL_FLOAT, 3, control_point_positions.data());
	vaCtrlPoints.SetArrayBuffer(1, GL_FLOAT, 3, control_points_color.data());
	vaCtrlPoints.SetArrayBuffer(2, GL_INT, 1, selected.data());
	vaCtrlPoints.SetElementBuffer(0, control_points_index.size(), control_points_index.data());

	// deallocate 
	selected.clear();
	selected.shrink_to_fit();
}

// debugging
int SurfaceVis::find_span_u(float u) {
	int n = knot_vec_u.size() - 3;
	if (u == knot_vec_u[n + 1]) return n;
	int low = 3;
	int high = n + 1;
	int mid = (low + high) / 2;
	while (u < knot_vec_u[mid] || u >= knot_vec_u[mid + 1]) {
		if (u < knot_vec_u[mid]) high = mid;
		else low = mid;
		mid = (low + high) / 2;
	}
	return mid;
}

void SurfaceVis::drawToFBO() {

	projMX = glm::perspective(glm::radians(static_cast<float>(fovY)), viewAspect, 0.01f, 100.0f);

	// enable wireframe
	if (doWireframe) {
		glPolygonMode(GL_FRONT, GL_LINE);
		glPolygonMode(GL_BACK, GL_LINE);
	}

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, wWidth, wHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// check current m and n
	int curr_m = static_cast<int>(nb_control_points_m);
	int curr_n = static_cast<int>(nb_control_points_n);

	if (curr_m != prev_m || curr_n != prev_n) {
		if (!predefined_points_loaded) {
			prev_m = curr_m;
			prev_n = curr_n;
			createVA_control_points();
		}
	}
	else {
		predefined_points_loaded = false;
	}

	// draw control points
	if (show_control_points) {
		shaderCtrlPoints.Bind();
		vaCtrlPoints.Bind();
		glUniform1i(shaderCtrlPoints.GetUniformLocation("point_size"), static_cast<int>(pointSize));
		glUniform1i(shaderCtrlPoints.GetUniformLocation("depth_test"), static_cast<int>(depth_test));
		glUniformMatrix4fv(shaderCtrlPoints.GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(viewMX));
		glUniformMatrix4fv(shaderCtrlPoints.GetUniformLocation("modelMX"), 1, GL_FALSE, glm::value_ptr(modelMX));
		glUniformMatrix4fv(shaderCtrlPoints.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));

		glDrawArrays(GL_POINTS, 0, curr_m * curr_n);
		if (grid_lines) glDrawElements(GL_LINES, control_points_index.size(), GL_UNSIGNED_INT, 0);
		vaCtrlPoints.Release();
		shaderCtrlPoints.Release();
	}

	// draw surface
	shaderBSplineSurface.Bind();

	// sending knot vectors
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * knot_vec_u.size(), knot_vec_u.data(), GL_STATIC_READ); // for n
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_u);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_v);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * knot_vec_v.size(), knot_vec_v.data(), GL_STATIC_READ); // for m
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_v);

	// sending control points
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_control_points);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * control_point_positions.size(), control_point_positions.data(), GL_STATIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_control_points);

	// sending uniforms
	glUniformMatrix4fv(shaderBSplineSurface.GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(viewMX));
	glUniformMatrix4fv(shaderBSplineSurface.GetUniformLocation("modelMX"), 1, GL_FALSE, glm::value_ptr(modelMX));
	glUniformMatrix4fv(shaderBSplineSurface.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));
	glUniform1i(shaderBSplineSurface.GetUniformLocation("n_seg_n"), curr_n - 3);
	glUniform1i(shaderBSplineSurface.GetUniformLocation("n_seg_m"), curr_m - 3);
	glUniform1i(shaderBSplineSurface.GetUniformLocation("u_size"), knot_vec_u.size());
	glUniform1i(shaderBSplineSurface.GetUniformLocation("v_size"), knot_vec_v.size());
	glUniform1i(shaderBSplineSurface.GetUniformLocation("p"), 3);
	glUniform1i(shaderBSplineSurface.GetUniformLocation("q"), 3);
	glUniform1i(shaderBSplineSurface.GetUniformLocation("nb_cp_m"), curr_m);
	glUniform1i(shaderBSplineSurface.GetUniformLocation("nb_cp_n"), curr_n);
	glUniform1i(shaderBSplineSurface.GetUniformLocation("tess_in"), static_cast<int>(tess_lvl_in));
	glUniform1i(shaderBSplineSurface.GetUniformLocation("tess_out"), static_cast<int>(tess_lvl_out));
	glUniform1i(shaderBSplineSurface.GetUniformLocation("depth_test"), static_cast<int>(depth_test));
	glUniform1i(shaderBSplineSurface.GetUniformLocation("render_surface"), static_cast<int>(render_b_surface));
	glUniform1i(shaderBSplineSurface.GetUniformLocation("show_normals"), static_cast<int>(show_normal_vectors));
	glUniform1i(shaderBSplineSurface.GetUniformLocation("how_many_checkers"), static_cast<int>(freq));




	glPatchParameteri(GL_PATCH_VERTICES, 4);
	glDrawArraysInstanced(GL_PATCHES, 0, 4, nb_patches);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	shaderBSplineSurface.Release();

	// disable wireframe
	if (doWireframe) {
		glPolygonMode(GL_FRONT, GL_FILL);
		glPolygonMode(GL_BACK, GL_FILL);
	}
}

bool SurfaceVis::Resize(int width, int height) {
	wWidth = width;
	wHeight = height;
	viewAspect = wWidth / static_cast<float>(wHeight);
	initFBO();
	return true;
}

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

/**
 * @brief Load control points from file
 * @param filename
 */
bool SurfaceVis::LoadCtrlPoints(std::string filename) {
	std::string pathName = this->GetCurrentPluginPath();
	std::string filePathName = pathName + std::string("/resources/models/") + filename;
	std::string line;
	std::ifstream myfile(filePathName);
	printf("loading control points...\n");

	// reading text file
	std::vector<std::vector<float>> color_map;
	if (myfile.is_open()) {
		int lineth = 0;

		control_point_positions.clear();
		control_points_index.clear();
		control_points_color.clear();
		list_of_control_points.clear();

		while (getline(myfile, line)) {
			std::vector<float> numbers = split(line);
			if (numbers.size() < 3) {
				if (lineth == 0) nb_control_points_m = numbers[0];
				else if (lineth == 1) nb_control_points_n = numbers[0];
				lineth++;
			}
			else {
				control_point_positions.push_back(numbers[0]);
				control_point_positions.push_back(numbers[1]);
				control_point_positions.push_back(numbers[2]);
			}
		}
		printf("done loading %d control points\n", control_point_positions.size() / 3);
		myfile.close();
	}
	else {
		std::cout << "loading failed:: unable to open file" << std::endl;
		return false;
	}

	int curr_m = static_cast<int>(nb_control_points_m);
	int curr_n = static_cast<int>(nb_control_points_n);
	std::vector<int> selected;

	float start_corner = 0.5f;
	float step_x = start_corner * 2.0f / (curr_m - 1);
	float step_y = start_corner * 2.0f / (curr_n - 1);

	for (int i = 0; i < curr_n; i++) {
		for (int j = 0; j < curr_m; j++) {
			// vertex position
			int curr_idx = i * curr_m + j;
			float x = control_point_positions[3 * curr_idx];
			float y = control_point_positions[3 * curr_idx + 1];
			float z = control_point_positions[3 * curr_idx + 2];

			// color information for picking
			Object o = Object(curr_idx + 1, x, y);
			o.z = z;
			glm::vec3 uniq_color = idToColor(curr_idx + 1);
			control_points_color.push_back(uniq_color.x);
			control_points_color.push_back(uniq_color.y);
			control_points_color.push_back(uniq_color.z);

			// default locations in case of resetting
			std::vector<float> coord = calculate_vertex_coord(i, j, start_corner, step_x, step_y);
			o.x_ori = coord[0];
			o.y_ori = coord[1];
			o.z_ori = 0.0;
			list_of_control_points.push_back(o);

			// selection information for changing color when selected
			selected.push_back(0);

			// ibo for drawing lines
			std::vector<int> neighbors = neighbor_vertices(i, j, curr_m, curr_n);
			for (int u = 0; u < neighbors.size(); u++) {
				control_points_index.push_back(curr_idx);
				control_points_index.push_back(neighbors[u]);
			}
		}
	}


	// vertex array for control points
	vaCtrlPoints.Create(control_point_positions.size());
	vaCtrlPoints.SetArrayBuffer(0, GL_FLOAT, 3, control_point_positions.data());
	vaCtrlPoints.SetArrayBuffer(1, GL_FLOAT, 3, control_points_color.data());
	vaCtrlPoints.SetArrayBuffer(2, GL_INT, 1, selected.data());
	vaCtrlPoints.SetElementBuffer(0, control_points_index.size(), control_points_index.data());

	// knot vectors
	knot_vec_u = create_knot_vector(curr_n);
	knot_vec_v = create_knot_vector(curr_m);
	printf("knot vectors u for n: ");
	for (int i = 0; i < knot_vec_u.size(); i++) printf("%f ", knot_vec_u[i]);
	printf(" nb=%d, ns=%d \n", curr_n, curr_n - 3);
	printf("knot vectors v for m: ");
	for (int i = 0; i < knot_vec_v.size(); i++) printf("%f ", knot_vec_v[i]);
	printf(" nb=%d, ns=%d \n", curr_m, curr_m - 3);
	nb_patches = (curr_m - 3) * (curr_n - 3);

	// deallocate
	selected.clear();
	selected.shrink_to_fit();

	return true;
}

// callback for loading control points
void SurfaceVis::LoadControlPointCb(EnumVar<SurfaceVis>& var) {
	std::map<int, std::string> ep = { {0,"cup.txt"},{1,"scuril1.txt"},{2,"scuril2.txt"},{3,"simple1,txt"}, {4,"recent.txt"} };
	LoadCtrlPoints(ep[var]);
	predefined_points_loaded = true;

}

/**
 * @brief Save control points to file
 * @param filenmae
 */
void SurfaceVis::SaveCtrlPoints(ButtonVar<SurfaceVis>& var) {
	std::string filename = dataFilename;
	std::string pathName = this->GetCurrentPluginPath();
	std::string filePathName = pathName + std::string("/resources/models/") + filename;
	std::ofstream myfile(filePathName);

	if (myfile.is_open()) {

		int curr_m = static_cast<int>(nb_control_points_m);
		int curr_n = static_cast<int>(nb_control_points_n);

		myfile << curr_m << " 3\n";
		myfile << curr_n << " 3\n";

		for (int i = 0; i < curr_n; i++) {
			for (int j = 0; j < curr_m; j++) {
				int curr_idx = 3 * (i * curr_m + j);
				myfile << control_point_positions[curr_idx] << " ";
				myfile << control_point_positions[curr_idx + 1] << " ";
				myfile << control_point_positions[curr_idx + 2] << "\n";
			}
		}
		myfile.close();
	}
	else std::cout << "saving control points failed: unable to open file\n";
	printf("saved control points...\n");
	return;
}

/**
 * @brief Initialize framebuffer object
 */
bool SurfaceVis::initFBO() {
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

	return false;
}

void SurfaceVis::createFBOTexture(GLuint& outID, const GLenum internalFormat,
	const GLenum format, const GLenum type,
	GLint filter, int width, int height) {
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
 *  Convert object ID to unique color
 * @param id   object ID.
 * @return color
 */
glm::vec3 SurfaceVis::idToColor(unsigned int id) {
	glm::vec3 col = glm::vec3(0.0f);
	glm::ivec3 color = glm::ivec3(0);
	unsigned int num = id;
	color.r = num % 256;
	num = num >> 8;
	color.g = num % 256;
	num = num >> 8;
	color.b = num % 256;
	num = num >> 8;
	col = glm::vec3(color) / 255.0f;
	return col;
}

/**
 *  Convert color to object ID.
 * @param buf  color defined as 3-array [red,green,blue]
 * @return object ID
 */
unsigned int SurfaceVis::colorToId(unsigned char buf[3]) {
	unsigned int num = 0;
	int b1 = (int)buf[0];
	int b2 = (int)buf[1];
	int b3 = (int)buf[2];
	num = (b3 << 16) + (b2 << 8) + b1;
	return num;
}
