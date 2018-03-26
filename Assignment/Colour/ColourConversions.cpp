//
// ColourConversions.cpp
//
// Source file for colour conversion functions. The key functions are initially
// empty - you need to implement them for the lab exercise
#include <math.h>
#include <string>
#include <iomanip>
#include <sstream>
#include "ColourConversions.h"

// Find the minimum of three numbers (helper function for exercise below)
float Min( float f1, float f2, float f3 )
{
	float fMin = f1;
	if (f2 < fMin)
	{
		fMin = f2;
	}
	if (f3 < fMin)
	{
		fMin = f3;
	}
	return fMin;
}

// Find the maximum of three numbers (helper function for exercise below)
float Max( float f1, float f2, float f3 )
{
	float fMax = f1;
	if (f2 > fMax)
	{
		fMax = f2;
	}
	if (f3 > fMax)
	{
		fMax = f3;
	}
	return fMax;
}

// converts from base 10 to base 16
// converts from 0 to 255
std::string Base10ToBase16(int base10)
{
	std::stringstream ss;
	ss << std::hex << base10;
	return ss.str();
}

// converts from base 16 to base 10
int Base16ToBase10(std::string base16)
{
	return std::stoi(base16);
}

// Convert an RGB colour to a HSL colour
void RGBToHSL( int R, int G, int B, int& H, int& S, int& L )
{
	// Fill in the correct code here for question 4, the functions Min and Max above will help
	float fR, fG, fB;
	fR = R / 255.0f;
	fG = G / 255.0f;
	fB = B / 255.0f;

	RGBToHSL(fR, fG, fB, H, S, L);
}

void RGBToHSL(float R, float G, float B, float & H, float & S, float & L)
{
	float max = Max(R, G, B);
	float min = Min(R, G, B);

	L = 50.0f * (max + min);

	if (min == max)
		H = 0;
	else
	{
		if (L < 50.0f)
			S = 100.0f * (max - min) / (max + min);
		else
			S = 100.0f * (max - min) / (2.0f - max - min);

		if (max == R)
			H = 60.0f * (G - B) / (max - min);
		else if (max == G)
			H = 60.0f * (B - R) / (max - min) + 120.0f;
		else if (max == B)
			H = 60.0f * (R - G) / (max - min) + 240.0f;
	}

	if (H < 0)
		H = H + 360.0f;
}

// Convert an RGB colour to a Hex colour
//void RGBToHex(int R, int G, int B, System::String^& Hex)
//{
//	std::string R_String = Base10ToBase16(R) + Base10ToBase16(G) + Base10ToBase16(B);
//	Hex = gcnew System::String(R_String.c_str());
//}

// Convert a HSL colour to an RGB colour
void HSLToRGB( int H, int S, int L, int& R, int& G, int& B )
{
	// Fill in the correct code here for question 7 (advanced)
	// http://www.rapidtables.com/convert/color/hsl-to-rgb.htm

	float fS = S / 100.0f;
	float fL = L / 100.0f;
	float fH = H / 60.0f;

	float fR, fG, fB;
	HSLToRGB(fH, fS, fL, fR, fG, fB);

	R = int(fR * 255);
	G = int(fG * 255);
	B = int(fB * 255);
}

void HSLToRGB(float H, float S, float L, float & R, float & G, float & B)
{
	float fS = S / 100.0f;
	float fL = L / 100.0f;
	float C = (1.0f - fabs(2.0f * fL - 1.0f)) * fS;
	H = fmod(H, 360);
	float fH = H / 60.0f;
	float X = C * (1.0f - fabs(fmodf(fH, 2.0f) - 1.0f));
	float m = fL - C / 2.0f;

	if (H >= 0.0f && H < 60.0f)
	{
		R = C;
		G = X;
		B = 0.0f;
	}
	else if (H >= 60.0f && H < 120.0f)
	{
		R = X;
		G = C;
		B = 0.0f;
	}
	else if (H >= 120.0f && H < 180.0f)
	{
		R = 0.0f;
		G = C;
		B = X;
	}
	else if (H >= 180.0f && H < 240.0f)
	{
		R = 0.0f;
		G = X;
		B = C;
	}
	else if (H >= 240.0f && H < 300.0f)
	{
		R = X;
		G = 0.0f;
		B = C;
	}
	else if (H >= 300.0f && H < 360.0f)
	{
		R = C;
		G = 0.0f;
		B = X;
	}
}

// Convert an Hex colour to a RGB colour
void HexToRGB(std::string Hex, int& R, int& G, int& B)
{

}