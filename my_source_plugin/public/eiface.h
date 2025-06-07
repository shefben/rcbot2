//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef EIFACE_H
#define EIFACE_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "tier1/bitbuf.h"

//-----------------------------------------------------------------------------
// Purpose: Interface from engine to server plugin.
//-----------------------------------------------------------------------------

// Used to return a value from a plugin function.
enum PLUGIN_RESULT
{
	PLUGIN_CONTINUE = 0, // keep going
	PLUGIN_OVERRIDE, // run the game dll function but use our return value instead
	PLUGIN_STOP, // don't run the game dll function at all
};

//
// Used by ISourceTVServer for client data.
//
struct ClientData_t
{
	unsigned int	m_UserID;			// Server user ID, unique within a server
	char			m_Name[32];			// Spectator UI name
	char			m_GUID[33];			// SteamID
	unsigned int	m_FriendsID;		// FriendsID
	char			m_FriendsName[32];	// Friends name
	bool			m_bFakePlayer;		// true, if player is a bot
	bool			m_bIsHLTV;			// true, if player is the HLTV proxy
	unsigned long	m_FilesDownloaded;	// number of files downloaded by this client
};

//
// Used by IEngineSound::PrecacheSound
//
struct SpatializationInfo_t
{
	// TODO: How do we want to handle this? Need to talk to Eric regarding what data is useful for clients and the engine to expose to the sound system.
};

