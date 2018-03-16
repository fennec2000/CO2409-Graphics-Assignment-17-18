//--------------------------------------------------------------------------------------
//	The camera class encapsulates the camera's view and projection matrix
//--------------------------------------------------------------------------------------

#include "Camera.h"
#include "Device.h"
#include "Scene.h"


//-------------------------------------
// Constructors / Destructors

// Constructor - initialise all camera settings - look at the constructor declaration in the
// header file to see that there are defaults provided for everything
Camera::Camera( D3DXVECTOR3 position, D3DXVECTOR3 rotation, float fov, float nearClip, float farClip )
{
	mPosition = position;
	mRotation = rotation;
	UpdateMatrices();

	SetFOV( fov );
	SetNearClip( nearClip );
	SetFarClip( farClip );
}


//-------------------------------------
// Camera Usage

// Update the matrices used for the camera in the rendering pipeline. 
void Camera::UpdateMatrices()
{
	// Treat the camera like a model and create a world matrix for it. Then convert that into the
	// view matrix that the rendering pipeline actually uses. Also create the projection matrix,
	// a second matrix that models don't have. It is used to project geometry from 3D into 2D

	// Make matrices for position and rotations, then multiply together to get a "camera world matrix"
	D3DXMATRIXA16 matrixXRot, matrixYRot, matrixZRot, matrixTranslation;
	D3DXMatrixRotationX( &matrixXRot, mRotation.x );
	D3DXMatrixRotationY( &matrixYRot, mRotation.y );
	D3DXMatrixRotationZ( &matrixZRot, mRotation.z );
	D3DXMatrixTranslation( &matrixTranslation, mPosition.x, mPosition.y, mPosition.z);
	mWorldMatrix = matrixZRot * matrixXRot * matrixYRot * matrixTranslation;

	// The rendering pipeline actually needs the inverse of the camera world matrix - called the
	// view matrix. Creating an inverse is easy with DirectX:
	D3DXMatrixInverse( &mViewMatrix, NULL, &mWorldMatrix );

	// Initialize the projection matrix. This determines viewing properties of the camera such as
	// field of view (FOV) and near clip distance. One other factor in the projection matrix is the
	// aspect ratio of screen (width/height) - used to adjust FOV between horizontal and vertical
	float aspect = (float)ViewportWidth / ViewportHeight; 
	D3DXMatrixPerspectiveFovLH( &mProjMatrix, mFOV, 1.33f, mNearClip, mFarClip );

	// Combine the view and projection matrix into a single matrix - which can (optionally) be used
	// in the vertex shaders to save one matrix multiply per vertex
	mViewProjMatrix = mViewMatrix * mProjMatrix;
}

// Read only access to camera matrices, created every frame from position, rotation and camera settings
D3DXMATRIX Camera::ViewMatrix()
{
	// Everytime there is a request for a matrix, it is recalculated from the current position, scale etc.
	UpdateMatrices();
	return mViewMatrix;
}
D3DXMATRIX Camera::ProjectionMatrix()
{
	// Everytime there is a request for a matrix, it is recalculated from the current position, scale etc.
	UpdateMatrices();
	return mProjMatrix;
}
D3DXMATRIX Camera::ViewProjectionMatrix()
{
	// Everytime there is a request for a matrix, it is recalculated from the current position, scale etc.
	UpdateMatrices();
	return mViewProjMatrix;
}


// Control the camera's position and rotation using keys provided. Amount of motion performed depends on frame time
void Camera::Control( float frameTime, EKeyCode turnUp, EKeyCode turnDown, EKeyCode turnLeft, EKeyCode turnRight,  
                       EKeyCode moveForward, EKeyCode moveBackward, EKeyCode moveLeft, EKeyCode moveRight)
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

	// Local X movement - move in the direction of the X axis, get axis from camera's "world" matrix
	if (KeyHeld( moveRight ))
	{
		mPosition.x += mWorldMatrix._11 * kMovementSpeed * frameTime;
		mPosition.y += mWorldMatrix._12 * kMovementSpeed * frameTime;
		mPosition.z += mWorldMatrix._13 * kMovementSpeed * frameTime;
	}
	if (KeyHeld( moveLeft ))
	{
		mPosition.x -= mWorldMatrix._11 * kMovementSpeed * frameTime;
		mPosition.y -= mWorldMatrix._12 * kMovementSpeed * frameTime;
		mPosition.z -= mWorldMatrix._13 * kMovementSpeed * frameTime;
	}

	// Local Z movement - move in the direction of the Z axis, get axis from view matrix
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
