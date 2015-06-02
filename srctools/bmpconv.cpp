#include <stdio.h>

typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;


int main (int argc, char **argv)
{
	int i;
	ULONG pPal[256];			// Palette and general fread()
	USHORT pHead[27];		// General use with fread()
	UCHAR pData[320*200];
	int numColors;

	// Open BMP file for binary reading
	
	if (argc != 3) {
		printf ("Number of arguments incorrect\n");
		return 0;
	}
	FILE *pInfile = fopen (argv[1],"rb");
	if (!pInfile) {
		printf ("Input file %s not found\n", argv[1]);
		return 0;
	}
	FILE *pOutfile = fopen (argv[2],"wt");
	if (!pOutfile) {
		printf ("Output file %s not found\n", argv[2]);
		fclose (pInfile); return 0;
	}

	fread (pHead, 2, 27, pInfile);
	if (pHead[0] != 0x4D42 || pHead[7] != 40) {
		printf ("Input file not a valid BMP file\n");
		fclose (pInfile); fclose (pOutfile); return 0;
	}

	if (pHead[9] != 320 || pHead[11] != 200 
		|| pHead[14] != 8 || pHead[15] != 0) {
		printf ("Input file not a 320x200x8 uncompressed BMP file\n");
		fclose (pInfile); fclose (pOutfile); return 0;
	}

	numColors = pHead[23];
	if (!numColors) numColors = 256;

	fread (pPal, 4, numColors, pInfile);		// Copy palette data
	for (i=199*320; i>=0; i-=320) fread (pData+i, 1, 320, pInfile);

	// Convert palette
	for (i=0; i<numColors; i++) pPal[i] = (pPal[i]>>2) & 0x3f3f3f;

	fprintf (pOutfile, "GLOBAL sheepPal\n");
	fprintf (pOutfile, "GLOBAL sheepData\n");
	fprintf (pOutfile, "\nSECTION .data\n");
	fprintf (pOutfile, "\nsheepPal:\n");
	for (i=0; i<256; i+=4) {
		fprintf (pOutfile, "\tdd 0x%06lx, 0x%06lx, 0x%06lx, 0x%06lx\n",
			pPal[i], pPal[i+1], pPal[i+2], pPal[i+3]);
	}

	fprintf (pOutfile, "\nsheepData:\n");
	for (i=0; i<320*200; i+=8) {
		fprintf (pOutfile, "\tdb 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
			pData[i], pData[i+1], pData[i+2], pData[i+3], 
			pData[i+4], pData[i+5], pData[i+6], pData[i+7]);
	}

	fclose (pInfile);
	fclose (pOutfile);
	return 0;
}
