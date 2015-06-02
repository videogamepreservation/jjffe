#include <SDL.h>
#include <SDL_opengl.h>

#include <stdlib.h>
#include <string.h>
#include "ffecfg.h"
#include "ffeapi.h"

extern "C" int asmmain (int argc, char **argv);

int main (int argc, char **argv)
{
	return asmmain (argc, argv);
}




static SDL_Surface *pScreen = 0;
static SDL_Overlay *pOverlay = 0;
static UCHAR pBackBufBase[320*200+32];
static UCHAR *pBackBuf;

static UCHAR pPalTable[3][64];
static SDL_Color pPalette[256];
static USHORT pPal15[256];
static USHORT pPal16[256];
static ULONG pPal32[256];
static ULONG pPalYUV[256];

static int fullscreen = 0;
static int glpackedtex = 0;
static int glreusetex = 0;
static int glbilinear = 0;
static int overlayrefresh = 1;

static int fsmode = 0;
static int fsletterbox = 1;
static int fswidth = 320;
static int fsheight = 200;
static int fsprefbpp = 16;

static int winmode = 0;
static int winwidth = 640;
static int winheight = 400;

static int mode;
static int letterbox;
static int width;
static int height;
static int drawheight;
static int yoff;

static unsigned int pTexNames[5];


static int GLVideoInit ()
{
	if (fullscreen) {
		if (fsprefbpp == 32) {
			SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute (SDL_GL_BUFFER_SIZE, 32);
		}
		else {
			SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 6);
			SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 5);
			SDL_GL_SetAttribute (SDL_GL_BUFFER_SIZE, 16);
		}
	}
	SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

	if (!fullscreen) pScreen = SDL_SetVideoMode (width, height, 0, SDL_OPENGL);
	else pScreen = SDL_SetVideoMode (width, height, 0, SDL_OPENGL | SDL_FULLSCREEN);
	if (pScreen == 0) return 0;

    glDisable (GL_LIGHTING);
	glDisable (GL_CULL_FACE);
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_BLEND);

    glViewport (0, 0, width, height);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
	glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();

	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);

	glEnable (GL_TEXTURE_2D);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glGenTextures (5, pTexNames);

	int i;
	for (i=0; i<5; i++)
	{
		glBindTexture (GL_TEXTURE_2D, pTexNames[i]);

		if (glpackedtex) glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, 64, 256,
			0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);
		else glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, 64, 256,
			0, GL_RGB, GL_UNSIGNED_BYTE, 0);

		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		if (glbilinear) {
			glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		else {
			glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	}

	for (i=0; i<5; i++)
	{
		glClear (GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapBuffers ();
	}

	return 1;
}

static int SDLVideoInit ()
{
	if (fsprefbpp != 16 && fsprefbpp != 32) fsprefbpp = 16;

	int flags = SDL_ANYFORMAT;
	if (fullscreen) flags |= SDL_FULLSCREEN;

	// Double buffering? back buffer?

	pScreen = SDL_SetVideoMode (width, height, fsprefbpp, flags);
	if (pScreen == 0) return 0;

	SDL_Rect r1 = { 0, 0, width, height };
	SDL_FillRect (pScreen, &r1, 0);

	if (mode != 2) return 1;		// No overlays

	pOverlay = SDL_CreateYUVOverlay (640, 200, SDL_YUY2_OVERLAY, pScreen);
	SDL_Rect r2 = { 0, (USHORT)yoff, (USHORT)width, (USHORT)drawheight };
	SDL_DisplayYUVOverlay (pOverlay, &r2);

	return 1;
}

static void VideoInitWorker ()
{
	if (fullscreen) VideoPointerDisable ();
	else VideoPointerEnable ();

	if (fullscreen)
	{
		mode = fsmode;
		letterbox = fsletterbox;
		width = fswidth;
		height = fsheight;
	}
	else {
		mode = winmode;
		letterbox = 0;
		width = winwidth;
		height = winheight;
	}	

	yoff = 0;
	drawheight = height;
	if (letterbox)
	{
		drawheight = (width * 200) / 320;
		yoff = (height - drawheight) >> 1;
	}
	if (drawheight > height) { drawheight = height; yoff = 0; }
	if (drawheight == height) letterbox = 0;

	int rval;
	if (mode == 0) rval = GLVideoInit ();
	else rval = SDLVideoInit ();

	if (rval == 0)
	{
		fprintf (stderr, "Failed to initialise video mode\n");
		fprintf (stderr, "Switching to backup 640x400 windowed non-GL\n");

		fullscreen = 0;
		mode = 1;
		letterbox = 0;
		width = 640;
		height = 400;

		VideoPointerEnable ();
		rval = SDLVideoInit ();
		if (rval != 0) return;

		fprintf (stderr, "Backup mode failed, exiting\n");
		SystemCleanup ();
		exit (0);
	}
}


void VideoInit (void)
{
	CfgStruct cfg;
	int pPalBase[3] = { 0, 0, 0 };
	int step, i, j, col;

	CfgOpen (&cfg, __CONFIGFILE__);
	CfgFindSection (&cfg, "VIDEO");
	CfgGetKeyVal (&cfg, "RedBase", pPalBase);
	CfgGetKeyVal (&cfg, "GreenBase", pPalBase+1);
	CfgGetKeyVal (&cfg, "BlueBase", pPalBase+2);

	CfgGetKeyVal (&cfg, "startfullscreen", &fullscreen);
	CfgGetKeyVal (&cfg, "glpackedtex", &glpackedtex);
	CfgGetKeyVal (&cfg, "glreusetex", &glreusetex);
	CfgGetKeyVal (&cfg, "glbilinear", &glbilinear);
	CfgGetKeyVal (&cfg, "overlayrefresh", &overlayrefresh);

	CfgGetKeyVal (&cfg, "fsmode", &fsmode);
	CfgGetKeyVal (&cfg, "fsletterbox", &fsletterbox);
	CfgGetKeyVal (&cfg, "fswidth", &fswidth);
	CfgGetKeyVal (&cfg, "fsheight", &fsheight);
	CfgGetKeyVal (&cfg, "fsprefbpp", &fsprefbpp);

	CfgGetKeyVal (&cfg, "winmode", &winmode);
	CfgGetKeyVal (&cfg, "winwidth", &winwidth);
	CfgGetKeyVal (&cfg, "winheight", &winheight);
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

	for (i=0; i<256; i++) {
		pPalette[i].r = 0; 
		pPalette[i].g = 0; 
		pPalette[i].b = 0; 
		pPal15[i] = 0;
		pPal16[i] = 0;
		pPal32[i] = 0;
	}

	pBackBuf = (UCHAR *)((ULONG)pBackBufBase + 31 & -32);

	VideoInitWorker ();
}

void VideoCleanup (void)
{
	if (mode == 0)	glDeleteTextures (5, pTexNames);
	if (pOverlay) SDL_FreeYUVOverlay (pOverlay);
	pOverlay = 0;
}



void GLFillPackedTex (int offset)
{
	USHORT pTexBuf[64*200];
	UCHAR *pBuf = pBackBuf + 64*offset;
	USHORT *pBuf16 = pTexBuf;
	int cw, ch;

	for (ch=0; ch<200; ++ch, pBuf+=320, pBuf16+=64)
		for (cw=0; cw<64; ++cw)
			pBuf16[cw] = (USHORT)pPal16[pBuf[cw]];

	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 64, 200, GL_RGB,
		GL_UNSIGNED_SHORT_5_6_5, pTexBuf);
}


