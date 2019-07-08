/*
 * SurfaceVis.h
 *
 * Copyright (c) 2014 by Universitaet Stuttgart (VISUS).
 * All rights reserved.
 *
 * Created by Thomas Mueller  <Thomas.Mueller@vis.uni-stuttgart.de>
 *
 * Description:
 *    Practical Course in Visual Computing
 *    Fourth assignment
 *
 * This file is part of OGL4Core.
 */

#include "RenderPlugin.h"
#include "glm/glm.hpp"
#include "GL/gl3w.h"
#include "FramebufferObject.h"
#include "GLShader.h"
#include "VertexArray.h"
#include <random>
#include <array>
#include <fstream>



typedef struct Object_t {
	int id;
	float x;
	float y;
	float z;
	float x_ori;
	float y_ori;
	float z_ori;
	int selected;
	Object_t() {
		id = 0;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float x_ori = 0.0f;
		float y_ori = 0.0f;
		float z_ori = 0.0f;
		int selected = 0;
	}
	Object_t(int id, float x, float y) {
		this->id = id;
		this->x = x;
		this->y = y;
		this->z = 0.0f;
		this->x_ori = x;
		this->y_ori = y;
		this->z_ori = 0.0f;
		this->selected = 0;

	}
} Object;

class OGL4COREPLUGIN_API SurfaceVis : public RenderPlugin {
public:
	SurfaceVis(COGL4CoreAPI *Api);
	~SurfaceVis(void);

	virtual bool Activate(void);
	virtual bool Deactivate(void);
	virtual bool Init(void);
	virtual bool Keyboard(int key, int action, int mods, int x, int y);
	virtual bool Motion(int x, int y);
	virtual bool Mouse(int button, int state, int mods, int x, int y);
	virtual bool Render(void);
	virtual bool Resize(int w, int h);

	bool LoadCtrlPoints(std::string filename);
	void SaveCtrlPoints(ButtonVar<SurfaceVis>& var);

private:
	bool initFBO();
	void drawToFBO();

	void LoadControlPointCb(EnumVar<SurfaceVis>& var);

	void createFBOTexture(GLuint& outID, const GLenum internalFormat,
		const GLenum format, const GLenum type,
		GLint filter, int width, int height);

	void createVA_control_points();
	void updateVA_control_points();

	int find_span_u(float u); // debugging


	glm::vec3    idToColor(unsigned int id);
	unsigned int colorToId(unsigned char buf[3]);

private:
	APIVar<SurfaceVis, UIntVarPolicy> tess_lvl_out;
	APIVar<SurfaceVis, UIntVarPolicy> tess_lvl_in;

	EnumVar<SurfaceVis> which_file_to_load; // which file to load as predefined control points

	APIVar<SurfaceVis, UIntVarPolicy> nb_control_points_m;
	APIVar<SurfaceVis, UIntVarPolicy> nb_control_points_n;
	int prev_m;
	int prev_n;
	int nb_patches;
	bool predefined_points_loaded = false;

	APIVar<SurfaceVis, BoolVarPolicy>  show_control_points;        // show control points
	APIVar<SurfaceVis, BoolVarPolicy>  show_normal_vectors;        //!< show normal vectors
	APIVar<SurfaceVis, BoolVarPolicy>  render_b_surface;           //!< render B surface 
	APIVar<SurfaceVis, BoolVarPolicy>  grid_lines;                 //!< draw grid lines
	APIVar<SurfaceVis, BoolVarPolicy>  depth_test;                 //!< toggle depth test
	APIVar<SurfaceVis, FloatVarPolicy> fovY;              //!< camera's vertical field of view
	APIVar<SurfaceVis, IntVarPolicy>   pointSize;         //!< point size of control points


	StringVar<SurfaceVis>  dataFilename;                  //!< Filename for loading/storing data

	APIVar<SurfaceVis, BoolVarPolicy>  doWireframe;       //!< toggle wireframe drawing

	APIVar<SurfaceVis, Color3FVarPolicy> ambientColor;
	APIVar<SurfaceVis, Color3FVarPolicy> diffuseColor;
	APIVar<SurfaceVis, Color3FVarPolicy> specularColor;
	APIVar<SurfaceVis, FloatVarPolicy>   k_ambient;
	APIVar<SurfaceVis, FloatVarPolicy>   k_diffuse;
	APIVar<SurfaceVis, FloatVarPolicy>   k_specular;
	APIVar<SurfaceVis, FloatVarPolicy>   k_exp;
	APIVar<SurfaceVis, IntVarPolicy>     freq;            //!< frequency of checkerboard texture

	ButtonVar<SurfaceVis> save_control_points;            // saving control points


	// FBO related variables
	GLuint fboID, dboID;
	GLuint colAttachID[3];


	int wWidth, wHeight;   //!< window width and height
	float viewAspect;     //!< view aspect ratio

	glm::mat4 modelMX;   //!< model matrix
	glm::mat4 viewMX;    //!< view matrix
	glm::mat4 projMX;    //!< projection matrix

	std::string boxVertShaderName;         //!< vertex shader filename for box rendering
	std::string boxFragShaderName;         //!< fragment shader filename for box rendering
	GLShader    shaderBox;                 //!< shader program for box rendering
	VertexArray vaBox;                     //!< vertex array for box

	std::string ctrlPointVertShaderName;   //!< vertex shader filename for control point rendering
	std::string ctrlPointFragShaderName;   //!< fragment shader filename for control point rendering
	GLShader    shaderCtrlPoints;          //!< shader program for control point rendering
	VertexArray vaCtrlPoints;              //!< vertex array for control points
	std::vector<int> control_points_index;      // index of control point in the net
	std::vector<float> control_points_color;    // color of control point when selected or not
	std::vector<Object> list_of_control_points; // control points stored as an object
	std::vector<float> knot_vec_u;              // knot vec U
	std::vector<float> knot_vec_v;              // knot vec V
	std::vector<float> control_point_positions; // positions of control points
	GLuint ssbo_control_points;                 // ssbo for control point position


	std::string bsVertShaderName;          //!< vertex shader filename for b-spline surface rendering
	std::string bsTCShaderName;            //!< tessellation control shader filename for b-spline surface rendering
	std::string bsTEShaderName;            //!< tessellation evaluation shader filename for b-spline surface rendering
	std::string bsGeomShaderName;          //!< geometry shader filename for b-spline surface rendering
	std::string bsFragShaderName;          //!< fragment shader filename for b-spline surface rendering
	GLShader    shaderBSplineSurface;      //!< shader program for b-spline surface rendering
	VertexArray vaBSplineSurface;          //!< vertex array for b-spline, not necessary
	GLuint ssbo_u;
	GLuint ssbo_v;


	std::string quadVertShaderName;        //!< vertex shader filename for window filling rectangle
	std::string quadFragShaderName;        //!< fragment shader filename for window filling rectangle
	GLShader    shaderQuad;                //!< shader program for window filling rectangle
	VertexArray vaQuad;                    //!< vertex array for window filling rectangle
};

extern "C" OGL4COREPLUGIN_API RenderPlugin* OGL4COREPLUGIN_CALL CreateInstance(COGL4CoreAPI *Api) {
	return new SurfaceVis(Api);
}
