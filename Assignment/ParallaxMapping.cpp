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
Model*  Cube2;
Model*  Teapot;
Model*  Floor;
Model*  Sphere;
Camera* MainCamera;

// Textures - including normal maps
ID3D10ShaderResourceView* CubeDiffuseMap   = NULL;
ID3D10ShaderResourceView* CubeNormalMap    = NULL;
ID3D10ShaderResourceView* Cube2DiffuseMap  = NULL;
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

///**** Shadow Maps ****//
// Very similar data to the render-to-texture (portal) lab

// Width and height of shadow map - controls resolution/quality of shadows
int ShadowMapSize = 2048;

// The shadow map textures and the view of it as a depth buffer and shader resource (see code comments)
ID3D10Texture2D*          ShadowMap1Texture = NULL;
ID3D10DepthStencilView*   ShadowMap1DepthView = NULL;
ID3D10ShaderResourceView* ShadowMap1 = NULL;

//*********************//

// Fixed light data
D3DXVECTOR3 AmbientColour = D3DXVECTOR3(0.2f, 0.2f, 0.3f);

const int NUM_LIGHTS = 3;
D3DXVECTOR3 LightColours[NUM_LIGHTS] = { D3DXVECTOR3(0.8f, 0.8f, 1.0f),
										 D3DXVECTOR3(1.0f, 0.8f, 0.2f) * 30,
										 D3DXVECTOR3(0.8f, 0.8f, 1.0f) * 20 }; // Colour * Intensity
Model*      Lights[NUM_LIGHTS];

// Light data still stored manually, a light class would be helpful - but it's an assignment task!
D3DXVECTOR3 Light2ColourDefault = LightColours[1];
D3DXVECTOR3 DirrectionalColour = D3DXVECTOR3(0, 0, 1.0f) * 0.1f;
D3DXVECTOR3 DirrectionalVec = D3DXVECTOR3(0, 1, 0);
D3DXVECTOR3 SpotLightColour = D3DXVECTOR3(1, 1, 1) * 50;
D3DXVECTOR3 SpotLightVec = D3DXVECTOR3(0, 0.707107f, -0.707107f); // normalised


float       SpotlightConeAngle = 90.0f; // Spot light cone angle (degrees), like the FOV (field-of-view) of the spot light
float SpotLightAngle = 0.52f; // aprox 30 degrees - 29.79381

D3DXVECTOR3 SphereColour  = D3DXVECTOR3(1.0f, 0.41f, 0.7f) * 0.3f;
float SpecularPower = 256.0f;

// Light 1 colour to HSL
// used to rotate colours
float HSL[3];
float ColourRotateRate = 1000.0f;
float Light1Power = 20.0f;


// Display models where the lights are. One of the lights will follow an orbit
Model* Light1;
Model* Light2;
Model* Light3;
Model* SpotLight;
const float LightOrbitRadius = 20.0f;
const float LightOrbitSpeed  = 0.7f;

//-------------------------------------

// Angular helper functions to convert from degrees to radians and back (D3DX_PI is a double)
inline float ToRadians( float deg ) { return deg * (float)D3DX_PI / 180.0f; }
inline float ToDegrees( float rad ) { return rad * 180.0f / (float)D3DX_PI; }

//--------------------------------------------------------------------------------------
// Light Helper Functions
//--------------------------------------------------------------------------------------

// Get "camera-like" view matrix for a spot light
D3DXMATRIXA16 CalculateLightViewMatrix(int lightNum)
{
	D3DXMATRIXA16 viewMatrix;

	// Get the world matrix of the light model and invert it to get the view matrix (that is more-or-less the definition of a view matrix)
	// We don't always have a physical model for a light, in which case we would need to store this data along with the light colour etc.
	D3DXMATRIXA16 worldMatrix = Lights[lightNum]->WorldMatrix();
	D3DXMatrixInverse(&viewMatrix, NULL, &worldMatrix);

	return viewMatrix;
}

