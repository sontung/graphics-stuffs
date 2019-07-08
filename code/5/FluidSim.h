/*
 * FluidSim.h
 *
 * Copyright (c) 2014 by Universitaet Stuttgart (VISUS).
 * All rights reserved.
 *
 * Created by Thomas Mueller  <Thomas.Mueller@vis.uni-stuttgart.de>
 *
 * Description:
 *    Practical Course in Visual Computing
 *    Second assignment
 *
 * This file is part of OGL4Core.
 */

#include "RenderPlugin.h"
#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"

#include "datraw/datRaw.h"
#include "GL/gl3w.h"
#include "GLShader.h"
#include "VertexArray.h"
#include "dirent.h"
#include "camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <regex>
#include <random>
#include <stdio.h>
#include <unordered_map>
#include <omp.h>


enum viewMode {
	e_lineOfSight = 0,
	e_mip,
	e_isosurface,
	e_volume
};

class OGL4COREPLUGIN_API FluidSim : public RenderPlugin {
public:
	FluidSim(COGL4CoreAPI* Api);
	~FluidSim(void);

	virtual bool Activate(void);
	virtual bool Deactivate(void);
	virtual bool Init(void);
	virtual bool Keyboard(int key, int action, int mods, int x, int y);
	virtual bool Motion(int x, int y);
	virtual bool Mouse(int button, int state, int mods, int x, int y);
	virtual bool Render(void);
	virtual bool Resize(int w, int h);

private:
	void modeChanged(EnumVar<FluidSim>& var); // callback for changing mode
	void datasetChanged(EnumVar<FluidSim>& var); // callback for changing dataset
	void read_sph_data(); // read in sph data
	void createSphere(); // create spheres in sphere rendering mode
	void loadSkybox(); // load sky box
	void generate_possible_cam_pos(); // sample all the possible camera postion for ray casting in auto rotate mode
	void extract_isosurface(int time_step); // extract the isosurface of this time step
	void load_grid3d2iso(); // load neighbor search into mem
	glm::vec3 compute_cam_pos(); // calculate current camera position based on theta and phi
	std::vector<float> compute_cube_position(std::vector<int> a_cube, std::vector<int>& iso_info, int time); // compute if a cube is submerge into fluid

private:

	APIVar<FluidSim, BoolVarPolicy> show_skybox;    // show the skybox
	APIVar<FluidSim, BoolVarPolicy> show_unitbox;   // show unitbox that covers all the particles
	APIVar<FluidSim, BoolVarPolicy> show_spheres;   // render particles as spheres
	APIVar<FluidSim, BoolVarPolicy> show_iso;       // render the isosurface
	APIVar<FluidSim, BoolVarPolicy> auto_rotate;    // auto rotate in raycasting mode
	APIVar<FluidSim, BoolVarPolicy> auto_change_ts; // auto change time step when rendering


	APIVar<FluidSim, FloatVarPolicy> fovY;                  // field of view
	APIVar<FluidSim, FloatVarPolicy> sphere_radius;         // radius when test sphere intersection
	APIVar<FluidSim, FloatVarPolicy> theta_cam;             // theta of the camere in raycasting mode
	APIVar<FluidSim, FloatVarPolicy> phi_cam;
	APIVar<FluidSim, FloatVarPolicy> visual_eff; // weight of reflection and refaction

	APIVar<FluidSim, IntVarPolicy> res_x; // X res of screen
	APIVar<FluidSim, IntVarPolicy> res_y; // Y res of screen


	EnumVar<FluidSim> dataset; // choose between DamBreak and Emitter
	EnumVar<FluidSim> mode;  
	int camHandle;

	// some strings define the cached computation
	std::string path_to_marching_cube_file;
	std::string path_to_time_step_folder;
	std::string path_to_time_step_file;

	// skybox
	GLuint cube_map_tex;
	VertexArray vaSkybox;

	// FBO related variables
	GLuint fboID, dboID;
	GLuint colAttachID[3];

	float time_step = 1.0f;   // first time step
	float step_size = 0.05f;  // step size to move to next time step
	int max_time_step = 13;   // last time step
	int min_time_step = 1;    // first time step

	glm::vec3 light_pos = glm::vec3(0, 0, 10); // position of light
	std::string pathName; // path name of program

	glm::mat4 modelMX;   //!< model matrix
	glm::mat4 viewMX;    //!< view matrix
	glm::mat4 projMX;    //!< projection matrix

	int wWidth, wHeight;   //!< window width and height
	float viewAspect;     //!< view aspect ratio

	std::string quadVertShaderName;        //!< vertex shader filename for window filling rectangle
	std::string quadFragShaderName;        //!< fragment shader filename for window filling rectangle
	GLShader    shaderQuad;                //!< shader program for window filling rectangle
	VertexArray vaQuad;

	std::string boxVertShaderName;    //!< vertex shader filename for box rendering
	std::string boxFragShaderName;    //!< fragment shader filename for box rendering
	GLShader    shaderBox;            //!< shader program for box rendering
	VertexArray vaBox;                //!< vertex array for box

	std::vector<float> grid_positions;        // positions of grids
	std::vector<int> grid_if_near_particles;  // 1 if this grid is close to any particle, 0 otherwise
	GLuint ssbo_grid_if_near_particles;       // same above but a ssbo
	std::map<std::tuple<int, int, int>, std::vector<int>> grid3dpos2isoval; // same as above but a map

	float cube_size = 0.1;                   // cube size in marching cube
	std::vector<std::vector<int>> all_cubes;  // cube 8 vertices' positions
	glm::vec3 cube_origin;                    // cube first vertex
	VertexArray vaGrid;                       // va for grid
	GLShader shaderGrid;                      // shader for grid

	// isosurface ray casting
	int cam_pos_index = 0; // camera position
	std::vector<float> possible_cam_pos; // all camera position will be sampled 
	GLShader shaderIsosurface; // isosurface as triangles
	GLShader shaderIsosurfaceRayTracing; // isosurface as ray tracing
	std::vector<float> all_triangles; // all the triangles for the isosurface for all time steps
	GLuint ssbo_triangles_pos; // same above
	std::vector<int> triangle_size_each_time_step; // size of isosurface for each time step
	GLuint vaIsosurfaceID;// va for isosurface
	

	// sphere ray casting
	std::string particleVertShaderName;    //!< vertex shader filename for box rendering
	std::string particleFragShaderName;    //!< fragment shader filename for box rendering
	GLShader    shaderParticle;            //!< shader program for box rendering
	VertexArray vaParticle;                //!< vertex array for box
	std::vector<VertexArray> VA_timesteps;
	std::vector<float> particles_position; // positions of particles for all time step
	GLuint ssbo_particles_pos;

	std::vector<int> nb_particles; // number of particles for each time step

	// render particles as spheres
	GLShader shaderSphere; 
	VertexArray vaSphere;
	int sphere_ibo_size;

	// for debugging
	GLShader shaderCube;
	VertexArray vaCube;
};

extern "C" OGL4COREPLUGIN_API RenderPlugin* OGL4COREPLUGIN_CALL CreateInstance(COGL4CoreAPI* Api) {
	return new FluidSim(Api);
}
