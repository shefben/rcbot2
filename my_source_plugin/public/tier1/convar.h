//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef CONVAR_H
#define CONVAR_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "tier1/iconvar.h"
#include "tier1/utlstring.h"
#include "tier1/utlvector.h"
#include "tier1/utldict.h"
#include "color.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ConVar;
class CCommand;
class ConCommand;
class ConCommandBase;
struct characterset_t;


//-----------------------------------------------------------------------------
// Any executable that wants to use ConVars need to implement one of
// these to hook up access to console variables.
//-----------------------------------------------------------------------------
class IConCommandBaseAccessor
{
public:
	// Flags is a combination of FCVAR flags in cvar.h.
	// hOut is filled in with a handle to the variable.
	virtual bool RegisterConCommandBase( ConCommandBase *pVar ) = 0;
};


//-----------------------------------------------------------------------------
// Helper method for registering convars (handled by the ConVarSystem)
//-----------------------------------------------------------------------------
void ConVar_Register( int nCVarFlag = 0, IConCommandBaseAccessor *pAccessor = NULL );
void ConVar_Unregister( );


//-----------------------------------------------------------------------------
// Called when a ConVar changes value
// NOTE: For FCVAR_NEVER_AS_STRING ConVars, pOldValue == NULL
//-----------------------------------------------------------------------------
typedef void ( *FnChangeCallback_t )( IConVar *var, const char *pOldValue, float flOldValue );


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
	virtual ConCommandBase *GetCommand( void ) = 0;
	virtual const ConCommandBase *GetCommand( void ) const = 0;

	// Sets the help text for this convar
	virtual void SetHelpText( const char *pszHelpText ) = 0;

	// Adds specified flags.
	virtual void AddFlags( int flags ) = 0;

	// This either points to the ConVar or the ConCommand; not ideal, but backward compatible
	ConCommandBase			*m_pNext;
};


//-----------------------------------------------------------------------------
// Called by the framework to register ConCommands.
//-----------------------------------------------------------------------------
typedef void ( *FnCommandCallbackV1_t )( void );
typedef void ( *FnCommandCallback_t )( const CCommand &command );

#define COMMAND_COMPLETION_MAXITEMS		64
#define COMMAND_COMPLETION_ITEM_LENGTH	64

//-----------------------------------------------------------------------------
// Returns 0 to COMMAND_COMPLETION_MAXITEMS worth of completion strings
//-----------------------------------------------------------------------------
typedef int ( *FnCommandCompletionCallback )( const char *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] );


//-----------------------------------------------------------------------------
// Interface to ConCommands
//-----------------------------------------------------------------------------
class ICommand : public IConVar
{
public:
	// Call this to execute the concommand
	virtual void Dispatch( const CCommand &command ) = 0;

	// Returns the callback function
	virtual FnCommandCallback_t GetCallback() const = 0;

	// Returns the completion callback function
	virtual FnCommandCompletionCallback GetCompletionCallback() const = 0;

	// Returns true if the command can be auto-completed
	virtual bool CanAutoComplete( void ) = 0;
};


//-----------------------------------------------------------------------------
// Command tokenizer
//-----------------------------------------------------------------------------
class CCommand
{
public:
	CCommand();
	CCommand( int nArgC, const char **ppArgV );
	bool Tokenize( const char *pCommand, characterset_t *pBreakSet = NULL );
	void Reset();

	int ArgC() const;
	const char **ArgV() const;
	const char *ArgS() const;					// All args that occur after the 0th arg, in string form
	const char *GetCommandString() const;		// The entire command in string form, including the 0th arg
	const char *operator[]( int nIndex ) const;	// Gets at arguments
	const char *Arg( int nIndex ) const;		// Gets at arguments

	// Helper functions to parse arguments to commands.
	const char* FindArg( const char *pName ) const;
	int FindArgInt( const char *pName, int nDefaultVal ) const;

	static int MaxCommandLength();
	static characterset_t* DefaultBreakSet();

private:
	enum
	{
		COMMAND_MAX_ARGC = 64,
		COMMAND_MAX_LENGTH = 512,
	};

	int		m_nArgc;
	int		m_nArgv0Size;
	char	m_pArgSBuffer[ COMMAND_MAX_LENGTH ];
	char	m_pArgvBuffer[ COMMAND_MAX_LENGTH ];
	const char*	m_ppArgv[ COMMAND_MAX_ARGC ];
};

