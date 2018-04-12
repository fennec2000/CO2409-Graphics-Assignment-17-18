//--------------------------------------------------------------------------------------
//	The model class collects together a model's geometry (vertex and index data) and
//	also manages its positioning in the world
//--------------------------------------------------------------------------------------

#include "Model.h"
#include "Device.h"
#include "Scene.h"
#include "CImportXFile.h" // Class to load meshes (taken from another graphics engine)

///////////////////////////////
// Constructors / Destructors

// Constructor - initialise all model settings - look at the constructor declaration in the header
// file to see that there are defaults provided for everything
Model::Model( D3DXVECTOR3 position, D3DXVECTOR3 rotation, float scale )
{
	mPosition = position;
	mRotation = rotation;
	SetScale( scale );
	UpdateMatrix();

	// Good practice to ensure all private data is sensibly initialised
	mVertexBuffer = NULL;
	mNumVertices = 0;
	mVertexSize = 0;
	mVertexLayout = NULL;

	mIndexBuffer = NULL;
	mNumIndices = 0;

	mHasGeometry = false;
}

// Model destructor
Model::~Model()
{
	ReleaseResources();
}

// Release resources used by model
void Model::ReleaseResources()
{
	// Release resources
	if (mIndexBuffer )  mIndexBuffer ->Release();
	if (mVertexBuffer)  mVertexBuffer->Release();
	if (mVertexLayout)  mVertexLayout->Release();
	mHasGeometry = false;
}

/////////////////////////////
// Model facing

// Added these functions for the shadow mapping lab - want spotlight models to face in a given directions

// Get the direction the model is facing
D3DXVECTOR3 Model::Facing()
{
	D3DXVECTOR3 facing;
	D3DXVec3Normalize(&facing, &D3DXVECTOR3(&mWorldMatrix(2, 0)));
	return facing;
}

// Make the model face a given point
void Model::FacePoint(D3DXVECTOR3 point)
{
	using gen::CVector3;
	using gen::CMatrix4x4;

	// Want to set position and rotation so the model faces a given point. Needs a little maths.
	// Using my own maths classes to do this (these classes are already used in the import .x file code)
	// Can do a similar process with the D3DX functions, but it's less convenient
	// Method: Quite easy to make a (world) matrix that faces a particular direction - just force the z-axis 
	// that way and put the other axes at right angles. Then extract the position and rotations from that matrix
	// Two function calls into the maths classes - have a look at these classes if you're interested
	CMatrix4x4 facingMatrix = MatrixFaceTarget(CVector3(mPosition), CVector3(point));
	facingMatrix.DecomposeAffineEuler((CVector3*)&mPosition, (CVector3*)&mRotation, 0);
}

// Make the model face a given direction (i.e. its z-axis will face in this direction) - almost same as above function
void Model::FaceDirection(D3DXVECTOR3 dir)
{
	using gen::CVector3;
	using gen::CMatrix4x4;

	CMatrix4x4 facingMatrix = MatrixFaceDirection(CVector3(mPosition), CVector3(dir));
	facingMatrix.DecomposeAffineEuler((CVector3*)&mPosition, (CVector3*)&mRotation, 0);
}

/////////////////////////////
// Model Loading

// The loading and parsing of ".X" files is supported using a class taken from another application.
// We will not look at the process (more to do with parsing than graphics). Ultimately we end up
// with arrays of data just like the previous labs where the geometry was typed in to the code

