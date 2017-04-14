#ifndef _HOOKSERVICE_H
#define _HOOKSERVICE_H
#include "ServiceBase.h"

class HookService :	public ServiceBase
{
public:
	HookService(LPWSTR serviceName, BOOL fCanStop = TRUE, BOOL fCanShutdown = TRUE,
		BOOL fCanPauseContinue = FALSE);

	~HookService();
};

#endif /*_HOOKSERVICE_H */

