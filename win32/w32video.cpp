#include <windows.h>
#include <ddraw.h>
#include <stdio.h>

#include "ffeapi.h"
#include "ffecfg.h"
#include "win32api.h"

static int fullscreen = 0;

static int fswidth = 320;
static int fsheight = 200;
static int fsbpp = 16;
static int fssoftscale = 0;
static int fstriplebuf = 0;
static int fsletterbox = 1;

static int winwidth = 640;
static int winheight = 400;
static int winsoftscale = 0;
static int winintermediatebuf = 1;

static int nodraw = 0;
static int vidInit = 0;
static int active = 1;
static int ptrEnabled = 1;
static int error = 0;
static int exclusive = 0;
static int bpp = 16;
static int softscale = 0;

static int wndwidth;		// window width/height including borders
static int wndheight;
static int curwidth;		// width/height of front surface
static int curheight;

static RECT clrect;
static RECT wndrect;
static POINT cursorpos;

static HWND hWnd = NULL;
static HINSTANCE hInst = NULL;

static UCHAR pBackBuf[320*200];
static LPDIRECTDRAW pDDraw = NULL;
static LPDIRECTDRAWSURFACE pDDSurf = NULL;		// writing surface
static LPDIRECTDRAWSURFACE pDDSurf2 = NULL;		// front surface
static LPDIRECTDRAWSURFACE pDDSurf3 = NULL;		// intermediate surface
static LPDIRECTDRAWPALETTE pDDPal = NULL;
static LPDIRECTDRAWCLIPPER pDDClip = NULL;
static ULONG pWinPal16[256];
static ULONG pWinPal15[256];
static ULONG pWinPal32[256];

static FILE *pLog = NULL;

extern "C" int asmmain (int argc, char **argv);

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	hInst = hInstance;
	return asmmain(0, NULL);
}

void Win32MsgHandler (void)
{
	while (1) 
	{	
		MSG msg;
		if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) {
				SystemCleanup();
				exit(0);
			}
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
		else if (active) return;
		else WaitMessage();
	}
}

HWND Win32GetWnd()
{
	return hWnd;
}

HINSTANCE Win32GetInst()
{
	return hInst;
}

extern "C" void InputMouseReadPos (long *pXPos, long *pYPos)
{
	POINT p;
	if (!fullscreen) {
		GetCursorPos (&p);
		ScreenToClient (Win32GetWnd(), &p);
		*pXPos = (p.x * 320) / curwidth;
		*pYPos = (p.y * 200) / curheight;
	}
	else *pXPos = *pYPos = 0;
}

static void ConvPalette (PALETTEENTRY *pPal)
{
	int i;
	for (i=0; i<256; i++) {
		pPal[i].peRed = (UCHAR)(pWinPal32[i] >> 16 & 0xfc);
		pPal[i].peGreen = (UCHAR)(pWinPal32[i] >> 8 & 0xfc);
		pPal[i].peBlue = (UCHAR)(pWinPal32[i] & 0xfc);
	}
}

char *ppFSError[] = {
	{ "Unknown error\n" },
	{ "Failed on SetCooperativeLevel\n" },
	{ "Failed on SetDisplayMode\n" },
	{ "Failed on primary surface creation\n" },
	{ "Failed on backbuffer selection\n" },
	{ "Failed on intermediate surface creation\n" },
	{ "Failed on palette creation\n" },
	{ "Failed on palette attachment\n" }
	};


