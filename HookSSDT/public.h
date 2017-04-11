#ifndef _PUBLIC_H
#define _PUBLIC_H

#include <devioctl.h>

#define IOCTL_REGISTER_EVENT \
   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS )

typedef enum {
	IRP_BASED,
	EVENT_BASED
} NOTIFY_TYPE;

typedef struct _REGISTER_EVENT
{
	NOTIFY_TYPE Type;
	HANDLE  hEvent;
	LARGE_INTEGER DueTime; // requested DueTime in 100-nanosecond units

} REGISTER_EVENT, *PREGISTER_EVENT;

#endif /* _PUBLIC_H */