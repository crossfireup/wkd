#ifndef _SERVICEBASE_H
#define _SERVICEBASE_H

#include <Windows.h>
#include <winsvc.h>

class ServiceBase {
public:
	BOOL Run(ServiceBase &service);

	ServiceBase(PWSTR pszServiceName, BOOL fCanStop = TRUE,	BOOL fCanShutdown = TRUE,
		BOOL fCanPauseContinue = FALSE);

	virtual ~ServiceBase();

	void Stop();

protected:
	virtual void OnStart(DWORD argc, PWSTR *argv);

	virtual void OnStop();

	virtual void OnPause();

	virtual void OnContinue();

	virtual void OnShutdown();

	void SetServiceStatus(DWORD dwCurrentState,	DWORD dwWin32ExitCode = NO_ERROR,
		DWORD dwWaitHint = 0);

	void WriteEventLogEntry(PWSTR pszMessage, WORD wType);

	void WriteErrorLogEntry(PWSTR pszFunction,
		DWORD dwError = GetLastError());

private:
	PWSTR name_;
	SERVICE_STATUS status_;
	SERVICE_STATUS_HANDLE statusHandle_;

	static ServiceBase *service_;

private:
	static void WINAPI ServiceMain(DWORD argc, LPWSTR *argv);
	static void WINAPI ServiceCtrlHandler(DWORD ctrlCode);

	void Start(DWORD argc, LPWSTR *argv);

	void Pause();

	void Continue();

	void Shutdown();

private:
	ServiceBase();
	ServiceBase(const ServiceBase &);
	ServiceBase& operator=(const ServiceBase &);

};


#endif /* _SERVICEBASE_H */