//--------------------------------------------------------------------------------------
// Parallax mapping technique
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

// This structure describes the vertex data to be sent into the vertex shader for models using standard rendering
struct VS_BASIC_INPUT
{
    float3 Pos    : POSITION;
    float3 Normal : NORMAL;
	float2 UV     : TEXCOORD0;
};

// Basic vertex output structure for models with no lighting
struct VS_BASIC_OUTPUT
{
    float4 ProjPos : SV_POSITION;  // 2D "projected" position for vertex (required output for vertex shader)
    float2 UV      : TEXCOORD0;
};


// The vertex shader input structure indicates that the model geometry will contain tangents when normal mapping
// Note that the import code provides vertex data in a fixed order, which can be seen in Model.cpp, e.g. here tangents come after normals and before UVs
struct VS_NORMALMAP_INPUT
{
    float3 Pos     : POSITION;
    float3 Normal  : NORMAL;
    float3 Tangent : TANGENT;
	float2 UV      : TEXCOORD0;
};


// Like per-pixel lighting, normal mapping expects the vertex shader to pass over the position and normal.
// However, it also expects the tangent (discussed below). Furthermore the normal and tangent are left in
// model space, i.e. they are not transformed by the world matrix in the vertex shader - just sent as is.
// This is because the pixel shader will do the matrix transformations for normals in this case
//
struct VS_NORMALMAP_OUTPUT
{
	float4 ProjPos      : SV_POSITION;
	float3 WorldPos     : POSITION;
	float3 ModelNormal  : NORMAL;
	float3 ModelTangent : TANGENT;
	float2 UV           : TEXCOORD0;
};


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
// All these variables are created & manipulated in the C++ code and passed into the shader here

// The matrices (4x4 matrix of floats) for transforming from 3D model to 2D projection (used in vertex shader)
float4x4 WorldMatrix;
float4x4 ViewMatrix;
float4x4 ProjMatrix;

// Information used for lighting (in the vertex or pixel shader)
float3 Light1Pos;
float3 Light2Pos;
float3 SpotLightPos;
float3 DirrectionalVec;
float3 SpotLightVec;
float3 Light1Colour;
float3 Light2Colour;
float3 SpotLightColour;
float3 DirrectionalColour;
float3 AmbientColour;
float  SpecularPower;
float3 CameraPos;
float  Mover;
float  Wiggle;

// Variable used to tint each light model to show the colour that it emits
float3 TintColour;

// Diffuse map (sometimes also containing specular map in alpha channel). Loaded from a bitmap in the C++ then sent over to the shader variable in the usual way
Texture2D DiffuseMap;

//****| INFO | Normal map, now contains depth per pixel in the alpha channel ****//
Texture2D NormalMap;

//****| INFO | Also store a factor to strengthen/weaken the parallax effect. Cannot exaggerate it too much or will get distortion ****//
float ParallaxDepth;


// Sampler to use with the diffuse/normal maps. Specifies texture filtering and addressing mode to use when accessing texture pixels
SamplerState TrilinearWrap
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};


//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------

//****| INFO |*******************************************************************************************//
// The vertex shader for parallax mapping is identical to normal mapping. The parallax adjustment occurs
// in the pixel shader
//*******************************************************************************************************//
//
VS_NORMALMAP_OUTPUT NormalMapTransform( VS_NORMALMAP_INPUT vIn )
{
	VS_NORMALMAP_OUTPUT vOut;

	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul( modelPos, WorldMatrix );
	vOut.WorldPos = worldPos.xyz;

	// Use camera matrices to further transform the vertex from world space into view space (camera's point of view) and finally into 2D "projection" space for rendering
	float4 viewPos = mul( worldPos, ViewMatrix );
	vOut.ProjPos   = mul( viewPos,  ProjMatrix );

	// Just send the model's normal and tangent untransformed (in model space). The pixel shader will do the matrix work on normals
	vOut.ModelNormal  = vIn.Normal;
	vOut.ModelTangent = vIn.Tangent;

	// Pass texture coordinates (UVs) on to the pixel shader, the vertex shader doesn't need them
	vOut.UV = vIn.UV;

	return vOut;
}