//-----------------------------------------------------------------------------
// Purpose: This is the primary interface for server plugins.
//-----------------------------------------------------------------------------
class IServerPluginCallbacks
{
public:
	// Initialize the plugin to run
	// The plugin should save a pointer to the interface passed in, if it is going to need it.
	// This is called before any other functions are called.
	// Return false if there is an error during startup.
	virtual void			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory ) = 0;

	// Called when the plugin should be shutdown
	virtual void			Unload( void ) = 0;

	// called on level start
	virtual void			LevelInit( char const *pMapName ) = 0;

	// called on level start, when the server is ready to accept client connections
	//		edictCount is the number of entities in the level, clientMax is the max client count
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax ) = 0;

	// Called once per server frame, do recurring work here (like checking for timeouts)
	virtual void			GameFrame( bool simulating ) = 0;

	// Called when a level is shutdown (including changing levels)
	virtual void			LevelShutdown( void ) = 0;

	// Client is going active
	virtual void			ClientActive( edict_t *pEntity ) = 0;

	// Client is fully connected (has received initial baseline of entities)
	virtual void			ClientFullyConnect( edict_t *pEntity ) {};

	// Client is disconnecting from server
	virtual void			ClientDisconnect( edict_t *pEntity ) = 0;

	// Client is connected and has sent his info. The entity is valid at this point.
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername ) = 0;

	// Sets the client index for the client who typed the command string
	virtual void			SetCommandClient( int index ) = 0;

	// A player changed one/several replicated cvars (name etc)
	virtual void			ClientSettingsChanged( edict_t *pEdict ) = 0;

	// Client is connecting to server ( return PLUGIN_CONTINUE to allow, PLUGIN_REJECT to disallow connection )
	//	You can specify a rejection message by writing it into reject, max SPROP_REJECT_MESSAGE_LENGTH characters
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen ) = 0;

	// The client has issued a command (console, chat, voice command etc).
	// The game will eventually parse it and send it to the game server unit (g_GameDLL),
	// unless the plugin handles it here and returns PLUGIN_STOP.
	// The plugin can modify the command by returning PLUGIN_OVERRIDE and writing the new command string into args.
	// Otherwise, if the plugin is not interested in this command, it should return PLUGIN_CONTINUE.
	// Note: The command is not necessarily a "say" command, it can be any client command (e.g. "+jump", "use weapon_hegrenade").
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args ) = 0;

	// A user has been authenticated (might be a Steam account, might be LAN)
	// Return PLUGIN_CONTINUE if the plugin is not interested in this event,
	// otherwise return PLUGIN_REJECT to disallow the user from joining the server for some reason.
	// If you return PLUGIN_REJECT, you should fill in the reject string with the reason for rejection.
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID ) = 0;

	// This is called when a query from IServerPluginHelpers::StartQueryCvarValue is finished.
	// iCookie is the value returned by IServerPluginHelpers::StartQueryCvarValue.
	// pPlayerEntity is the player entity who initiated the query, or NULL if the query was initiated by the server.
	// eStatus is the result of the query.
	// pCvarName is the name of the cvar.
	// pCvarValue is the value of the cvar.
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue ) = 0;

	// This is called when an edict is allocated. The edict hasn't been initialized yet.
	// This is called before the edict is added to the edict list.
	virtual void			OnEdictAllocated( edict_t *edict ) = 0;

	// This is called when an edict is about to be freed. The edict is still valid.
	// This is called before the edict is removed from the edict list.
	virtual void			OnEdictFreed( edict_t *edict ) = 0;

	// Called when the server pauses
	virtual void			Pause( void ) = 0;

	// Called when the server unpauses
	virtual void			UnPause( void ) = 0;

	// Let the plugin know that a player is talk starting or stopping
	virtual void			ClientVoiceState( edict_t *pEntity, bool bSpeaking ) {};

	// This is called by the engine when it is about to create a game event.
	// Return false to prevent the event from being created.
	virtual bool			CanCreateGameEvent( const char *pEventName ) { return true; }

	// This is called by the engine after it has created a game event.
	// The plugin can modify the event data before it is sent to clients.
	virtual void			PostCreateGameEvent( IGameEvent *pEvent ) {}

	// This is called by the engine when it is about to fire a game event.
	// Return false to prevent the event from being fired.
	virtual bool			CanFireGameEvent( IGameEvent *pEvent ) { return true; }

	// This is called by the engine after it has fired a game event.
	virtual void			PostFireGameEvent( IGameEvent *pEvent ) {}

	// This is called by the engine when it is about to load a map.
	// The plugin can modify the map name.
	virtual void			PreLoadMap( const char *pMapName ) {}

	// This is called by the engine after it has loaded a map.
	virtual void			PostLoadMap( const char *pMapName ) {}

	// This is called by the engine when it is about to save a game.
	// The plugin can modify the save game name.
	virtual void			PreSaveGame( const char *pSaveName ) {}

	// This is called by the engine after it has saved a game.
	virtual void			PostSaveGame( const char *pSaveName ) {}

	// This is called by the engine when it is about to load a game.
	// The plugin can modify the load game name.
	virtual void			PreLoadGame( const char *pLoadName ) {}

	// This is called by the engine after it has loaded a game.
	virtual void			PostLoadGame( const char *pLoadName ) {}

	// This is called by the engine when it is about to change levels.
	// The plugin can modify the next level name.
	virtual void			PreChangeLevel( const char *pNextLevelName, const char *pLandmarkName ) {}

	// This is called by the engine after it has changed levels.
	virtual void			PostChangeLevel( const char *pNextLevelName, const char *pLandmarkName ) {}

	// This is called by the engine when it is about to disconnect a client.
	// The plugin can modify the disconnect reason.
	virtual void			PreClientDisconnect( edict_t *pEntity, const char *pReason ) {}

	// This is called by the engine after it has disconnected a client.
	virtual void			PostClientDisconnect( edict_t *pEntity, const char *pReason ) {}

	// This is called by the engine when it is about to kick a client.
	// The plugin can modify the kick reason.
	virtual void			PreClientKick( edict_t *pEntity, const char *pReason ) {}

	// This is called by the engine after it has kicked a client.
	virtual void			PostClientKick( edict_t *pEntity, const char *pReason ) {}

	// This is called by the engine when it is about to ban a client.
	// The plugin can modify the ban reason.
	virtual void			PreClientBan( edict_t *pEntity, int iMinutes, bool bBySteamID, const char *pReason ) {}

	// This is called by the engine after it has banned a client.
	virtual void			PostClientBan( edict_t *pEntity, int iMinutes, bool bBySteamID, const char *pReason ) {}

	// This is called by the engine when it is about to unban a client.
	virtual void			PreClientUnban( const char *pNetID, const char *pIPAddr ) {}

	// This is called by the engine after it has unbanned a client.
	virtual void			PostClientUnban( const char *pNetID, const char *pIPAddr ) {}

	// This is called by the engine when it is about to issue a server command.
	// The plugin can modify the command.
	virtual void			PreServerCommand( const char *pCommand ) {}

	// This is called by the engine after it has issued a server command.
	virtual void			PostServerCommand( const char *pCommand ) {}

	// This is called by the engine when it is about to issue a client command.
	// The plugin can modify the command.
	virtual void			PreClientCommand( edict_t *pEntity, const CCommand &args ) {}

	// This is called by the engine after it has issued a client command.
	virtual void			PostClientCommand( edict_t *pEntity, const CCommand &args ) {}

	// This is called by the engine when it is about to issue a cvar query.
	// The plugin can modify the cvar name.
	virtual void			PreQueryCvarValue( edict_t *pPlayerEntity, const char *pCvarName ) {}

	// This is called by the engine after it has issued a cvar query.
	virtual void			PostQueryCvarValue( edict_t *pPlayerEntity, const char *pCvarName, const char *pCvarValue ) {}

	// This is called by the engine when it is about to issue a cvar set.
	// The plugin can modify the cvar name and value.
	virtual void			PreSetCvarValue( const char *pCvarName, const char *pCvarValue ) {}

	// This is called by the engine after it has issued a cvar set.
	virtual void			PostSetCvarValue( const char *pCvarName, const char *pCvarValue ) {}

	// This is called by the engine when it is about to issue a cvar print.
	// The plugin can modify the cvar name.
	virtual void			PrePrintCvarValue( const char *pCvarName ) {}

	// This is called by the engine after it has issued a cvar print.
	virtual void			PostPrintCvarValue( const char *pCvarName, const char *pCvarValue ) {}

	// This is called by the engine when it is about to issue a cvar register.
	// The plugin can modify the cvar name and default value.
	virtual void			PreRegisterCvar( const char *pCvarName, const char *pDefaultValue, int iFlags ) {}

	// This is called by the engine after it has issued a cvar register.
	virtual void			PostRegisterCvar( const char *pCvarName, const char *pDefaultValue, int iFlags ) {}

	// This is called by the engine when it is about to issue a cvar unregister.
	// The plugin can modify the cvar name.
	virtual void			PreUnregisterCvar( const char *pCvarName ) {}

	// This is called by the engine after it has issued a cvar unregister.
	virtual void			PostUnregisterCvar( const char *pCvarName ) {}

	// This is called by the engine when it is about to issue a cvar command.
	// The plugin can modify the command.
	virtual void			PreCvarCommand( const CCommand &args ) {}

	// This is called by the engine after it has issued a cvar command.
	virtual void			PostCvarCommand( const CCommand &args ) {}

	// This is called by the engine when it is about to issue a command.
	// The plugin can modify the command.
	virtual void			PreCommand( const CCommand &args ) {}

	// This is called by the engine after it has issued a command.
	virtual void			PostCommand( const CCommand &args ) {}

	// This is called by the engine when it is about to issue a message.
	// The plugin can modify the message.
	virtual void			PreMessage( const netadr_t &addr, int type, const char *fmt, ... ) {}

	// This is called by the engine after it has issued a message.
	virtual void			PostMessage( const netadr_t &addr, int type, const char *fmt, ... ) {}

	// This is called by the engine when it is about to issue a voice message.
	// The plugin can modify the message.
	virtual void			PreVoiceMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine after it has issued a voice message.
	virtual void			PostVoiceMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine when it is about to issue a reliable message.
	// The plugin can modify the message.
	virtual void			PreReliableMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine after it has issued a reliable message.
	virtual void			PostReliableMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine when it is about to issue an unreliable message.
	// The plugin can modify the message.
	virtual void			PreUnreliableMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine after it has issued an unreliable message.
	virtual void			PostUnreliableMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine when it is about to issue a broadcast message.
	// The plugin can modify the message.
	virtual void			PreBroadcastMessage( int iChannel, const bf_read &data ) {}

	// This is called by the engine after it has issued a broadcast message.
	virtual void			PostBroadcastMessage( int iChannel, const bf_read &data ) {}

	// This is called by the engine when it is about to issue a sound.
	// The plugin can modify the sound.
	virtual void			PreSound( edict_t *pEntity, int iChannel, const char *pSample, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity ) {}

	// This is called by the engine after it has issued a sound.
	virtual void			PostSound( edict_t *pEntity, int iChannel, const char *pSample, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity ) {}

	// This is called by the engine when it is about to issue a spatialized sound.
	// The plugin can modify the sound.
	virtual void			PreSpatializedSound( edict_t *pEntity, int iChannel, const char *pSample, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity, SoundFlags_t soundflags, SpatializationInfo_t& info ) {}

	// This is called by the engine after it has issued a spatialized sound.
	virtual void			PostSpatializedSound( edict_t *pEntity, int iChannel, const char *pSample, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity, SoundFlags_t soundflags, SpatializationInfo_t& info ) {}

	// This is called by the engine when it is about to issue a sentence.
	// The plugin can modify the sentence.
	virtual void			PreSentence( edict_t *pEntity, int iChannel, const char *pSentence, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity ) {}

	// This is called by the engine after it has issued a sentence.
	virtual void			PostSentence( edict_t *pEntity, int iChannel, const char *pSentence, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity ) {}

	// This is called by the engine when it is about to issue a temp entity.
	// The plugin can modify the temp entity.
	virtual void			PreTempEntity( edict_t *pEntity, int iType, const Vector *pPos, const Vector *pAngles, int iMagnitude, int iPriority, int iFlags, const SendTable *pSendTable, const void *pData ) {}

	// This is called by the engine after it has issued a temp entity.
	virtual void			PostTempEntity( edict_t *pEntity, int iType, const Vector *pPos, const Vector *pAngles, int iMagnitude, int iPriority, int iFlags, const SendTable *pSendTable, const void *pData ) {}

	// This is called by the engine when it is about to issue a user message.
	// The plugin can modify the user message.
	virtual void			PreUserMessage( edict_t *pEntity, int iType, const ::google::protobuf::Message *pMsg, bool bReliable ) {}

	// This is called by the engine after it has issued a user message.
	virtual void			PostUserMessage( edict_t *pEntity, int iType, const ::google::protobuf::Message *pMsg, bool bReliable ) {}

	// This is called by the engine when it is about to issue a client print.
	// The plugin can modify the client print.
	virtual void			PreClientPrint( edict_t *pEntity, int iType, const char *pText ) {}

	// This is called by the engine after it has issued a client print.
	virtual void			PostClientPrint( edict_t *pEntity, int iType, const char *pText ) {}

	// This is called by the engine when it is about to issue a server print.
	// The plugin can modify the server print.
	virtual void			PreServerPrint( const char *pText ) {}

	// This is called by the engine after it has issued a server print.
	virtual void			PostServerPrint( const char *pText ) {}

	// This is called by the engine when it is about to issue a console print.
	// The plugin can modify the console print.
	virtual void			PreConsolePrint( const char *pText ) {}

	// This is called by the engine after it has issued a console print.
	virtual void			PostConsolePrint( const char *pText ) {}

	// This is called by the engine when it is about to issue a console command.
	// The plugin can modify the console command.
	virtual void			PreConsoleCommand( const char *pCommand ) {}

	// This is called by the engine after it has issued a console command.
	virtual void			PostConsoleCommand( const char *pCommand ) {}

	// This is called by the engine when it is about to issue a client command.
	// The plugin can modify the client command.
	virtual void			PreClientConsoleCommand( edict_t *pEntity, const char *pCommand ) {}

	// This is called by the engine after it has issued a client command.
	virtual void			PostClientConsoleCommand( edict_t *pEntity, const char *pCommand ) {}

	// This is called by the engine when it is about to issue a server command.
	// The plugin can modify the server command.
	virtual void			PreServerConsoleCommand( const char *pCommand ) {}

	// This is called by the engine after it has issued a server command.
	virtual void			PostServerConsoleCommand( const char *pCommand ) {}

	// This is called by the engine when it is about to issue a cvar command.
	// The plugin can modify the cvar command.
	virtual void			PreCvarConsoleCommand( const CCommand &args ) {}

	// This is called by the engine after it has issued a cvar command.
	virtual void			PostCvarConsoleCommand( const CCommand &args ) {}

	// This is called by the engine when it is about to issue a command.
	// The plugin can modify the command.
	virtual void			PreEngineCommand( const CCommand &args ) {}

	// This is called by the engine after it has issued a command.
	virtual void			PostEngineCommand( const CCommand &args ) {}

	// This is called by the engine when it is about to issue a message.
	// The plugin can modify the message.
	virtual void			PreEngineMessage( const netadr_t &addr, int type, const char *fmt, ... ) {}

	// This is called by the engine after it has issued a message.
	virtual void			PostEngineMessage( const netadr_t &addr, int type, const char *fmt, ... ) {}

	// This is called by the engine when it is about to issue a voice message.
	// The plugin can modify the message.
	virtual void			PreEngineVoiceMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine after it has issued a voice message.
	virtual void			PostEngineVoiceMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine when it is about to issue a reliable message.
	// The plugin can modify the message.
	virtual void			PreEngineReliableMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine after it has issued a reliable message.
	virtual void			PostEngineReliableMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine when it is about to issue an unreliable message.
	// The plugin can modify the message.
	virtual void			PreEngineUnreliableMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine after it has issued an unreliable message.
	virtual void			PostEngineUnreliableMessage( edict_t *pEntity, int iChannel, const bf_read &data ) {}

	// This is called by the engine when it is about to issue a broadcast message.
	// The plugin can modify the message.
	virtual void			PreEngineBroadcastMessage( int iChannel, const bf_read &data ) {}

	// This is called by the engine after it has issued a broadcast message.
	virtual void			PostEngineBroadcastMessage( int iChannel, const bf_read &data ) {}

	// This is called by the engine when it is about to issue a sound.
	// The plugin can modify the sound.
	virtual void			PreEngineSound( edict_t *pEntity, int iChannel, const char *pSample, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity ) {}

	// This is called by the engine after it has issued a sound.
	virtual void			PostEngineSound( edict_t *pEntity, int iChannel, const char *pSample, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity ) {}

	// This is called by the engine when it is about to issue a spatialized sound.
	// The plugin can modify the sound.
	virtual void			PreEngineSpatializedSound( edict_t *pEntity, int iChannel, const char *pSample, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity, SoundFlags_t soundflags, SpatializationInfo_t& info ) {}

	// This is called by the engine after it has issued a spatialized sound.
	virtual void			PostEngineSpatializedSound( edict_t *pEntity, int iChannel, const char *pSample, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity, SoundFlags_t soundflags, SpatializationInfo_t& info ) {}

	// This is called by the engine when it is about to issue a sentence.
	// The plugin can modify the sentence.
	virtual void			PreEngineSentence( edict_t *pEntity, int iChannel, const char *pSentence, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity ) {}

	// This is called by the engine after it has issued a sentence.
	virtual void			PostEngineSentence( edict_t *pEntity, int iChannel, const char *pSentence, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime, int speakerentity ) {}

	// This is called by the engine when it is about to issue a temp entity.
	// The plugin can modify the temp entity.
	virtual void			PreEngineTempEntity( edict_t *pEntity, int iType, const Vector *pPos, const Vector *pAngles, int iMagnitude, int iPriority, int iFlags, const SendTable *pSendTable, const void *pData ) {}

	// This is called by the engine after it has issued a temp entity.
	virtual void			PostEngineTempEntity( edict_t *pEntity, int iType, const Vector *pPos, const Vector *pAngles, int iMagnitude, int iPriority, int iFlags, const SendTable *pSendTable, const void *pData ) {}

	// This is called by the engine when it is about to issue a user message.
	// The plugin can modify the user message.
	virtual void			PreEngineUserMessage( edict_t *pEntity, int iType, const ::google::protobuf::Message *pMsg, bool bReliable ) {}

	// This is called by the engine after it has issued a user message.
	virtual void			PostEngineUserMessage( edict_t *pEntity, int iType, const ::google::protobuf::Message *pMsg, bool bReliable ) {}

	// This is called by the engine when it is about to issue a client print.
	// The plugin can modify the client print.
	virtual void			PreEngineClientPrint( edict_t *pEntity, int iType, const char *pText ) {}

	// This is called by the engine after it has issued a client print.
	virtual void			PostEngineClientPrint( edict_t *pEntity, int iType, const char *pText ) {}

	// This is called by the engine when it is about to issue a server print.
	// The plugin can modify the server print.
	virtual void			PreEngineServerPrint( const char *pText ) {}

	// This is called by the engine after it has issued a server print.
	virtual void			PostEngineServerPrint( const char *pText ) {}

	// This is called by the engine when it is about to issue a console print.
	// The plugin can modify the console print.
	virtual void			PreEngineConsolePrint( const char *pText ) {}

	// This is called by the engine after it has issued a console print.
	virtual void			PostEngineConsolePrint( const char *pText ) {}

	// This is called by the engine when it is about to issue a console command.
	// The plugin can modify the console command.
	virtual void			PreEngineConsoleCommand( const char *pCommand ) {}

	// This is called by the engine after it has issued a console command.
	virtual void			PostEngineConsoleCommand( const char *pCommand ) {}

	// This is called by the engine when it is about to issue a client command.
	// The plugin can modify the client command.
	virtual void			PreEngineClientConsoleCommand( edict_t *pEntity, const char *pCommand ) {}

	// This is called by the engine after it has issued a client command.
	virtual void			PostEngineClientConsoleCommand( edict_t *pEntity, const char *pCommand ) {}

	// This is called by the engine when it is about to issue a server command.
	// The plugin can modify the server command.
	virtual void			PreEngineServerConsoleCommand( const char *pCommand ) {}

	// This is called by the engine after it has issued a server command.
	virtual void			PostEngineServerConsoleCommand( const char *pCommand ) {}

	// This is called by the engine when it is about to issue a cvar command.
	// The plugin can modify the cvar command.
	virtual void			PreEngineCvarConsoleCommand( const CCommand &args ) {}

	// This is called by the engine after it has issued a cvar command.
	virtual void			PostEngineCvarConsoleCommand( const CCommand &args ) {}

	// This is called by the engine when it is about to issue a command.
	// The plugin can modify the command.
	virtual void			PreEngineEngineCommand( const CCommand &args ) {}

	// This is called by the engine after it has issued a command.
	virtual void			PostEngineEngineCommand( const CCommand &args ) {}
};

