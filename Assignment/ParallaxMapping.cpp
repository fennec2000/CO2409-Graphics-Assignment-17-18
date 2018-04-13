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

enum ELightType { point, directional, spot };
enum EID3D10EffectTechnique { Parallax, VertexLit, AdditiveTintTex, VertexAdditive };

struct Light {
	ELightType type;
	D3DXVECTOR3 colour;
	float power;
	D3DXVECTOR3 vector;
	Model* model;
};

struct SModel {
	string fileName;
	EID3D10EffectTechnique Etechnique;
	bool tangents;
	LPCWSTR DiffuseMapName;
	LPCWSTR NormalMapName;
	D3DXVECTOR3 tintColour;
	bool effectsAlways;
	ID3D10ShaderResourceView* DiffuseMap;
	ID3D10ShaderResourceView* NormalMap;
	ID3D10EffectTechnique* technique;
	Model* model;
};

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
bool UseMover = false; // invert mover on object
bool UseWiggle = false; // invert wiggle on object
const float WIGGLE_POWER_DEFAULT = 0.1f;
const float WIGGLE_POWER_RATE = 0.001f;
float WigglePower = 0.1f;    // power of the wiggle effect

//-------------------------------------

// Models and cameras. Now encapsulated in classes for flexibity and convenience
// The CModel class collects together geometry and world matrix, and provides functions to control the model and render it
// The CCamera class handles the view and projections matrice, and provides functions to control the camera
const int MODEL_COUNT = 6;
SModel ModelArr[MODEL_COUNT] = {
	{ "Cube.x",   Parallax,       true,  L"TechDiffuseSpecular.dds",    L"TechNormalDepth.dds",    D3DXVECTOR3(),                         false, NULL, NULL },
	{ "Cube.x",   VertexLit,      true,  L"StoneDiffuseSpecular.dds",    L"",                      D3DXVECTOR3(),                         false, NULL, NULL },
	{ "Decal.x",  VertexAdditive, false, L"Moogle.png",                 L"",                       D3DXVECTOR3(1,1,1) * 10,               false, NULL, NULL },
	{ "Teapot.x", Parallax,       true,  L"PatternDiffuseSpecular.dds", L"PatternNormalDepth.dds", D3DXVECTOR3(),                         false, NULL, NULL },
	{ "Sphere.x", Parallax,       true,  L"BrainDiffuseSpecular.dds",   L"BrainNormalDepth.dds",   D3DXVECTOR3(1.0f, 0.41f, 0.7f) * 0.3f, true,  NULL, NULL },
	{ "Hills.x",  Parallax,       true,  L"CobbleDiffuseSpecular.dds",  L"CobbleNormalDepth.dds",  D3DXVECTOR3(),                         false, NULL, NULL },
};

Camera* MainCamera;

//**** Portal Data ****//

//*** Some of this data might better belong in device.cpp/.h where there are similar variables for
//    the back buffer. However, keeping all this lab's new material in this one file for clarity

// Dimensions of portal texture - controls quality of rendered scene in portal
int PortalWidth = 1024;
int PortalHeight = 1024;

Model*  Portal;       // The model on which the portal appears
Camera* PortalCamera; // The camera view shown in the portal

					  // The portal texture and the view of it as a render target (see code comments)
ID3D10Texture2D*          PortalTexture = NULL;
ID3D10RenderTargetView*   PortalRenderTarget = NULL;
ID3D10ShaderResourceView* PortalMap = NULL;

// Also need a depth/stencil buffer for the portal
// NOTE: ***Can share the depth buffer between multiple portals of the same size***
ID3D10Texture2D*        PortalDepthStencil = NULL;
ID3D10DepthStencilView* PortalDepthStencilView = NULL;

//*********************//

// Textures - including normal maps

ID3D10ShaderResourceView* LightDiffuseMap  = NULL;
float ParallaxDepth = 0.08f; // Overall depth of bumpiness for parallax mapping
bool UseParallax    = true;  // Toggle for parallax 

//-------------------------------------

// Light data still stored manually, a light class would be helpful - but it's an assignment task!
D3DXVECTOR4 BackgroundColour = D3DXVECTOR4(0.2f, 0.2f, 0.3f, 1.0f);
D3DXVECTOR3 AmbientColour = D3DXVECTOR3( 0.2f, 0.2f, 0.3f );

