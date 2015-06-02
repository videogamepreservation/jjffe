//#include <stdio.h>

void BlitClipWrapper (void *pData, int xpos,
		int ypos, int width, int height, int jump,
		void (*BlitFunc)(void *, int, int, int, int, int))
{
	int srcx, srcy, t;
	int scanw = width + jump;

	if (xpos >= 0) srcx = 0;
	else { srcx = -xpos; width += xpos; xpos = 0; }
	if (ypos >= 0) srcy = 0;
	else { srcy = -ypos; height += ypos; ypos = 0; }

	if ((t = xpos + width - 320) > 0) { width -= t; }
	if ((t = ypos + height - 200) > 0) { height -= t; }

	jump = scanw - width;
	pData = (void *)((char *)pData + srcy*scanw + srcx);

	BlitFunc (pData, xpos, ypos, width, height, jump);
}	

