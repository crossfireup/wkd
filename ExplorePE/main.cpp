#include "stdafx.h"
#include "ExplorePEWindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	ExplorePEWindow w;
	w.show();

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
	return a.exec();
}
