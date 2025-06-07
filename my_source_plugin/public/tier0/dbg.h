//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef DBG_H
#define DBG_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"


//-----------------------------------------------------------------------------
// DLL export for tier0
//-----------------------------------------------------------------------------
#ifndef TIER0_DLL_EXPORT
#ifdef TIER0_DLL_EXPORT_VALVE
#define TIER0_DLL_EXPORT __declspec( dllexport )
#elif TIER0_DLL_IMPORT_VALVE
#define TIER0_DLL_EXPORT __declspec( dllimport )
#else
#define TIER0_DLL_EXPORT
#endif
#endif


//-----------------------------------------------------------------------------
// DLL export for tier1
//-----------------------------------------------------------------------------
#ifndef TIER1_DLL_EXPORT
#ifdef TIER1_DLL_EXPORT_VALVE
#define TIER1_DLL_EXPORT __declspec( dllexport )
#elif TIER1_DLL_IMPORT_VALVE
#define TIER1_DLL_EXPORT __declspec( dllimport )
#else
#define TIER1_DLL_EXPORT
#endif
#endif


//-----------------------------------------------------------------------------
// Used to mark a variable as not being in the bss section
//-----------------------------------------------------------------------------
#ifndef NO_INIT
#define NO_INIT
#endif


//-----------------------------------------------------------------------------
// For now, just including these directly
//-----------------------------------------------------------------------------
#include <assert.h>


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CCommand;
class ConCommandBase;
class ConVar;


//-----------------------------------------------------------------------------
// ConVar flags
//-----------------------------------------------------------------------------
// The default, no flags at all
#define FCVAR_NONE				0

// Command to ConVars and ConCommands
// ConVar Systems
#define FCVAR_UNREGISTERED		(1<<0)	// If this is set, don't add to linked list, etc.
#define FCVAR_DEVELOPMENTONLY	(1<<1)	// Hidden in released products. Flag is removed automatically if ALLOW_DEVELOPMENT_CVARS is defined.
#define FCVAR_GAMEDLL			(1<<2)	// defined by the game DLL
#define FCVAR_CLIENTDLL			(1<<3)  // defined by the client DLL
#define FCVAR_HIDDEN			(1<<4)	// Hidden. Doesn't appear in find or auto complete. Like DEVELOPMENTONLY, but can't be compiled out.

// ConVar only
#define FCVAR_PROTECTED			(1<<5)  // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value.
#define FCVAR_SPONLY			(1<<6)  // This cvar cannot be changed by clients connected to a multiplayer server.
#define	FCVAR_ARCHIVE			(1<<7)	// set to cause it to be saved to vars.rc
#define	FCVAR_NOTIFY			(1<<8)	// notifies players when changed
#define	FCVAR_USERINFO			(1<<9)	// changes the client's info string

#define FCVAR_PRINTABLEONLY		(1<<10)  // This cvar's string cannot contain unprintable characters ( e.g., used for player name etc ).
#define FCVAR_UNLOGGED			(1<<11)  // If this is a FCVAR_SERVER, don't log to server log file when its changed
#define FCVAR_NEVER_AS_STRING	(1<<12)  // Never try to print this variable to the console (e.g., raw binary data)
#define FCVAR_REPLICATED		(1<<13)	// Server setting enforced on clients
#define FCVAR_CHEAT				(1<<14) // Only useable in singleplayer / debug / multiplayer & sv_cheats
#define FCVAR_SS				(1<<15) // causes varnameN where N is slot number to be saved
#define FCVAR_DEMO				(1<<16) // record this cvar when starting a demo file
#define FCVAR_DONTRECORD		(1<<17) // don't record these command in demofiles
#define FCVAR_NOT_CONNECTED		(1<<20)	// cvar cannot be changed by a client connected to a server
#define FCVAR_ARCHIVE_XBOX		(1<<21) // cvar written to config.cfg on the Xbox

#define FCVAR_SERVER_CAN_EXECUTE	(1<<28)// the server is allowed to execute this command on clients via ClientCommand/NET_StringCmd/CBaseClientState::ProcessStringCmd.
#define FCVAR_SERVER_CANNOT_QUERY	(1<<29)// If this is set, then the server is not allowed to query this cvar's value (via IServerPluginHelpers::StartQueryCvarValue).
#define FCVAR_CLIENTCMD_CAN_EXECUTE	(1<<30)	// IVEngineClient::ClientCmd is allowed to execute this command.
											// Note: IVEngineClient::ClientCmd_Unrestricted can run any command. TODO: Fix spelling of CLIENTCMD.

