/*
 * VolumeVis.cpp
 * 
 * Copyright (c) 2014 by Universitaet Stuttgart (VISUS).
 * All rights reserved.
 * 
 * This file is part of OGL4Core.
 */
 
#include "stdafx.h"
#include "VolumeVis.h"
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

VolumeVis::VolumeVis(COGL4CoreAPI *Api) : RenderPlugin(Api),
    wWidth(512),wHeight(512),volumeTex(0) {
    this->myName = "PCVC/VolumeVis";
    this->myDescription = "Third assignment of PCVC";
    transferTex = 0;
}

VolumeVis::~VolumeVis() {
}

bool VolumeVis::Activate(void) {
    // --------------------------------------------------
    //  Get the path name of the plugin folder, e.g.
    //  "/path/to/oglcore/Plugins/PCVC/VolumeVis"
    // --------------------------------------------------
    pathName = this->GetCurrentPluginPath();
    
    // Initialize file selector
    files.Set(this, "Volume", (pathName+"/resources/volumes").c_str(), ".dat", &VolumeVis::fileChanged);
    files.Register();
    
    // Initialize manipulator for camera view
    int camHandle = this->AddManipulator("View", &this->viewMX, Manipulator::MANIPULATOR_ORBIT_VIEW_3D);
    this->SelectCurrentManipulator(camHandle);
    this->SetManipulatorRotation(camHandle, glm::vec3(1,0,0), -10.0f);
    this->SetManipulatorDolly(camHandle, -2.0f );
    
    // --------------------------------------------------
    //  TODO: Initialize API variables here
    // --------------------------------------------------

	EnumPair ep2[] = { {0,"red"},{1,"green"},{2,"blue"},{3,"alpha"} };
	mode_for_rgba_channel_inputs.Set(this, "ChannelInputMode", ep2, 4, &VolumeVis::input_channel_callback);
	mode_for_rgba_channel_inputs.Register();
	mode_for_rgba_channel_inputs = 0;

	EnumPair ep[] = { {e_lineOfSight,"line-of-sight"},{e_mip,"mip"},{e_isosurface,"isosurface"},{e_volume,"volume"} };
	mode.Set(this, "Mode", ep, 4, &VolumeVis::modeChanged);
	mode.Register();
	mode = e_isosurface;

	load_default_transfer_func.Set(this, "LoadDefaultTF", &VolumeVis::loading_default_callback);
	load_default_transfer_func.Register();

	load_recent_transfer_func.Set(this, "LoadRecentTF", &VolumeVis::loading_recent_callback);
	load_recent_transfer_func.Register();

	save_transfer_func.Set(this, "SaveTF", &VolumeVis::saving_callback);
	save_transfer_func.Register();

	res_x.Set(this, "ResX");
	res_x.Register();
	res_x.SetReadonly(true);

	res_y.Set(this, "ResY");
	res_y.Register();
	res_y.SetReadonly(true);

	res_z.Set(this, "ResZ");
	res_z.Register();
	res_z.SetReadonly(true);

    fovY.Set(this, "FoVy");
    fovY.Register();
    fovY.SetMinMax(5.0f, 90.0f);
    fovY = 45.0f;

	ambient.Set(this, "Ambient");
	ambient.Register();
	ambient.SetMinMax(0.0f, 10.0f);
	ambient.SetStep(0.1f);
	ambient = 0.2f;
	float_buttons.push_back(&ambient);

	diffuse.Set(this, "Diffuse");
	diffuse.Register();
	diffuse.SetMinMax(0.0f, 10.0f);
	diffuse.SetStep(0.1f);
	diffuse = 1.0f;
	float_buttons.push_back(&diffuse);

	k_amb.Set(this, "Kamb");
	k_amb.Register();
	k_amb.SetMinMax(0.1f, 0.5f);
	k_amb.SetStep(0.01f);
	k_amb = 0.2f;
	float_buttons.push_back(&k_amb);

	k_diff.Set(this, "Kdiff");
	k_diff.Register();
	k_diff.SetMinMax(0.1f, 1.0f);
	k_diff.SetStep(0.01f);
	k_diff = 0.8f;
	float_buttons.push_back(&k_diff);

	k_spec.Set(this, "Kspec");
	k_spec.Register();
	k_spec.SetMinMax(0.01f, 0.5f);
	k_spec.SetStep(0.01f);
	k_spec = 0.5f;
	float_buttons.push_back(&k_spec);

	k_exp.Set(this, "Kexp");
	k_exp.Register();
	k_exp.SetMinMax(0.1f, 10.0f);
	k_exp.SetStep(0.1f);
	k_exp = 0.5f;
	float_buttons.push_back(&k_exp);

	iso_value.Set(this, "IsoValue");
	iso_value.Register();
	iso_value.SetMinMax(0.01f, 30.0f);
	iso_value.SetStep(0.01f);
	iso_value = 0.5f;
	float_buttons.push_back(&iso_value);

	nb_steps.Set(this, "NbSteps");
	nb_steps.Register();
	nb_steps.SetStep(2);
	nb_steps = 100;

	step_size.Set(this, "StepSize");
	step_size.Register();
	step_size.SetStep(0.001f);
	step_size = 0.01f;

	hist_panel_height.Set(this, "HistPanelHeight");
	hist_panel_height.Register();
	hist_panel_height.SetMinMax(100, 200);
	hist_panel_height.SetStep(1);
	hist_panel_height = 150;
	uint_buttons.push_back(&hist_panel_height);

	c_global.Set(this, "GlobalScaling");
	c_global.Register();
	c_global.SetMinMax(0.05f, 0.5f);
	c_global.SetStep(0.01f);
	c_global = 0.05f;
	float_buttons.push_back(&c_global);
    
    showBox.Set(this, "ShowBox");
    showBox.Register();
    showBox.SetKeyShortcut("x");
    showBox = true;

	log_hist_display.Set(this, "LogHist");
	log_hist_display.Register();
	log_hist_display = true;
	bool_buttons.push_back(&log_hist_display);
    
    
    // --------------------------------------------------
    //  Initialize shader and VA for box rendering
    // --------------------------------------------------
    boxVertShaderName = pathName + std::string("/resources/box.vert");
    boxFragShaderName = pathName + std::string("/resources/box.frag");
    shaderBox.CreateProgramFromFile( boxVertShaderName.c_str(), boxFragShaderName.c_str() );
    
    vaBox.Create(ogl4_num4dBoxVerts);
    vaBox.SetArrayBuffer(0, GL_FLOAT, 4, ogl4_4dBoxVerts);
    vaBox.SetElementBuffer(0, ogl4_numBoxEdges*2, ogl4_BoxEdges);

    // --------------------------------------------------
    //  Initialize shader and VA for volume rendering
    // --------------------------------------------------
    volVertShaderName = pathName + std::string("/resources/volume.vert");
    volFragShaderName = pathName + std::string("/resources/volume.frag");
    shaderVolume.CreateProgramFromFile( volVertShaderName.c_str(), volFragShaderName.c_str() );
    
    vaCube.Create(ogl4_num4dBoxVerts);
    vaCube.SetArrayBuffer(0, GL_FLOAT, 4, ogl4_4dBoxVerts);
    vaCube.SetElementBuffer(0, ogl4_numBoxFaces*6, ogl4_boxFaces);
    
    // --------------------------------------------------
    //  Initialize shader for histogram rendering
    // --------------------------------------------------
    histoVertShaderName = pathName + std::string("/resources/histo.vert");
    histoGeomShaderName = pathName + std::string("/resources/histo.geom");
    histoFragShaderName = pathName + std::string("/resources/histo.frag");
    shaderHisto.CreateProgramFromFile( histoVertShaderName.c_str(), histoGeomShaderName.c_str(), histoFragShaderName.c_str() );
    

    histoBGVertShaderName = pathName + std::string("/resources/histoBG.vert");
    histoBGFragShaderName = pathName + std::string("/resources/histoBG.frag");
    shaderHistoBG.CreateProgramFromFile( histoBGVertShaderName.c_str(), histoBGFragShaderName.c_str() );
    vaHistoBG.Create(4);
    vaHistoBG.SetArrayBuffer(0, GL_FLOAT, 2, ogl4_2dQuadVerts);
    
    transferFuncVertShaderName = pathName + std::string("/resources/transferfunc.vert");
    transferFuncFragShaderName = pathName + std::string("/resources/transferfunc.frag");
    shaderTransferFunc.CreateProgramFromFile( transferFuncVertShaderName.c_str(), transferFuncFragShaderName.c_str() );
    
    initTransferFunc();
    
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClearDepth(1.0f);  
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_3D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	modelMX = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f));
	modeChanged(mode);
    return true;
}

