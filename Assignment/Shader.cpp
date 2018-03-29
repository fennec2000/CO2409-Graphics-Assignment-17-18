//--------------------------------------------------------------------------------------
// Setting up shaders, rendering techniques and shader variables
//--------------------------------------------------------------------------------------

#include "Shader.h"
#include "Device.h"
#include <d3d10.h>
#include <d3dx10.h>
#include <atlbase.h>


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
// *************
// * IMPORTANT - if you add a new variable here, also add an extern to the header file
// *************

// Variable to hold the loaded .fx file
ID3D10Effect* Effect = NULL;

// Variable for each technique used from the .fx file (found at the end of the .fx file)
ID3D10EffectTechnique* ParallaxMappingTechnique   = NULL;
ID3D10EffectTechnique* ParallaxMappingTechniqueSphere = NULL;
ID3D10EffectTechnique* AdditiveTintTexTechnique = NULL;

// Matrices - variables used to send values from C++ to shader (fx file) variables
// Note, even though we may have many models, the shader only renders one thing at
// a time. So in the shaders there is only one world matrix, view matrix etc 
ID3D10EffectMatrixVariable* WorldMatrixVar    = NULL;
ID3D10EffectMatrixVariable* ViewMatrixVar     = NULL;
ID3D10EffectMatrixVariable* ProjMatrixVar     = NULL;
ID3D10EffectMatrixVariable* ViewProjMatrixVar = NULL;

// Lights - variables used to send values from C++ to shader (fx file) variables
// Unlike matrices, if whe have many lights we need many shader variables because
// a single model can be affected by many lights
ID3D10EffectVectorVariable* Light1PosVar     = NULL;
ID3D10EffectVectorVariable* Light1ColourVar  = NULL;
ID3D10EffectVectorVariable* Light2PosVar     = NULL;
ID3D10EffectVectorVariable* Light2ColourVar  = NULL;
ID3D10EffectVectorVariable* DirrectionalVecVar = NULL;
ID3D10EffectVectorVariable* DirrectionalColourVar = NULL;
ID3D10EffectVectorVariable* SpotLightPosVar = NULL;
ID3D10EffectVectorVariable* SpotLightVecVar = NULL;
ID3D10EffectVectorVariable* SpotLightColourVar = NULL;
ID3D10EffectVectorVariable* SphereColourVar  = NULL;
ID3D10EffectVectorVariable* AmbientColourVar = NULL;
ID3D10EffectVectorVariable* CameraPosVar     = NULL; // Camera position used for specular light
ID3D10EffectScalarVariable* SpecularPowerVar = NULL;

// Textures - two textures in the pixel shader now - diffuse/specular map and normal/depth map
ID3D10EffectShaderResourceVariable* DiffuseMapVar = NULL;
ID3D10EffectShaderResourceVariable* NormalMapVar  = NULL;

// Miscellaneous variables to send values from C++ to shaders
ID3D10EffectScalarVariable* ParallaxDepthVar = NULL; // To set the depth of the parallax mapping effect
ID3D10EffectVectorVariable* TintColourVar    = NULL; // For tinting the light models

// Effects
ID3D10EffectScalarVariable* MoverVar         = NULL;
ID3D10EffectScalarVariable* WiggleVar        = NULL;


//--------------------------------------------------------------------------------------
// Initialise shaders - load an effect file (.fx file containing shaders)
//--------------------------------------------------------------------------------------
// An effect file contains a set of "Techniques". A technique is a combination of vertex,
// geometry and pixel shaders (and some states) used for rendering in a particular way. 
// We load the effect file at runtime (it's written in HLSL and has the extension ".fx"). 
// The effect code is compiled *at runtime* into low-level GPU language. When rendering a
// particular model we specify which technique from the effect file that it will use.

