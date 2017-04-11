#ifndef _PEANALYZE_H
#define _PEANALYZE_H

#ifdef __cplusplus
extern "C" {
#endif

	BOOL GetDosHeader(PVOID pvFileBase);
	PVOID GetImageMapView(LPCTSTR szFileName);


#ifdef __cplusplus
}
#endif

#endif /* _PEANALYZE_H */