// generating texture for transfer function
void VolumeVis::update_bins_color() {
	int bin_width2 = wWidth / numBins;
	float val;
	for (int i = 0; i < numBins; i++) {
		for (int j = 0; j < 4; j++) {
			val = 0.0f;
			for (int g = i * bin_width2; g < (i + 1) * bin_width2; g++) {
				if (g >= wWidth) continue;
				val += rgba_vertices[g * 4 + j];
			}
			val /= bin_width2;
			transferFuncDict[i][j] = val;
		}
	}

	printf("generating texture\n");
	float* buf;
	buf = (float*)malloc(4*256*sizeof(float));
	bin_width = 254.0f / numBins;
	for (int i = 0; i < 256; i++) {
		int bin = (int)std::ceil((i - 1) / bin_width) - 1;
		for (int j = 0; j < 4; j++) {
			if (i == 0) buf[i * 4 + j] = 0.0f;
			else if (i == 1) buf[i * 4 + j] = transferFuncDict[0][j];
			else if (i == 255) buf[i * 4 + j] = transferFuncDict[numBins-1][j];
			else buf[i * 4 + j] = transferFuncDict[bin][j];
		}
	}

	glGenTextures(1, &transferTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D, transferTex);

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_FLOAT, buf);
	delete[] buf;

	return;
}