static int InitFullscreen (void)
{
	DDSURFACEDESC ddsd;
	PALETTEENTRY pTempPal[256];
	int rval;

	exclusive = 1;
	softscale = fssoftscale;
	if (fsheight <= (200*fswidth) / 320) fsletterbox = 0;
	curwidth = fswidth; curheight = fsheight;

	rval = pDDraw->SetCooperativeLevel (hWnd, DDSCL_EXCLUSIVE 
		| DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT | DDSCL_ALLOWMODEX);
	if (rval != DD_OK) { error = 1; return 0; }

	if (fsbpp != 8)
	{
		rval = pDDraw->SetDisplayMode (fswidth, fsheight, fsbpp);
		if (rval != DD_OK) { error = 2; return 0; }

		VideoPointerDisable();
		Win32SetMouseExclusive();

		memset (&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		rval = pDDraw->CreateSurface (&ddsd, &pDDSurf2, NULL);
		if (rval != DD_OK) { error = 3; return 0; }

		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		if (!fssoftscale) { ddsd.dwHeight = 200; ddsd.dwWidth = 320; }
		else { ddsd.dwHeight = fsheight; ddsd.dwWidth = fswidth; }
		rval = pDDraw->CreateSurface (&ddsd, &pDDSurf, NULL);
		if (rval != DD_OK) { error = 4; return 0; }

		pDDSurf->GetSurfaceDesc (&ddsd); bpp = 32;
		if (ddsd.ddpfPixelFormat.dwRBitMask == 0xff0000 && 
			ddsd.ddpfPixelFormat.dwRGBBitCount == 32) return 1;
		if (ddsd.ddpfPixelFormat.dwRGBBitCount == 24) { bpp = 24; return 1; }
		if (ddsd.ddpfPixelFormat.dwGBitMask == 0x07e0) { bpp = 16; return 1; }
		if (ddsd.ddpfPixelFormat.dwGBitMask == 0x03e0) { bpp = 15; return 1; }
		return 0;
	}

	LPDIRECTDRAW4 pDD4 = 0;
	rval = pDDraw->QueryInterface (IID_IDirectDraw4, (void **)&pDD4);
	if (rval != DD_OK) { error = 2; return 0; }
	rval = pDD4->SetDisplayMode (320, 200, 8, 0, DDSDM_STANDARDVGAMODE);
	pDD4->Release ();
	if (rval != DD_OK) { error = 2; return 0; }

	VideoPointerDisable();
	Win32SetMouseExclusive();

	if (!fstriplebuf) 
	{
		memset (&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		rval = pDDraw->CreateSurface (&ddsd, &pDDSurf, NULL);
		if (rval != DD_OK) { error = 3; return 0; }
	}
	else
	{
		memset (&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX |
			DDSCAPS_FLIP;
		ddsd.dwBackBufferCount = 2;
		rval = pDDraw->CreateSurface (&ddsd, &pDDSurf2, NULL);
		if (rval != DD_OK) { error = 3; return 0; }

		ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
		rval = pDDSurf2->GetAttachedSurface (&ddsd.ddsCaps, &pDDSurf);
		if (rval != DD_OK) { error = 4; return 0; }
	}

	ConvPalette (pTempPal);
	rval = pDDraw->CreatePalette (DDPCAPS_8BIT | DDPCAPS_ALLOW256 |
		DDPCAPS_INITIALIZE, pTempPal, &pDDPal, NULL);
	if (rval != DD_OK) { error = 6; return 0; }
	if (!fstriplebuf) rval = pDDSurf->SetPalette (pDDPal);
	else rval = pDDSurf2->SetPalette (pDDPal);
	if (rval != DD_OK) { error = 7; return 0; }

	bpp = 8;
	return 1;
}

char *ppWinError[] = {
	{ "Unknown error\n" },
	{ "Failed on SetCooperativeLevel\n" },
	{ "Failed on primary surface creation\n" },
	{ "Failed on clipper creation\n" },
	{ "Failed on clipper window setting\n" },
	{ "Failed on clipper attachment\n" },
	{ "Failed on backbuffer creation\n" },
	{ "Failed on intermediate surface creation\n" },
	{ "Failed to find usable bit depth\n" },
	};


static int InitWindowed (void)
{
	DDSURFACEDESC ddsd;
	int rval;

	softscale = winsoftscale;
	curwidth = winwidth; curheight = winheight;

	rval = pDDraw->SetCooperativeLevel (hWnd, DDSCL_NORMAL);
	if (rval != DD_OK) { error = 1; return 0; }
	ShowWindow (hWnd, SW_SHOWNORMAL);
	SetWindowPos (hWnd, HWND_NOTOPMOST, wndrect.left, wndrect.top,
		wndrect.right-wndrect.left, wndrect.bottom-wndrect.top, 0);

	memset (&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	rval = pDDraw->CreateSurface (&ddsd, &pDDSurf2, NULL);
	if (rval != DD_OK) { error = 2; return 0; }

	rval = pDDraw->CreateClipper (0, &pDDClip, NULL);
	if (rval != DD_OK) { error = 3; return 0; }
    rval = pDDClip->SetHWnd(0, hWnd);
	if (rval != DD_OK) { error = 4; return 0; }
    rval = pDDSurf2->SetClipper(pDDClip);
	if (rval != DD_OK) { error = 5; return 0; }

	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	if (!softscale) { ddsd.dwHeight = 200; ddsd.dwWidth = 320; }
	else { ddsd.dwHeight = winheight; ddsd.dwWidth = winwidth; }
	rval = pDDraw->CreateSurface (&ddsd, &pDDSurf, NULL);
	if (rval != DD_OK) { error = 6; return 0; }

	if (winintermediatebuf) {
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		ddsd.dwHeight = winheight; ddsd.dwWidth = winwidth;
		rval = pDDraw->CreateSurface (&ddsd, &pDDSurf3, NULL);
		if (rval != DD_OK) { error = 7; return 0; }
	}

	pDDSurf->GetSurfaceDesc (&ddsd); bpp = 32;
	if (ddsd.ddpfPixelFormat.dwRBitMask == 0xff0000 && 
		ddsd.ddpfPixelFormat.dwRGBBitCount == 32) return 1;
	if (ddsd.ddpfPixelFormat.dwRGBBitCount == 24) { bpp = 24; return 1; }
	if (ddsd.ddpfPixelFormat.dwGBitMask == 0x07e0) { bpp = 16; return 1; }
	if (ddsd.ddpfPixelFormat.dwGBitMask == 0x03e0) { bpp = 15; return 1; }
	error = 8; return 0;
}

static void DDrawCleanup()
{
	if (pDDraw)
	{
		if (pDDPal) { pDDPal->Release(); pDDPal = NULL; }
		if (pDDClip) { pDDClip->Release(); pDDClip = NULL; }
		if (pDDSurf) { pDDSurf->Release(); pDDSurf = NULL; }
		if (pDDSurf2) { pDDSurf2->Release(); pDDSurf2 = NULL; }
		if (pDDSurf3) { pDDSurf3->Release(); pDDSurf3 = NULL; }

		pDDraw->RestoreDisplayMode();
		pDDraw->SetCooperativeLevel (hWnd, DDSCL_NORMAL);
		exclusive = 0;
		Win32SetMouseNorm();
		VideoPointerEnable();
	}
}

static void ResetWindowParams()
{
	POINT pos = { 0, 0 };
	WINDOWPLACEMENT wp;

	if (exclusive) return;
	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement (hWnd, &wp);
	if (wp.showCmd == SW_SHOWMINIMIZED) return;

	GetClientRect (hWnd, &clrect);
	ClientToScreen (hWnd, &pos);
	OffsetRect (&clrect, pos.x, pos.y);

	wndrect.left = clrect.left - GetSystemMetrics(SM_CXSIZEFRAME);
	wndrect.top = clrect.top - GetSystemMetrics(SM_CYSIZEFRAME)
		- GetSystemMetrics(SM_CYMENU);
	wndrect.right = clrect.right + GetSystemMetrics(SM_CXSIZEFRAME);
	wndrect.bottom = clrect.bottom + GetSystemMetrics(SM_CYSIZEFRAME);
}

static void VideoReset (void)
{
	int rval;

	DDrawCleanup();
	Win32SetMouseNorm();

	while (1)
	{
		if (fullscreen) rval = InitFullscreen();
		else rval = InitWindowed();
		if (rval != 0) return;

		DDrawCleanup();
		Win32SetMouseNorm();

		if (!fullscreen) 
		{
			rval = MessageBox (hWnd, ppWinError[error], 
				"Windowed mode switching error", MB_RETRYCANCEL | MB_ICONWARNING);
			if (rval == 0 || rval == IDCANCEL) {
				SystemCleanup();
				exit(0);
			}
		}
		else if (error == 2)
		{
			rval = MessageBox (hWnd, "Failed to set display mode\nTry compatibility mode?",
				"Fullscreen mode switching error", MB_YESNO | MB_ICONWARNING);
			if (rval == IDNO) fullscreen = 0;
			else if (rval == IDYES)
			{
				fswidth = 640; fsheight = 480; fsbpp = 16;
				fssoftscale = 0; fsletterbox = 0;
			}
		}
		else
		{
			rval = MessageBox (hWnd, ppFSError[error], 
				"Fullscreen mode switching error", MB_RETRYCANCEL | MB_ICONWARNING);
			if (rval == 0 || rval == IDCANCEL) fullscreen = 0;
		}
	}
}

static void RestoreSurfaces()
{
	int rv1, rv2, rv3;
	while (1) {
		rv1 = rv2 = rv3 = DD_OK;
		if (pDDSurf) rv1 = pDDSurf->Restore();
		if (pDDSurf2) rv2 = pDDSurf2->Restore();
		if (pDDSurf3) rv3 = pDDSurf3->Restore();
		if (rv1 == DDERR_WRONGMODE) VideoReset();
		if (rv1 == DD_OK && rv2 == DD_OK && rv3 == DD_OK) break;
	}
}

static LRESULT CALLBACK WndProc (HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
		case WM_LBUTTONDOWN:
		case WM_ACTIVATEAPP:
		case WM_ACTIVATE:
			Win32InputReacquire();
			Win32SoundReacquire();
			break;

		case WM_SIZE:
			if (SIZE_MAXHIDE==wparam || SIZE_MINIMIZED==wparam) {
				active = 0;
				TimerCleanup();
				Win32SetMouseNorm();
			}
			else {
				active = 1;
				TimerInit();
				if (exclusive) Win32SetMouseExclusive();
				ResetWindowParams();
			}
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			return 0L;

		case WM_DESTROY:
			return 0L;

		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lparam)->ptMinTrackSize.x = wndwidth;
			((MINMAXINFO*)lparam)->ptMinTrackSize.y = wndheight;
			((MINMAXINFO*)lparam)->ptMaxTrackSize.x = wndwidth;
			((MINMAXINFO*)lparam)->ptMaxTrackSize.y = wndheight;
			break;

		case WM_MOVE:
			ResetWindowParams ();
			break;

		case WM_KEYDOWN:
			if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000)) break;
			if (wparam == VK_F11) {
				nodraw ^= 1;
				return 0L;
			}
			if (wparam == VK_F12) {
				fullscreen ^= 1;
				VideoReset();
				return 0L;
			}

		case WM_SYSKEYDOWN:
			return 0L;
	}
	return DefWindowProc (wnd, msg, wparam, lparam);
}


