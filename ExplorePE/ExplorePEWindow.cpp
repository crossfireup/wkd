#include "stdafx.h"
#include "ExplorePEWindow.h"
#include "PEAnalyze.h"
#include "ProcessUtil.h"

ExplorePEWindow::ExplorePEWindow(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	createMenus();
}

BOOL ExplorePEWindow::chooseProcess()
{
	LPTSTR lpBuffer = NULL;
	DWORD pid;
	PVOID pImageBase;
	const size_t nMaxCount = 512 * sizeof(wchar_t);

	lpBuffer = (LPWSTR)::LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, nMaxCount);
	if (lpBuffer == NULL) {
		DisplayError(_T(__FUNCTION__)_T("LocalAlloc"));
		return FALSE;
	}
	::memset(static_cast<void *>(lpBuffer), '\0', nMaxCount);

#if 0
if (!GetDlgItemText(hDlg, IDC_EDIT_PROCNAME, lpBuffer, nMaxCount))
	{
		DisplayError(_T(__FUNCTION__)_T("GetDlgItemText"));
		::LocalFree(lpBuffer);
		return FALSE;
	}
#endif

	if (!GetPidByName(lpBuffer, &pid)) {
		::LocalFree(lpBuffer);
		return FALSE;
	}

	memset(lpBuffer, '\0', nMaxCount);
	if (!GetProcessImageName(pid, lpBuffer)) {
		::LocalFree(lpBuffer);
		return FALSE;
	}

	pImageBase = GetImageMapView(lpBuffer);

	if (!pImageBase) {
		::LocalFree(lpBuffer);
		return FALSE;
	}

	GetDosHeader(pImageBase);
	::LocalFree(lpBuffer);
	return TRUE;
}


void ExplorePEWindow::showAboutInfo()
{
	QMessageBox::about(this, tr("About"), tr("ExplorePE Version 0.1 @Microports"));
}

void ExplorePEWindow::createMenus()
{
	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(tr("&Open"), this, &ExplorePEWindow::chooseProcess, QKeySequence::Open);
	fileMenu->addAction(tr("Exit"), this, &ExplorePEWindow::close, QKeySequence::Close);

	QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(tr("A&bout"), this, &ExplorePEWindow::showAboutInfo);

}