/**
 * @brief Initialize transfer function
 * @param num
 * @param tf
 */
void VolumeVis::initTransferFunc() {
	for (int i = 0; i < numBins; i++) {
		std::array<float, 4> v = { 0.0f, 0.0f, 0.0f, 0.0f };
		transferFuncDict.push_back(v);
	}

	for (int i = 0; i < wWidth; i++) {
		x_vertices.push_back(i / (float)wWidth);
		x_vertices.push_back(0.0f);
		rgba_vertices.push_back(0.1f);
		rgba_vertices.push_back(0.3f);
		rgba_vertices.push_back(0.5f);
		rgba_vertices.push_back(0.7f);

	}

	vaTransferFunc.Create(x_vertices.size());
	vaTransferFunc.SetArrayBuffer(0, GL_FLOAT, 2, x_vertices.data());
	vaTransferFunc.SetArrayBuffer(1, GL_FLOAT, 4, rgba_vertices.data());
	update_bins_color();
}

bool VolumeVis::Deactivate(void) {
    // --------------------------------------------------
    //  TODO: Do not forget to clear all allocated sources.
    // --------------------------------------------------
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_3D);
	glDisable(GL_BLEND);
	vaBox.Delete();
	vaCube.Delete();
	vaHisto.Delete();
	vaHistoBG.Delete();
	vaTransferFunc.Delete();
	shaderBox.RemoveAllShaders();
	shaderHisto.RemoveAllShaders();
	shaderHistoBG.RemoveAllShaders();
	shaderVolume.RemoveAllShaders();
	shaderTransferFunc.RemoveAllShaders();
	glDeleteTextures(1, &volumeTex);
	glDeleteTextures(1, &transferTex);
	x_vertices.clear();
	x_vertices.shrink_to_fit();
	rgba_vertices.clear();
	rgba_vertices.shrink_to_fit();
    return true;
}