const int LIGHT_COUNT = 4;
Light LightArr[LIGHT_COUNT] = {
	{ point, D3DXVECTOR3(0.8f, 0.8f, 1.0f), 20 },
	{ point, D3DXVECTOR3(1.0f, 0.8f, 0.2f), 30 },
	{ directional, D3DXVECTOR3(0, 0, 1.0f), 0.1f, D3DXVECTOR3(0, 1, 0) },
	{ spot, D3DXVECTOR3(1, 1, 1), 50, D3DXVECTOR3(0, 0.707107f, -0.707107f) }
};

D3DXVECTOR3 PulseDefault = LightArr[1].colour;

float SpotLightAngle = 0.52f; // aprox 30 degrees - 29.79381
float SpecularPower = 256.0f;

// Light 1 colour to HSL
// used to rotate colours
float HSL[3];
float ColourRotateRate = 1000.0f;

// Display models where the lights are. One of the lights will follow an orbit
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

	//**** Portal camera is the view shown in the portal object's texture ****//
	PortalCamera = new Camera();
	PortalCamera->SetPosition(D3DXVECTOR3(45, 45, 85));
	PortalCamera->SetRotation(D3DXVECTOR3(ToRadians(20.0f), ToRadians(215.0f), 0.));

	//---------------------------
	// Load/Create models

	Portal    = new Model;

	// Load the model's geometry from ".X" files
	// The third parameter is set to true on the first 2 lines below. This asks the import code to generate
	// tangents for those models being loaded. Tangents are required for normal mapping and parallax mapping
	// A tangent is rather like a "second normal" for a vertex, but one that is parallel to the model surface
	// (in the direction of the texture U axis). However, whilst artists will provide normals with their
	// models, they won't provide tangents. They must be calculated by looking at the geometry and UVs. The
	// process is done in the import code, but the detail is beyond the scope of this lab exercise
	//
	bool success = true;
	for (int i = 0; i < MODEL_COUNT; i++)
	{
		if (ModelArr[i].Etechnique == Parallax)
			ModelArr[i].technique = ParallaxMappingTechnique;
		else if (ModelArr[i].Etechnique == VertexLit || ModelArr[i].Etechnique == VertexAdditive)
			ModelArr[i].technique = VertexLitTexTechnique;
		else if (ModelArr[i].Etechnique == AdditiveTintTex)
			ModelArr[i].technique = AdditiveTintTexTechnique;

		ModelArr[i].model = new Model;
		if (!ModelArr[i].model->Load(ModelArr[i].fileName, ModelArr[i].technique, ModelArr[i].tangents))  success = false;

		if (ModelArr[i].Etechnique == VertexAdditive)
			ModelArr[i].technique = AdditiveTintTexTechnique;
	}
	if (!Portal->   Load( "Portal.x", VertexLitTexTechnique                 ))  success = false;
	if (!success)
	{
		MessageBox(NULL, L"Error loading model files. Ensure your files are correctly named and in the same folder as this executable.", L"Error", MB_OK);
		return false;
	}

	// lights
	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		LightArr[i].model = new Model;
		if (!LightArr[i].model->Load("Light.x", AdditiveTintTexTechnique))  success = false;
	}
	if (!success)
	{
		MessageBox(NULL, L"Error loading model files. Ensure your files are correctly named and in the same folder as this executable.", L"Error", MB_OK);
		return false;
	}

	// Initial model positions
	ModelArr[0].model->SetPosition( D3DXVECTOR3( 10, 15, -40) );
	ModelArr[1].model->SetPosition( D3DXVECTOR3( 10, 15, -80) );
	ModelArr[2].model->SetPosition(ModelArr[1].model->Position() + D3DXVECTOR3(0, 0, -0.1f));
	ModelArr[3].model->SetPosition( D3DXVECTOR3( 40, 10,  10) );
	ModelArr[4].model->SetPosition( D3DXVECTOR3(  0, 20,  10) );

	LightArr[0].model->SetPosition( D3DXVECTOR3( 30, 15, -40) );
	LightArr[0].model->SetScale( 5.0f );
	LightArr[1].model->SetPosition( D3DXVECTOR3( 20, 40, -20) );
	LightArr[1].model->SetScale( 12.0f );
	LightArr[3].model->SetPosition( D3DXVECTOR3(60, 20, -60));
	LightArr[3].model->SetScale( 12.0f );

	Portal->SetPosition(D3DXVECTOR3(40, 20, 40));
	Portal->SetRotation(D3DXVECTOR3(0.0f, ToRadians(-130.0f), 0.0f));

	// Setup Light1's colour
	// Light 1 colour to HSL
	// used to rotate colours
	RGBToHSL(LightArr[0].colour.x, LightArr[0].colour.y, LightArr[0].colour.z, HSL[0], HSL[1], HSL[2]);


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
	for (int i = 0; i < MODEL_COUNT; i++)
	{
		if (ModelArr[i].DiffuseMapName != L"")
			if (FAILED(D3DX10CreateShaderResourceViewFromFile(Device, ModelArr[i].DiffuseMapName, NULL, NULL, &ModelArr[i].DiffuseMap, NULL)))  success = false;
		if (ModelArr[i].NormalMapName != L"")
			if (FAILED(D3DX10CreateShaderResourceViewFromFile(Device, ModelArr[i].NormalMapName, NULL, NULL, &ModelArr[i].NormalMap, NULL)))  success = false;
	}
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( Device, L"flare.jpg",                  NULL, NULL, &LightDiffuseMap,  NULL ) ))  success = false;
	if (!success)
	{
		MessageBox(NULL, L"Error loading texture files. Ensure your files are correctly named and in the same folder as this executable.", L"Error", MB_OK);
		return false;
	}

	//**** Portal Texture ****//

	//*** As noted with the portal variables, some/all of this code might better be in device.cpp, but showing all new code in this file

	// Create the portal texture itself, above we used a D3DX... helper function to create a texture in one line. Here, we need to do things manually
	// as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
	D3D10_TEXTURE2D_DESC portalDesc;
	portalDesc.Width = PortalWidth;  // Size of the portal texture determines its quality
	portalDesc.Height = PortalHeight;
	portalDesc.MipLevels = 1; // No mip-maps when rendering to textures (or we would have to render every level)
	portalDesc.ArraySize = 1;
	portalDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA texture (8-bits each)
	portalDesc.SampleDesc.Count = 1;
	portalDesc.SampleDesc.Quality = 0;
	portalDesc.Usage = D3D10_USAGE_DEFAULT;
	portalDesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE; // Indicate we will use texture as render target, and pass it to shaders
	portalDesc.CPUAccessFlags = 0;
	portalDesc.MiscFlags = 0;
	if (FAILED(Device->CreateTexture2D(&portalDesc, NULL, &PortalTexture))) return false;

	// We created the portal texture above, now we get a "view" of it as a render target, i.e. get a special pointer to the texture that
	// we use when rendering to it (see RenderScene function below)
	if (FAILED(Device->CreateRenderTargetView(PortalTexture, NULL, &PortalRenderTarget))) return false;

	// We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
	D3D10_SHADER_RESOURCE_VIEW_DESC srDesc;
	srDesc.Format = portalDesc.Format;
	srDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;
	if (FAILED(Device->CreateShaderResourceView(PortalTexture, &srDesc, &PortalMap))) return false;


	//**** Portal Depth Buffer ****//

	// We also need a depth buffer to go with our portal
	//**** This depth buffer can be shared with any other portals of the same size
	portalDesc.Width = PortalWidth;
	portalDesc.Height = PortalHeight;
	portalDesc.MipLevels = 1;
	portalDesc.ArraySize = 1;
	portalDesc.Format = DXGI_FORMAT_D32_FLOAT;
	portalDesc.SampleDesc.Count = 1;
	portalDesc.SampleDesc.Quality = 0;
	portalDesc.Usage = D3D10_USAGE_DEFAULT;
	portalDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	portalDesc.CPUAccessFlags = 0;
	portalDesc.MiscFlags = 0;
	if (FAILED(Device->CreateTexture2D(&portalDesc, NULL, &PortalDepthStencil))) return false;

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D10_DEPTH_STENCIL_VIEW_DESC portalDescDSV;
	portalDescDSV.Format = portalDesc.Format;
	portalDescDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	portalDescDSV.Texture2D.MipSlice = 0;
	if (FAILED(Device->CreateDepthStencilView(PortalDepthStencil, &portalDescDSV, &PortalDepthStencilView))) return false;

	//*****************************//

	return true;
}


