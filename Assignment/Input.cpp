//--------------------------------------------------------------------------------------
// Key/mouse input functions. [Gamers: Used in the same way as the TL-Engine]
//--------------------------------------------------------------------------------------

#include "Input.h"


//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

// Pressed state of each key
EKeyState KeyStates[kMaxKeyCodes];

// Mouse position
unsigned int MouseX = 0;
unsigned int MouseY = 0;


//--------------------------------------------------------------------------------------
// Initialisation
//--------------------------------------------------------------------------------------

void InitInput()
{
	// Initialise input data
	for (int i = 0; i < kMaxKeyCodes; ++i)
	{
		KeyStates[i] = kNotPressed;
	}
}


//--------------------------------------------------------------------------------------
// Events
//--------------------------------------------------------------------------------------

// Event called to indicate that a key has been pressed down
void KeyDownEvent(EKeyCode Key)
{
	if (KeyStates[Key] == kNotPressed)
	{
		KeyStates[Key] = kPressed;
	}
	else
	{
		KeyStates[Key] = kHeld;
	}
}

// Event called to indicate that a key has been lifted up
void KeyUpEvent(EKeyCode Key)
{
	KeyStates[Key] = kNotPressed;
}

// Event called to indicate the mouse has moved
void MouseMoveEvent(unsigned int x, unsigned int y)
{
	MouseX = x;
	MouseY = y;
}


//--------------------------------------------------------------------------------------
// Input functions
//--------------------------------------------------------------------------------------

// Returns true when a given key or button is first pressed down. Use
// for one-off actions or toggles. Example key codes: Key_A or
// Mouse_LButton, see input.h for a full list.
bool KeyHit(EKeyCode eKeyCode)
{
	if (KeyStates[eKeyCode] == kPressed)
	{
		KeyStates[eKeyCode] = kHeld;
		return true;
	}
	return false;
}

// Returns true as long as a given key or button is held down. Use for
// continuous action or motion. Example key codes: Key_A or
// Mouse_LButton, see input.h for a full list.
bool KeyHeld(EKeyCode eKeyCode)
{
	if (KeyStates[eKeyCode] == kNotPressed)
	{
		return false;
	}
	KeyStates[eKeyCode] = kHeld;
	return true;
}

		
// Return current mouse X coordinate
unsigned int GetMouseX()
{
	return MouseX;
}

// Return current mouse Y coordinate
unsigned int GetMouseY()
{
	return MouseY;
}