bool VolumeVis::Init(void) {
    if (gl3wInit()) {
       fprintf(stderr,"Error: Failed to initialize gl3w.\n");
       return false;
    }
    return true;
}

bool VolumeVis::Keyboard(int key, int action, int mods, int x, int y) {
    std::string pathName = this->GetCurrentPluginPath();
    PostRedisplay();
	if (key == 82) mode_for_rgba_channel_inputs = 0;
	else if (key == 71) mode_for_rgba_channel_inputs = 1;
	else if (key == 66) mode_for_rgba_channel_inputs = 2;
	else if (key == 65) mode_for_rgba_channel_inputs = 3;
	else return false;
	input_channel_callback(mode_for_rgba_channel_inputs);
    return false;
}

/**
 * @brief VolumeVis mouse motion callback function
 * @param x
 * @param y
 */
bool VolumeVis::Motion(int x, int y) {
	if (IsLeftButtonPressed() && IsShiftPressed() && y >= wHeight - hist_panel_height) {
		if (x >= 0 && x < wWidth) {
			float val_x = x / (float)wWidth;
			float val_y = (wHeight - y) / (float)hist_panel_height;
			int current_channel = static_cast<int>(mode_for_rgba_channel_inputs);

			if (x_last != x - 1) { 
				printf("mouse %d %d => val=(%f, %f) skipping detected\n", x, y, val_x, val_y);
				for (int g = x_last + 1; g < x; g++) {
					printf("  interpolating %d\n", g);
					float y1 = rgba_vertices[x_last * 4 + current_channel];
					float interpolated_val = (val_y - y1) / (x - x_last) * (g - x_last) + y1;
					rgba_vertices[g * 4 + current_channel] = interpolated_val;
				}
			}
			else printf("mouse %d %d => val=(%f, %f)\n", x, y, val_x, val_y);

			rgba_vertices[x * 4 + current_channel] = val_y;

			vaTransferFunc.Create(x_vertices.size());
			vaTransferFunc.SetArrayBuffer(0, GL_FLOAT, 2, x_vertices.data());
			vaTransferFunc.SetArrayBuffer(1, GL_FLOAT, 4, rgba_vertices.data());
			x_last = x;
			update_bins_color();
		}
	}
    return false;
}

/**
 * @brief VolumeVis mouse callback function
 * @param button
 * @param state
 * @param x
 * @param y
 */
bool VolumeVis::Mouse(int button, int state, int mods, int x, int y) {
    PostRedisplay();
    return false;
}

/**
 * @brief VolumeVis render method
 */