// Get "camera-like" projection matrix for a spot light
D3DXMATRIXA16 CalculateLightProjMatrix(int lightNum)
{
	D3DXMATRIXA16 projMatrix;

	// Create a projection matrix for the light. Use the spotlight cone angle as an FOV, just set default values for everything else.
	D3DXMatrixPerspectiveFovLH(&projMatrix, ToRadians(SpotlightConeAngle), 1, 0.1f, 1000.0f);

	return projMatrix;
}


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

	Cube      = new Model;
	Cube2     = new Model;
	Teapot    = new Model;
	Sphere    = new Model;
	Floor     = new Model;
	Light1    = new Model;
	Light2    = new Model;
	Light3    = new Model;
	SpotLight = new Model;

	// Load the model's geometry from ".X" files
	// The third parameter is set to true on the first 2 lines below. This asks the import code to generate
	// tangents for those models being loaded. Tangents are required for normal mapping and parallax mapping
	// A tangent is rather like a "second normal" for a vertex, but one that is parallel to the model surface
	// (in the direction of the texture U axis). However, whilst artists will provide normals with their
	// models, they won't provide tangents. They must be calculated by looking at the geometry and UVs. The
	// process is done in the import code, but the detail is beyond the scope of this lab exercise
	//
	bool success = true;
	if (!Cube->     Load( "Cube.x",   ParallaxMappingTechnique,        true ))  success = false;
	if (!Cube2->    Load( "Cube.x",   VertexLitTexTechnique,           true ))  success = false;
	if (!Teapot->   Load( "Teapot.x", ParallaxMappingTechnique,        true ))  success = false;
	if (!Sphere->   Load( "Sphere.x", ParallaxMappingTechniqueSphere,  true ))  success = false;
	if (!Floor->    Load( "Hills.x",  ParallaxMappingTechnique,        true ))  success = false;
	if (!Light1->   Load( "Light.x",  AdditiveTintTexTechnique              ))  success = false;
	if (!Light2->   Load( "Light.x",  AdditiveTintTexTechnique              ))  success = false;
	if (!Light3->   Load( "Light.x",  AdditiveTintTexTechnique              ))  success = false;
	if (!SpotLight->Load( "Light.x",  AdditiveTintTexTechnique              ))  success = false;
	if (!success)
	{
		MessageBox(NULL, L"Error loading model files. Ensure your files are correctly named and in the same folder as this executable.", L"Error", MB_OK);
		return false;
	}

	// Initial model positions
	Cube->  SetPosition( D3DXVECTOR3( 10, 15, -40) );
	Cube2-> SetPosition( D3DXVECTOR3( 10, 15, -80));
	Teapot->SetPosition( D3DXVECTOR3( 40, 10,  10) );
	Sphere->SetPosition( D3DXVECTOR3(  0, 20,  10) );
	Light1->SetPosition( D3DXVECTOR3( 30, 15, -40) );
	Light1->SetScale( 5.0f );
	Light2->SetPosition( D3DXVECTOR3( 20, 40, -20) );
	Light2->SetScale( 12.0f );
	Light3->SetPosition(D3DXVECTOR3(30, 15, -80));
	Light3->SetScale(5.0f);
	SpotLight->SetPosition( D3DXVECTOR3(60, 20, -60));
	SpotLight->SetScale( 12.0f );

	// Setup Light1's colour
	// Light 1 colour to HSL
	// used to rotate colours
	RGBToHSL(LightColours[0].x, LightColours[0].y, LightColours[0].z, HSL[0], HSL[1], HSL[2]);


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
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( Device, L"StoneDiffuseSpecular.dds",   NULL, NULL, &Cube2DiffuseMap,  NULL ) ))  success = false;
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

	/////////////////////////
	//**** Shadow Maps ****//

	// As the code to handle textures becomes complex, we probably need a texture class, and .cpp/.h file
	// However, as usual, keeping the lab's new material here in the main cpp file

	// Create the shadow map textures, above we used a D3DX... helper function to create basic textures in one line. Here, we need to
	// do things manually as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
	D3D10_TEXTURE2D_DESC texDesc;
	texDesc.Width = ShadowMapSize; // Size of the shadow map determines quality / resolution of shadows
	texDesc.Height = ShadowMapSize;
	texDesc.MipLevels = 1; // 1 level, means just the main texture, no additional mip-maps. Usually don't use mip-maps when rendering to textures (or we would have to render every level)
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS; // The shadow map contains a single 32-bit value [tech gotcha: have to say typeless because depth buffer and texture see things slightly differently]
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D10_USAGE_DEFAULT;
	texDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE; // Indicate we will use texture as render target, and will also pass it to shaders
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	if (FAILED(Device->CreateTexture2D(&texDesc, NULL, &ShadowMap1Texture))) return false;

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = DXGI_FORMAT_D32_FLOAT; // See "tech gotcha" above
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	if (FAILED(Device->CreateDepthStencilView(ShadowMap1Texture, &descDSV, &ShadowMap1DepthView))) return false;

	// We also need to send this texture (a GPU memory resource) to the shaders. To do that we must create a shader-resource "view"	
	D3D10_SHADER_RESOURCE_VIEW_DESC srDesc;
	srDesc.Format = DXGI_FORMAT_R32_FLOAT; // See "tech gotcha" above
	srDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;
	if (FAILED(Device->CreateShaderResourceView(ShadowMap1Texture, &srDesc, &ShadowMap1))) return false;

	//*****************************//

	return true;
}