VS_NORMALMAP_OUTPUT NormalMapTransformSphere(VS_NORMALMAP_INPUT vIn)
{
	VS_NORMALMAP_OUTPUT vOut;

	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul(modelPos, WorldMatrix);
	float4 normalPos = float4(vIn.Normal, 1.0f);
	float4 worldNormal = mul(normalPos, WorldMatrix);
	worldNormal = normalize(worldNormal);

	worldPos.x += sin(modelPos.y + Wiggle) * 0.1f;
	worldPos.y += sin(modelPos.x + Wiggle) * 0.1f;
	worldPos += worldNormal * (sin(Wiggle) + 1.0f) * 0.1f;

	vOut.WorldPos = worldPos.xyz;

	// Use camera matrices to further transform the vertex from world space into view space (camera's point of view) and finally into 2D "projection" space for rendering
	float4 viewPos = mul(worldPos, ViewMatrix);
	vOut.ProjPos = mul(viewPos, ProjMatrix);

	// Just send the model's normal and tangent untransformed (in model space). The pixel shader will do the matrix work on normals
	vOut.ModelNormal = vIn.Normal;
	vOut.ModelTangent = vIn.Tangent;

	// Pass texture coordinates (UVs) on to the pixel shader, the vertex shader doesn't need them
	vOut.UV = vIn.UV;

	return vOut;
}


// Basic vertex shader to transform 3D model vertices to 2D and pass UVs to the pixel shader
//
VS_BASIC_OUTPUT BasicTransform( VS_BASIC_INPUT vIn )
{
	VS_BASIC_OUTPUT vOut;
	
	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul( modelPos, WorldMatrix );
	float4 viewPos  = mul( worldPos, ViewMatrix );
	vOut.ProjPos    = mul( viewPos,  ProjMatrix );
	
	// Pass texture coordinates (UVs) on to the pixel shader
	vOut.UV = vIn.UV;

	return vOut;
}


//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------

