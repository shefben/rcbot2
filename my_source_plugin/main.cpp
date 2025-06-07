//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "interface.h"
#include "eiface.h"
#include "tier1/convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Global Vars
//-----------------------------------------------------------------------------
IVEngineServer	*engine = NULL;

//-----------------------------------------------------------------------------
// Purpose: This is the plugin class.
//-----------------------------------------------------------------------------
class CEmptyPlugin : public IServerPluginCallbacks
{
public:
	CEmptyPlugin();
	~CEmptyPlugin();

	// IServerPluginCallbacks methods
	virtual void			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void );
	virtual void			UnPause( void );
	virtual const char     *GetPluginDescription( void );
	virtual void			LevelInit( char const *pMapName );
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	virtual void			GameFrame( bool simulating );
	virtual void			LevelShutdown( void );
	virtual void			ClientActive( edict_t *pEntity );
	virtual void			ClientDisconnect( edict_t *pEntity );
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername );
	virtual void			SetCommandClient( int index );
	virtual void			ClientSettingsChanged( edict_t *pEdict );
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args );
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );
	virtual void			OnEdictAllocated( edict_t *edict );
	virtual void			OnEdictFreed( edict_t *edict );
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEmptyPlugin::CEmptyPlugin()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CEmptyPlugin::~CEmptyPlugin()
{
}

//-----------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//-----------------------------------------------------------------------------
void CEmptyPlugin::Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	if (!engine)
	{
		Warning("Unable to load IVEngineServer interface!\n");
	}
	ConMsg("CEmptyPlugin::Load\n");
}

//-----------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded
//-----------------------------------------------------------------------------
void CEmptyPlugin::Unload( void )
{
	ConMsg("CEmptyPlugin::Unload\n");
}

//-----------------------------------------------------------------------------
// Purpose: called when the server pauses
//-----------------------------------------------------------------------------
void CEmptyPlugin::Pause( void )
{
	ConMsg("CEmptyPlugin::Pause\n");
}

//-----------------------------------------------------------------------------
// Purpose: called when the server unpauses
//-----------------------------------------------------------------------------
void CEmptyPlugin::UnPause( void )
{
	ConMsg("CEmptyPlugin::UnPause\n");
}

//-----------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in status results
//-----------------------------------------------------------------------------
const char *CEmptyPlugin::GetPluginDescription( void )
{
	return "Empty Plugin, 1.0";
}

//-----------------------------------------------------------------------------
// Purpose: called on level start
//-----------------------------------------------------------------------------
void CEmptyPlugin::LevelInit( char const *pMapName )
{
	ConMsg("CEmptyPlugin::LevelInit\n");
}

//-----------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//-----------------------------------------------------------------------------
void CEmptyPlugin::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	ConMsg("CEmptyPlugin::ServerActivate\n");
}

//-----------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//-----------------------------------------------------------------------------
void CEmptyPlugin::GameFrame( bool simulating )
{
}

//-----------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//-----------------------------------------------------------------------------
void CEmptyPlugin::LevelShutdown( void )
{
	ConMsg("CEmptyPlugin::LevelShutdown\n");
}

//-----------------------------------------------------------------------------
// Purpose: a client is fully connected (i.e. already received all baseline entities)
//-----------------------------------------------------------------------------
void CEmptyPlugin::ClientActive( edict_t *pEntity )
{
	ConMsg("CEmptyPlugin::ClientActive\n");
}

//-----------------------------------------------------------------------------
// Purpose: client disconnecting
//-----------------------------------------------------------------------------
void CEmptyPlugin::ClientDisconnect( edict_t *pEntity )
{
	ConMsg("CEmptyPlugin::ClientDisconnect\n");
}

//-----------------------------------------------------------------------------
// Purpose: client is connecting to server
//-----------------------------------------------------------------------------
void CEmptyPlugin::ClientPutInServer( edict_t *pEntity, char const *playername )
{
	ConMsg("CEmptyPlugin::ClientPutInServer\n");
}

//-----------------------------------------------------------------------------
// Purpose: client is typing a command (cl_something)
//-----------------------------------------------------------------------------
void CEmptyPlugin::SetCommandClient( int index )
{
}

//-----------------------------------------------------------------------------
// Purpose: called when a client changed their name/cl_cmdrate/cl_updaterate/etc
//-----------------------------------------------------------------------------
void CEmptyPlugin::ClientSettingsChanged( edict_t *pEdict )
{
	ConMsg("CEmptyPlugin::ClientSettingsChanged\n");
}

//-----------------------------------------------------------------------------
// Purpose: client is connecting to server
//-----------------------------------------------------------------------------
PLUGIN_RESULT CEmptyPlugin::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	ConMsg("CEmptyPlugin::ClientConnect\n");
	return PLUGIN_CONTINUE;
}

//-----------------------------------------------------------------------------
// Purpose: a command Issued by a client (say, chat commands)
//-----------------------------------------------------------------------------
PLUGIN_RESULT CEmptyPlugin::ClientCommand( edict_t *pEntity, const CCommand &args )
{
	return PLUGIN_CONTINUE;
}

//-----------------------------------------------------------------------------
// Purpose: a client is authenticated
//-----------------------------------------------------------------------------
PLUGIN_RESULT CEmptyPlugin::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
	return PLUGIN_CONTINUE;
}

//-----------------------------------------------------------------------------
// Purpose: Callback for network / CVar queries
//-----------------------------------------------------------------------------
void CEmptyPlugin::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
}

void CEmptyPlugin::OnEdictAllocated( edict_t *edict )
{
}

void CEmptyPlugin::OnEdictFreed( edict_t *edict )
{
}

//-----------------------------------------------------------------------------
// Purpose: Class exposure
//-----------------------------------------------------------------------------
CEmptyPlugin g_EmptyPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CEmptyPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_EmptyPlugin );