//--------------------------------------------------------------------------------------
// Release scene related objects (models, textures etc.) to free memory when quitting
//--------------------------------------------------------------------------------------
void ReleaseScene()
{
	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		delete LightArr[i].model;  LightArr[i].model = NULL;
	}

	for (int i = 0; i < MODEL_COUNT; i++)
	{
		delete ModelArr[i].model;  ModelArr[i].model = NULL;
	}

	delete Portal;        Portal = NULL;
	delete PortalCamera;  PortalCamera = NULL;
	delete MainCamera;    MainCamera = NULL;

	if (PortalDepthStencilView)  PortalDepthStencilView->Release();
	if (PortalDepthStencil)      PortalDepthStencil->Release();
	if (PortalMap)               PortalMap->Release();
	if (PortalRenderTarget)      PortalRenderTarget->Release();
	if (PortalTexture)           PortalTexture->Release();

    if (LightDiffuseMap)  LightDiffuseMap->Release();
	for (int i = 0; i < MODEL_COUNT; i++)
	{
		if (ModelArr[i].NormalMap)  ModelArr[i].NormalMap->Release();
		if (ModelArr[i].DiffuseMap) ModelArr[i].DiffuseMap->Release();
	}
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
	PortalCamera->Control(frameTime, Key_T, Key_G, Key_F, Key_H, Key_N, Key_B, Key_V, Key_M);
	
	// Control cube position and update its world matrix each frame
	ModelArr[1].model->Control(frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Comma, Key_Period);
	ModelArr[2].model->Control(frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Comma, Key_Period);
	Portal->Control(frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma);

	// Update the orbiting light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float Rotate = 0.0f;
	LightArr[0].model->SetPosition(ModelArr[0].model->Position() + D3DXVECTOR3(cos(Rotate)*LightOrbitRadius, 0, sin(Rotate)*LightOrbitRadius) );
	Rotate -= LightOrbitSpeed * frameTime;

	// lighting
	Pulse += frameTime;
	LightArr[1].colour = PulseDefault * abs(sin(Pulse));
	
	HSL[0] += frameTime * ColourRotateRate;
	HSLToRGB(HSL[0], HSL[1], HSL[2], LightArr[0].colour.x, LightArr[0].colour.y, LightArr[0].colour.z);

	// Update mover
	Mover += 0.1f * frameTime;

	// Update wiggle
	Wiggle += 6 * frameTime;

	// Toggle parallax
	if (KeyHit( Key_1 ))
	{
		UseParallax = !UseParallax;
	}

	// Toggle move
	if (KeyHit(Key_2))
	{
		UseMover = !UseMover;
	}

	// Toggle wiggle
	if (KeyHit(Key_3))
	{
		UseWiggle = !UseWiggle;
	}

	// Wiggle Power
	if (KeyHeld(Key_8))
	{
		WigglePower -= WIGGLE_POWER_RATE;
	}
	if (KeyHit(Key_9))
	{
		WigglePower = WIGGLE_POWER_DEFAULT;
	}
	if (KeyHeld(Key_0))
	{
		WigglePower += WIGGLE_POWER_RATE;
	}

}