// Main pixel shader function. Same as normal mapping but offsets given texture coordinates using the camera direction to correct
// for the varying height of the texture (see lecture)
float4 NormalMapLighting( VS_NORMALMAP_OUTPUT vOut ) : SV_Target
{
	//************************
	// Normal Map Extraction
	//************************

	// Renormalise pixel normal/tangent that were *interpolated* from the vertex normals/tangents (and may have been scaled too)
	float3 modelNormal = normalize( vOut.ModelNormal );
	float3 modelTangent = normalize( vOut.ModelTangent );

	// Calculate bi-tangent to complete the three axes of tangent space. Then create the *inverse* tangent matrix to convert *from* tangent space into model space
	float3 modelBiTangent = cross( modelNormal, modelTangent );
	float3x3 invTangentMatrix = float3x3(modelTangent, modelBiTangent, modelNormal);

	//****| INFO |**********************************************************************************//
	// The following few lines are the parallax mapping. Converts the camera direction into model
	// space and adjusts the UVs based on that and the bump depth of the texel we are looking at
	// Although short, this code involves some intricate matrix work / space transformations
	//**********************************************************************************************//

	// Get normalised vector to camera for parallax mapping and specular equation (this vector was calculated later in previous shaders)
	float3 CameraDir = normalize(CameraPos - vOut.WorldPos.xyz);

	// Transform camera vector from world into model space. Need *inverse* world matrix for this.
	// Only need 3x3 matrix to transform vectors, to invert a 3x3 matrix we transpose it (flip it about its diagonal)
	float3x3 invWorldMatrix = transpose( WorldMatrix );
	float3 cameraModelDir = normalize( mul( CameraDir, invWorldMatrix ) ); // Normalise in case world matrix is scaled

	// Then transform model-space camera vector into tangent space (texture coordinate space) to give the direction to offset texture
	// coordinate, only interested in x and y components. Calculated inverse tangent matrix above, so invert it back for this step
	float3x3 tangentMatrix = transpose( invTangentMatrix ); 
	float2 textureOffsetDir = mul( cameraModelDir, tangentMatrix );
	
	// Get the depth info from the normal map's alpha channel at the given texture coordinate
	// Rescale from 0->1 range to -x->+x range, x determined by ParallaxDepth setting
	float texDepth = ParallaxDepth * (NormalMap.Sample( TrilinearWrap, vOut.UV ).a - 0.5f);
	
	// Use the depth of the texture to offset the given texture coordinate - this corrected texture coordinate will be used from here on
	float2 offsetTexCoord = vOut.UV + texDepth * textureOffsetDir * 0.75f;

	//*******************************************
	
	//****| INFO |**********************************************************************************//
	// The above chunk of code is used only to calculate "offsetTexCoord", which is the offset in 
	// which part of the texture we see at this pixel due to it being bumpy. The remaining code is 
	// exactly the same as normal mapping, but uses offsetTexCoord instead of the usual vOut.UV
	//**********************************************************************************************//


	// Get the texture normal from the normal map. The r,g,b pixel values actually store x,y,z components of a normal. However, r,g,b
	// values are stored in the range 0->1, whereas the x, y & z components should be in the range -1->1. So some scaling is needed
	float3 textureNormal = 2.0f * NormalMap.Sample( TrilinearWrap, offsetTexCoord ) - 1.0f; // Scale from 0->1 to -1->1

	// Now convert the texture normal into model space using the inverse tangent matrix, and then convert into world space using the world
	// matrix. Normalise, because of the effects of texture filtering and in case the world matrix contains scaling
	float3 worldNormal = normalize( mul( mul( textureNormal, invTangentMatrix ), WorldMatrix ) );

	// Now use this normal for lighting calculations in world space as usual - the remaining code same as per-pixel lighting


	///////////////////////
	// Calculate lighting

	// Would normally calculate "CameraDir" here, but it has already been calculated for use in the parallax mapping
	
	//// LIGHT 1
	float3 Light1Dir = normalize(Light1Pos - vOut.WorldPos.xyz);   // Direction for each light is different
	float3 Light1Dist = length(Light1Pos - vOut.WorldPos.xyz); 
	float3 DiffuseLight1 = Light1Colour * max( dot(worldNormal.xyz, Light1Dir), 0 ) / Light1Dist;
	float3 halfway = normalize(Light1Dir + CameraDir);
	float3 SpecularLight1 = DiffuseLight1 * pow( max( dot(worldNormal.xyz, halfway), 0 ), SpecularPower );

	//// LIGHT 2
	float3 Light2Dir = normalize(Light2Pos - vOut.WorldPos.xyz);
	float3 Light2Dist = length(Light2Pos - vOut.WorldPos.xyz);
	float3 DiffuseLight2 = Light2Colour * max( dot(worldNormal.xyz, Light2Dir), 0 ) / Light2Dist;
	halfway = normalize(Light2Dir + CameraDir);
	float3 SpecularLight2 = DiffuseLight2 * pow( max( dot(worldNormal.xyz, halfway), 0 ), SpecularPower );

	//// DIRRECTIONAL
	float3 DiffuseDir = DirrectionalColour * max(dot(worldNormal.xyz, DirrectionalVec), 0);
	halfway = normalize(DirrectionalVec + CameraDir);
	float3 SpecularDir = DiffuseDir * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);

	//// SPOTLIGHT
	float3 SpotToPixelVec = normalize(SpotLightPos - vOut.WorldPos.xyz);
	float3 SpotToPixelDist = length(SpotLightPos - vOut.WorldPos.xyz);
	float SpotDot = dot(SpotToPixelVec, SpotLightVec);

	float3 DiffuseSpot = { 0, 0, 0 };
	float3 SpecularSpot = { 0, 0, 0 };

	if (SpotDot > 0.0f)
	{
		DiffuseSpot = SpotLightColour * max(dot(worldNormal.xyz, SpotToPixelVec), 0) / SpotToPixelDist;
		halfway = normalize(SpotToPixelVec + CameraDir);
		SpecularSpot = DiffuseSpot * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);
	}

	// Sum the effect of the two lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
	float3 DiffuseLight = AmbientColour + DiffuseLight1 + DiffuseLight2 + DiffuseDir + DiffuseSpot;
	float3 SpecularLight = SpecularLight1 + SpecularLight2 + SpecularDir + SpecularSpot;


	////////////////////
	// Sample texture

	// Extract diffuse and specular material colour for this pixel from a texture (use offset texture coordinate from parallax mapping)
	float4 DiffuseMaterial = DiffuseMap.Sample( TrilinearWrap, offsetTexCoord );
	float3 SpecularMaterial = DiffuseMaterial.a;

	
	////////////////////
	// Combine colours 
	
	// Combine maps and lighting for final pixel colour
	float4 combinedColour;
	combinedColour.rgb = DiffuseMaterial * DiffuseLight + SpecularMaterial * SpecularLight;
	combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1

	return combinedColour;
}