bool VolumeVis::Render(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (mode == e_volume) {
		glViewport(0, static_cast<int>(hist_panel_height), wWidth, wHeight - static_cast<int>(hist_panel_height));
		viewAspect = wWidth / static_cast<float>(wHeight - static_cast<int>(hist_panel_height));
	}
	else {
		glViewport(0, 0, wWidth, wHeight);
		viewAspect = wWidth / static_cast<float>(wHeight);
	}
	projMX = glm::perspective(glm::radians(static_cast<float>(fovY)), viewAspect, 0.01f, 100.0f);

	if (showBox) {
		shaderBox.Bind();
		glUniformMatrix4fv(shaderBox.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));
		glUniformMatrix4fv(shaderBox.GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(viewMX));
		glUniform3f(shaderBox.GetUniformLocation("translate"), 0.5f, 0.5f, 0.5f);

		vaBox.Bind();
		glDrawElements(GL_LINES, ogl4_numBoxEdges * 2, GL_UNSIGNED_INT, 0);
		vaBox.Release();
		shaderBox.Release();
	}

	glm::vec3 cam_pos = glm::inverse(viewMX) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);


	// --------------------------------------------------
	//  TODO: Draw volume
	// --------------------------------------------------
	shaderVolume.Bind();
	vaCube.Bind();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, volumeTex);
	glUniform1i(glGetUniformLocation(shaderVolume.GetProgHandle(), "volume"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D, transferTex);
	glUniform1i(glGetUniformLocation(shaderVolume.GetProgHandle(), "transferTex"), 1);

	glUniform3i(shaderVolume.GetUniformLocation("volume_dims"), static_cast<int>(res_x), static_cast<int>(res_y), static_cast<int>(res_z));
	glUniform3f(shaderVolume.GetUniformLocation("volume_scale"), volume_scale.x, volume_scale.y, volume_scale.z);
	glUniform3f(shaderVolume.GetUniformLocation("cam_pos"), cam_pos.x, cam_pos.y, cam_pos.z);
	glUniform1f(shaderVolume.GetUniformLocation("c_global"), static_cast<float>(c_global));
	glUniform1f(shaderVolume.GetUniformLocation("dt"), static_cast<float>(step_size));
	glUniform1f(shaderVolume.GetUniformLocation("isovalue"), static_cast<float>(iso_value));
	glUniform1i(shaderVolume.GetUniformLocation("N"), static_cast<int>(nb_steps));
	glUniform1i(shaderVolume.GetUniformLocation("mode"), static_cast<int>(mode));

	glUniform1f(shaderVolume.GetUniformLocation("k_amb"), static_cast<float>(k_amb));
	glUniform1f(shaderVolume.GetUniformLocation("k_diff"), static_cast<float>(k_diff));
	glUniform1f(shaderVolume.GetUniformLocation("k_spec"), static_cast<float>(k_spec));
	glUniform1f(shaderVolume.GetUniformLocation("k_exp"), static_cast<float>(k_exp));

	glUniform1f(shaderVolume.GetUniformLocation("ambient"), static_cast<float>(ambient));
	glUniform1f(shaderVolume.GetUniformLocation("diffuse"), static_cast<float>(diffuse));


	glUniformMatrix4fv(shaderVolume.GetUniformLocation("projMX"), 1, GL_FALSE, glm::value_ptr(projMX));
	glUniformMatrix4fv(shaderVolume.GetUniformLocation("viewMX"), 1, GL_FALSE, glm::value_ptr(viewMX));
	glUniformMatrix4fv(shaderVolume.GetUniformLocation("modelMX"), 1, GL_FALSE, glm::value_ptr(modelMX));


	glDrawElements(GL_TRIANGLES, ogl4_numBoxFaces * 6, GL_UNSIGNED_INT, 0);

	vaCube.Release();
	shaderVolume.Release();

	// --------------------------------------------------
	//  TODO: Draw histogram
	// --------------------------------------------------
	if (mode == e_volume) {
		glViewport(0, 0, wWidth, static_cast<int>(hist_panel_height));
		viewAspect = wWidth / static_cast<float>(hist_panel_height);

		shaderHistoBG.Bind();
		vaHistoBG.Bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		vaHistoBG.Release();
		shaderHistoBG.Release();

		shaderHisto.Bind();
		vaHisto.Bind();
		glUniform1f(shaderHisto.GetUniformLocation("most"), most);
		glUniform1f(shaderHisto.GetUniformLocation("least"), least);
		glUniform1i(shaderHisto.GetUniformLocation("logPlot"), static_cast<int>(log_hist_display));
		glDrawElements(GL_TRIANGLES, hist_ibo_size, GL_UNSIGNED_INT, nullptr);
		vaHisto.Release();
		shaderHisto.Release();

		shaderTransferFunc.Bind();
		vaTransferFunc.Bind();
		glUniform1i(shaderTransferFunc.GetUniformLocation("channel"), 0);
		glDrawArrays(GL_LINE_STRIP, 0, wWidth);
		glUniform1i(shaderTransferFunc.GetUniformLocation("channel"), 1);
		glDrawArrays(GL_LINE_STRIP, 0, wWidth);
		glUniform1i(shaderTransferFunc.GetUniformLocation("channel"), 2);
		glDrawArrays(GL_LINE_STRIP, 0, wWidth);
		glUniform1i(shaderTransferFunc.GetUniformLocation("channel"), 3);
		glDrawArrays(GL_LINE_STRIP, 0, wWidth);
		vaTransferFunc.Release();
		shaderTransferFunc.Release();

	}
    return false;
}

