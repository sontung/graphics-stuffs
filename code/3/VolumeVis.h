/*
 * VolumeVis.h
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
#include "datraw/datRaw.h"
#include "GL/gl3w.h"
#include "GLShader.h"
#include "VertexArray.h"
#include <iostream>
#include <fstream>
#include <string>
#include <array>


enum viewMode {
    e_lineOfSight = 0,
    e_mip,
    e_isosurface,
    e_volume
};

class OGL4COREPLUGIN_API VolumeVis : public RenderPlugin {
public:
    VolumeVis(COGL4CoreAPI *Api);
    ~VolumeVis(void);

    virtual bool Activate(void);
    virtual bool Deactivate(void);
    virtual bool Init(void);
    virtual bool Keyboard(int key, int action, int mods, int x, int y);
    virtual bool Motion(int x, int y);
    virtual bool Mouse(int button, int state, int mods, int x, int y);
    virtual bool Render(void);
    virtual bool Resize(int w, int h);

private:
    void fileChanged( FileEnumVar<VolumeVis> &var );
    void modeChanged( EnumVar<VolumeVis> &var );
    void modeChanged( viewMode mode );

	void input_channel_callback(EnumVar<VolumeVis>& var);
	void loading_default_callback(ButtonVar<VolumeVis>& var);
	void loading_recent_callback(ButtonVar<VolumeVis>& var);
	void saving_callback(ButtonVar<VolumeVis>& var);

    
    void loadTransferFunc( std::string filename );
    void saveTransferFunc( std::string filename );
    void initTransferFunc();
	void update_bins_color();
    
    void setVolumeData( const void* buf, const int* res, const DatRawDataFormat format );

private:
    FileEnumVar<VolumeVis>  files;                 //!< file selector
    
    APIVar<VolumeVis, BoolVarPolicy> showBox;      //!< toggle box drawing
    APIVar<VolumeVis, FloatVarPolicy> fovY;        // field of view
	APIVar<VolumeVis, UIntVarPolicy> res_x;        // dim x
	APIVar<VolumeVis, UIntVarPolicy> res_y;        // dim y
	APIVar<VolumeVis, UIntVarPolicy> res_z;        // dim z
	APIVar<VolumeVis, UIntVarPolicy> nb_steps;     // number of steps to trace
	APIVar<VolumeVis, FloatVarPolicy> step_size;   // size of each step


	APIVar<VolumeVis, UIntVarPolicy> hist_panel_height;   // histogram panel height
	APIVar<VolumeVis, FloatVarPolicy> c_global;           // global scaling
	APIVar<VolumeVis, FloatVarPolicy> iso_value;          // iso value
	APIVar<VolumeVis, FloatVarPolicy> k_amb;              // K ambient
	APIVar<VolumeVis, FloatVarPolicy> k_diff;             // K diffuse
	APIVar<VolumeVis, FloatVarPolicy> k_spec;             // K spectacular
	APIVar<VolumeVis, FloatVarPolicy> k_exp;              // K exponential
	APIVar<VolumeVis, FloatVarPolicy> ambient;            // Ca
	APIVar<VolumeVis, FloatVarPolicy> diffuse;            // Cin
	APIVar<VolumeVis, BoolVarPolicy>  log_hist_display;   // toggle display log histogram


    EnumVar<VolumeVis> mode;
	EnumVar<VolumeVis> mode_for_rgba_channel_inputs;    // changing which channel to input

	ButtonVar<VolumeVis> save_transfer_func;            // saving transfer func
	ButtonVar<VolumeVis> load_default_transfer_func;    // loading default transfer func
	ButtonVar<VolumeVis> load_recent_transfer_func;     // loading recently save transfer func


	// to set visible api for each mode 
	std::vector<APIVar<VolumeVis, FloatVarPolicy>*> float_buttons;
	std::vector<APIVar<VolumeVis, UIntVarPolicy>*> uint_buttons;
	std::vector<APIVar<VolumeVis, BoolVarPolicy>*> bool_buttons;

	glm::ivec3 volume_dims;
	glm::vec3 volume_scale;

	std::string pathName;

    glm::mat4 modelMX;   //!< model matrix
    glm::mat4 viewMX;    //!< view matrix
    glm::mat4 projMX;    //!< projection matrix
    
    int wWidth,wHeight;   //!< window width and height
    float viewAspect;     //!< view aspect ratio
            
    std::string boxVertShaderName;    //!< vertex shader filename for box rendering
    std::string boxFragShaderName;    //!< fragment shader filename for box rendering
    GLShader    shaderBox;            //!< shader program for box rendering
    VertexArray vaBox;                //!< vertex array for box
    
    std::string volVertShaderName;    //!< vertex shader filename for volume rendering
    std::string volFragShaderName;    //!< fragment shader filename for volume rendering
    GLShader    shaderVolume;         //!< shader program for volume rendering
    VertexArray vaCube;               //!< vertex array for volume faces
    GLuint      volumeTex;            //!< texture handle for volume data
    
    std::string histoVertShaderName;    //!< vertex shader filename for histogram rendering
    std::string histoGeomShaderName;    //!< geometry shader filename for histogram rendering
    std::string histoFragShaderName;    //!< fragment shader filename for histogram rendering
    GLShader    shaderHisto;            //!< shader program for histogram rendering
    VertexArray vaHisto;                //!< vertex array for histogram data
	int hist_ibo_size = 0;              // Size of IBO for drawing histogram
	float most = -1.0f;                 // most freq value in the hist
	float least = -1.0f;                // least freq value in the hist
    
    std::string histoBGVertShaderName;  //!< vertex shader filename for histogram background
    std::string histoBGFragShaderName;  //!< fragment shader filename for histogram background
    GLShader    shaderHistoBG;          //!< shader program for histogram background
    VertexArray vaHistoBG;              //!< vertex array for histogram background
    
    std::string transferFuncVertShaderName;  //!< vertex shader filename for transfer functions
    std::string transferFuncFragShaderName;  //!< fragment shader filename for transfer functions
    GLShader    shaderTransferFunc;          //!< shader program for transfer functions
    VertexArray vaTransferFunc;              //!< vertex array for transfer functions
    GLuint       transferTex;                //!< transfer function texture handle
	std::vector<std::array<float, 4>> transferFuncDict; // store RGBA value for each bin
	std::vector<float> rgba_vertices; // VBO color for visualizing transfer function
	std::vector<float> x_vertices;    // VBO position for visualizing transfer function
	int x_last = 0;                   // last position of mouse motion

	float bin_width;
    unsigned int numBins = 100;        //!< number of bins for histogram
        
private:    
    /**
     * @brief create histogram depending on data type 'T'.
     * @param bins   number of bins
     * @param min    minimum value
     * @param max    maximum value
     * @param values pointer to data values
     * @param numValues number of values in the array pointed to by values.
     */
    template<class T>
    void genHistogram( unsigned int bins, 
                       T* values, unsigned int numValues ) {
        if (bins == 0 || values == nullptr || numValues == 0 ) {
            return;
        }
		float val;
		float prev_val = 0;
		std::vector<float> hist;
		for (int i = 0; i < bins; i++) hist.push_back(0.0f);

		// find min max values
		float min_val = -1.0f;
		float max_val = -1.0f;
		for (int i = 0; i < numValues; i++) {
			val = static_cast<float>(values[i]);
			if (val <= 0.0f) continue;
			if (max_val == -1.0f) max_val = val;
			if (min_val == -1.0f) min_val = val;
			if (val > max_val) max_val = val;
			if (val < min_val) min_val = val;
		}
		printf("found min=%f max=%f\n", min_val, max_val);

		// assign bin
		bin_width = (max_val - min_val) / bins;
		for (int i = 0; i < numValues; i++) {
			val = static_cast<float>(values[i]);
			if (val <= 0.0f) continue;

			if (val == min_val) hist[0]++;
			else if (val == max_val) hist[bins-1]++;
			else {
				int ind = (int)std::ceil((val - min_val) / bin_width) - 1;
				hist[ind]++;
			}
		}

		for (int i = 0; i < bins; i++) std::cout << hist[i] << " ";
		printf("\n");

		// find most and least freq value
		for (int i = 0; i < bins; i++) {
			if (i == 0) {
				most = hist[i];
				least = hist[i];
				continue;
			}
			if (hist[i] > most) most = hist[i];
			if (hist[i] < least && hist[i] != 0) least = hist[i];
		}
		printf("found most=%f %f least=%f %f\n", most, std::log(most), least, std::log(least));
		for (int i = 0; i < bins; i++) hist[i] = std::log(hist[i]);
		most = std::log(most);
		least = std::log(least);


		// add vertices and indices
		std::vector<float> vertices;
		std::vector<int> indices;
		bin_width = 1 / (float)bins;
		int base;
		for (int i = 0; i < bins; i++) {
			base = i * 4;
			vertices.push_back(bin_width * i);
			vertices.push_back(0.0f);
			vertices.push_back(bin_width * (i + 1));
			vertices.push_back(0.0f);
			vertices.push_back(bin_width * i);
			vertices.push_back(hist[i]);
			vertices.push_back(bin_width * (i + 1));
			vertices.push_back(hist[i]);

			indices.push_back(base);
			indices.push_back(base + 1);
			indices.push_back(base + 2);
			indices.push_back(base + 1);
			indices.push_back(base + 2);
			indices.push_back(base + 3);
		}

		vaHisto.Create(vertices.size());
		vaHisto.SetArrayBuffer(0, GL_FLOAT, 2, vertices.data());
		vaHisto.SetElementBuffer(0, indices.size(), indices.data());
		hist_ibo_size = indices.size();

		// free up vector
		hist.clear();
		hist.shrink_to_fit();
		vertices.clear();
		vertices.shrink_to_fit();
		indices.clear();
		indices.shrink_to_fit();

    }
};

extern "C" OGL4COREPLUGIN_API RenderPlugin* OGL4COREPLUGIN_CALL CreateInstance(COGL4CoreAPI *Api) {
    return new VolumeVis(Api);
}
