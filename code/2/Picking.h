/*
 * Picking.h
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
 
#include <vector>
#include "RenderPlugin.h"
#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"


#include "GL/gl3w.h"
#include "GLShader.h"
#include "VertexArray.h"
#include "Manipulator.h"

/**
 * The Object structure combines 
 *   - the id of the object and the corresponding color
 *   - the model matrix
 *   - a pointer to the vertex array
 *   - the number of elements
 *   - a pointer to the object shader
 *   - a texture ID
 */
typedef struct Object_t {
    int          id;
	int          color_id;
    glm::vec3    idcol;
    glm::mat4    modelMX;
    VertexArray* va;
    int          numElements;
    GLShader*    shader;
    GLuint       texID;
    Object_t() {
        id = 0;
		color_id = 1;
        idcol = glm::vec3(0,0,0);
        modelMX = glm::mat4(1.0f);
        va = nullptr;
        numElements = 0;
        shader = nullptr;
        texID = 0;
    }
    Object_t( int id, int color_id, glm::mat4 modelMX, VertexArray* va, int numElements, GLShader* shader, GLuint texID ) {
        this->id = id;
		this->color_id = color_id;
        this->modelMX = modelMX;
        this->va = va;
        this->numElements = numElements;
        this->shader = shader;
        this->texID = texID;
    }
} Object;

class OGL4COREPLUGIN_API Picking : public RenderPlugin {
public:
    Picking(COGL4CoreAPI *Api);
    ~Picking(void);

    virtual bool Activate(void);
    virtual bool Deactivate(void);
    virtual bool Init(void);
    virtual bool Keyboard(int key, int action, int mods, int x, int y);
    virtual bool Motion(int x, int y);   
    virtual bool Mouse(int button, int state, int mods, int x, int y);
    virtual bool Render(void);      
    virtual bool Resize(int w, int h);

private:
    void initVAs();
    void createBoxAndCube();
    void createSphere();
    void createTorus();
    
    void initFBOs();
    void deleteFBOs();
    void checkFBOStatus();    
    void createFBOTexture( GLuint &outID, const GLenum internalFormat, 
                           const GLenum format, const GLenum type, 
                           GLint filter, int width, int height );        
    void drawToFBO();
    
    glm::vec3 idToColor( unsigned int id );
    unsigned int colorToId(unsigned char buf[3]);

	void transform_mode_func(EnumVar<Picking>& var);             // change the way to manipulate positions of objects
	void show_fbo_att_func(EnumVar<Picking>& var);               // change which render target
	void change_proj_mx(APIVar<Picking, FloatVarPolicy>& var);   // change near far components of projection matrix
	void change_light_pos(APIVar<Picking, Dir3FVarPolicy>& var);
	void toggle_light_on_off(ButtonVar<Picking>& var);


private:
	APIVar<Picking, Dir3FVarPolicy>  manipulate_light_pos;          //!<  Change light position
    APIVar<Picking, FloatVarPolicy>  fovY;          //!<  Camera's vertical field of view
    APIVar<Picking, FloatVarPolicy>  zNear;         //!<  near clipping plane
    APIVar<Picking, FloatVarPolicy>  zFar;          //!<  far clipping plane
    APIVar<Picking, BoolVarPolicy>   useWireframe;  //!<  toggle wireframe mode
    EnumVar<Picking> showFBOAtt;                    //!<  selector to show the different fbo attachments
	EnumVar<Picking> transform_mode;
	ButtonVar<Picking> toggle_light; // turn on off shading

	int transformation_mode = 0; // which mode to manipulate world positions (trans or rotate)
	int att_id = 0; // which attachment ID to render
	int depth_map = 0; // render depth map or not
	bool show_light = false; // shading or not
	glm::vec3 light_pos = glm::vec3(1.0f);

    
    GLShader shaderCube;
    std::string vsCube;
    std::string gsCube;
    std::string fsCube;
    
    GLShader shaderBox;
    std::string vsBox;
    std::string fsBox;
    
    GLShader shaderSphere;
    std::string vsSphere;
    std::string gsSphere;
    std::string fsSphere;

    GLShader shaderTorus;
    std::string vsTorus;
    std::string gsTorus;
    std::string fsTorus;
    
    GLShader shaderQuad;
    std::string vsQuad;
    std::string fsQuad;

    VertexArray vaBox;
    VertexArray vaCube;
	VertexArray vaQuad;
    VertexArray vaSphere;
    VertexArray vaTorus;

	GLuint vaCubeTex;
	GLuint vaSphereTex;

	glm::vec3 scaleCube = glm::vec3(1.05f, 1.05f, 1.05f);
	glm::vec3 scaleTorus = glm::vec3();
	glm::vec3 scaleSphere = glm::vec3();


    glm::mat4 projMX;       //!< Camera's projection matrix
    glm::mat4 viewMX = glm::translate(glm::mat4(), glm::vec3(-3.0f, 0.0f, 0.0f));       //!< Camera's view matrix
    
    int   wWidth, wHeight;  //!< width and height of the window
    float aspect;           //!< aspect ratio of window
    
    GLuint fboID,dboID;     //!< handles for FBO and depth buffer attachment
    GLuint colAttachID[3];  //!< handles for color attachments

	GLuint cubeIBO;
	GLuint sphereIBO;
	GLuint torusIBO;
	int sphereIBOsize;
	int torusIBOsize;
    
    GLuint texEarth;
    GLuint texCrate;
    GLuint texBoard;
	GLuint texScreen;
	GLuint texDice;

	std::vector<glm::vec3> translate_vectors;
	std::vector<glm::vec3> rotation_angles;
	std::vector<glm::vec3> scale_vectors;

    // <0 : no object picked
    int pickedObjNum;
    std::vector<Object> mObjectList;   // list of the objects
	std::vector<int> picking_indices;  // indicating if the object is picked
    
    
    // --------------------------------------------------
    //  Variables and parameters for the BONUS task:
    //  Deferred Lighting 
    // --------------------------------------------------
    APIVar<Picking, FloatVarPolicy>  lightLong;  //!< position of light, longitude in degree
    APIVar<Picking, FloatVarPolicy>  lightLat;   //!< position of light, latitude in degree
    APIVar<Picking, FloatVarPolicy>  lightDist;  //!< position of light, distance from origin
    APIVar<Picking, FloatVarPolicy>  lightFoV;   //!< field of view of spot light
    
    GLuint lightFboID, lightDboID;    //!< handle for FBO and depth buffer attachment
    GLuint lightFboSize[2];           //!< size of FBO
    GLuint lightColAttachID;          //!< handle for color attachment
    float  lightZnear, lightZfar;     //!< near and far clipping plane of spot light
    
    glm::mat4 lightProjMX;   //!< spot light's projection matrix
    glm::mat4 lightViewMX;   //!< spot light's view matrix
};

extern "C" OGL4COREPLUGIN_API RenderPlugin* OGL4COREPLUGIN_CALL CreateInstance(COGL4CoreAPI *Api) {
    return new Picking(Api);
}
