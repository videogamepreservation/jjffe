#include <SDL_timer.h>
#include "ffeapi.h"
#include "ffecfg.h"

extern int SDLMsgHandler (void);

static ULONG tmStamp = 0;
static ULONG tmLastClock = 0;
static int tmInit = 0;


extern "C" void TimerFrameUpdate (void)
{
	TimerGetTimeStamp ();
	while (!SDLMsgHandler ()) SDL_Delay (100);
	tmLastClock = SDL_GetTicks ();
}

extern "C" ULONG TimerGetTimeStamp (void)
{
	ULONG curTime;

	curTime = SDL_GetTicks ();
	while (tmLastClock + 5 <= curTime)
	{
		tmStamp += 5;
		tmLastClock += 5;
	}

	return tmStamp;
}

void TimerSleep (void)
{
	SDL_Delay (1);
}

void TimerInit (void)
{
	tmLastClock = SDL_GetTicks ();
}

void TimerCleanup (void)
{
}

