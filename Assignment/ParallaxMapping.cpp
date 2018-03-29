//--------------------------------------------------------------------------------------
// Parallax mapping - scene setup, update and rendering
//--------------------------------------------------------------------------------------

#include "Scene.h"
#include "Device.h"
#include "Model.h"
#include "Camera.h"
#include "Shader.h"
#include "Input.h"  // Input functions - not DirectX
#include "Colour\ColourConversions.h"  // my hsl and rbs conversions


//--------------------------------------------------------------------------------------
// Global Scene Variables
//--------------------------------------------------------------------------------------

// Enums
enum LightState { R, G, B };        // current target for the light to become

// Constants controlling speed of movement/rotation. [Note for Games Dev 1 students: measured in units per second because we're using frame time]
const float kRotationSpeed = 2.0f;  // 4 radians per second for rotation
const float kMovementSpeed = 50.0f; // 10 units per second for movement (what a unit of length is depends on 3D model - i.e. an artist decision usually)
const float kScaleSpeed    = 2.0f;  // 2x or 1/2x scaling each second - this is used as a multiplier/divider
float Pulse;                        // pulsating light value
float Mover;                        // mover value
float Wiggle;                       // wiggle value

//-------------------------------------

// Models and cameras. Now encapsulated in classes for flexibity and convenience
// The CModel class collects together geometry and world matrix, and provides functions to control the model and render it
// The CCamera class handles the view and projections matrice, and provides functions to control the camera
Model*  Cube;
Model*  Teapot;
Model*  Floor;
Model*  Sphere;
Camera* MainCamera;

// Textures - including normal maps
ID3D10ShaderResourceView* CubeDiffuseMap   = NULL;
ID3D10ShaderResourceView* CubeNormalMap    = NULL;
ID3D10ShaderResourceView* TeapotDiffuseMap = NULL;
ID3D10ShaderResourceView* TeapotNormalMap  = NULL;
ID3D10ShaderResourceView* SphereDiffuseMap = NULL;
ID3D10ShaderResourceView* SphereNormalMap  = NULL;
ID3D10ShaderResourceView* FloorDiffuseMap  = NULL;
ID3D10ShaderResourceView* FloorNormalMap   = NULL;
ID3D10ShaderResourceView* LightDiffuseMap  = NULL;
float ParallaxDepth = 0.08f; // Overall depth of bumpiness for parallax mapping
bool UseParallax    = true;  // Toggle for parallax 

//-------------------------------------

// Light data still stored manually, a light class would be helpful - but it's an assignment task!
D3DXVECTOR3 AmbientColour = D3DXVECTOR3( 0.2f, 0.2f, 0.3f );
D3DXVECTOR3 Light1Colour  = D3DXVECTOR3( 0.8f, 0.8f, 1.0f );
D3DXVECTOR3 Light2Colour  = D3DXVECTOR3( 1.0f, 0.8f, 0.2f ) * 30;
D3DXVECTOR3 Light2ColourDefault = Light2Colour;
D3DXVECTOR3 DirrectionalColour = D3DXVECTOR3(0, 0, 1.0f) * 0.1f;
D3DXVECTOR3 DirrectionalVec = D3DXVECTOR3(0, 1, 0);

D3DXVECTOR3 SphereColour  = D3DXVECTOR3(1.0f, 0.41f, 0.7f) * 0.3f;
float SpecularPower = 256.0f;

// Light 1 colour to HSL
// used to rotate colours
float HSL[3];
float ColourRotateRate = 100.0f;
float Light1Power = 20.0f;

// Display models where the lights are. One of the lights will follow an orbit
Model* Light1;
Model* Light2;
const float LightOrbitRadius = 20.0f;
const float LightOrbitSpeed  = 0.7f;

//-------------------------------------

// Angular helper functions to convert from degrees to radians and back (D3DX_PI is a double)
inline float ToRadians( float deg ) { return deg * (float)D3DX_PI / 180.0f; }
inline float ToDegrees( float rad ) { return rad * 180.0f / (float)D3DX_PI; }