//
// FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS specific defines
//
// This cvar is read from the server by remote clients (clients running on a different machine than the server).
// FCVAR_GAMEDLL must be set for this to work.
#define FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS	FCVAR_GAMEDLL

// This is the max # of split screen players using the same CVar on this system
#define MAX_SPLITSCREEN_CLIENT_BITS 2
// Maximum number of splitscreen clients shown on console, tools, etc.
#define MAX_SPLITSCREEN_CLIENTS	( 1 << MAX_SPLITSCREEN_CLIENT_BITS ) // 4

// Automatically set FCVAR_USERINFO whenever FCVAR_CLIENTSENT is set
#define FCVAR_CLIENTSENT		FCVAR_USERINFO

// This is a helper macro used to define constants with a specific server/client context.
// e.g. CVAR_MY_CVAR = CVAR_SERVER | CVAR_REPLICATED
// It's used in the definition of cvars that are used by both the server and client.
#define CVAR_SERVER (0)
#define CVAR_CLIENT (0)

// This is for ConVars that are created by the engine, but only the client uses them.
#define FCVAR_ACCESSIBLE_FROM_THREADS	(1<<18)	// used by VEngineCvar
#define FCVAR_PHYSICS				(1<<19) // Cvar only used by physics system

// A cvar that is part of the game state and saved in saved games
#define FCVAR_SAVED				(1<<22)

// A cvar that enables/disables profiling code
#define FCVAR_PROFILE			(1<<23)

// A cvar that is linked to another cvar
#define FCVAR_LINKED			(1<<24)

// A cvar that is used for material system
#define FCVAR_MATERIAL_SYSTEM	(1<<25)

// A cvar that is used for tool framework
#define FCVAR_TOOLFRAMEWORK		(1<<26)

// A cvar that is used for vgui panel
#define FCVAR_VGUIPANEL			(1<<27)

// This is a special flag used by the ConVar constructor to indicate that it is being constructed
// in order to look up an existing ConVar. This is needed to allow ConVars to be created with the
// same name as an existing ConVar, but with different flags or default value.
#define FCVAR_LOOKUP			(1<<31)


//-----------------------------------------------------------------------------
// Warnings
//-----------------------------------------------------------------------------
TIER0_DLL_EXPORT void Warning( const char *pFmt, ... ) FMTFUNCTION( 1, 2 );
TIER0_DLL_EXPORT void Warning_Continue( const char *pFmt, ... ) FMTFUNCTION( 1, 2 );


//-----------------------------------------------------------------------------
// Output functions
//-----------------------------------------------------------------------------
#ifndef Assert
#if defined( DBGFLAG_ASSERT )
	#define Assert( _exp )	\
		( (void)0 )
#else
	#define Assert( _exp )					\
		if ( !(_exp) )					\
		{								\
			/* TODO: Add assert code */	\
		}
#endif
#endif

#ifndef AssertMsg
#if defined( DBGFLAG_ASSERT )
	#define AssertMsg( _exp, _msg )	\
		( (void)0 )
#else
	#define AssertMsg( _exp, _msg )			\
		if ( !(_exp) )					\
		{								\
			/* TODO: Add assert code */	\
		}
#endif
#endif

#ifndef AssertFunc
#if defined( DBGFLAG_ASSERT )
	#define AssertFunc( _exp, _f ) \
		( (void)0 )
#else
	#define AssertFunc( _exp, _f )			\
		if ( !(_exp) )					\
		{								\
			/* TODO: Add assert code */	\
			_f;							\
		}
#endif
#endif

#ifndef AssertEquals
#if defined( DBGFLAG_ASSERT )
	#define AssertEquals( _exp, _expectedValue ) \
		( (void)0 )
#else
	#define AssertEquals( _exp, _expectedValue )	\
		if ( (_exp) != (_expectedValue) )		\
		{										\
			/* TODO: Add assert code */			\
		}
#endif
#endif

#ifndef AssertFloatEquals
#if defined( DBGFLAG_ASSERT )
	#define AssertFloatEquals( _exp, _expectedValue, _flTolerance ) \
		( (void)0 )
#else
	#define AssertFloatEquals( _exp, _expectedValue, _flTolerance )	\
		if ( fabsf((_exp) - (_expectedValue)) > (_flTolerance) )	\
		{															\
			/* TODO: Add assert code */								\
		}