#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS_VERSION_1 "ISERVERPLUGINCALLBACKS001"
#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS_VERSION_2 "ISERVERPLUGINCALLBACKS002"
#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS "ISERVERPLUGINCALLBACKS003"

// Forward declarations for complex types
class IGameEvent;
class SendTable;
namespace google { namespace protobuf { class Message; } }

// Pointer to the engine interface.
class IVEngineServer;
class IServerPluginHelpers;
class IGameEventManager2;
class IPlayerInfoManager;
class IServerConfig;
class IServerPluginEntOutput;
class IServerPluginNetworking;
class IServerGameDLL;
class IServerGameClients;
class IServerGameEnts;
class IServerPlugin;
class IHLTVServer;
class IServerNetworkStringTableContainer;
class IFileSystem;
class IStaticPropMgrServer;
class IVoiceServer;
class ISoundEmitterSystemBase;
class INetworkStringTableContainer;
class IServerPluginTags;

extern IVEngineServer	*engine;	// global pointer to engine interface
extern IServerPluginHelpers *helpers; // global pointer to plugin helpers
extern IGameEventManager2 *gameeventmanager; // global pointer to game event manager
extern IPlayerInfoManager *playerinfomanager; // global pointer to player info manager
extern IServerConfig *serverconfig; // global pointer to server config interface
extern IServerPluginEntOutput *serverpluginentoutput; // global pointer to server plugin ent output interface
extern IServerPluginNetworking *g_pNetwork; // global pointer to server plugin networking interface
extern IServerGameDLL *servergamedll; // global pointer to server game dll interface
extern IServerGameClients *servergameclients; // global pointer to server game clients interface
extern IServerGameEnts *servergameents; // global pointer to server game ents interface
extern IHLTVServer *hltv; // global pointer to HLTV interface
extern IServerNetworkStringTableContainer *networkstringtable; // global pointer to network string table container
extern IFileSystem *filesystem; // global pointer to filesystem interface
extern IStaticPropMgrServer *staticpropmgr; // global pointer to static prop mgr interface
extern IVoiceServer *g_pVoiceServer; // global pointer to voice server interface
extern ISoundEmitterSystemBase *soundemitterbase; // global pointer to sound emitter system interface
extern INetworkStringTableContainer *stringtablecontainer; // global pointer to string table container
extern IServerPluginTags *serverplugintags; // global pointer to server plugin tags