float4 NormalMapLightingSphere(VS_NORMALMAP_OUTPUT vOut) : SV_Target
{
	//************************
	// Normal Map Extraction
	//************************

	// Renormalise pixel normal/tangent that were *interpolated* from the vertex normals/tangents (and may have been scaled too)
	float3 modelNormal = normalize(vOut.ModelNormal);
	float3 modelTangent = normalize(vOut.ModelTangent);

	// Calculate bi-tangent to complete the three axes of tangent space. Then create the *inverse* tangent matrix to convert *from* tangent space into model space
	float3 modelBiTangent = cross(modelNormal, modelTangent);
	float3x3 invTangentMatrix = float3x3(modelTangent, modelBiTangent, modelNormal);

	//****| INFO |**********************************************************************************//
	// The following few lines are the parallax mapping. Converts the camera direction into model
	// space and adjusts the UVs based on that and the bump depth of the texel we are looking at
	// Although short, this code involves some intricate matrix work / space transformations
	//**********************************************************************************************//

	// Get normalised vector to camera for parallax mapping and specular equation (this vector was calculated later in previous shaders)
	float3 CameraDir = normalize(CameraPos - vOut.WorldPos.xyz);

	// Transform camera vector from world into model space. Need *inverse* world matrix for this.
	// Only need 3x3 matrix to transform vectors, to invert a 3x3 matrix we transpose it (flip it about its diagonal)
	float3x3 invWorldMatrix = transpose(WorldMatrix);
	float3 cameraModelDir = normalize(mul(CameraDir, invWorldMatrix)); // Normalise in case world matrix is scaled

	// Then transform model-space camera vector into tangent space (texture coordinate space) to give the direction to offset texture
	// coordinate, only interested in x and y components. Calculated inverse tangent matrix above, so invert it back for this step
	float3x3 tangentMatrix = transpose(invTangentMatrix);
	float2 textureOffsetDir = mul(cameraModelDir, tangentMatrix);

	// Get the depth info from the normal map's alpha channel at the given texture coordinate
	// Rescale from 0->1 range to -x->+x range, x determined by ParallaxDepth setting
	float texDepth = ParallaxDepth * (NormalMap.Sample(TrilinearWrap, vOut.UV).a - 0.5f);

	// Use the depth of the texture to offset the given texture coordinate - this corrected texture coordinate will be used from here on
	float2 offsetTexCoord = vOut.UV + texDepth * textureOffsetDir * 0.75f + Mover;

	//*******************************************

	//****| INFO |**********************************************************************************//
	// The above chunk of code is used only to calculate "offsetTexCoord", which is the offset in 
	// which part of the texture we see at this pixel due to it being bumpy. The remaining code is 
	// exactly the same as normal mapping, but uses offsetTexCoord instead of the usual vOut.UV
	//**********************************************************************************************//


	// Get the texture normal from the normal map. The r,g,b pixel values actually store x,y,z components of a normal. However, r,g,b
	// values are stored in the range 0->1, whereas the x, y & z components should be in the range -1->1. So some scaling is needed
	float3 textureNormal = 2.0f * NormalMap.Sample(TrilinearWrap, offsetTexCoord) - 1.0f; // Scale from 0->1 to -1->1

																						  // Now convert the texture normal into model space using the inverse tangent matrix, and then convert into world space using the world
																						  // matrix. Normalise, because of the effects of texture filtering and in case the world matrix contains scaling
	float3 worldNormal = normalize(mul(mul(textureNormal, invTangentMatrix), WorldMatrix));

	// Now use this normal for lighting calculations in world space as usual - the remaining code same as per-pixel lighting


	///////////////////////
	// Calculate lighting

	// Would normally calculate "CameraDir" here, but it has already been calculated for use in the parallax mapping

	//// LIGHT 1
	float3 Light1Dir = normalize(Light1Pos - vOut.WorldPos.xyz);   // Direction for each light is different
	float3 Light1Dist = length(Light1Pos - vOut.WorldPos.xyz);
	float3 DiffuseLight1 = Light1Colour * max(dot(worldNormal.xyz, Light1Dir), 0) / Light1Dist;
	float3 halfway = normalize(Light1Dir + CameraDir);
	float3 SpecularLight1 = DiffuseLight1 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);

	//// LIGHT 2
	float3 Light2Dir = normalize(Light2Pos - vOut.WorldPos.xyz);
	float3 Light2Dist = length(Light2Pos - vOut.WorldPos.xyz);
	float3 DiffuseLight2 = Light2Colour * max(dot(worldNormal.xyz, Light2Dir), 0) / Light2Dist;
	halfway = normalize(Light2Dir + CameraDir);
	float3 SpecularLight2 = DiffuseLight2 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);

	//// DIRRECTIONAL
	float3 DiffuseDir = DirrectionalColour * max(dot(worldNormal.xyz, DirrectionalVec), 0);
	halfway = normalize(DirrectionalVec + CameraDir);
	float3 SpecularDir = DiffuseDir * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);

	// Sum the effect of the two lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
	float3 DiffuseLight = AmbientColour + DiffuseLight1 + DiffuseLight2 + DiffuseDir;
	float3 SpecularLight = SpecularLight1 + SpecularLight2 + SpecularDir;


	////////////////////
	// Sample texture

	// Extract diffuse and specular material colour for this pixel from a texture (use offset texture coordinate from parallax mapping)
	float4 DiffuseMaterial = DiffuseMap.Sample(TrilinearWrap, offsetTexCoord);
	float3 SpecularMaterial = DiffuseMaterial.a;


	////////////////////
	// Combine colours 

	// Combine maps and lighting for final pixel colour
	float4 combinedColour;
	combinedColour.rgb = DiffuseMaterial * DiffuseLight + SpecularMaterial * SpecularLight + TintColour;
	combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1

	return combinedColour;
}