static UCHAR pPalTable[3][64];

void VideoInit()
{
	WNDCLASS wc;
	CfgStruct cfg;
	int pPalBase[3] = { 0, 0, 0 };
	int step, i, j, col;
	int rval;

	if (vidInit != 0) return;

	memset (pWinPal15, 0, 256*4);
	memset (pWinPal16, 0, 256*4);
	memset (pWinPal32, 0, 256*4);

	CfgOpen (&cfg, __CONFIGFILE__);
	CfgFindSection (&cfg, "VIDEO");
	CfgGetKeyVal (&cfg, "fullscreen", &fullscreen);
	CfgGetKeyVal (&cfg, "nodraw", &nodraw);

	CfgGetKeyVal (&cfg, "fswidth", &fswidth);
	CfgGetKeyVal (&cfg, "fsheight", &fsheight);
	CfgGetKeyVal (&cfg, "fsbpp", &fsbpp);
	CfgGetKeyVal (&cfg, "fssoftscale", &fssoftscale);
	CfgGetKeyVal (&cfg, "fsletterbox", &fsletterbox);
	CfgGetKeyVal (&cfg, "fstriplebuf", &fstriplebuf);

	CfgGetKeyVal (&cfg, "winwidth", &winwidth);
	CfgGetKeyVal (&cfg, "winheight", &winheight);
	CfgGetKeyVal (&cfg, "winsoftscale", &winsoftscale);
	CfgGetKeyVal (&cfg, "winintermediatebuf", &winintermediatebuf);

	CfgGetKeyVal (&cfg, "RedBase", pPalBase);
	CfgGetKeyVal (&cfg, "GreenBase", pPalBase+1);
	CfgGetKeyVal (&cfg, "BlueBase", pPalBase+2);
	CfgClose (&cfg);

	for (i=0; i<3; i++) 
	{
		col = pPalBase[i] << 8;
		step = ((64<<8) - col) / 63;

		for (j=0; j<64; j++) {
			int tcol = col >> 8;
			if (tcol < 0) tcol = 0;
			pPalTable[i][j] = (UCHAR)tcol;
			col += step;
		}
	}


	wc.lpszClassName = "WIN32FFE";
	wc.lpfnWndProc = WndProc;
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor (NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	rval = RegisterClass (&wc);
	if (rval == 0) { 
		MessageBox (NULL, "Failed on class registration",
			"WinFFE startup error", MB_OK | MB_ICONWARNING);
		SystemCleanup(); exit(0);
	}

	if (winwidth < 160) winwidth = 160;
	if (winheight < 100) winheight = 100;
	wndwidth = winwidth + GetSystemMetrics(SM_CXSIZEFRAME)*2;
	wndheight = winheight + GetSystemMetrics(SM_CYSIZEFRAME)*2
		+ GetSystemMetrics(SM_CYMENU);

	hWnd = CreateWindow ("WIN32FFE", "FFE", WS_OVERLAPPEDWINDOW,
		100, 100, wndwidth, wndheight, NULL, NULL, hInst, NULL);
	if (hWnd == 0) { 
		MessageBox (NULL, "Failed on window creation",
			"WinFFE startup error", MB_OK | MB_ICONWARNING);
		SystemCleanup(); exit(0);
	}
	ShowWindow (hWnd, SW_SHOWNORMAL);
	UpdateWindow (hWnd);
	ResetWindowParams ();

	rval = DirectDrawCreate (NULL, &pDDraw, NULL);
	if (rval != DD_OK) { SystemCleanup(); exit (0); }

	VideoReset();
	vidInit = 1;
}

void VideoCleanup()
{
	if (pDDraw) {
		DDrawCleanup();
		pDDraw->Release(); pDDraw = NULL;
	}
	if (hWnd) { DestroyWindow (hWnd); hWnd = NULL; }
}

extern "C" void VideoPointerEnable()
{
	if (ptrEnabled == 0) {
		SetCursorPos (cursorpos.x, cursorpos.y);
		ClipCursor (NULL);
		ShowCursor (TRUE);
		ptrEnabled = 1;
	}
}

extern "C" void VideoPointerDisable()
{
	if (ptrEnabled != 0) {
		GetCursorPos (&cursorpos);
		ShowCursor (FALSE);
		ClipCursor (&clrect);
		ptrEnabled = 0;
	}
}

extern "C" long VideoPointerExclusive()
{
	return exclusive;
}


void Win32Stretch16 (void *pDest, int dw, int dh, int pitch, ULONG *pPal)
{
	int xfrac, yfrac;
	int xpos, ypos, x, y;

	xfrac = 0x1400000 / dw;		// 320 << 16
	yfrac = 0xc80000 / dh;		// 200 << 16

	ypos = yfrac >> 1;
	for (y=0; y<dh; ++y, ypos+=yfrac)
	{
		UCHAR *pSrcL = pBackBuf + 320*(ypos>>16);
		USHORT *pDestL = (USHORT *)((UCHAR *)pDest + pitch*y);
		xpos = xfrac >> 1;
		for (x=0; x<dw; ++x, xpos+=xfrac) {
			pDestL[x] = (USHORT)pPal[pSrcL[xpos>>16]];
		}
	}
}

void Win32Stretch32 (void *pDest, int dw, int dh, int pitch, ULONG *pPal)
{
	int xfrac, yfrac;
	int xpos, ypos, x, y;

	xfrac = 0x1400000 / dw;		// 320 << 16
	yfrac = 0xc80000 / dh;		// 200 << 16

	ypos = yfrac >> 1;
	for (y=0; y<dh; ++y, ypos+=yfrac)
	{
		UCHAR *pSrcL = pBackBuf + 320*(ypos>>16);
		ULONG *pDestL = (ULONG *)((UCHAR *)pDest + pitch*y);
		xpos = xfrac >> 1;
		for (x=0; x<dw; x++, xpos+=xfrac) {
			pDestL[x] = pPal[pSrcL[(xpos>>16)]];
		}
	}
}

void Win32ClearBoxEdges (int yoff, int w, int h)
{
	DDBLTFX ddbltfx;
	ddbltfx.dwSize = sizeof (DDBLTFX);
	ddbltfx.dwFillColor = 0;
	
	RECT r1 = { 0, 0, w, yoff };
	RECT r2 = { 0, yoff+h, w, h+yoff*2 };
	int flags = DDBLT_COLORFILL | DDBLT_WAIT | DDBLT_ASYNC;

	pDDSurf2->Blt (&r1, NULL, NULL, flags, &ddbltfx);
	pDDSurf2->Blt (&r2, NULL, NULL, flags, &ddbltfx);
}

extern "C" void VideoBlit (UCHAR *pData, long x, long y, long w, long h, long jump)
{
	if (nodraw) return;

	// copy to backbuffer...

	UCHAR *pBuf = pBackBuf + x + y*320;
	for (; h; --h, pData+=jump+w, pBuf+=320) memcpy (pBuf, pData, w);
	pBuf = pBackBuf;

	// If w=320, write and flip to rear surface

	if (w != 320) return;		// One flip per frame
	DDSURFACEDESC ddsd;
	ddsd.dwSize=sizeof(ddsd);
	while (pDDSurf->Lock (NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) 
		RestoreSurfaces ();

	// Simple code for 8bpp

	if (bpp == 8)
	{
		int sw = ddsd.lPitch;
		UCHAR *pSurf = (UCHAR *)ddsd.lpSurface;
		for (h=200; h; --h, pBuf+=320, pSurf+=sw) memcpy (pSurf, pBuf, w);

		pDDSurf->Unlock (NULL);
		if (fstriplebuf) pDDSurf2->Flip (NULL, DDFLIP_WAIT);
		return;
	}

	// Check aspect ratio for letterbox

	int yoff = 0;
	RECT boxrect;
	w = curwidth;
	h = curheight;
	if (fsletterbox && exclusive)
	{
		h = (curwidth * 200) / 320;
		yoff = (curheight - h) >> 1;
	}
	boxrect.left = 0; boxrect.right = w;
	boxrect.top = yoff; boxrect.bottom = h+yoff;

	if (softscale)
	{
		// Scale straight to pDDSurf

		void *pSurf = (void *)((UCHAR *)ddsd.lpSurface + yoff*ddsd.lPitch);
		if (bpp==15) Win32Stretch16 (pSurf, w, h, ddsd.lPitch, pWinPal15);
		if (bpp==16) Win32Stretch16 (pSurf, w, h, ddsd.lPitch, pWinPal16);
		if (bpp==24) return;
		if (bpp==32) Win32Stretch32 (pSurf, w, h, ddsd.lPitch, pWinPal32);

		pDDSurf->Unlock (NULL);

		// Use fast blit? Nah...
		if (exclusive) pDDSurf2->BltFast (0, yoff, pDDSurf, &boxrect,
			DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT);
		else pDDSurf2->Blt (&clrect, pDDSurf, NULL, DDBLT_WAIT, NULL);
		if (yoff) Win32ClearBoxEdges (yoff, w, h);
		return;
	}

	// Copy unscaled straight to pDDSurf

	if (bpp==15)
	{
		int dJump = (ddsd.lPitch>>1) - 320; int cw, ch;
		USHORT *pSurf = (USHORT *)ddsd.lpSurface;
		for (ch=200; ch; --ch, pSurf+=dJump)
			for (cw=320; cw; --cw, ++pBuf, ++pSurf)
				*pSurf = (USHORT)pWinPal15[*pBuf];
	}
	else if (bpp==16)
	{
		int dJump = (ddsd.lPitch>>1) - 320; int cw, ch;
		USHORT *pSurf = (USHORT *)ddsd.lpSurface;
		for (ch=200; ch; --ch, pSurf+=dJump)
			for (cw=320; cw; --cw, ++pBuf, ++pSurf)
				*pSurf = (USHORT)pWinPal16[*pBuf];
	}
	else if (bpp==24)
	{
		int dJump = ddsd.lPitch - 320*3; int cw, ch;
		UCHAR *pSurf = (UCHAR *)ddsd.lpSurface;
		for (ch=200; ch; --ch, pSurf+=dJump)
			for (cw=320; cw; --cw, ++pBuf, pSurf+=3) {
				UCHAR *pSrc = (UCHAR *)&pWinPal32[*pBuf];
				pSurf[0] = pSrc[0];	pSurf[1] = pSrc[1];
				pSurf[2] = pSrc[2];
			}
	}
	else if (bpp==32)
	{
		int dJump = (ddsd.lPitch>>2) - 320; int cw, ch;
		ULONG *pSurf = (ULONG *)ddsd.lpSurface;
		for (ch=200; ch; --ch, pSurf+=dJump)
			for (cw=320; cw; --cw, ++pBuf, ++pSurf)
				*pSurf = pWinPal32[*pBuf];
	}
	pDDSurf->Unlock (NULL);

	// Scale with hardware

	if (exclusive) {
		if (yoff) {
			pDDSurf2->Blt (&boxrect, pDDSurf, NULL, DDBLT_WAIT, NULL);
			Win32ClearBoxEdges (yoff, w, h);
		}
		else pDDSurf2->Blt (NULL, pDDSurf, NULL, DDBLT_WAIT, NULL);
	}
	else if (winintermediatebuf) {
		pDDSurf3->Blt (NULL, pDDSurf, NULL, DDBLT_WAIT, NULL);
		pDDSurf2->Blt (&clrect, pDDSurf3, NULL, DDBLT_WAIT, NULL);
	}
	else pDDSurf2->Blt (&clrect, pDDSurf, NULL, DDBLT_WAIT, NULL);
}

extern "C" void VideoMaskedBlit (UCHAR *pData, long x, long y,
	long w, long h, long jump)
{
	if (nodraw) return;

	// Copy to backbuffer

	int cw; int dJump = 320-w;
	UCHAR *pBuf = (UCHAR *)pBackBuf + x + 320*y;
	for (; h; --h, pData+=jump, pBuf+=dJump)
		for (cw=w; cw; --cw, ++pData, ++pBuf)
			if (*pData) *pBuf = *pData;
}

extern "C" void VideoReverseBlit (UCHAR *pData, long x, long y,
	long w, long h, long jump)
{
	if (nodraw) return;

	// Copy from backbuffer...

	UCHAR *pBuf = pBackBuf + x + y*320;
	for (; h; --h, pData+=jump+w, pBuf+=320) memcpy (pData, pBuf, w);
	if (w != 320) return;		// One flip per frame
}

extern "C" void VideoGetPalValue (long palindex, UCHAR *pVal)
{
	ULONG col = pWinPal32[palindex];
	pVal[0] = (UCHAR)(col >> 18 & 0x3f);
	pVal[1] = (UCHAR)(col >> 10 & 0x3f);
	pVal[2] = (UCHAR)(col >> 2 & 0x3f);
}

extern "C" void VideoSetPalValue (long palindex, UCHAR *pVal)
{
	UCHAR pCol[4];
	pCol[0] = pPalTable[0][pVal[0]];
	pCol[1] = pPalTable[1][pVal[1]];
	pCol[2] = pPalTable[2][pVal[2]];

	if (bpp == 8) {
		PALETTEENTRY pe;
		pe.peRed = pCol[0] << 2;
		pe.peGreen = pCol[1] << 2;
		pe.peBlue = pCol[2] << 2;
		pDDPal->SetEntries (0, palindex, 1, &pe);
	}

	pWinPal15[palindex] = (ULONG)((pCol[0] << 9 & 0x7c00)
		+ (pCol[1] << 4 & 0x03e0) + (pCol[2] >> 1));
	pWinPal16[palindex] = (ULONG)((pCol[0] << 10 & 0xf800)
		+ (pCol[1] << 5 & 0x07e0) + (pCol[2] >> 1));
	pWinPal32[palindex] = (ULONG)((pCol[0] << 18 & 0xfc0000)
		+ (pCol[1] << 10 & 0xfc00) + (pCol[2] << 2));
	pWinPal15[palindex] |= pWinPal15[palindex] << 16;
	pWinPal16[palindex] |= pWinPal16[palindex] << 16;
}
