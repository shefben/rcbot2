//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef ICONVAR_H
#define ICONVAR_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IConVar;
class CCommand;


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


//-----------------------------------------------------------------------------
// Called when a ConVar changes value
// NOTE: For FCVAR_NEVER_AS_STRING ConVars, pOldValue == NULL
//-----------------------------------------------------------------------------
typedef void ( *FnChangeCallback )( IConVar *var, const char *pOldValue, float flOldValue );


//-----------------------------------------------------------------------------
// Abstract interface for shared ConVar code
//-----------------------------------------------------------------------------
class IConVar
{
public:
	// Value set
	virtual void SetValue( const char *pValue ) = 0;
	virtual void SetValue( float flValue ) = 0;
	virtual void SetValue( int nValue ) = 0;

	// Return name of cvar
	virtual const char *GetName( void ) const = 0;

	// Return name of command (if it's a ConCommand)
	virtual const char *GetBaseName( void ) const = 0;

	// Accessors.. not as efficient as using the ConVar directly
	virtual bool IsFlagSet( int nFlag ) const = 0;

	// Gets the ConVar flags
	virtual int GetFlags() const = 0;

	// Checks if the ConVar is a command
	virtual bool IsCommand( void ) const = 0;

	// Checks if the ConVar is a ConVar
	virtual bool IsConVar( void ) const = 0;

	// Gets the integer value
	virtual int GetInt( void ) const = 0;

	// Gets the float value
	virtual float GetFloat( void ) const = 0;

	// Gets the string value
	virtual const char *GetString( void ) const = 0;

	// Get the max value.
	virtual float GetMax( void ) const = 0;

	// Get the min value.
	virtual float GetMin( void ) const = 0;

	// Get the default value.
	virtual const char *GetDefault( void ) const = 0;

	// Returns the help text for this convar
	virtual const char *GetHelpText( void ) const = 0;

	// For ConVars, this is the ConVar itself.
	// For ConCommands, this is the ConCommand itself.
	virtual class ConCommandBase *GetCommand( void ) = 0;
	virtual const class ConCommandBase *GetCommand( void ) const = 0;

	// Sets the help text for this convar
	virtual void SetHelpText( const char *pszHelpText ) = 0;

	// Adds specified flags.
	virtual void AddFlags( int flags ) = 0;

	// This either points to the ConVar or the ConCommand; not ideal, but backward compatible
	class ConCommandBase *m_pNext;
};


#endif // ICONVAR_H