// Load the model geometry from a file. This function only reads the geometry using the first
// material in the file, so multi-material models will load but will have parts missing. May 
// optionally request for tangents to be created for the model (for normal or parallax mapping)
// We need to pass an example technique that the model will use to help DirectX understand how 
// to connect this data with the vertex shaders. Returns true if the load was successful
bool Model::Load( const string& fileName, ID3D10EffectTechnique* exampleTechnique, bool tangents )
{
	// Release any existing geometry in this object
	ReleaseResources();

	// Use CImportXFile class (from another application) to load the given file. The import code is wrapped in the namespace 'gen'
	gen::CImportXFile mesh;
	if (mesh.ImportFile( fileName.c_str() ) != gen::kSuccess)
	{
		return false;
	}

	// Get first sub-mesh from loaded file
	gen::SSubMesh subMesh;
	if (mesh.GetSubMesh( 0, &subMesh, tangents ) != gen::kSuccess)
	{
		return false;
	}


	// Create vertex element list & layout. We need a vertex layout to say what data we have per vertex in this model (e.g. position, normal, uv, etc.)
	// In previous projects the element list was a manually typed in array as we knew what data we would provide. However, as we can load models with
	// different vertex data this time we need flexible code. The array is built up one element at a time: ask the import class if it loaded normals, 
	// if so then add a normal line to the array, then ask if it loaded UVS...etc
	unsigned int numElts = 0;
	unsigned int offset = 0;
	// Position is always required
	mVertexElts[numElts].SemanticName = "POSITION";   // Semantic in HLSL (what is this data for)
	mVertexElts[numElts].SemanticIndex = 0;           // Index to add to semantic (a count for this kind of data, when using multiple of the same type, e.g. TEXCOORD0, TEXCOORD1)
	mVertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT; // Type of data - this one will be a float3 in the shader. Most data communicated as though it were colours
	mVertexElts[numElts].AlignedByteOffset = offset;  // Offset of element from start of vertex data (e.g. if we have position (float3), uv (float2) then normal, the normal's offset is 5 floats = 5*4 = 20)
	mVertexElts[numElts].InputSlot = 0;               // For when using multiple vertex buffers (e.g. instancing - an advanced topic)
	mVertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA; // Use this value for most cases (only changed for instancing)
	mVertexElts[numElts].InstanceDataStepRate = 0;                     // --"--
	offset += 12;
	++numElts;
	// Repeat for each kind of vertex data
	if (subMesh.hasNormals)
	{
		mVertexElts[numElts].SemanticName = "NORMAL";
		mVertexElts[numElts].SemanticIndex = 0;
		mVertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		mVertexElts[numElts].AlignedByteOffset = offset;
		mVertexElts[numElts].InputSlot = 0;
		mVertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		mVertexElts[numElts].InstanceDataStepRate = 0;
		offset += 12;
		++numElts;
	}
	if (subMesh.hasTangents)
	{
		mVertexElts[numElts].SemanticName = "TANGENT";
		mVertexElts[numElts].SemanticIndex = 0;
		mVertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		mVertexElts[numElts].AlignedByteOffset = offset;
		mVertexElts[numElts].InputSlot = 0;
		mVertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		mVertexElts[numElts].InstanceDataStepRate = 0;
		offset += 12;
		++numElts;
	}
	if (subMesh.hasTextureCoords)
	{
		mVertexElts[numElts].SemanticName = "TEXCOORD";
		mVertexElts[numElts].SemanticIndex = 0;
		mVertexElts[numElts].Format = DXGI_FORMAT_R32G32_FLOAT;
		mVertexElts[numElts].AlignedByteOffset = offset;
		mVertexElts[numElts].InputSlot = 0;
		mVertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		mVertexElts[numElts].InstanceDataStepRate = 0;
		offset += 8;
		++numElts;
	}
	if (subMesh.hasVertexColours)
	{
		mVertexElts[numElts].SemanticName = "COLOR";
		mVertexElts[numElts].SemanticIndex = 0;
		mVertexElts[numElts].Format = DXGI_FORMAT_R8G8B8A8_UNORM; // A RGBA colour with 1 byte (0-255) per component
		mVertexElts[numElts].AlignedByteOffset = offset;
		mVertexElts[numElts].InputSlot = 0;
		mVertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		mVertexElts[numElts].InstanceDataStepRate = 0;
		offset += 4;
		++numElts;
	}
	mVertexSize = offset;

	// Given the vertex element list, pass it to DirectX to create a vertex layout. We also need to pass an example of a technique that will
	// render this model. We will only be able to render this model with techniques that have the same vertex input as the example we use here
	D3D10_PASS_DESC PassDesc;
	exampleTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	Device->CreateInputLayout( mVertexElts, numElts, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &mVertexLayout );


	// Create the vertex buffer and fill it with the loaded vertex data
	mNumVertices = subMesh.numVertices;
	D3D10_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D10_USAGE_DEFAULT; // Not a dynamic buffer
	bufferDesc.ByteWidth = mNumVertices * mVertexSize; // Buffer size
	bufferDesc.CPUAccessFlags = 0;   // Indicates that CPU won't access this buffer at all after creation
	bufferDesc.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA initData; // Initial data
	initData.pSysMem = subMesh.vertices;   
	if (FAILED( Device->CreateBuffer( &bufferDesc, &initData, &mVertexBuffer )))
	{
		return false;
	}


	// Create the index buffer - assuming 2-byte (WORD) index data
	mNumIndices = static_cast<unsigned int>(subMesh.numFaces) * 3;
	bufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
	bufferDesc.Usage = D3D10_USAGE_DEFAULT;
	bufferDesc.ByteWidth = mNumIndices * sizeof(WORD);
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	initData.pSysMem = subMesh.faces;   
	if (FAILED( Device->CreateBuffer( &bufferDesc, &initData, &mIndexBuffer )))
	{
		return false;
	}

	mHasGeometry = true;
	return true;
}