//--------------------------------------------------------------------------------------
// Scene Setup / Update / Rendering
//--------------------------------------------------------------------------------------

// Create / load the camera, models and textures for the scene
bool InitScene()
{
	//---------------------------
	// Create camera

	MainCamera = new Camera();
	MainCamera->SetPosition( D3DXVECTOR3(40, 30, -90) );
	MainCamera->SetRotation( D3DXVECTOR3(ToRadians(8.0f), ToRadians(-18.0f), 0.0f) ); // ToRadians is a new helper function to convert degrees to radians


	//---------------------------
	// Load/Create models

	Cube   = new Model;
	Teapot = new Model;
	Sphere = new Model;
	Floor  = new Model;
	Light1 = new Model;
	Light2 = new Model;

	// Load the model's geometry from ".X" files
	// The third parameter is set to true on the first 2 lines below. This asks the import code to generate
	// tangents for those models being loaded. Tangents are required for normal mapping and parallax mapping
	// A tangent is rather like a "second normal" for a vertex, but one that is parallel to the model surface
	// (in the direction of the texture U axis). However, whilst artists will provide normals with their
	// models, they won't provide tangents. They must be calculated by looking at the geometry and UVs. The
	// process is done in the import code, but the detail is beyond the scope of this lab exercise
	//
	bool success = true;
	if (!Cube->  Load( "Cube.x",   ParallaxMappingTechnique,        true ))  success = false;
	if (!Teapot->Load( "Teapot.x", ParallaxMappingTechnique,        true ))  success = false;
	if (!Sphere->Load( "Sphere.x", ParallaxMappingTechniqueSphere,  true ))  success = false;
	if (!Floor-> Load( "Hills.x",  ParallaxMappingTechnique,        true ))  success = false;
	if (!Light1->Load( "Light.x",  AdditiveTintTexTechnique              ))  success = false;
	if (!Light2->Load( "Light.x",  AdditiveTintTexTechnique              ))  success = false;
	if (!success)
	{
		MessageBox(NULL, L"Error loading model files. Ensure your files are correctly named and in the same folder as this executable.", L"Error", MB_OK);
		return false;
	}

	// Initial model positions
	Cube->  SetPosition( D3DXVECTOR3( 10, 15, -40) );
	Teapot->SetPosition( D3DXVECTOR3( 40, 10,  10) );
	Sphere->SetPosition( D3DXVECTOR3(  0, 20,  10) );
	Light1->SetPosition( D3DXVECTOR3( 30, 15, -40) );
	Light1->SetScale( 5.0f );
	Light2->SetPosition( D3DXVECTOR3( 20, 40, -20) );
	Light2->SetScale( 12.0f );

	// Setup Light1's colour
	// Light 1 colour to HSL
	// used to rotate colours
	RGBToHSL(Light1Colour.x, Light1Colour.y, Light1Colour.z, HSL[0], HSL[1], HSL[2]);


	//---------------------------
	// Load textures

	//****| INFO |*******************************************************************************************//
	// In the last lab we used normal maps, with the rgb components of the colours reused to store the xyz
	// components of a normal for each texel. These normals adjusted the lighting for each texel to give an
	// *impression* of bumpiness. However, parallax mapping attempts to show the true depth of the bumpy
	// surface. So it also needs to know the "depth" of the surface: the distance of the bumps above or below
	// the flat plane of the surface geometry. This depth is held in the alpha channel of the normal map (in
	// a similar way that the specular map is held in the alpha channel of the diffuse map)
	//*******************************************************************************************************//
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( Device, L"TechDiffuseSpecular.dds",    NULL, NULL, &CubeDiffuseMap,   NULL ) ))  success = false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( Device, L"TechNormalDepth.dds",        NULL, NULL, &CubeNormalMap,    NULL ) ))  success = false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( Device, L"PatternDiffuseSpecular.dds", NULL, NULL, &TeapotDiffuseMap, NULL ) ))  success = false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( Device, L"PatternNormalDepth.dds",     NULL, NULL, &TeapotNormalMap,  NULL ) ))  success = false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( Device, L"BrainDiffuseSpecular.dds",   NULL, NULL, &SphereDiffuseMap, NULL ) ))  success = false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( Device, L"BrainNormalDepth.dds",       NULL, NULL, &SphereNormalMap,  NULL ) ))  success = false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( Device, L"CobbleDiffuseSpecular.dds",  NULL, NULL, &FloorDiffuseMap,  NULL ) ))  success = false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( Device, L"CobbleNormalDepth.dds",      NULL, NULL, &FloorNormalMap,   NULL ) ))  success = false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( Device, L"flare.jpg",                  NULL, NULL, &LightDiffuseMap,  NULL ) ))  success = false;
	if (!success)
	{
		MessageBox(NULL, L"Error loading texture files. Ensure your files are correctly named and in the same folder as this executable.", L"Error", MB_OK);
		return false;
	}

	return true;
}