void GLFillUnpackedTex (int offset)
{
	UCHAR pTexBuf[64*200*3];
	UCHAR *pBuf = pBackBuf + 64*offset;
	UCHAR *pBuf24 = pTexBuf;
	int cw, ch;

	for (ch=0; ch<200; ++ch, pBuf+=320, pBuf24+=192) {
		for (cw=0; cw<64; ++cw) {
			SDL_Color *pCol = pPalette + pBuf[cw];
			pBuf24[3*cw+0] = pCol->r;
			pBuf24[3*cw+1] = pCol->g;
			pBuf24[3*cw+2] = pCol->b;
		}
	}

	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 64, 200, GL_RGB,
		GL_UNSIGNED_BYTE, pTexBuf);
}

static void VideoStretch16 (void *pDest, int dw, int dh, int pitch, USHORT *pPal)
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
			pDestL[x] = pPal[pSrcL[xpos>>16]];
		}
	}
}

static void VideoStretch32 (void *pDest, int dw, int dh, int pitch, ULONG *pPal)
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


extern "C" void VideoBlit (UCHAR *pData, long x, long y, long w, long h, long jump)
{
	UCHAR *pBuf = pBackBuf + x + y*320;
	for (; h; --h, pData+=jump+w, pBuf+=320) memcpy (pBuf, pData, w);
	if (w != 320) return;		// One flip per frame

	if (mode == 0)
	{
		float ypos = 1.0f;
		if (letterbox) ypos = (0.625f * width) / height;

		int i;
		for (i=0; i<5; i++)
		{
			if (!glreusetex) glBindTexture (GL_TEXTURE_2D, pTexNames[i]);
			if (glpackedtex) GLFillPackedTex (i);
			else GLFillUnpackedTex (i);

			float xs = -1.0f + 0.4f*i;
			float xe = xs + 0.4f;
			
			glBegin (GL_TRIANGLE_STRIP);

			glTexCoord2f (0.0f, 0.0f);
			glVertex3f (xs, ypos, 0.5f);
			glTexCoord2f (1.0f, 0.0f);
			glVertex3f (xe, ypos, 0.5f);
			glTexCoord2f (0.0f, 0.78125f);
			glVertex3f (xs, -ypos, 0.5f);
			glTexCoord2f (1.0f, 0.78125f);
			glVertex3f (xe, -ypos, 0.5f);

			glEnd ();
		}

		SDL_GL_SwapBuffers ();
		return;
	}

	// SDL version?

	w = width;
	h = drawheight;

	if (mode == 2)
	{
		SDL_LockYUVOverlay (pOverlay);

		pBuf = pBackBuf;
		ULONG *pDataYUV = (ULONG *)pOverlay->pixels[0];
		int djump = pOverlay->pitches[0] >> 2;

		for (y=0; y<200; y++, pDataYUV+=djump, pBuf+=320)
			for (x=0; x<320; x++)
				pDataYUV[x] = pPalYUV[pBuf[x]];

		SDL_UnlockYUVOverlay (pOverlay);

		if (!overlayrefresh) return;
		SDL_Rect r = { 0, (USHORT)yoff, (USHORT)w, (USHORT)h };
		SDL_DisplayYUVOverlay (pOverlay, &r);
		return;
	}

	SDL_LockSurface (pScreen);
	
	UCHAR *pSurf = (UCHAR *)pScreen->pixels + yoff*pScreen->pitch;
	int bpp = pScreen->format->BitsPerPixel;

	if (bpp==15) VideoStretch16 (pSurf, w, h, pScreen->pitch, pPal15);
	if (bpp==16) VideoStretch16 (pSurf, w, h, pScreen->pitch, pPal16);
	if (bpp==24) return;
	if (bpp==32) VideoStretch32 (pSurf, w, h, pScreen->pitch, pPal32);

	SDL_UnlockSurface (pScreen);
	SDL_UpdateRect (pScreen, 0, 0, 0, 0);
}