/////////////////////////////
// Model Usage

// Update the world matrix of the model from its position, rotation and scaling
void Model::UpdateMatrix()
{
	// Make a matrix for position and scaling, and one each for X, Y & Z rotations
	D3DXMATRIX matrixXRot, matrixYRot, matrixZRot, matrixTranslation, matrixScaling;
	D3DXMatrixRotationX( &matrixXRot, mRotation.x );
	D3DXMatrixRotationY( &matrixYRot, mRotation.y );
	D3DXMatrixRotationZ( &matrixZRot, mRotation.z );
	D3DXMatrixTranslation( &matrixTranslation, mPosition.x, mPosition.y, mPosition.z );
	D3DXMatrixScaling( &matrixScaling, mScale.x, mScale.y, mScale.z );

	// Multiply above matrices together to get the effect of them all combined - this makes the world
	// matrix for the rendering pipeline. Order of multiplication affects how the controls will operate
	mWorldMatrix = matrixScaling * matrixZRot * matrixXRot * matrixYRot * matrixTranslation;
}

// Read only access to model world matrix, created every frame from position, rotation and scale
D3DXMATRIX Model::WorldMatrix()
{
	// Everytime there is a request for the world matrix, it is recalculated from the current position, scale etc.
	UpdateMatrix();
	return mWorldMatrix;
}

// Control the model's position and rotation using keys provided. Amount of motion performed depends on frame time
void Model::Control( float frameTime, EKeyCode turnUp, EKeyCode turnDown, EKeyCode turnLeft, EKeyCode turnRight,  
                      EKeyCode turnCW, EKeyCode turnCCW, EKeyCode moveForward, EKeyCode moveBackward )
{
	if (KeyHeld( turnDown ))
	{
		mRotation.x += kRotationSpeed * frameTime;
	}
	if (KeyHeld( turnUp ))
	{
		mRotation.x -= kRotationSpeed * frameTime;
	}
	if (KeyHeld( turnRight ))
	{
		mRotation.y += kRotationSpeed * frameTime;
	}
	if (KeyHeld( turnLeft ))
	{
		mRotation.y -= kRotationSpeed * frameTime;
	}
	if (KeyHeld( turnCW ))
	{
		mRotation.z += kRotationSpeed * frameTime;
	}
	if (KeyHeld( turnCCW ))
	{
		mRotation.z -= kRotationSpeed * frameTime;
	}

	// Local Z movement - move in the direction of the Z axis, get axis from world matrix
	if (KeyHeld( moveForward ))
	{
		mPosition.x += mWorldMatrix._31 * kMovementSpeed * frameTime;
		mPosition.y += mWorldMatrix._32 * kMovementSpeed * frameTime;
		mPosition.z += mWorldMatrix._33 * kMovementSpeed * frameTime;
	}
	if (KeyHeld( moveBackward ))
	{
		mPosition.x -= mWorldMatrix._31 * kMovementSpeed * frameTime;
		mPosition.y -= mWorldMatrix._32 * kMovementSpeed * frameTime;
		mPosition.z -= mWorldMatrix._33 * kMovementSpeed * frameTime;
	}
}


// Render the model with the given technique. Assumes any shader variables for the technique
// have already been set up (e.g. matrices and textures)
void Model::Render( ID3D10EffectTechnique* technique )
{
	// Don't render if no geometry
	if (!mHasGeometry)
	{
		return;
	}

	// Select vertex and index buffer - assuming all data will be as triangle lists
	UINT offset = 0;
	Device->IASetVertexBuffers( 0, 1, &mVertexBuffer, &mVertexSize, &offset );
	Device->IASetInputLayout( mVertexLayout );
	Device->IASetIndexBuffer( mIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );
	Device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	// Render the model. Vertex buffers are prepared abovce, calling code must have prepared textures,
	// states, shaders and shader variables.
	D3D10_TECHNIQUE_DESC techDesc;
	technique->GetDesc( &techDesc );
	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		technique->GetPassByIndex( p )->Apply( 0 );
		Device->DrawIndexed( mNumIndices, 0, 0 );
	}
}