#endif
#endif


//-----------------------------------------------------------------------------
// This is a DLL identifier for ConVars/ConCommands, used by the ConVar system
// to determine which DLL is creating a particular ConVar/ConCommand.
//-----------------------------------------------------------------------------
typedef int CVarDLLIdentifier_t;

// The default DLL identifier
#define DEFAULT_DLL_IDENTIFIER	-1

// This is used to identify ConVars/ConCommands created by the engine
#define ENGINE_DLL_IDENTIFIER	0

// This is used to identify ConVars/ConCommands created by the game DLL
#define GAMEDLL_IDENTIFIER		1

// This is used to identify ConVars/ConCommands created by the client DLL
#define CLIENTDLL_IDENTIFIER	2

// This is used to identify ConVars/ConCommands created by the material system DLL
#define MATERIALSYSTEM_DLL_IDENTIFIER	3

// This is used to identify ConVars/ConCommands created by the tools framework DLL
#define TOOLFRAMEWORK_DLL_IDENTIFIER	4

// This is used to identify ConVars/ConCommands created by the vgui panel DLL
#define VGUIPANEL_DLL_IDENTIFIER		5

// This is used to identify ConVars/ConCommands created by the physics DLL
#define PHYSICS_DLL_IDENTIFIER			6


//-----------------------------------------------------------------------------
// Purpose: Standard functions for handling assert failures
//-----------------------------------------------------------------------------
enum AssertRetval_t
{
	ASSERT_RETRY,
	ASSERT_IGNORE,
	ASSERT_BREAK,
	ASSERT_ABORT,
};

typedef AssertRetval_t (*AssertFailedNotifyFunc_t)( const char *pchFile, int nLine, const char *pchMessage );

// You can hook this to override the normal assert behavior
TIER0_DLL_EXPORT void SetAssertFailedNotifyFunc( AssertFailedNotifyFunc_t func );

// This is the default assert failed notify func
TIER0_DLL_EXPORT AssertRetval_t DefaultAssertFailedNotifyFunc( const char *pchFile, int nLine, const char *pchMessage );


//-----------------------------------------------------------------------------
// Spew functions
//-----------------------------------------------------------------------------
enum SpewRetval_t
{
	SPEW_CONTINUE,		// Continue processing the spew
	SPEW_ABORT,			// Abort the spew after this handler
	SPEW_DEBUGGER,		// Trigger a debugger break after this spew
};

// All spew output goes through this function
typedef SpewRetval_t (*SpewOutputFunc_t)( SpewType_t type, const char *pMsg );

// You can hook this to override the normal spew behavior
TIER0_DLL_EXPORT void AddSpewOutputFunc( SpewOutputFunc_t func, SpewType_t type );
TIER0_DLL_EXPORT void RemoveSpewOutputFunc( SpewOutputFunc_t func, SpewType_t type );

// This is the default spew output func (output to the console and debugger)
TIER0_DLL_EXPORT SpewRetval_t DefaultSpewFunc( SpewType_t type, char const *pMsg );

// Use this to spew information out to the console and debugger
TIER0_DLL_EXPORT void Msg( const char *pFmt, ... ) FMTFUNCTION( 1, 2 );
TIER0_DLL_EXPORT void DevMsg( int level, const char *pFmt, ... ) FMTFUNCTION( 2, 3 );
TIER0_DLL_EXPORT void DevWarning( int level, const char *pFmt, ... ) FMTFUNCTION( 2, 3 );
TIER0_DLL_EXPORT void DevMsg( const char *pFmt, ... ) FMTFUNCTION( 1, 2 );
TIER0_DLL_EXPORT void DevWarning( const char *pFmt, ... ) FMTFUNCTION( 1, 2 );

TIER0_DLL_EXPORT void ConMsg( const char *pFmt, ... ) FMTFUNCTION( 1, 2 );
TIER0_DLL_EXPORT void ConDMsg( const char *pFmt, ... ) FMTFUNCTION( 1, 2 );


//-----------------------------------------------------------------------------
// Macro to protect functions that are not thread safe
//-----------------------------------------------------------------------------
#define ThreadSafe static CThreadMutex __ts##__LINE__; CThreadSpinRWLock __tsrw##__LINE__(__ts##__LINE__); AutoLockT<CThreadSpinRWLock> __al##__LINE__(__tsrw##__LINE__, true);

#endif // DBG_H