// A pixel shader that just tints a (diffuse) texture with a fixed colour
//
float4 TintDiffuseMap( VS_BASIC_OUTPUT vOut ) : SV_Target
{
	// Extract diffuse material colour for this pixel from a texture
	float4 diffuseMapColour = DiffuseMap.Sample( TrilinearWrap, vOut.UV );

	// Tint by global colour (set from C++)
	diffuseMapColour.rgb *= TintColour / 10;

	return diffuseMapColour;
}


//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------

// States are needed to switch between additive blending for the lights and no blending for other models

RasterizerState CullNone  // Cull none of the polygons, i.e. show both sides
{
	CullMode = None;
};
RasterizerState CullBack  // Cull back side of polygon - normal behaviour, only show front of polygons
{
	CullMode = Back;
};


DepthStencilState DepthWritesOff // Don't write to the depth buffer - polygons rendered will not obscure other polygons
{
	DepthWriteMask = ZERO;
};
DepthStencilState DepthWritesOn  // Write to the depth buffer - normal behaviour 
{
	DepthWriteMask = ALL;
};


BlendState NoBlending // Switch off blending - pixels will be opaque
{
    BlendEnable[0] = FALSE;
};

BlendState AdditiveBlending // Additive blending is used for lighting effects
{
    BlendEnable[0] = TRUE;
    SrcBlend = ONE;
    DestBlend = ONE;
    BlendOp = ADD;
};


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

// Techniques are used to render models in our scene. They select a combination of vertex, geometry and pixel shader from those provided above. Can also set states.

// Vertex lighting with diffuse map
technique10 ParallaxMapping
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, NormalMapTransform() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, NormalMapLighting() ) );

		// Switch off blending states
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullBack ); 
		SetDepthStencilState( DepthWritesOn, 0 );
	}
}

technique10 ParallaxMappingSphere
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, NormalMapTransformSphere()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, NormalMapLightingSphere()));

		// Switch off blending states
		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(CullBack);
		SetDepthStencilState(DepthWritesOn, 0);
	}
}

// Additive blended texture. No lighting, but uses a global colour tint. Used for light models
technique10 AdditiveTexTint
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, BasicTransform() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, TintDiffuseMap() ) );

		SetBlendState( AdditiveBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullNone ); 
		SetDepthStencilState( DepthWritesOff, 0 );
     }
}