/**
 * @brief VolumeVis resize method
 * @param width
 * @param height
 */
bool VolumeVis::Resize(int width, int height) {
    wWidth = width;
    wHeight = height;
    return true;
}

/**
 * @brief file changed callback
 * @param var  file name
 */
void VolumeVis::fileChanged( FileEnumVar<VolumeVis> &var ) {
    // --------------------------------------------------
    //  TODO: Read data from file using DatRawLoader.
    //    Set model matrix such that volume is centered
    //    and scaled into unit volume keeping aspect 
    //    ratio of the volume.
    // --------------------------------------------------
	DatRawLoader loader(var.GetSelectedFileName());

	int* res;
	res = (int*)malloc(3 * sizeof(int));
	loader.GetResolution(res);

	printf("change to volume data %s: dim=%d component=%d resolution=(%d, %d, %d) format=%d\n", 
		var.GetSelectedFileName(),
		loader.GetDimensions(),
		loader.GetNumComponents(), res[0], res[1], res[2], loader.GetDataFormat());

	volume_dims = glm::ivec3(res[0], res[1], res[2]);
	
	res_x = res[0];
	res_y = res[1];
	res_z = res[2];
	float max_res = (float)std::max(res[0], std::max(res[1], res[2]));
	volume_scale = glm::vec3(res_x/max_res, res_y/max_res, res_z/max_res);
	volume_scale = glm::vec3(1.0f);

	// reading volume data
	void* buf;
	buf = (unsigned char*)malloc(loader.GetBufferSize());
	loader.ReadTimestepToMemory(0, &buf);


	glGenTextures(1, &volumeTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, volumeTex);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, res[0], res[1], res[2], 0, GL_RED, GL_UNSIGNED_BYTE, buf);
	delete[] buf;

	/*glm::mat4 trans_mat = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f));
	glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(0.9f));
	modelMX = trans_mat * scale_mat;*/
	modelMX = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f));


}

/**
 * @brief callback function if rendering mode has changed
 * @param var
 */
void VolumeVis::modeChanged( EnumVar<VolumeVis> &var ) {
    int m = var;
    modeChanged(viewMode(m));
}

// callback for changing which channel to input
void VolumeVis::input_channel_callback(EnumVar<VolumeVis>& var) {
	printf("input channel changed to %d\n", static_cast<int>(mode_for_rgba_channel_inputs));
	return;
}

// callback for loading default transfer function
void VolumeVis::loading_default_callback(ButtonVar<VolumeVis>& var) {
	loadTransferFunc(pathName + std::string("/resources/transfer/engine.tf"));
	return;
}

// callback for loading recently saved transfer function
void VolumeVis::loading_recent_callback(ButtonVar<VolumeVis>& var) {
	loadTransferFunc(pathName + std::string("/resources/transfer/recent.tf"));
	return;
}

// callback for saving transfer function
void VolumeVis::saving_callback(ButtonVar<VolumeVis>& var) {
	saveTransferFunc(pathName + std::string("/resources/transfer/recent.tf"));
	return;
}

/**
 * @brief If rendering mode has changed
 * @param mode
 */
