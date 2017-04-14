#ifndef _HOOKCONTROL_H
#define _HOOKCONTROL_H

#include <windows.h>
#include "ServiceBase.h"
#include "ServiceManager.h"

class HookControl {
public:
	HookControl(ServiceBase& service, ServiceManager& serviceManager);

	~HookControl();

	BOOL InitDriver();

	BOOL DriverIoControl(int ctrlCode);

	BOOL GetDriverMsg();

private:
	ServiceManager& serviceManager_;
	HookService& service_;
	HANDLE hDevice_;
	REGISTER_EVENT registerEvt_;

	//HookControl();
}

#endif /* _HOOKCONTROL_H */