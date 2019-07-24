#ifndef __BOT_SIGSCAN_H__
#define __BOT_SIGSCAN_H__

#include "bot_const.h"

struct DynLibInfo
{
	void *baseAddress;
	size_t memorySize;
};

class CRCBotKeyValueList;

class CSignatureFunction
{
public:
	CSignatureFunction() { m_func = 0x0; }
private:
	size_t decodeHexString(unsigned char *buffer, size_t maxlength, const char *hexstr);

	bool getLibraryInfo(const void *libPtr, DynLibInfo &lib);

	void *findPattern(const void *libPtr, const char *pattern, size_t len);

	void *findSignature ( void *addrInBase, const char *signature );
protected:
	void findFunc ( CRCBotKeyValueList *kv, const char *pKey, void *pAddrBase, const char *defaultsig );

	void *m_func;
};

class CGameRulesObject : public CSignatureFunction
{
public:
	CGameRulesObject(CRCBotKeyValueList *list, void *pAddrBase);

	bool found() { return m_func != NULL; }

	void **getGameRules() { return reinterpret_cast<void **>(m_func); }
};

class CCreateGameRulesObject : public CSignatureFunction
{
public:
	CCreateGameRulesObject(CRCBotKeyValueList *list, void *pAddrBase);

	bool found() { return m_func != NULL; }

	void **getGameRules();
};

/*
CEconItemAttribute *UTIL_AttributeList_GetAttributeByID ( CAttributeList *list, int id )
{
	void *pret = NULL;

	if ( list && AttributeList_GetAttributeByID )
	{
#ifdef _WIN32
		__asm
	   {
		  mov ecx, list;
		  push id;
		  call AttributeList_GetAttributeByID;
		  mov pret, eax;
	   };
#else
	   FUNC_ATTRIBLIST_GET_ATTRIB_BY_ID func = (FUNC_ATTRIBLIST_GET_ATTRIB_BY_ID)AttributeList_GetAttributeByID;

	   pret = (void*)func(list,id);
#endif
	}

	return (CEconItemAttribute*)pret;
}
*/

extern CGameRulesObject *g_pGameRules_Obj;
extern CCreateGameRulesObject *g_pGameRules_Create_Obj;

void *GetGameRules();
#endif