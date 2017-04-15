#include <assert.h>
#include <strsafe.h>
#include "ServiceBase.h"

#define SERVICE_CONTROL_HANDLER(startStatus, OnFunc, finishStatus, errStatus, errMsg, failMsg, ...)   \
	try {															\
			SetServiceStatus(startStatus);    						\
			(OnFunc)(__VA_ARGS__);     										\
			SetServiceStatus(finishStatus);    						\
				}																\
	catch (DWORD dwError) {											\
			WriteErrorLogEntry(errMsg, dwError);     				\
			SetServiceStatus(errStatus);    					 	\
				}																\
	catch (...) {													\
			WriteEventLogEntry(failMsg, EVENTLOG_ERROR_TYPE);  	   	\
			SetServiceStatus(errStatus);     						\
				}

ServiceBase *ServiceBase::service_ = NULL;

BOOL ServiceBase::Run(ServiceBase &service)
{
	service_ = &service;

	SERVICE_TABLE_ENTRY serviceTable[] = {
		{ service.name_, ServiceMain },
		{ NULL, NULL }
	};

	return StartServiceCtrlDispatcher(serviceTable);
}



void WINAPI ServiceBase::ServiceMain(DWORD argc, LPWSTR *argv)
{
	assert(service_ != NULL);

	service_->statusHandle_ = RegisterServiceCtrlHandler(service_->name_, ServiceCtrlHandler);

	if (service_->statusHandle_ == NULL){
		throw GetLastError();
	}

	service_->Start(argc, argv);

	return;
}
void WINAPI ServiceBase::ServiceCtrlHandler(DWORD ctrlCode)
{
	switch (ctrlCode){
	case SERVICE_CONTROL_STOP: service_->Stop(); break;
	case SERVICE_CONTROL_PAUSE: service_->Pause(); break;
	case SERVICE_CONTROL_CONTINUE: service_->Continue(); break;
	case SERVICE_CONTROL_SHUTDOWN: service_->Shutdown(); break;
	case SERVICE_CONTROL_INTERROGATE: break;
	default: break;
	}

	return;
}

ServiceBase::ServiceBase(PWSTR pszServiceName, BOOL fCanStop /*= TRUE*/,
	BOOL fCanShutdown /*= TRUE*/, BOOL fCanPauseContinue /*= FALSE*/)
{
	name_ = (pszServiceName == NULL) ? L"" : pszServiceName;
	statusHandle_ = NULL;
	status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status_.dwCurrentState = SERVICE_START_PENDING;

	DWORD dwControlsAccepted = 0;
	if (fCanStop)
		dwControlsAccepted |= SERVICE_ACCEPT_STOP;
	if (fCanShutdown)
		dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
	if (fCanPauseContinue)
		dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
	status_.dwControlsAccepted = dwControlsAccepted;

	status_.dwWin32ExitCode = NO_ERROR;
	status_.dwServiceSpecificExitCode = 0;
	status_.dwCheckPoint = 0;
	status_.dwWaitHint = 0;

	return;
}

ServiceBase::~ServiceBase()
{

}

void ServiceBase::Start(DWORD argc, LPWSTR *argv)
{
	SERVICE_CONTROL_HANDLER(SERVICE_START_PENDING, OnStart, SERVICE_RUNNING,
		SERVICE_STOPPED, L"Start service", L"Service start failed", argc, argv);

	return;
	/*try{
		SetServiceStatus();

		OnStart(argc, argv);

		SetServiceStatus(SERVICE_RUNNING);
		}
		catch (DWORD dwError){
		WriteErrorLogEntry(L"Start service", dwError);
		SetServiceStatus(SERVICE_STOPPED);
		}
		catch (...){
		WriteEventLogEntry(L"Service start failed", EVENTLOG_ERROR_TYPE);

		SetServiceStatus(SERVICE_STOPPED);
		}*/
}

void ServiceBase::OnStart(DWORD argc, PWSTR *argv)
{

}

void ServiceBase::Stop()
{

}