inline int CCommand::MaxCommandLength()
{
	return COMMAND_MAX_LENGTH - 1;
}

inline int CCommand::ArgC() const
{
	return m_nArgc;
}

inline const char **CCommand::ArgV() const
{
	return m_nArgc ? (const char**)m_ppArgv : NULL;
}

inline const char *CCommand::ArgS() const
{
	return m_nArgv0Size ? &m_pArgSBuffer[m_nArgv0Size] : "";
}

inline const char *CCommand::GetCommandString() const
{
	return m_nArgc ? m_pArgSBuffer : "";
}

inline const char *CCommand::Arg( int nIndex ) const
{
	// FIXME: Many command handlers appear to not be parsing numbers for arguments properly
	// So we're going to allow accessing out of bounds args, and having it return an empty string
	if ( nIndex < 0 || nIndex >= m_nArgc )
		return "";
	return m_ppArgv[nIndex];
}

inline const char *CCommand::operator[]( int nIndex ) const
{
	return Arg( nIndex );
}


//-----------------------------------------------------------------------------
// Purpose: The base console invoked command/cvar interface
//-----------------------------------------------------------------------------
class ConCommandBase
{
	friend void ConVar_Register( int nCVarFlag, IConCommandBaseAccessor *pAccessor );
	friend void ConVar_Unregister( );
	friend class CCvar;
	friend class ConVar;
	friend class ConCommand;

public:
								ConCommandBase( void );
								ConCommandBase( const char *pName, const char *pHelpString = 0,
									int flags = 0, FnCommandCallbackV1_t callback = 0 );
	virtual						~ConCommandBase( void );

	virtual	bool				IsCommand( void ) const;

	// Check flags
	bool						IsFlagSet( int flag ) const;
	// Set flags
	void						AddFlags( int flags );

	// Get name of the command/cvar
	const char					*GetName( void ) const;

	// Get help text for the command/cvar
	const char					*GetHelpText( void ) const;

	// Deal with next pointer; not ideal, but it's tricky to redo CCommandBase owing to backward compat
	ConCommandBase				*GetNext( void ) const;
	void						SetNext( ConCommandBase *next );

	bool						IsRegistered( void ) const;

	// Returns the DLL Cvar owns
	CVarDLLIdentifier_t			GetDLLIdentifier() const;

protected:
	virtual void				Create( const char *pName, const char *pHelpString = 0,
									int flags = 0, FnCommandCallbackV1_t callback = 0 ) = 0;

	// Used internally by OneTimeInit to initialize/shutdown
	virtual void				Init();
	void						Shutdown();

	// Internal copy routine
	void						Copy( const ConCommandBase &src );

	// Internal method to check if a ConCommandBase has already been registered
	static bool					IsCommandBaseRegistered( ConCommandBase *pVar );

	// Internal method to access the ConCommandBase list
	static ConCommandBase		*GetCommands( void );

	// Accessor for default meta data
	static IConCommandBaseAccessor	*GetAccessor();
	static void						SetAccessor( IConCommandBaseAccessor *pAccessor );

protected:
	// Next ConVar in chain
	// Prior to register, it points to the next convar in the DLL.
	// Once registered, though, it points to the next convar in the global list
	ConCommandBase				*m_pNext;

	// Has the cvar been registered yet?
	bool						m_bRegistered;

	// Static data
	const char					*m_pszName;
	const char					*m_pszHelpString;

	// ConVar flags
	int							m_nFlags;

protected:
	// ConVars add themselves to this list for the executable.
	// Then ConVar_Register runs through  all the console variables
	// and registers them.
	static ConCommandBase		*s_pConCommandBases;

	// ConVars in this executable use this 'global' accessor.
	static IConCommandBaseAccessor	*s_pAccessor;

	// Others DLLs Cvars list
	static ConCommandBase		*s_pConCommandBasesFromOtherDLLs;
};


//-----------------------------------------------------------------------------
// Purpose: The console invoked command
//-----------------------------------------------------------------------------
class ConCommand : public ConCommandBase
{
friend class CCvar;

public:
	typedef ConCommandBase BaseClass;