bool InitShaders()
{
	HRESULT hr = S_OK;

	ID3D10Blob* pErrors; // This strangely typed variable stores any compliation errors
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS; // Shader compiler options

	// Load and compile the effect file
	hr = D3DX10CreateEffectFromFile(L"ParallaxMapping.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, Device, NULL, NULL, &Effect, &pErrors, NULL);
	if (FAILED(hr))
	{
		if (pErrors != 0)
		{
			MessageBox(NULL, CA2CT(reinterpret_cast<char*>(pErrors->GetBufferPointer())), L"Error", MB_OK); // Compiler error: display error message
		}
		else
		{
			// No error message, which probably means the fx file was not found
			MessageBox(NULL, L"Error loading FX file. Ensure your FX file is correctly named and in the same folder as this executable.", L"Error", MB_OK);
		}
		return false;
	}

	// Now we can select techniques from the compiled effect file
	ParallaxMappingTechnique = Effect->GetTechniqueByName("ParallaxMapping");
	ParallaxMappingTechniqueSphere = Effect->GetTechniqueByName("ParallaxMappingSphere");
	AdditiveTintTexTechnique = Effect->GetTechniqueByName("AdditiveTexTint");

	// Create variables to allow us to access global variables in the shaders from C++
	// First model and camera matrices
	WorldMatrixVar    = Effect->GetVariableByName("WorldMatrix"   )->AsMatrix();
	ViewMatrixVar     = Effect->GetVariableByName("ViewMatrix"    )->AsMatrix();
	ProjMatrixVar     = Effect->GetVariableByName("ProjMatrix"    )->AsMatrix();
	ViewProjMatrixVar = Effect->GetVariableByName("ViewProjMatrix")->AsMatrix();

	// Also access shader variables needed for lighting
	Light1PosVar          = Effect->GetVariableByName("Light1Pos"         )->AsVector();
	Light1ColourVar       = Effect->GetVariableByName("Light1Colour"      )->AsVector();
	Light2PosVar          = Effect->GetVariableByName("Light2Pos"         )->AsVector();
	Light2ColourVar       = Effect->GetVariableByName("Light2Colour"      )->AsVector();
	DirrectionalVecVar    = Effect->GetVariableByName("DirrectionalVec"   )->AsVector();
	DirrectionalColourVar = Effect->GetVariableByName("DirrectionalColour")->AsVector();
	SpotLightPosVar       = Effect->GetVariableByName("SpotLightPos"      )->AsVector();
	SpotLightVecVar       = Effect->GetVariableByName("SpotLightVec"      )->AsVector();
	SpotLightColourVar    = Effect->GetVariableByName("SpotLightColour"   )->AsVector();
	SphereColourVar       = Effect->GetVariableByName("SphereColour"      )->AsVector();
	AmbientColourVar      = Effect->GetVariableByName("AmbientColour"     )->AsVector();
	CameraPosVar          = Effect->GetVariableByName("CameraPos"         )->AsVector();
	SpecularPowerVar      = Effect->GetVariableByName("SpecularPower"     )->AsScalar();

	// Miscellaneous shader variables
	ParallaxDepthVar = Effect->GetVariableByName( "ParallaxDepth" )->AsScalar();
	TintColourVar    = Effect->GetVariableByName( "TintColour"    )->AsVector();

	// Also access the texture used in the shader in the same way (note that this variable is a "Shader Resource")
	// Both diffuse and normal maps have variables
	DiffuseMapVar    = Effect->GetVariableByName("DiffuseMap"     )->AsShaderResource();
	NormalMapVar     = Effect->GetVariableByName("NormalMap"      )->AsShaderResource();

	// Effects
	MoverVar         = Effect->GetVariableByName("Mover"          )->AsScalar();
	WiggleVar        = Effect->GetVariableByName("Wiggle"         )->AsScalar();

	return true;
}


//--------------------------------------------------------------------------------------
// Release shader objects to free memory when quitting
//--------------------------------------------------------------------------------------
void ReleaseShaders()
{
	if (Effect)  Effect->Release();
}