//--------------------------------------------------------------------------------------
// Release scene related objects (models, textures etc.) to free memory when quitting
//--------------------------------------------------------------------------------------
void ReleaseScene()
{
	delete SpotLight;  SpotLight = NULL;
	delete Light3;     Light3 = NULL;
	delete Light2;     Light2 = NULL;
	delete Light1;     Light1 = NULL;
	delete Floor;      Floor = NULL;
	delete Teapot;     Teapot = NULL;
	delete Cube2;      Cube2 = NULL;
	delete Cube;       Cube = NULL;
	delete MainCamera; MainCamera = NULL;

	if (ShadowMap1)           ShadowMap1->Release();
	if (ShadowMap1DepthView)  ShadowMap1DepthView->Release();
	if (ShadowMap1Texture)    ShadowMap1Texture->Release();

    if (LightDiffuseMap)  LightDiffuseMap->Release();
	if (FloorNormalMap)   FloorNormalMap->Release();
	if (FloorDiffuseMap)  FloorDiffuseMap->Release();
    if (TeapotNormalMap)  TeapotNormalMap->Release();
    if (TeapotDiffuseMap) TeapotDiffuseMap->Release();
	if (SphereNormalMap)  SphereNormalMap->Release();
	if (SphereDiffuseMap) SphereDiffuseMap->Release();
	if (Cube2DiffuseMap)  Cube2DiffuseMap->Release();
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
	MainCamera->Control(frameTime, Key_W, Key_S, Key_A, Key_D, Key_E, Key_Q, Key_Z, Key_X);
	
	// Control cube position and update its world matrix each frame
	Cube->Control(frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Comma, Key_Period);

	// Update the orbiting light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float Rotate = 0.0f;
	D3DXVECTOR3 circle = D3DXVECTOR3(cos(Rotate)*LightOrbitRadius, 0, sin(Rotate)*LightOrbitRadius);
	Light1->SetPosition( Cube->Position() + circle);
	Light3->SetPosition( Cube2->Position() + circle);
	Rotate -= LightOrbitSpeed * frameTime;

	// lighting
	Pulse += frameTime;
	LightColours[1] = Light2ColourDefault * abs(sin(Pulse));
	
	HSL[0] += frameTime * ColourRotateRate;
	HSLToRGB(HSL[0], HSL[1], HSL[2], LightColours[0].x, LightColours[0].y, LightColours[0].z);
	LightColours[0] *= Light1Power;

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
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render all the models (normally) from the point of view of the given camera
void RenderMain(Camera* camera)
{
	// Pass the camera's matrices to the vertex shader and position to the vertex shader
	ViewMatrixVar->SetMatrix(camera->ViewMatrix());
	ProjMatrixVar->SetMatrix(camera->ProjectionMatrix());
	CameraPosVar->SetRawValue(camera->Position(), 0, 12);


	// Send the shadow maps rendered in the RenderShadowMap function to the shader to use for shadow testing
	ShadowMap1Var->SetResource(ShadowMap1);


	//---------------------------
	// Render each model

	// The model class contains the rendering code from previous labs but does not set any model specific shader variables, so we do that here.
	// [Reason: we often vary shaders and textures, putting the shader code in the model class would reduce flexibility unless we developed a much more complex system]

	// Render cube
	WorldMatrixVar->SetMatrix((float*)Cube->WorldMatrix()); // Send the cube's world matrix to the shader
	DiffuseMapVar->SetResource(CubeDiffuseMap);             // Send the cube's diffuse/specular map to the shader
	NormalMapVar->SetResource(CubeNormalMap);               // Send the cube's normal/depth map to the shader
	Cube->Render(ParallaxMappingTechnique);                 // Pass rendering technique to the model class

															// Same for the other models in the scene
	WorldMatrixVar->SetMatrix((float*)Cube2->WorldMatrix());
	DiffuseMapVar->SetResource(Cube2DiffuseMap);
	Cube2->Render(VertexLitTexTechnique);

	WorldMatrixVar->SetMatrix((float*)Teapot->WorldMatrix());
	DiffuseMapVar->SetResource(TeapotDiffuseMap);
	NormalMapVar->SetResource(TeapotNormalMap);
	Teapot->Render(ParallaxMappingTechnique);

	WorldMatrixVar->SetMatrix((float*)Sphere->WorldMatrix());
	DiffuseMapVar->SetResource(SphereDiffuseMap);
	NormalMapVar->SetResource(SphereNormalMap);
	TintColourVar->SetRawValue(SphereColour, 0, 12);
	Sphere->Render(ParallaxMappingTechniqueSphere);

	WorldMatrixVar->SetMatrix((float*)Floor->WorldMatrix());
	DiffuseMapVar->SetResource(FloorDiffuseMap);
	NormalMapVar->SetResource(FloorNormalMap);
	Floor->Render(ParallaxMappingTechnique);

	WorldMatrixVar->SetMatrix((float*)Light1->WorldMatrix());
	DiffuseMapVar->SetResource(LightDiffuseMap);
	TintColourVar->SetRawValue(LightColours[0], 0, 12); // Using special shader that tints the light model to match the light colour
	Light1->Render(AdditiveTintTexTechnique);

	WorldMatrixVar->SetMatrix((float*)Light2->WorldMatrix());
	DiffuseMapVar->SetResource(LightDiffuseMap);
	TintColourVar->SetRawValue(LightColours[1], 0, 12);
	Light2->Render(AdditiveTintTexTechnique);

	WorldMatrixVar->SetMatrix((float*)Light3->WorldMatrix());
	DiffuseMapVar->SetResource(LightDiffuseMap);
	TintColourVar->SetRawValue(LightColours[2], 0, 12);
	Light3->Render(AdditiveTintTexTechnique);

	WorldMatrixVar->SetMatrix((float*)SpotLight->WorldMatrix());
	DiffuseMapVar->SetResource(LightDiffuseMap);
	TintColourVar->SetRawValue(SpotLightColour, 0, 12);
	SpotLight->Render(AdditiveTintTexTechnique);
}

// Render a shadow map for the given light number: render only the models that can obscure other objects, using a special depth-rendering
// shader technique. Assumes the shadow map texture is already set as a render target.
void RenderShadowMap(int lightNum)
{
	//---------------------------------
	// Set "camera" matrices in shader

	// Pass the light's "camera" matrices to the vertex shader - use helper functions above to turn spotlight settings into "camera" matrices
	ViewMatrixVar->SetMatrix(CalculateLightViewMatrix(lightNum));
	ProjMatrixVar->SetMatrix(CalculateLightProjMatrix(lightNum));


	//-----------------------------------
	// Render each model into shadow map

	// Render troll - no need to set its texture as shadow maps just render to the depth buffer
	WorldMatrixVar->SetMatrix(Cube->WorldMatrix());
	Cube->Render(DepthOnlyTechnique);  // Use special rendering technique to render depths only

	// Same for the other models in the scene
	WorldMatrixVar->SetMatrix((float*)Cube2->WorldMatrix());
	Cube2->Render(DepthOnlyTechnique);

	WorldMatrixVar->SetMatrix((float*)Teapot->WorldMatrix());
	Teapot->Render(DepthOnlyTechnique);

	WorldMatrixVar->SetMatrix((float*)Sphere->WorldMatrix());
	Sphere->Render(DepthOnlyTechnique);

	WorldMatrixVar->SetMatrix((float*)Floor->WorldMatrix());
	Floor->Render(DepthOnlyTechnique);
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
	/*Light1FacingVar->      SetRawValue(Lights[0]->Facing(), 0, 12);
	Light1ViewMatrixVar->  SetMatrix(CalculateLightViewMatrix(0));
	Light1ProjMatrixVar->  SetMatrix(CalculateLightProjMatrix(0));*/
	Light1ColourVar->      SetRawValue(LightColours[0], 0, 12 );

	Light2PosVar->         SetRawValue( Light2->Position(), 0, 12 );
	Light2ColourVar->      SetRawValue(LightColours[1], 0, 12 );

	Light3PosVar->         SetRawValue( Light3->Position(), 0, 12);
	Light3ColourVar->      SetRawValue(LightColours[2], 0, 12);
	DirrectionalVecVar->   SetRawValue( DirrectionalVec, 0, 12);
	DirrectionalColourVar->SetRawValue( DirrectionalColour, 0, 12);
	SpotLightPosVar->      SetRawValue( SpotLight->Position(), 0, 12);
	SpotLightVecVar->      SetRawValue( SpotLightVec, 0, 12);
	SpotLightColourVar->   SetRawValue( SpotLightColour, 0, 12);
	SpotLightAngleVar->    SetFloat( SpotLightAngle );
	SphereColourVar->      SetRawValue( SphereColour, 0, 12);
	AmbientColourVar->     SetRawValue( AmbientColour, 0, 12 );
	CameraPosVar->         SetRawValue( MainCamera->Position(), 0, 12 );
	SpecularPowerVar->     SetFloat( SpecularPower );

	// Parallax mapping depth
	ParallaxDepthVar->SetFloat( UseParallax ? ParallaxDepth : 0.0f );

	//---------------------------
	// Render shadow maps

	// Setup the viewport - defines which part of the shadow map we will render to (usually all of it)
	D3D10_VIEWPORT vp;
	vp.Width = ShadowMapSize;
	vp.Height = ShadowMapSize;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	Device->RSSetViewports(1, &vp);

	// Rendering a single shadow map for a light
	// 1. Select the shadow map texture as the current depth buffer. We will not be rendering any pixel colours
	// 2. Clear the shadow map texture (as a depth buffer)
	// 3. Render everything from point of view of light 0
	Device->OMSetRenderTargets(0, 0, ShadowMap1DepthView);
	Device->ClearDepthStencilView(ShadowMap1DepthView, D3D10_CLEAR_DEPTH, 1.0f, 0);
	RenderShadowMap(0);


	//---------------------------
	// Render main scene

	// Setup the viewport within - defines which part of the back-buffer we will render to (usually all of it)
	vp.Width = ViewportWidth;
	vp.Height = ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	Device->RSSetViewports(1, &vp);

	// Select the back buffer and depth buffer to use for rendering
	Device->OMSetRenderTargets(1, &BackBufferRenderTarget, DepthStencilView);

	// Clear the back buffer and its depth buffer
	Device->ClearRenderTargetView(BackBufferRenderTarget, &BackgroundColour[0]);
	Device->ClearDepthStencilView(DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);

	// Render everything from the main camera's point of view
	RenderMain(MainCamera);

	//---------------------------
	// Display the Scene

	// After we've finished drawing to the off-screen back buffer, we "present" it to the front buffer (the screen)
	SwapChain->Present( 0, 0 );
}