// QueryCvarCookie_t is a cookie used to track asynchronous Cvar queries
typedef int QueryCvarCookie_t;
#define InvalidQueryCvarCookie -1

// Return values for Cvar query status
enum EQueryCvarValueStatus
{
	eQueryCvarValueStatus_ValueIntact = 0,	// The Cvar value was not trampled on while being queried
	eQueryCvarValueStatus_CvarNotFound = 1,
	eQueryCvarValueStatus_NotACvar = 2,		// The Cvar name specified is not a Cvar
	eQueryCvarValueStatus_CvarProtected = 3	// The Cvar was marked with FCVAR_SERVER_CAN_NOT_QUERY, so the value is not returned by the engine.
											// The value is returned as an empty string / 0 instead.
};

// edict_t is the server's representation of an entity (like a player or a prop).
// It is a C style struct, so it has no constructor or destructor.
// It is a fixed size, so it can be used in arrays.
// It is a POD type, so it can be copied with memcpy.
// It is defined in edict.h.
// typedef struct edict_s edict_t; // Now in basetypes.h

// CCommand is a class that represents a command issued by a client or the server.
// It is defined in const.h.
// class CCommand; // Now in basetypes.h (definition and forward declaration)

// Placeholder for SoundFlags_t
typedef int SoundFlags_t;

// EQueryCvarValueStatus is already defined below, ensure it's correct.
// enum EQueryCvarValueStatus ...

// INTERFACEVERSION_VENGINESERVER is the version of the IVEngineServer interface that this plugin is expecting.
// It is defined in const.h.
#define INTERFACEVERSION_VENGINESERVER "VEngineServer023"


#endif // EIFACE_H