	ConCommand( const char *pName, FnCommandCallbackV1_t callback,
		const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommand( const char *pName, FnCommandCallback_t callback,
		const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommand( const char *pName, ICommand *pCommand,
		const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 );

	virtual ~ConCommand( void );

	virtual	bool				IsCommand( void ) const;

	virtual int					AutoCompleteSuggest( const char *partial, CUtlVector< CUtlString > &commands );

	virtual bool				CanAutoComplete( void );

	// Invoke the function
	virtual void				Dispatch( const CCommand &command );

	// Returns the callback function
	FnCommandCallback_t			GetCallback() const;

	// Returns the completion callback function
	FnCommandCompletionCallback GetCompletionCallback() const;

protected:
	// NOTE: To maintain backward compat, we have to be very careful with
	// the way this class is laid out. Certain data members came first in the V1 interface
	// and must remain first here.
	union
	{
		FnCommandCallbackV1_t m_fnCommandCallbackV1;
		FnCommandCallback_t m_fnCommandCallback;
		ICommand *m_pCommand;
	};

	FnCommandCompletionCallback	m_fnCompletionCallback;
	bool m_bHasCompletionCallback : 1;
	bool m_bUsingNewCommandCallback : 1;
	bool m_bUsingCommandCallbackInterface : 1;
};


//-----------------------------------------------------------------------------
// Purpose: A console variable
//-----------------------------------------------------------------------------
class ConVar : public ConCommandBase, public IConVar
{
friend class CCvar;
friend class ConVarRef;
friend class SplitScreenConVar;

public:
	typedef ConCommandBase BaseClass;

								ConVar( const char *pName, const char *pDefaultValue, int flags = 0 );

								ConVar( const char *pName, const char *pDefaultValue, int flags,
									const char *pHelpString );
								ConVar( const char *pName, const char *pDefaultValue, int flags,
									const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax );

								ConVar( const char *pName, const char *pDefaultValue, int flags,
									const char *pHelpString, FnChangeCallback_t callback );
								ConVar( const char *pName, const char *pDefaultValue, int flags,
									const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax,
									FnChangeCallback_t callback );

	virtual						~ConVar( void );

	// Implementation of IConVar interface
	virtual bool				IsCommand( void ) const;
	virtual bool				IsFlagSet( int flag ) const;
	virtual void				AddFlags( int flags );
	virtual const char			*GetName( void ) const;
	virtual const char			*GetBaseName( void ) const;
	virtual int					GetFlags( ) const;
	virtual const char			*GetHelpText( void ) const;
	virtual bool				IsRegistered( void ) const;
	virtual CVarDLLIdentifier_t	GetDLLIdentifier() const;

	// Retrieve value
	float						GetFloat( void ) const;
	int							GetInt( void ) const;
	Color						GetColor( void ) const;
	bool						GetBool( ) const { return !!GetInt(); }
	const char					*GetString( void ) const;

	// Any function that allocates a ConVar needs to inherit from IConVarAllocator
	class IConVarAllocator
	{
	public:
		virtual ~IConVarAllocator() {}
		virtual void *Allocate( size_t nBytes ) = 0;
		virtual void Free( void *pMem ) = 0;
	};

	// Method to allocate memory for ConVar
	static void SetAllocator( IConVarAllocator* pAllocator );
	// Method to deallocate memory for ConVar
	static void RemoveAllocator( IConVarAllocator* pAllocator );

	// Value functions
	void						SetValue( const char *value );
	void						SetValue( float value );
	void						SetValue( int value );
	void						SetValue( Color value );

	// Reset to default value
	void						Revert( void );

	// True if it has a min/max setting
	bool						HasMin() const;
	bool						HasMax() const;

	float						GetMin() const;
	float						GetMax() const;
	const char					*GetDefault( void ) const;

	// Returns the ConVar itself if it's a ConVar, or the ConCommand if it's a ConCommand
	virtual ConCommandBase *GetCommand( void );
	virtual const ConCommandBase *GetCommand( void ) const;

	// Sets the help text for this convar
	virtual void SetHelpText( const char *pszHelpText );

	// Called internally by OneTimeInit to initialize.
	virtual void				Init();

private:
	// This either points to the ConVar or the ConCommand; not ideal, but backward compatible
	// ConCommandBase			*m_pNext; // In ConCommandBase
	// bool					m_bRegistered; // In ConCommandBase
	// const char				*m_pszName; // In ConCommandBase
	// const char				*m_pszHelpString; // In ConCommandBase
	// int						m_nFlags; // In ConCommandBase

	// Values
	char						*m_pszString;
	int							m_StringLength;

	// Values
	float						m_fValue;
	int							m_nValue;

	// Min/max values
	bool						m_bHasMin;
	float						m_fMinVal;
	bool						m_bHasMax;
	float						m_fMaxVal;

	// Default value
	const char					*m_pszDefaultValue;

	// Call this function when ConVar changes
	FnChangeCallback_t			m_fnChangeCallback;

	// ConVar is a part of a chain accessed through the s_pConCommandBases pointer
	// ConVar					*m_pParent;	// In ConCommandBase
};


//-----------------------------------------------------------------------------
// Purpose: Used to link ConVars to controls
//-----------------------------------------------------------------------------
class ConVarRef
{
public:
	ConVarRef( const char *pName );
	ConVarRef( const char *pName, bool bIgnoreMissing );
	ConVarRef( IConVar *pConVar );

	void Init( const char *pName, bool bIgnoreMissing );
	bool IsValid() const;
	bool IsFlagSet( int nFlag ) const;
	IConVar *GetPtr();
	const IConVar *GetPtr() const;

	// Get/Set value
	float GetFloat( void ) const;
	int GetInt( void ) const;
	Color GetColor( void ) const;
	const char *GetString( void ) const;
	bool GetBool() const { return !!GetInt(); }

	void SetValue( const char *pValue );
	void SetValue( float flValue );
	void SetValue( int nValue );
	void SetValue( Color clrValue );
	void SetValue( bool bValue );

	const char *GetName( void ) const;
	const char *GetDefault( void ) const;
	const char *GetHelpText( void ) const;

	// Add flags. Not that FCVAR_NEVER_AS_STRING will prevent the
	//  string version of the CVar from ever being set directly.
	//  FCVAR_SPONLY and FCVAR_GAMEDLL can only be set from the same DLL.
	void AddFlags( int nFlags );

private:
	// Used by ConVarRefPanel
	friend class ConVarRefPanel;
	ConVarRef() : m_pConVar(0), m_pszName(0), m_bIgnoreMissing(false) {}

	IConVar *m_pConVar;
	const char *m_pszName;
	bool m_bIgnoreMissing;
};


//-----------------------------------------------------------------------------
// Helper macro to declare a ConVar
//-----------------------------------------------------------------------------
#ifdef USE_TIER1_DLL_FOR_CONVARS
#define CON_COMMAND( name, description ) \
	static void name( const CCommand &args ); \
	static ConCommand name##_command( #name, name, description ); \
	static void name( const CCommand &args )
#define CON_COMMAND_F( name, description, flags ) \
	static void name( const CCommand &args ); \
	static ConCommand name##_command( #name, name, description, flags ); \
	static void name( const CCommand &args )
#define CON_COMMAND_F_COMPLETION( name, description, flags, completion ) \
	static void name( const CCommand &args ); \
	static ConCommand name##_command( #name, name, description, flags, completion ); \
	static void name( const CCommand &args )
#else
#define CON_COMMAND( name, description ) \
	void name( const CCommand &args ); \
	ConCommand name##_command( #name, name, description ); \
	void name( const CCommand &args )
#define CON_COMMAND_F( name, description, flags ) \
	void name( const CCommand &args ); \
	ConCommand name##_command( #name, name, description, flags ); \
	void name( const CCommand &args )
#define CON_COMMAND_F_COMPLETION( name, description, flags, completion ) \
	void name( const CCommand &args ); \
	ConCommand name##_command( #name, name, description, flags, completion ); \
	void name( const CCommand &args )
#endif

// Auto-registers the ConVar
#define ConVar_AutoRegister( pConVar ) \
	static ConVarRegister s_VarRegister_##pConVar( #pConVar, pConVar )

//-----------------------------------------------------------------------------
// Purpose: Utility class to handle ConVar creation
//-----------------------------------------------------------------------------
class ConVarRegister
{
public:
	ConVarRegister( const char *pszName, ConVar *pCVar, int nFlags = 0, IConCommandBaseAccessor *pAccessor = NULL );
	~ConVarRegister( void );

private:
	ConVar *m_pCVar;
	int m_nFlags;
};


// Inlines.
//
// Ideally, these would be in a ConVar.inl, but that adds total link time, so much so that it's not worth it.
//

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float
//-----------------------------------------------------------------------------
inline float ConVar::GetFloat( void ) const
{
	return m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an int
//-----------------------------------------------------------------------------
inline int ConVar::GetInt( void ) const
{
	return m_nValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color
//-----------------------------------------------------------------------------
inline Color ConVar::GetColor( void ) const
{
	return Color( (int)( m_fValue * 255.0f ), (int)( m_fValue * 255.0f ), (int)( m_fValue * 255.0f ), 255 );
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string, return "" (empty string) if not set
//-----------------------------------------------------------------------------
inline const char *ConVar::GetString( void ) const
{
	if ( m_nFlags & FCVAR_NEVER_AS_STRING )
		return "FCVAR_NEVER_AS_STRING";
	return ( m_pszString ) ? m_pszString : "";
}


//-----------------------------------------------------------------------------
// Purpose: Returns the help text for this convar
//-----------------------------------------------------------------------------
inline const char *ConVar::GetHelpText( void ) const
{
	return m_pszHelpString;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the help text for this convar
//-----------------------------------------------------------------------------
inline void ConVar::SetHelpText( const char *pszHelpText )
{
	m_pszHelpString = pszHelpText;
}


//-----------------------------------------------------------------------------
// Purpose: Check if ConVar has a minimum value
//-----------------------------------------------------------------------------
inline bool ConVar::HasMin() const
{
	return m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: Check if ConVar has a maximum value
//-----------------------------------------------------------------------------
inline bool ConVar::HasMax() const
{
	return m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: Get minimum value of ConVar
//-----------------------------------------------------------------------------
inline float ConVar::GetMin() const
{
	return m_fMinVal;
}

//-----------------------------------------------------------------------------
// Purpose: Get maximum value of ConVar
//-----------------------------------------------------------------------------
inline float ConVar::GetMax() const
{
	return m_fMaxVal;
}

//-----------------------------------------------------------------------------
// Purpose: Get default value of ConVar
//-----------------------------------------------------------------------------
inline const char *ConVar::GetDefault( void ) const
{
	return m_pszDefaultValue;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the name of the ConVar
//-----------------------------------------------------------------------------
inline const char *ConVar::GetName( void ) const
{
	return m_pszName;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the base name of the ConVar
//-----------------------------------------------------------------------------
inline const char *ConVar::GetBaseName( void ) const
{
	return m_pszName;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the ConVar flags
//-----------------------------------------------------------------------------
inline int ConVar::GetFlags( void ) const
{
	return m_nFlags;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the ConVar itself if it's a ConVar, or the ConCommand if it's a ConCommand
//-----------------------------------------------------------------------------
inline ConCommandBase *ConVar::GetCommand( void )
{
	return this;
}

inline const ConCommandBase *ConVar::GetCommand( void ) const
{
	return this;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the ConVar is a command
//-----------------------------------------------------------------------------
inline bool ConVar::IsCommand( void ) const
{
	// If it's a ConVar, it can't be a command
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the ConVar is registered
//-----------------------------------------------------------------------------
inline bool ConVar::IsRegistered( void ) const
{
	return m_bRegistered;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the DLL Cvar owns
//-----------------------------------------------------------------------------
inline CVarDLLIdentifier_t ConVar::GetDLLIdentifier() const
{
	// FIXME: How do I get this?
	return അന്വേഷിക്കുകDLLIdentifier_t();
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the ConVar is flagged with nFlag
//-----------------------------------------------------------------------------
inline bool ConVar::IsFlagSet( int flag ) const
{
	return ( m_nFlags & flag ) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Adds specified flags.
//-----------------------------------------------------------------------------
inline void ConVar::AddFlags( int flags )
{
	m_nFlags |= flags;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the callback function
//-----------------------------------------------------------------------------
inline FnCommandCallback_t ConCommand::GetCallback() const
{
	return m_bUsingNewCommandCallback ? m_fnCommandCallback : NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the completion callback function
//-----------------------------------------------------------------------------
inline FnCommandCompletionCallback ConCommand::GetCompletionCallback() const
{
	return m_bHasCompletionCallback ? m_fnCompletionCallback : NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the command can be auto-completed
//-----------------------------------------------------------------------------
inline bool ConCommand::CanAutoComplete( void )
{
	return m_bHasCompletionCallback;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the command is a command
//-----------------------------------------------------------------------------
inline bool ConCommand::IsCommand( void ) const
{
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the name of the ConCommand
//-----------------------------------------------------------------------------
inline const char *ConCommandBase::GetName( void ) const
{
	return m_pszName;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the help text for this convar
//-----------------------------------------------------------------------------
inline const char *ConCommandBase::GetHelpText( void ) const
{
	return m_pszHelpString;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the next ConVar in the chain
//-----------------------------------------------------------------------------
inline ConCommandBase *ConCommandBase::GetNext( void ) const
{
	return m_pNext;
}

inline void ConCommandBase::SetNext( ConCommandBase *next )
{
	m_pNext = next;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the ConVar is registered
//-----------------------------------------------------------------------------
inline bool ConCommandBase::IsRegistered( void ) const
{
	return m_bRegistered;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the ConVar is flagged with nFlag
//-----------------------------------------------------------------------------
inline bool ConCommandBase::IsFlagSet( int flag ) const
{
	return ( m_nFlags & flag ) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Adds specified flags.
//-----------------------------------------------------------------------------
inline void ConCommandBase::AddFlags( int flags )
{
	m_nFlags |= flags;
}


//-----------------------------------------------------------------------------
// Returns the DLL Cvar owns
//-----------------------------------------------------------------------------
inline CVarDLLIdentifier_t ConCommandBase::GetDLLIdentifier() const
{
	// FIXME: How do I get this?
	return അന്വേഷിക്കുകDLLIdentifier_t();
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
inline ConVarRef::ConVarRef( const char *pName )
{
	Init( pName, false );
}

inline ConVarRef::ConVarRef( const char *pName, bool bIgnoreMissing )
{
	Init( pName, bIgnoreMissing );
}

inline ConVarRef::ConVarRef( IConVar *pConVar )
 : m_pConVar(pConVar), m_pszName(pConVar->GetName()), m_bIgnoreMissing(false)
{
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the ConVar is valid
//-----------------------------------------------------------------------------
inline bool ConVarRef::IsValid() const
{
	return m_pConVar != NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the ConVar is flagged with nFlag
//-----------------------------------------------------------------------------
inline bool ConVarRef::IsFlagSet( int nFlag ) const
{
	return m_pConVar->IsFlagSet( nFlag );
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the ConVar
//-----------------------------------------------------------------------------
inline IConVar *ConVarRef::GetPtr()
{
	return m_pConVar;
}

inline const IConVar *ConVarRef::GetPtr() const
{
	return m_pConVar;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the name of the ConVar
//-----------------------------------------------------------------------------
inline const char *ConVarRef::GetName( void ) const
{
	return m_pszName;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the default value of the ConVar
//-----------------------------------------------------------------------------
inline const char *ConVarRef::GetDefault( void ) const
{
	return m_pConVar->GetDefault();
}


//-----------------------------------------------------------------------------
// Purpose: Returns the help text for this convar
//-----------------------------------------------------------------------------
inline const char *ConVarRef::GetHelpText( void ) const
{
	return m_pConVar->GetHelpText();
}


//-----------------------------------------------------------------------------
// Purpose: Returns the float value of the ConVar
//-----------------------------------------------------------------------------
inline float ConVarRef::GetFloat( void ) const
{
	return m_pConVar->GetFloat();
}


//-----------------------------------------------------------------------------
// Purpose: Returns the int value of the ConVar
//-----------------------------------------------------------------------------
inline int ConVarRef::GetInt( void ) const
{
	return m_pConVar->GetInt();
}


//-----------------------------------------------------------------------------
// Purpose: Returns the color value of the ConVar
//-----------------------------------------------------------------------------
inline Color ConVarRef::GetColor( void ) const
{
	return m_pConVar->GetColor();
}


//-----------------------------------------------------------------------------
// Purpose: Returns the string value of the ConVar
//-----------------------------------------------------------------------------
inline const char *ConVarRef::GetString( void ) const
{
	return m_pConVar->GetString();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the value of the ConVar
//-----------------------------------------------------------------------------
inline void ConVarRef::SetValue( const char *pValue )
{
	m_pConVar->SetValue( pValue );
}

inline void ConVarRef::SetValue( float flValue )
{
	m_pConVar->SetValue( flValue );
}

inline void ConVarRef::SetValue( int nValue )
{
	m_pConVar->SetValue( nValue );
}

inline void ConVarRef::SetValue( Color clrValue )
{
	m_pConVar->SetValue( clrValue );
}

inline void ConVarRef::SetValue( bool bValue )
{
	m_pConVar->SetValue( bValue ? 1 : 0 );
}


//-----------------------------------------------------------------------------
// Purpose: Adds specified flags.
//-----------------------------------------------------------------------------
inline void ConVarRef::AddFlags( int nFlags )
{
	m_pConVar->AddFlags( nFlags );
}

#endif // CONVAR_H
