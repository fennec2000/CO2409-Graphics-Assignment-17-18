//--------------------------------------------------------------------------------------
// Initialise Direct3D (viewport, back buffer, depth buffer etc.)
//--------------------------------------------------------------------------------------

// Header guard - prevents this file being included more than once
#ifndef CO2409_DEVICE_H_INCLUDED
#define CO2409_DEVICE_H_INCLUDED

#include <d3d10.h>

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
// Using externs to make global variables from the .cpp file available to other files.
// It would be better to use classes and avoid globals to make the code tidier. However
// that might hide the overall picture and might make things more difficult to follow.

// The main D3D interface, this pointer is used to access most D3D functions
extern ID3D10Device* Device;

// Other variables used to setup D3D
extern IDXGISwapChain*         SwapChain;
extern ID3D10Texture2D*        DepthStencil;
extern ID3D10DepthStencilView* DepthStencilView;
extern ID3D10RenderTargetView* BackBufferRenderTarget;

// Width and height of the window viewport
extern unsigned int ViewportWidth;
extern unsigned int ViewportHeight;


//--------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------
// Make functions available to other files

// Create Direct3D device and swap chain (pass the window to attach Direct3D to)
bool InitDevice(HWND hWnd);

// Release Direct3D objects to free memory when quitting
void ReleaseDevice();


#endif // End of header guard (see top of file)