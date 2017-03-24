#ifndef _HOOKEVENT_H
#define _HOOKEVENT_H

#include <ntddk.h>

#ifdef __cpluplus
extern "C" {
#endif

	DRIVER_DISPATCH RegisterEventBasedNotification;

	DRIVER_DISPATCH RegisterIrpBasedNotification;

#ifdef __cplusplus
}
#endif 

#endif /* _HOOKEVENT_H */