void VolumeVis::modeChanged( viewMode mode ) {
	printf("selecting appropriate UI\n");
	for (int i = 0; i < float_buttons.size(); i++) float_buttons[i]->SetVisible(false);
	for (int i = 0; i < uint_buttons.size(); i++) uint_buttons[i]->SetVisible(false);
	for (int i = 0; i < bool_buttons.size(); i++) bool_buttons[i]->SetVisible(false);
	mode_for_rgba_channel_inputs.SetVisible(false);
	//load_default_transfer_func.SetVisible(false);
	//load_recent_transfer_func.SetVisible(false);
	//save_transfer_func.SetVisible(false);

	if (mode == e_lineOfSight) {
		c_global.SetVisible(true);
	}
	else if (mode == e_mip) {

	}
	else if (mode == e_isosurface) {
		ambient.SetVisible(true);
		diffuse.SetVisible(true);
		k_amb.SetVisible(true);
		k_diff.SetVisible(true);
		k_exp.SetVisible(true);
		k_spec.SetVisible(true);
		iso_value.SetVisible(true);
	}
	else if (mode == e_volume) {
		log_hist_display.SetVisible(true);
		hist_panel_height.SetVisible(true);
		mode_for_rgba_channel_inputs.SetVisible(true);
		//load_default_transfer_func.SetVisible(true);
		//load_recent_transfer_func.SetVisible(true);
		//save_transfer_func.SetVisible(true);
	}

}

// splitting string, used to load transfer func from text file
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
 * @brief Load transfer function from file
 * @param filename
 */
void VolumeVis::loadTransferFunc( std::string filename ) {
	std::string line;
	std::ifstream myfile(filename);
	printf("loading transfer function...\n");

	// reading text file
	std::vector<std::vector<float>> color_map;
	if (myfile.is_open()) {
		while (getline(myfile, line)) {
			std::vector<float> numbers = split(line);
			if (numbers.size() < 4) continue;  // ignore first line
			color_map.push_back(numbers);
		}
		printf("done loading %d RGBA colors\n", color_map.size());
		myfile.close();
	}
	else {
		std::cout << "loading failed:: unable to open file" << std::endl;
		return;
	}

	// adding to rgba_vertices
	x_last = 0;
	for (int i = 0; i < color_map.size()-1; i++) {
		float dummy = (float)(color_map.size() - 1);
		int x = (int)i / dummy * wWidth;
		for (int current_channel = 0; current_channel < 4; current_channel++) {
			float val_y = color_map[i][current_channel];
			rgba_vertices[x * 4 + current_channel] = val_y;
			if (x_last != x - 1) {
				for (int g = x_last + 1; g < x; g++) {
					float y1 = rgba_vertices[x_last * 4 + current_channel];
					float interpolated_val = (val_y - y1) / (x - x_last) * (g - x_last) + y1;
					rgba_vertices[g * 4 + current_channel] = interpolated_val;
				}
			}
		}
		x_last = x;

	}
	
	vaTransferFunc.Create(x_vertices.size());
	vaTransferFunc.SetArrayBuffer(0, GL_FLOAT, 2, x_vertices.data());
	vaTransferFunc.SetArrayBuffer(1, GL_FLOAT, 4, rgba_vertices.data());
	update_bins_color();
}

/**
 * @brief Save transfer function to file
 * @param filename
 */
void VolumeVis::saveTransferFunc( std::string filename ) {
	std::ofstream myfile(filename);

	if (myfile.is_open())
	{
		for (int i = 0; i < wWidth; i++) {
			for (int j = 0; j < 4; j++) {
				if (j == 3) myfile << rgba_vertices[i * 4 + j] << "\n";
				else myfile << rgba_vertices[i * 4 + j] << " ";
			}
		}
		myfile.close();
	}
	else std::cout << "saving transfer function failed: unable to open file\n";
	printf("saved transfer function...\n");
}

/**
 * @brief Set volume data
 * @param buf
 * @param res
 * @param format
 */
void VolumeVis::setVolumeData( const void* buf, const int* res, const DatRawDataFormat format ) {
}
