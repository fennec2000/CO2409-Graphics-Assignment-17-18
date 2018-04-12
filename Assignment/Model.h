//--------------------------------------------------------------------------------------
//	The model class collects together a model's geometry (vertex and index data) and
//	also manages its positioning in the world
//--------------------------------------------------------------------------------------

// Header guard - prevents this file being included more than once
#ifndef CO2409_MODEL_H_INCLUDED
#define CO2409_MODEL_H_INCLUDED

#include "Input.h"
#include <d3d10.h>
#include <d3dx10.h>
#include <string>
using namespace std;

class Model
{
	//-------------------------------------
	// Private member variables
	//-------------------------------------
private:
	//-------------------------------------
	// Postioning

	// Positions, rotations and scaling for the model
	D3DXVECTOR3 mPosition;
	D3DXVECTOR3 mRotation;
	D3DXVECTOR3 mScale;

	// World matrix for the model - built from the above
	D3DXMATRIX mWorldMatrix;


	//-------------------------------------
	// Geometry data

	// Does this model have any geometry to render
	bool                     mHasGeometry;

	// Model vertices a stored in a vertex buffer and the number of the vertices in the buffer
	ID3D10Buffer*            mVertexBuffer;
	unsigned int             mNumVertices;

	// Description of the elements in a single vertex (position, normal, UVs etc.)
	static const int         MAX_VERTEX_ELTS = 64;
	D3D10_INPUT_ELEMENT_DESC mVertexElts[MAX_VERTEX_ELTS];
	ID3D10InputLayout*       mVertexLayout; // Layout of a vertex (derived from above)
	unsigned int             mVertexSize;   // Size of vertex calculated from contained elements

											// Index data for the model stored in a index buffer and the number of indices in the buffer
	ID3D10Buffer*            mIndexBuffer;
	unsigned int             mNumIndices;


	//-------------------------------------
	// Public member functions
	//-------------------------------------
public:

	//-------------------------------------
	// Constructors / Destructors

	// Constructor - initialise all settings, sensible defaults provided for everything.
	Model(D3DXVECTOR3 position = D3DXVECTOR3(0, 0, 0), D3DXVECTOR3 rotation = D3DXVECTOR3(0, 0, 0), float scale = 1);

	// Destructor
	~Model();

	// Release resources used by model
	void ReleaseResources();


	//-------------------------------------
	// Data access

	// Getters / setters for data we want to expose
	D3DXVECTOR3 Position() { return mPosition; }
	D3DXVECTOR3 Rotation() { return mRotation; }
	D3DXVECTOR3 Scale() { return mScale; }
	void SetPosition(D3DXVECTOR3 position) { mPosition = position; }
	void SetRotation(D3DXVECTOR3 rotation) { mRotation = rotation; }
	// Two ways to set scale: x,y,z separately, or all to the same value
	void SetScale(D3DXVECTOR3 scale) { mScale = scale; }
	void SetScale(float scale) { mScale = D3DXVECTOR3(scale, scale, scale); }

	// Added these functions for the shadow mapping lab - want spotlight models to face in a given directions
	D3DXVECTOR3 Facing();
	void FacePoint(D3DXVECTOR3 point);   // Make the model face a given point
	void FaceDirection(D3DXVECTOR3 dir); // Make the model face a given direction (i.e. its z-axis will face in this direction)

										 // Read only access to model world matrix, created every frame from position, rotation and scale
	D3DXMATRIX WorldMatrix();

	//-------------------------------------
	// Model Loading

	// Load the model geometry from a file. This function only reads the geometry using the first
	// material in the file, so multi-material models will load but will have parts missing. May 
	// optionally request for tangents to be created for the model (for normal or parallax mapping)
	// We need to pass an example technique that the model will use to help DirectX understand how 
	// to connect this data with the vertex shaders. Returns true if the load was successful
	bool Load(const string& fileName, ID3D10EffectTechnique* shaderCode, bool tangents = false);


	//-------------------------------------
	// Model Usage

	// Update the world matrix of the model from its position, rotation and scaling
	void UpdateMatrix();

	// Control the model's position and rotation using keys provided. Amount of motion performed depends on frame time
	void Control(float frameTime, EKeyCode turnUp, EKeyCode turnDown, EKeyCode turnLeft, EKeyCode turnRight,
		EKeyCode turnCW, EKeyCode turnCCW, EKeyCode moveForward, EKeyCode moveBackward);

	// Render the model with the given technique. Assumes any shader variables for the technique
	// have already been set up (e.g. matrices and textures)
	void Render(ID3D10EffectTechnique* technique);
};


#endif // End of header guard (see top of file)
