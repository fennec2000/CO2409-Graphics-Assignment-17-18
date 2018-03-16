//--------------------------------------------------------------------------------------
// Parallax mapping - scene setup, update and rendering
//--------------------------------------------------------------------------------------

// Header guard - prevents this file being included more than once
#ifndef CO2409_SCENE_H_INCLUDED
#define CO2409_SCENE_H_INCLUDED

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
// Using externs to make global variables from the .cpp file available to other files.
// It would be better to use classes and avoid globals to make the code tidier. However
// that might hide the overall picture and might make things more difficult to follow.

// Constants controlling speed of movement/rotation
extern const float kRotationSpeed;
extern const float kMovementSpeed;
extern const float kScaleSpeed;


//--------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------
// Make functions available to other files

// Create / load the camera, models and textures for the scene
bool InitScene();

// Release scene related objects (models, textures etc.) to free memory when quitting
void ReleaseScene();

// Update everything in the scene
void UpdateScene(float frameTime);

// Render everything in the scene
void RenderScene();


#endif // End of header guard (see top of file)
