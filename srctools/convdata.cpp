#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h> 	// nasty...

typedef unsigned char UCHAR;

static void ConvSection (char *&pCurDest, char *&pCurSrc, int asc);
static int ReadData (char *&pCurSrc, UCHAR *pData);
static void WriteAscData (char *&pCurDest, UCHAR *pData, int len);
static void WriteBinData (char *&pCurDest, UCHAR *pData, int len);

char *NextLine (const char *pData)
{
	char *pNext = strchr (pData, 0xa);
	return ++pNext;
}

int CopyLine (char *pDest, const char *pSrc)
{
	char *pEnd = strchr (pSrc, 0xa);
	int strsize = int(pEnd - pSrc);
	strncpy (pDest, pSrc, strsize+1);
	return strsize;
}

int LineToStr (char *pDest, const char *pSrc)
{
	char *pEnd = strchr (pSrc, 0xa);
	int strsize = int(pEnd - pSrc);
	strncpy (pDest, pSrc, strsize);
	pDest[strsize] = NULL;
	return strsize;
}


int main (int argc, char **argv)
{
	if (argc!=5) {
		fprintf (stderr, "Wrong number of arguments\n");
		fprintf (stderr, "Syntax: convdata [a|b] [firstsym] [endsym] [datafile]\n");
		return 0;
	}

	int sym1len = strlen (argv[2]);
	int sym2len = strlen (argv[3]);
	int asc = 0;
	if (argv[1][0] != 'b') asc = 1;


	// Read file into memory

	FILE *pFile = fopen (argv[4], "rt");
	if (pFile == NULL) {
		fprintf (stderr, "Couldn't open file %s\n", argv[4]);
		return 0;
	}

	fseek (pFile, 0, SEEK_END);
	int filesize = ftell (pFile);
	fseek (pFile, 0, SEEK_SET);
	char *pImage = (char *) malloc (filesize);
	filesize = fread (pImage, 1, filesize, pFile);
	fclose (pFile);

	// Find start and end points

	char *pStart, *pEnd;
	pStart = NextLine (pImage);
	while (strncmp (pStart, argv[2], sym1len)) {
		pStart = NextLine (pStart);
		if ((pStart - pImage) > filesize) {
			fprintf (stderr, "First symbol not found\n");
			return 0;
		}
	}
	pStart = NextLine (pStart);
	pEnd = pStart;

	printf ("Start symbol found at %i bytes\n", int(pStart-pImage));

	while (strncmp (pEnd, argv[3], sym2len)) {
		pEnd = NextLine (pEnd);	
		if ((pEnd - pImage) > filesize) {
			fprintf (stderr, "Last symbol not found\n");
			return 0;
		}
	}
	pEnd = NextLine (pEnd);
	int startsize = int(pStart-pImage);
	int endsize = filesize - int(pEnd-pImage);

	printf ("End symbol found at %i bytes\n", int(pEnd-pImage));


	// Set up source/dest buffers for converted section

	char *pConvBuf = (char *) malloc (6*int(pEnd-pStart));
	memset (pConvBuf, 0, 6*int(pEnd-pStart));
	char *pCurSrc = pStart;
	char *pCurDest = pConvBuf;
	

	// Convert sections

	while (pCurSrc < pEnd) {
		ConvSection (pCurDest, pCurSrc, asc);
		CopyLine (pCurDest, pCurSrc);
		pCurSrc = NextLine (pCurSrc);
		pCurDest = NextLine (pCurDest);
	}
	int convsize = int(pCurDest-pConvBuf);


	// Write the modified file

	pFile = fopen (argv[4], "wt");
printf ("Writing start of file, size %i bytes\n", startsize);
	fwrite (pImage, 1, startsize, pFile);
printf ("Writing middle of file, size %i bytes\n", convsize);
	fwrite (pConvBuf, 1, convsize, pFile);
printf ("Writing end of file, size %i bytes\n", endsize);
	fwrite (pEnd, 1, endsize, pFile);
	fclose (pFile);


	free (pConvBuf);
	free (pImage);
	return 0;
}


static void ConvSection (char *&pCurDest, char *&pCurSrc, int asc)
{
	static UCHAR pData[100000];

	int datasize = ReadData (pCurSrc, pData);
	if (asc) WriteAscData (pCurDest, pData, datasize);
	else WriteBinData (pCurDest, pData, datasize);
}


static int ReadData (char *&pSrc, UCHAR *pData)
{
	int n = 0, i = 0;
	static char pLStr[100000];

	LineToStr (pLStr, pSrc);

	while (1)		// Per-line loop
	{
		i = strspn (pLStr, "\t ");
		if (strncmp (pLStr+i, "db ", 3)) {
			fprintf (stderr, "ReadData broke on %s\n", pLStr+i);
			break;
		}

		i += strcspn (pLStr+i, "0'");
		while (pLStr[i] != 0)
		{
			if (pLStr[i] == '0') {
				pData[n++] = strtol (pLStr+i, NULL, 0);
				i += strcspn (pLStr+i, ",");
			} else {
				int nextq = strcspn (pLStr+i+1, "'");
				strncpy ((char *)pData+n, pLStr+i+1, nextq);
				i += nextq + 2; n += nextq;
			}
			i += strcspn (pLStr+i, "0'");
		}	

		pSrc = NextLine (pSrc);
		LineToStr (pLStr, pSrc);
	}
	return n;
}


static void WriteAscData (char *&pDest, UCHAR *pData, int len)
{
	int i = 0, a = 0, n = 0;

	while (i<len)
	{
		if (n>65) {
			if (a==1) pDest[n++] = 0x27;
			pDest[n++] = '\n';
			pDest += n; n = 0;
		}

		if (n==0) {
			if (pData[i]>=0x20 && pData[i]<0x7e && pData[i]!=0x27)
			{ n += sprintf (pDest+n, "\tdb '%c", pData[i++]); a=1; }
			else { n += sprintf (pDest+n, "\tdb 0x%x", pData[i++]); a=0; }
		}

		if (pData[i]>=0x20 && pData[i]<0x7e && pData[i]!=0x27)
		{
			if (a==0) { n += sprintf (pDest+n, ", '"); a=1; }
			pDest[n++] = char(pData[i++]);
		}
		else {
			if (a==1) { pDest[n++] = 0x27; a=0; }
			n += sprintf (pDest+n, ", 0x%x", pData[i++]);
		}
	}

	if (a==1) pDest[n++] = 0x27;
	pDest[n++] = '\n';
	pDest += n;
	return;
}


static void WriteBinData (char *&pDest, UCHAR *pData, int len)
{
	int i=0; int bytes = 0;

	while (i<len)
	{
		if (bytes == 8) { pDest += sprintf (pDest, "\n"); bytes = 0; }
		if (!bytes) pDest += sprintf (pDest, "\tdb 0x%x", pData[i]);
		else pDest += sprintf (pDest, ", 0x%x", pData[i]);
		bytes++; i++;
	}

	*pDest = '\n'; pDest++;
	return;
}
