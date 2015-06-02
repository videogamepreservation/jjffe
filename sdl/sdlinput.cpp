#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ffeapi.h"
#include "ffecfg.h"

void VideoSwitchMode ();


static int active = 1;
static int ptrdisabled = 0;

static UCHAR pSDLKeys[512];

static int repeatrate = 30;
static int repeatdelay = 300;
static int sensitivity = 100;


int SDLMsgHandler (void)
{
	SDL_Event event;
	while (SDL_PollEvent (&event)) switch (event.type)
	{
		case SDL_ACTIVEEVENT:
			if (event.active.state & SDL_APPACTIVE)
			{
				if (event.active.gain == 1 && !active) active = 1;
				if (event.active.gain == 0 && active) active = 0;
			}
			break;

		case SDL_KEYDOWN:
			if (pSDLKeys[SDLK_LCTRL] || pSDLKeys[SDLK_RCTRL])
			{
				if (event.key.keysym.sym == SDLK_F12) {
					VideoSwitchMode ();
					break;
				}
			}
			if (event.key.keysym.sym > 512) break;
			pSDLKeys[event.key.keysym.sym] = 1;
			break;

		case SDL_KEYUP:
			if (event.key.keysym.sym > 512) break;
			pSDLKeys[event.key.keysym.sym] = 0;
			break;

		case SDL_QUIT:
			SystemCleanup ();
			exit (0);
	}

	return active;
}


extern "C" long InputMouseReadButtons (void)
{
	int but = SDL_GetMouseState (0, 0);
	return (but&1) | ((but>>1)&2);
}

extern "C" void InputMouseReadMickeys (long *pXMick, long *pYMick)
{
	SDL_GetRelativeMouseState ((int *)pXMick, (int *)pYMick);
	if (*pXMick == 0 && *pYMick == 0) return;

	if (!ptrdisabled && SDL_ShowCursor (SDL_QUERY) == SDL_DISABLE)
	{
		ptrdisabled = 1;
		*pXMick = *pYMick = 0;
	}
	if (ptrdisabled && SDL_ShowCursor (SDL_QUERY) == SDL_ENABLE)
	{
		ptrdisabled = 0;
		*pXMick = *pYMick = 0;
	}
	*pXMick = (*pXMick * sensitivity) / 100;
	*pYMick = (*pYMick * sensitivity) / 100;
}

extern "C" void InputMouseReadPos (long *pXPos, long *pYPos)
{
	SDL_GetMouseState ((int *)pXPos, (int *)pYPos);
	SDL_Surface *tsurf = SDL_GetVideoSurface ();
	*pXPos = *pXPos * 320 / tsurf->w;
	*pYPos = *pYPos * 200 / tsurf->h;
}