extern "C" void VideoMaskedBlit (UCHAR *pData, long x, long y, long w, long h, long jump)
{
	int cw; int dJump = 320-w;
	UCHAR *pBuf = (UCHAR *)pBackBuf + x + 320*y;
	for (; h; --h, pData+=jump, pBuf+=dJump)
		for (cw=w; cw; --cw, ++pData, ++pBuf)
			if (*pData) *pBuf = *pData;
}

extern "C" void VideoReverseBlit (UCHAR *pData, long x, long y, long w, long h, long jump)
{
	UCHAR *pBuf = pBackBuf + x + y*320;
	for (; h; --h, pData+=jump+w, pBuf+=320) memcpy (pData, pBuf, w);
}

extern "C" void VideoSetPalValue (long palindex, UCHAR *pVal)
{
	int pCol[4];
	int Y, U, V;

	pCol[0] = pPalette[palindex].r = pPalTable[0][pVal[0]] << 2;
	pCol[1] = pPalette[palindex].g = pPalTable[1][pVal[1]] << 2;
	pCol[2] = pPalette[palindex].b = pPalTable[2][pVal[2]] << 2;

	pPal15[palindex] = (USHORT)((pCol[0] << 7 & 0x7c00)
		+ (pCol[1] << 2 & 0x03e0) + (pCol[2] >> 3));
	pPal16[palindex] = (USHORT)(((pCol[0] << 8) & 0xf800)
		+ ((pCol[1] << 3) & 0x07e0) + (pCol[2] >> 3));
	pPal32[palindex] = (ULONG)((pCol[0] << 16 & 0xfc0000)
		+ (pCol[1] << 8 & 0xfc00) + (pCol[2]));

	Y = ((9798*pCol[0] + 19235*pCol[1] + 3736*pCol[2] + 0x4000) >> 15);
	U = ((-4784*pCol[0] - 9437*pCol[1] + 14221*pCol[2] + 0x4000) >> 15) + 128;
	V = ((20218*pCol[0] - 16941*pCol[1] - 3277*pCol[2] + 0x4000) >> 15) + 128;

	if (Y<0) Y=0; if (Y>255) Y=255;
	if (U<0) U=0; if (U>255) U=255;
	if (V<0) V=0; if (V>255) V=255;

	pPalYUV[palindex] = (V<<24) | (Y<<16) | (U<<8) | Y;		// YUY2
}

extern "C" void VideoGetPalValue (long palindex, UCHAR *pVal)
{
	pVal[0] = pPalette[palindex].r >> 2;
	pVal[1] = pPalette[palindex].g >> 2;
	pVal[2] = pPalette[palindex].b >> 2;
}

extern "C" long VideoPointerExclusive (void)
{
	return fullscreen;
}

extern "C" void VideoPointerEnable (void) 
{
	SDL_WM_GrabInput (SDL_GRAB_OFF);
	SDL_ShowCursor (SDL_ENABLE);
}

extern "C" void VideoPointerDisable (void) 
{
	SDL_WM_GrabInput (SDL_GRAB_ON);
	SDL_ShowCursor (SDL_DISABLE);
}




void InputInit ();
void InputCleanup ();
void TimerInit ();
void TimerCleanup ();
void SoundInit ();
void SoundCleanup ();

extern "C" void DirInit ();
extern "C" void DirCleanup ();


extern "C" void SystemInit (void)
{
	int rval;
	rval = SDL_Init (SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	if (rval != 0) exit (0);

	TimerInit();
	DirInit();
	VideoInit();
	InputInit();
	SoundInit();
}

extern "C" void SystemCleanup (void)
{
	SoundCleanup();
	InputCleanup();
	VideoCleanup();
	DirCleanup();
	TimerCleanup();

	SDL_Quit ();
}

void VideoSwitchMode ()
{
	VideoCleanup ();

	fullscreen ^= 1;
	VideoInitWorker ();
	return;
}