//--------------------------------------------------------------------------------------
// Render scene
//--------------------------------------------------------------------------------------

//**** Scene rendering has been split up. Since the models are rendered twice, once into the portal ****//
//**** texture, and once for the viewport, that part of the code has been seperated into a function ****//

// Render all the models from the point of view of the given camera
void RenderModels(Camera* camera)
{
	//---------------------------
	// Render each model

	// The model class contains the rendering code from previous labs but does not set any model specific shader variables, so we do that here.
	// [Reason: we often vary shaders and textures, putting the shader code in the model class would reduce flexibility unless we developed a much more complex system]

	// Pass the camera's matrices to the vertex shader
	ViewMatrixVar->SetMatrix((float*)&camera->ViewMatrix());
	ProjMatrixVar->SetMatrix((float*)&camera->ProjectionMatrix());

	// Render cube
	for (int i = 0; i < MODEL_COUNT; i++)
	{
		WorldMatrixVar->SetMatrix((float*)ModelArr[i].model->WorldMatrix()); // Send the cube's world matrix to the shader
		DiffuseMapVar->SetResource(ModelArr[i].DiffuseMap);             // Send the cube's diffuse/specular map to the shader
		NormalMapVar->SetResource(ModelArr[i].NormalMap);               // Send the cube's normal/depth map to the shader
		TintColourVar->SetRawValue(ModelArr[i].tintColour, 0, 12);

		if (ModelArr[i].effectsAlways || UseMover)
			MoverVar->SetFloat(Mover);
		else
			MoverVar->SetFloat(0);
		if (ModelArr[i].effectsAlways || UseWiggle)
			WiggleVar->SetFloat(Wiggle);
		else
			WiggleVar->SetFloat(0);

		ModelArr[i].model->Render(ModelArr[i].technique);                 // Pass rendering technique to the model class
	}

	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		WorldMatrixVar->SetMatrix((float*)LightArr[i].model->WorldMatrix());
		DiffuseMapVar->SetResource(LightDiffuseMap);
		TintColourVar->SetRawValue(LightArr[i].colour * LightArr[i].power, 0, 12); // Using special shader that tints the light model to match the light colour
		WiggleVar->SetFloat(0);
		MoverVar->SetFloat(0);
		LightArr[i].model->Render(AdditiveTintTexTechnique);
	}

	WorldMatrixVar->SetMatrix((float*)Portal->WorldMatrix());
	DiffuseMapVar->SetResource(PortalMap);
	Portal->Render(VertexLitTexTechnique);
}

