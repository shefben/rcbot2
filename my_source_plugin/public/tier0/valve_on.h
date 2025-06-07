//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

// This file is used to turn specific compiler warnings back on after they have been
// turned off with valve_off.h.
// It is included at the end of a block of code, after valve_off.h has been included
// at the beginning of the block.

#ifdef _MSC_VER

// Restore the warning state that was saved by valve_off.h
#pragma warning( pop )

#endif // _MSC_VER
