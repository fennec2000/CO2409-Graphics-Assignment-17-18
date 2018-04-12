//--------------------------------------------------------------------------------------
// Setting up shaders, rendering techniques and shader variables
//--------------------------------------------------------------------------------------

#include <d3d10.h>

// Header guard - prevents this file being included more than once
#ifndef CO2409_SHADER_H_INCLUDED
#define CO2409_SHADER_H_INCLUDED

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
// Using externs to make global variables from the .cpp file available to other files.
// It would be better to use classes and avoid globals to make the code tidier. However
// that might hide the overall picture and might make things more difficult to follow.

// Variable to hold the loaded .fx file
extern ID3D10Effect*               Effect;

// Variable for each technique used from the .fx file (found at the end of the .fx file)
extern ID3D10EffectTechnique*      ParallaxMappingTechnique;
extern ID3D10EffectTechnique*      ParallaxMappingTechniqueSphere;
extern ID3D10EffectTechnique*      VertexLitTexTechnique;
extern ID3D10EffectTechnique*      AdditiveTintTexTechnique;

// Matrices - variables used to send values from C++ to shader (fx file) variables
extern ID3D10EffectMatrixVariable* WorldMatrixVar;
extern ID3D10EffectMatrixVariable* ViewMatrixVar;
extern ID3D10EffectMatrixVariable* ProjMatrixVar;
extern ID3D10EffectMatrixVariable* ViewProjMatrixVar;

// Lights - variables used to send values from C++ to shader (fx file) variables
extern ID3D10EffectVectorVariable* Light1PosVar;
extern ID3D10EffectVectorVariable* Light1ColourVar;
extern ID3D10EffectVectorVariable* Light2PosVar;
extern ID3D10EffectVectorVariable* Light2ColourVar;
extern ID3D10EffectVectorVariable* Light3PosVar;
extern ID3D10EffectVectorVariable* Light3ColourVar;
extern ID3D10EffectVectorVariable* DirrectionalVecVar;
extern ID3D10EffectVectorVariable* DirrectionalColourVar;
extern ID3D10EffectVectorVariable* SpotLightPosVar;
extern ID3D10EffectVectorVariable* SpotLightVecVar;
extern ID3D10EffectVectorVariable* SpotLightColourVar;
extern ID3D10EffectScalarVariable* SpotLightAngleVar;
extern ID3D10EffectVectorVariable* SphereColourVar;
extern ID3D10EffectVectorVariable* AmbientColourVar;
extern ID3D10EffectVectorVariable* CameraPosVar;
extern ID3D10EffectScalarVariable* SpecularPowerVar;

// Textures - variables used to send values from C++ to shader (fx file) variables
extern ID3D10EffectShaderResourceVariable* DiffuseMapVar;
extern ID3D10EffectShaderResourceVariable* NormalMapVar;

// Miscellaneous variables to send values from C++ to shaders 
extern ID3D10EffectScalarVariable* ParallaxDepthVar;
extern ID3D10EffectVectorVariable* TintColourVar;

// Effects
extern ID3D10EffectScalarVariable* MoverVar;
extern ID3D10EffectScalarVariable* WiggleVar;


//--------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------
// Make functions available to other files

// Initialise shaders - load an effect file (.fx file containing shaders)
bool InitShaders();

// Release shader objects to free memory when quitting
void ReleaseShaders();


#endif // End of header guard (see top of file)