// Render everything in the scene
void RenderScene()
{
	//---------------------------
	// Common rendering settings

	// There are some common features all models that we will be rendering, set these once only


	// Pass light information to the vertex shader - lights are the same for each model
	Light1PosVar->SetRawValue(LightArr[0].model->Position(), 0, 12);  // Send 3 floats (12 bytes) from C++ LightPos variable (x,y,z) to shader counterpart (middle parameter is unused) 
	Light1ColourVar->SetRawValue(LightArr[0].colour * LightArr[0].power, 0, 12);
	Light2PosVar->SetRawValue(LightArr[1].model->Position(), 0, 12);
	Light2ColourVar->SetRawValue(LightArr[1].colour * LightArr[1].power, 0, 12);
	DirrectionalVecVar->SetRawValue(LightArr[2].vector, 0, 12);
	DirrectionalColourVar->SetRawValue(LightArr[2].colour * LightArr[2].power, 0, 12);
	SpotLightPosVar->SetRawValue(LightArr[3].model->Position(), 0, 12);
	SpotLightVecVar->SetRawValue(LightArr[3].vector, 0, 12);
	SpotLightColourVar->SetRawValue(LightArr[3].colour * LightArr[3].power, 0, 12);
	SpotLightAngleVar->SetFloat(SpotLightAngle);
	AmbientColourVar->SetRawValue(AmbientColour, 0, 12);
	CameraPosVar->SetRawValue(MainCamera->Position(), 0, 12);
	SpecularPowerVar->SetFloat(SpecularPower);

	// Parallax mapping depth
	ParallaxDepthVar->SetFloat(UseParallax ? ParallaxDepth : 0.0f);

	// Sending Wiggle power
	WigglePowerVar->SetFloat(WigglePower);

	//---------------------------
	// Render portal scene
	//---------------------------

	// Setup the viewport - defines which part of the texture we will render to (usually all of it)
	D3D10_VIEWPORT vp;
	vp.Width = PortalWidth;
	vp.Height = PortalHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	Device->RSSetViewports(1, &vp);

	// Select the portal texture to use for rendering, will share the depth/stencil buffer with the backbuffer though
	Device->OMSetRenderTargets(1, &PortalRenderTarget, PortalDepthStencilView);

	// Clear the portal texture and its depth buffer
	Device->ClearRenderTargetView(PortalRenderTarget, &BackgroundColour[0]);
	Device->ClearDepthStencilView(PortalDepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);

	// Render everything from the portal camera's point of view (into the portal render target [texture] set above)
	RenderModels(PortalCamera);

	//---------------------------
	// Render main scene
	//---------------------------

	// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
	vp.Width = ViewportWidth;
	vp.Height = ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	Device->RSSetViewports(1, &vp);

	// Select the back buffer and depth buffer to use for rendering
	Device->OMSetRenderTargets(1, &BackBufferRenderTarget, DepthStencilView);

	// Clear the back buffer  and its depth buffer
	Device->ClearRenderTargetView(BackBufferRenderTarget, &BackgroundColour[0]);
	Device->ClearDepthStencilView(DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);

	// Render everything from the main camera's point of view (into the portal render target [texture] set above)
	RenderModels(MainCamera);

	//---------------------------
	// Display the Scene

	// After we've finished drawing to the off-screen back buffer, we "present" it to the front buffer (the screen)
	SwapChain->Present( 0, 0 );
}