//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef COLOR_H
#define COLOR_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h" // Needed for uint8
#include "tier0/basetypes.h" // Needed for CUtlMemory, CUtlVector


//-----------------------------------------------------------------------------
// Purpose: Basic RGBA color class
//-----------------------------------------------------------------------------
class Color
{
public:
	// constructors
	Color() { SetColor(0,0,0,0); }
	Color(int r, int g, int b) { SetColor(r,g,b,255); }
	Color(int r, int g, int b, int a) { SetColor(r,g,b,a); }
	Color( const Color &rhs ) { SetColor( rhs.r(), rhs.g(), rhs.b(), rhs.a() ); }

	// set the color
	// r - red component (0-255)
	// g - green component (0-255)
	// b - blue component (0-255)
	// a - alpha component, controls transparency (0 - transparent, 255 - opaque);
	void SetColor(int r, int g, int b, int a = 255);

	// get the color
	// returns true if the color is valid (i.e. r,g,b,a are all in the range 0-255)
	void GetColor(int &r, int &g, int &b, int &a) const;

	// set the color (byte access)
	void SetRawColor( int color32 );
	// get the color (byte access)
	int GetRawColor() const;

	// returns the red component
	int r() const { return _color[0]; }
	// returns the green component
	int g() const { return _color[1]; }
	// returns the blue component
	int b() const { return _color[2]; }
	// returns the alpha component
	int a() const { return _color[3]; }

	// set the red component
	void SetR(int r) { _color[0] = (unsigned char)r; }
	// set the green component
	void SetG(int g) { _color[1] = (unsigned char)g; }
	// set the blue component
	void SetB(int b) { _color[2] = (unsigned char)b; }
	// set the alpha component
	void SetA(int a) { _color[3] = (unsigned char)a; }

	// array access
	unsigned char operator[](int index) const;
	unsigned char& operator[](int index);

	// comparison
	bool operator == (const Color &rhs) const;
	bool operator != (const Color &rhs) const;

	// assignment
	Color& operator=( const Color &rhs );

private:
	unsigned char _color[4];
};


//-----------------------------------------------------------------------------
// Color inline functions
//-----------------------------------------------------------------------------
inline void Color::SetColor(int r, int g, int b, int a)
{
	_color[0] = (unsigned char)r;
	_color[1] = (unsigned char)g;
	_color[2] = (unsigned char)b;
	_color[3] = (unsigned char)a;
}

inline void Color::GetColor(int &r, int &g, int &b, int &a) const
{
	r = _color[0];
	g = _color[1];
	b = _color[2];
	a = _color[3];
}

inline void Color::SetRawColor( int color32 )
{
	*((int*)this) = color32;
}

inline int Color::GetRawColor() const
{
	return *((int*)this);
}

inline unsigned char Color::operator[](int index) const
{
	return _color[index];
}

inline unsigned char& Color::operator[](int index)
{
	return _color[index];
}

inline bool Color::operator == (const Color &rhs) const
{
	return ( (*((int *)this)) == (*((int *)&rhs)) );
}

inline bool Color::operator != (const Color &rhs) const
{
	return !(operator==(rhs));
}

inline Color& Color::operator=( const Color &rhs )
{
	SetRawColor( rhs.GetRawColor() );
	return *this;
}


#endif // COLOR_H
