#include <windows.h>
#include <mmsystem.h>
#include <winbase.h>
#include <signal.h>
#include "win32api.h"
#include "ffeapi.h"
#include "ffecfg.h"

#include <time.h>

static int tmInit = 0;
static LARGE_INTEGER tmFrequency;


void TimerFrameUpdate (void)
{
	Win32MsgHandler();
	Win32StreamUpdate();
}

ULONG TimerGetTimeStamp (void)
{
	LARGE_INTEGER tempclock;
	QueryPerformanceCounter (&tempclock);
	return (ULONG)(tempclock.QuadPart / tmFrequency.QuadPart);
}

void TimerSleep (void)
{
	Sleep (1);
}

void TimerInit ()
{
	if (tmInit != 0) return;
	int rval = QueryPerformanceFrequency (&tmFrequency);
	if (rval != 0) {
		tmFrequency.QuadPart /= 1000;
		tmInit = 1;	return;
	}
	SystemCleanup();
	exit(0);
}

void TimerCleanup ()
{
	if (tmInit != 1) return;
	tmInit = 0;
}


void SystemInit (void)
{
	TimerInit();
	DirInit();
	VideoInit();		// Must be before sound/input because of window
	InputInit();
	SoundInit();
}

void SystemCleanup (void)
{
	SoundCleanup();
	InputCleanup();
	VideoCleanup();
	DirCleanup();
	TimerCleanup();
}
