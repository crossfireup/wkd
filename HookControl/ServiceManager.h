#ifndef _SERVICEINSTALLER_H
#define _SERVICEINSTALLER_H

#include "ServiceBase.h"

#define SERVICE_CTRL_INSTALL 0x01
#define SERVICE_CTRL_REMOVE  0X02

class ServiceManager {
public:
	ServiceManager(LPTSTR imgPath, LPTSTR serviceName);

	~ServiceManager();

	/* service control */
	int InstallService();
	int BeginService();
	int StopService();
	int RemoveService();
	int ServiceControl(int ctrlCode);

	/* service information handler */
	BOOL SetImagePath(LPCWSTR imgPath);
	BOOL SetServiceName(LPCWSTR serviceName);
	
	LPCWSTR GetImagePath() const;
	LPCWSTR GetServiceName() const;

	BOOL IsInitOk() const;

private:
	static SC_HANDLE hSCM_;
	LPTSTR serviceName_;
	LPTSTR imgPath_;
	BOOLEAN initOk_;

private:
	ServiceManager();
};


#endif /* _SERVICEINSTALLER_H */