void ServiceBase::OnStop()
{
	DWORD dwOriginalState = status_.dwCurrentState;

	SERVICE_CONTROL_HANDLER(SERVICE_STOP_PENDING, OnStop, SERVICE_STOPPED,
		dwOriginalState, L"Service Stop", L"Service failed to stop.");

	return;
	/*try
	{
	SetServiceStatus(SERVICE_STOP_PENDING);

	OnStop();

	SetServiceStatus(SERVICE_STOPPED);
	}
	catch (DWORD dwError)
	{
	WriteErrorLogEntry(L"Service Stop", dwError);

	SetServiceStatus(dwOriginalState);
	}
	catch (...)
	{
	WriteEventLogEntry(L"Service failed to stop.", EVENTLOG_ERROR_TYPE);

	SetServiceStatus(dwOriginalState);
	}*/
}

void ServiceBase::Pause()
{
	SERVICE_CONTROL_HANDLER(SERVICE_PAUSE_PENDING, OnPause, SERVICE_PAUSED,
		SERVICE_RUNNING, L"Service Pause", L"Service failed to pause.");

	return;
	/*try
	{
		SetServiceStatus(SERVICE_PAUSE_PENDING);

		OnPause();

		SetServiceStatus(SERVICE_PAUSED);
	}
	catch (DWORD dwError)
	{
		WriteErrorLogEntry(L"Service Pause", dwError);

		SetServiceStatus(SERVICE_RUNNING);
	}
	catch (...)
	{
		WriteEventLogEntry(L"Service failed to pause.", EVENTLOG_ERROR_TYPE);

		SetServiceStatus(SERVICE_RUNNING);
	}*/
}

void ServiceBase::OnPause()
{

}

void ServiceBase::Continue()
{
	SERVICE_CONTROL_HANDLER(SERVICE_CONTINUE_PENDING, OnContinue, SERVICE_RUNNING, 
		SERVICE_PAUSED, L"Service Continue", L"Service failed to resume.");

	return;
}

void ServiceBase::OnContinue()
{

}


void ServiceBase::Shutdown()
{
	try
	{
		OnShutdown();

		SetServiceStatus(SERVICE_STOPPED);
	}
	catch (DWORD dwError)
	{
		WriteErrorLogEntry(L"Service Shutdown", dwError);
	}
	catch (...)
	{
		WriteEventLogEntry(L"Service failed to shut down.", EVENTLOG_ERROR_TYPE);
	}

	return;
}

void ServiceBase::OnShutdown()
{

}

void ServiceBase::SetServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode /*= NO_ERROR*/, DWORD dwWaitHint /*= 0*/)
{
	static DWORD dwCheckPoint = 1;
	status_.dwCurrentState = dwCurrentState;
	status_.dwWin32ExitCode = dwWin32ExitCode;
	status_.dwWaitHint = dwWaitHint;

	status_.dwCheckPoint = ((dwCurrentState == SERVICE_RUNNING) \
		|| (dwCurrentState == SERVICE_STOPPED)) ? 0 : dwCheckPoint++;

	::SetServiceStatus(statusHandle_, &status_);

	return;
}

void ServiceBase::WriteEventLogEntry(PWSTR pszMessage, WORD wType)
{
	HANDLE hEventSrc = NULL;
	LPWSTR lpszStrings[2] = { NULL, NULL };

	hEventSrc = RegisterEventSource(NULL, name_);
	if (hEventSrc){
		lpszStrings[0] = name_;
		lpszStrings[1] = pszMessage;

		ReportEvent(hEventSrc, wType, 0, 0, NULL, 2, 0, &lpszStrings[0], NULL);

		DeregisterEventSource(hEventSrc);
	}

	return;
}

void ServiceBase::WriteErrorLogEntry(PWSTR pszFunction, DWORD dwError /*= GetLastError()*/)
{
	wchar_t szMessages[512];
	
	StringCbPrintf(szMessages, ARRAYSIZE(szMessages), L"%s failed with errorCode: %08lx", pszFunction, dwError);
	WriteErrorLogEntry(szMessages, dwError);

	return;
}