//--------------------------------------------------------------------------------------
// Release scene related objects (models, textures etc.) to free memory when quitting
//--------------------------------------------------------------------------------------
void ReleaseScene()
{
	delete Light2;     Light2 = NULL;
	delete Light1;     Light1 = NULL;
	delete Floor;      Floor = NULL;
	delete Teapot;     Teapot = NULL;
	delete Cube;       Cube = NULL;
	delete MainCamera; MainCamera = NULL;

    if (LightDiffuseMap)  LightDiffuseMap->Release();
	if (FloorNormalMap)   FloorNormalMap->Release();
	if (FloorDiffuseMap)  FloorDiffuseMap->Release();
    if (TeapotNormalMap)  TeapotNormalMap->Release();
    if (TeapotDiffuseMap) TeapotDiffuseMap->Release();
	if (SphereNormalMap)  SphereNormalMap->Release();
	if (SphereDiffuseMap) SphereDiffuseMap->Release();
    if (CubeNormalMap)    CubeNormalMap->Release();
    if (CubeDiffuseMap)   CubeDiffuseMap->Release();
}

//--------------------------------------------------------------------------------------
// Update scene every frame
//--------------------------------------------------------------------------------------

// Update the scene - move/rotate each model and the camera, then update their matrices
void UpdateScene( float frameTime )
{
	// Control camera position and update its matrices (view matrix, projection matrix) each frame
	// Don't be deceived into thinking that this is a new method to control models - the same code we used previously is in the camera class
	MainCamera->Control(frameTime, Key_W, Key_S, Key_A, Key_D, Key_Q, Key_E, Key_Z, Key_X);
	
	// Control cube position and update its world matrix each frame
	Cube->Control(frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Comma, Key_Period);

	// Update the orbiting light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float Rotate = 0.0f;
	Light1->SetPosition( Cube->Position() + D3DXVECTOR3(cos(Rotate)*LightOrbitRadius, 0, sin(Rotate)*LightOrbitRadius) );
	Rotate -= LightOrbitSpeed * frameTime;

	// lighting
	Pulse += frameTime;
	Light2Colour = Light2ColourDefault * abs(sin(Pulse));
	
	HSL[0] += frameTime * ColourRotateRate;
	HSLToRGB(HSL[0], HSL[1], HSL[2], Light1Colour.x, Light1Colour.y, Light1Colour.z);
	Light1Colour *= Light1Power;

	// Update mover
	Mover += 0.1f * frameTime;
	MoverVar->SetFloat(Mover);

	// Update wiggle
	Wiggle += 6 * frameTime;
	WiggleVar->SetFloat(Wiggle);

	// Toggle parallax
	if (KeyHit( Key_1 ))
	{
		UseParallax = !UseParallax;
	}
}


//--------------------------------------------------------------------------------------
// Render scene
//--------------------------------------------------------------------------------------

