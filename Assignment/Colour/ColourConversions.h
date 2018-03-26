//
// ColourConversions.h
//
// Header file for colour conversion functions
#include <string>

// Convert an RGB colour to a HSL colour
void RGBToHSL( int R, int G, int B, int& H, int& S, int& L );
void RGBToHSL(float R, float G, float B, float & H, float & S, float & L);

// Convert an RGB colour to a Hex colour
//void RGBToHex( int R, int G, int B, System::String^& Hex);

// Convert a HSL colour to an RGB colour
void HSLToRGB( int H, int S, int L, int& R, int& G, int& B );
void HSLToRGB(float H, float S, float L, float& R, float& G, float& B);

// Convert an Hex colour to a RGB colour
void HexToRGB(std::string Hex, int& R, int& G, int& B);
