# Fluid Visualization - Final project practical Visual Computing 
This is the final project for the course "Practical Visual Computing" at University of Stuttgart. The project is built on top of the OGL4CORE from University Stuttgart's VIS institute.
## Screenshots
[Video](https://youtu.be/XzIJiEZsJQ8)
[All screenshots](https://github.com/sontung/graphics-stuffs/tree/master/vis)

![isosurface](https://raw.githubusercontent.com/sontung/graphics-stuffs/master/vis/iso-dam.PNG)

![isosurface with ray tracing](https://raw.githubusercontent.com/sontung/graphics-stuffs/master/vis/iso-rc-dam.PNG)
 ## Manual
 The program has two modes: rasterization and ray casting. The table below
 summarizes all buttons of the UI. 
 
  | Buttons | Effect | 
  | ------------- |:-------------:| 
  | Mode | switch between two modes | 
  | Dataset| switch between two datasets| 
  | Mode | switch between two modes | 
  | Radius| radius of spheres | 
  | AutoChangeTimestep| auto advance to next time step | 
  | ShowSkybox| render skybox |
  | ShowUnitBox| render the bounding box |
  | ShowIsoSurface| render the isosurface |
  | ShowSpheres| render particles as spheres |
  | Theta| spherical coordinates of the camera * |  
  | Phi| spherical coordinates of the camera * |    
  | VisualEffect| weight of reflection and refraction * | 
  | ResX| resolution of image plane * | 
  | ResY| resolution of image plane * | 
  | AutoRotate| auto change camera position * | 
  
  Note * : only in raycasting mode
  ## Implementation
 ![Flowchart](https://raw.githubusercontent.com/sontung/graphics-stuffs/master/vis/diag.png)


| Function | Effect |
|--|--|
| Activate | initial setup |
| modeChanged | callback when changing render mode|
| datasetChanged| callback when changing dataset |
| generate_possible_cam_pos| calculate all possible camera position to randomly choose later |
| createSphere| render one set sphere vertices  |
| loadSkybox| read and load skybox texture |
| load_grid3d2iso| read in neighborhood search |
| read_sph_data| read in SPH data, i.e, positions of all particles in all time steps |
| extract_isosurface| extract isosurface for each time step|
| Render| render the scene|