// Render everything in the scene
void RenderScene()
{
	// Clear the back buffer - before drawing anything clear the entire window to a fixed colour
	float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
	Device->ClearRenderTargetView( RenderTargetView, ClearColor );
	Device->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );


	//---------------------------
	// Common rendering settings

	// There are some common features all models that we will be rendering, set these once only

	// Pass the camera's matrices to the vertex shader
	ViewMatrixVar->SetMatrix( (float*)&MainCamera->ViewMatrix() );
	ProjMatrixVar->SetMatrix( (float*)&MainCamera->ProjectionMatrix() );

	// Pass light information to the vertex shader - lights are the same for each model
	Light1PosVar->         SetRawValue( Light1->Position(), 0, 12 );  // Send 3 floats (12 bytes) from C++ LightPos variable (x,y,z) to shader counterpart (middle parameter is unused) 
	Light1ColourVar->      SetRawValue( Light1Colour, 0, 12 );
	Light2PosVar->         SetRawValue( Light2->Position(), 0, 12 );
	Light2ColourVar->      SetRawValue( Light2Colour, 0, 12 );
	DirrectionalVecVar->   SetRawValue( DirrectionalVec, 0, 12);
	DirrectionalColourVar->SetRawValue( DirrectionalColour, 0, 12);
	SphereColourVar->      SetRawValue( SphereColour, 0, 12);
	AmbientColourVar->     SetRawValue( AmbientColour, 0, 12 );
	CameraPosVar->         SetRawValue( MainCamera->Position(), 0, 12 );
	SpecularPowerVar->     SetFloat( SpecularPower );

	// Parallax mapping depth
	ParallaxDepthVar->SetFloat( UseParallax ? ParallaxDepth : 0.0f );


	//---------------------------
	// Render each model

	// The model class contains the rendering code from previous labs but does not set any model specific shader variables, so we do that here.
	// [Reason: we often vary shaders and textures, putting the shader code in the model class would reduce flexibility unless we developed a much more complex system]
	
	// Render cube
	WorldMatrixVar->SetMatrix( (float*)Cube->WorldMatrix() ); // Send the cube's world matrix to the shader
	DiffuseMapVar->SetResource( CubeDiffuseMap );             // Send the cube's diffuse/specular map to the shader
    NormalMapVar->SetResource( CubeNormalMap );               // Send the cube's normal/depth map to the shader
	Cube->Render( ParallaxMappingTechnique );                 // Pass rendering technique to the model class

	// Same for the other models in the scene
	WorldMatrixVar->SetMatrix( (float*)Teapot->WorldMatrix() );
    DiffuseMapVar->SetResource( TeapotDiffuseMap );
    NormalMapVar->SetResource( TeapotNormalMap );
	Teapot->Render( ParallaxMappingTechnique );

	WorldMatrixVar->SetMatrix((float*)Sphere->WorldMatrix());
	DiffuseMapVar->SetResource(SphereDiffuseMap);
	NormalMapVar->SetResource(SphereNormalMap);
	TintColourVar->SetRawValue(SphereColour, 0, 12);
	Sphere->Render(ParallaxMappingTechniqueSphere);

	WorldMatrixVar->SetMatrix( (float*)Floor->WorldMatrix() );
	DiffuseMapVar->SetResource( FloorDiffuseMap );
    NormalMapVar->SetResource( FloorNormalMap );
	Floor->Render( ParallaxMappingTechnique );

	WorldMatrixVar->SetMatrix( (float*)Light1->WorldMatrix() );
	DiffuseMapVar->SetResource( LightDiffuseMap );
	TintColourVar->SetRawValue( Light1Colour, 0, 12 ); // Using special shader that tints the light model to match the light colour
	Light1->Render( AdditiveTintTexTechnique );

	WorldMatrixVar->SetMatrix( (float*)Light2->WorldMatrix() );
	DiffuseMapVar->SetResource( LightDiffuseMap );
	TintColourVar->SetRawValue(Light2Colour, 0, 12 );
	Light2->Render( AdditiveTintTexTechnique );


	//---------------------------
	// Display the Scene

	// After we've finished drawing to the off-screen back buffer, we "present" it to the front buffer (the screen)
	SwapChain->Present( 0, 0 );
}