static int pKeybConvTable[256] = {
	0, SDLK_ESCAPE, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, 
	SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_MINUS, SDLK_EQUALS, SDLK_BACKSPACE, SDLK_TAB,
	SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u, SDLK_i,
	SDLK_o, SDLK_p, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET, SDLK_RETURN, SDLK_LCTRL, SDLK_a, SDLK_s,
	SDLK_d, SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k, SDLK_l, SDLK_SEMICOLON,
	SDLK_QUOTE, SDLK_HASH, SDLK_LSHIFT, SDLK_BACKSLASH, SDLK_z, SDLK_x, SDLK_c, SDLK_v,
	SDLK_b, SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH, SDLK_RSHIFT, SDLK_KP_MULTIPLY,
	SDLK_LALT, SDLK_SPACE, SDLK_CAPSLOCK, SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5,
	SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_NUMLOCK, SDLK_SCROLLOCK, SDLK_KP7,
	SDLK_KP8, SDLK_KP9, SDLK_KP_MINUS, SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP_PLUS, SDLK_KP1,
	SDLK_KP2, SDLK_KP3, SDLK_KP0, SDLK_KP_PERIOD, 0, 0, 0, SDLK_F11,
	SDLK_F12, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,			// 0x60-0x67
	0, 0, 0, 0, 0, 0, 0, 0,			// 0x68-0x6f
	0, 0, 0, 0, 0, 0, 0, 0,			// 0x70-0x77
	0, 0, 0, 0, 0, 0, 0, 0,			// 0x78-0x7f
	0, 0, 0, 0, 0, 0, 0, 0,			// 0x80-0x87
	0, 0, 0, 0, 0, 0, 0, 0,			// 0x88-0x8f
	0, 0, 0, 0, 0, 0, 0, 0,			// 0x90-0x97
	0, 0, 0, 0, SDLK_KP_ENTER, SDLK_RCTRL, 0, 0,			// 0x98-0x9f
	0, 0, 0, 0, 0, 0, 0, 0,			// 0xa0-0xa7
	0, 0, 0, 0, 0, 0, 0, 0,			// 0xa8-0xaf
	0, 0, 0, 0, 0, SDLK_KP_DIVIDE, 0, SDLK_SYSREQ,			// 0xb0-0xb7
	SDLK_RALT, 0, 0, 0, 0, 0, 0, 0,			// 0xb8-0xbf
	0, 0, 0, 0, 0, 0, 0, SDLK_HOME,			// 0xc0-0xc7
	SDLK_UP, SDLK_PAGEUP, 0, SDLK_LEFT, 0, SDLK_RIGHT, 0, SDLK_END,			// 0xc8-0xcf
	SDLK_DOWN, SDLK_PAGEDOWN, SDLK_INSERT, SDLK_DELETE, 0, 0, 0, 0,			// 0xd0-0xd7
	0, 0, 0, SDLK_LSUPER, SDLK_RSUPER, SDLK_MENU, 0, 0,			// 0xd8-0xdf
	0, 0, 0, 0, 0, 0, 0, 0,			// 0xe0-0xe7
	0, 0, 0, 0, 0, 0, 0, 0,			// 0xe8-0xef
	0, 0, 0, 0, 0, 0, 0, 0,			// 0xf0-0xf7
	0, 0, 0, 0, 0, 0, 0, 0,			// 0xf8-0xff
};

static UCHAR pKeybStates[256];
static int keybLastKey = 0xff;
static ULONG pKeybTimes[256];


void InputKeybReadStates (UCHAR *pKeys)
{
	int i;
	UCHAR *pCur;
	ULONG curtime;

	pCur = pSDLKeys;		//SDL_GetKeyState (&i);
	curtime = TimerGetTimeStamp ();

	for (i=0; i<256; i++)
	{
		if (pCur[pKeybConvTable[i]] == 1)
		{
			if (pKeybStates[i] == 0x80) {		// Initial press
				keybLastKey = i;
				pKeybStates[i] = 0;
				pKeybTimes[i] = curtime + repeatdelay;
			}
			else if (pKeybStates[i] == 0) {			// hold
				if (pKeybTimes[i] < curtime) {
					pKeybStates[i] = 0xff;
				}
			}
			if (pKeybStates[i] == 0xff) {		// Repeat
				if (pKeybTimes[i] < curtime) {
					keybLastKey = i;
					pKeybTimes[i] += repeatrate;
				}
			}
		}
		else pKeybStates[i] = 0x80;
	}
	for (i=0; i<256; i++) {
		if (pKeybStates[i] == 0x80) pKeys[i] = 0x80;
		else pKeys[i] = 0x0;
	}
}

extern "C" long InputKeybGetLastKey (void)
{
	return keybLastKey;
}

extern "C" void InputKeybSetLastKey (long key)
{
	keybLastKey = key;
}


extern "C" void InputJoyReadPos (long *xpos, long *ypos)
{
	*xpos = 0; *ypos = 0;
}

extern "C" long InputJoyReadButtons (void)
{
	return 0;
}


void InputInit ()
{
	CfgStruct cfg;

	CfgOpen (&cfg, __CONFIGFILE__);
	CfgFindSection (&cfg, "MOUSE");
	CfgGetKeyVal (&cfg, "sensitivity", &sensitivity);
	CfgFindSection (&cfg, "KEYBOARD");
	CfgGetKeyVal (&cfg, "repeatrate", &repeatrate);
	CfgGetKeyVal (&cfg, "repeatdelay", &repeatdelay);
	CfgClose (&cfg);

	memset (pSDLKeys, 0, 512);
	memset (pKeybStates, 0x80, 256);
	memset (pKeybTimes, 0, 256*4);
	keybLastKey = 0xff;
}

void InputCleanup ()
{

}