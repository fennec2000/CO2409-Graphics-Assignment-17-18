//--------------------------------------------------------------------------------------
//	The camera class encapsulates the camera's view and projection matrix
//--------------------------------------------------------------------------------------

// Header guard - prevents this file being included more than once
#ifndef CO2409_CAMERA_H_INCLUDED
#define CO2409_CAMERA_H_INCLUDED

#include "Input.h"
#include <d3d10.h>
#include <d3dx10.h>

class Camera
{
//-------------------------------------
// Private member variables
//-------------------------------------
private:

	// Postition and rotations for the camera (rarely scale cameras)
	D3DXVECTOR3 mPosition;
	D3DXVECTOR3 mRotation;

	// Camera settings: field of view, near and far clip plane distances.
	// Note that the FOV angle is measured in radians (radians = degrees * PI/180)
	float mFOV;
	float mNearClip;
	float mFarClip;

	// Current view, projection and combined view-projection matrices (DirectX matrix type)
	D3DXMATRIX mWorldMatrix;    // Easiest to treat the camera like a model and give it a "world" matrix...
	D3DXMATRIX mViewMatrix;     // ... the view matrix used in the pipeline is the inverse of its world matrix
	D3DXMATRIX mProjMatrix;     // Projection matrix to set field of view and near/far clip distances
	D3DXMATRIX mViewProjMatrix; // Combine (multiply) the view and projection matrices together, which
	                             // saves a matrix multiply in the shader (optional optimisation)


//-------------------------------------
// Public member functions
//-------------------------------------
public:

	//-------------------------------------
	// Constructors / Destructors

	// Constructor - initialise all settings, sensible defaults provided for everything.
	Camera( D3DXVECTOR3 position = D3DXVECTOR3(0,0,0), D3DXVECTOR3 rotation = D3DXVECTOR3(0,0,0),
	        float fov = D3DX_PI/4, float nearClip = 0.1f, float farClip = 10000.0f );


	//-------------------------------------
	// Data access

	// Getters / setters for data we want to expose
	D3DXVECTOR3 Position()  { return mPosition; }
	D3DXVECTOR3 Rotation()  { return mRotation;	}
	void SetPosition( D3DXVECTOR3 position )  { mPosition = position; }
	void SetRotation( D3DXVECTOR3 rotation )  { mRotation = rotation; }

	float FOV()       { return mFOV;      }
	float NearClip()  { return mNearClip; }
	float FarClip()   { return mFarClip;  }
	void SetFOV     ( float fov      )  { mFOV      = fov;      }
	void SetNearClip( float nearClip )  { mNearClip = nearClip; }
	void SetFarClip ( float farClip  )  { mFarClip  = farClip;  }

	// Read only access to camera matrices, created every frame from position, rotation and camera settings
	D3DXMATRIX ViewMatrix();
	D3DXMATRIX ProjectionMatrix();
	D3DXMATRIX ViewProjectionMatrix();

	
	//-------------------------------------
	// Camera Usage

	// Update the matrices used for the camera in the rendering pipeline
	void UpdateMatrices();

	// Control the camera's position and rotation using keys provided
	void Control( float frameTime, EKeyCode turnUp, EKeyCode turnDown, EKeyCode turnLeft, EKeyCode turnRight,  
	              EKeyCode moveForward, EKeyCode moveBackward, EKeyCode moveLeft, EKeyCode moveRight);
};


#endif // End of header guard - see top of file
