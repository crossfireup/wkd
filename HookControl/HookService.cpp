#include "HookService.h"

HookService::HookService(LPWSTR serviceName, BOOL fCanStop /*= TRUE*/, 
	BOOL fCanShutdown /*= TRUE*/, BOOL fCanPauseContinue /*= FALSE*/) :
	ServiceBase(serviceName, fCanStop, fCanShutdown, fCanPauseContinue)
{

}

HookService::~HookService()
{

}
