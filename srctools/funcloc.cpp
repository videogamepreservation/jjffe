#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ReadLine (FILE *pF, char *pStr)
{
	int i=0, c = fgetc (pF); pStr[0] = 0;
	while (c>=0 && c!='\n' && i<255) {
		pStr[i] = c; pStr[++i] = 0;
		c = fgetc (pF);
	}
}

typedef unsigned int UINT;

#define MAXFUNCS 2000

struct OffsetVal
{
	UINT offset;
	char *pStr;
};

int CompareOffsets (const void *p1, const void *p2)
{
	OffsetVal *c1 = (OffsetVal *)p1;
	OffsetVal *c2 = (OffsetVal *)p2;
	return (int)c1->offset - (int)c2->offset;
}

int main (int argc, char **argv)
{
	int i;
	FILE *pOutFile, *pMapFile;
	OffsetVal pOff[MAXFUNCS];
	int numfuncs = 0;
	
	if (argc!=4) {
		printf ("Wrong number of arguments\n");
		printf ("Syntax: reloc [mapfile] [objfile] [outfile]\n");
		return 0;
	}

	pMapFile = fopen (argv[1], "rt");
	if (pMapFile == NULL) {
		printf ("Couldn't open file %s\n", argv[1]);
		return 0;
	}

	// Process mapfile

	do {
		char pS1[255], pS2[255], pS3[255];
		ReadLine (pMapFile, pS1);
		sscanf (pS1, "%s %*s %*s %s", pS2, pS3);
		if (!strcmp (pS2, ".text") && !strcmp (pS3, argv[2])) break;
	} while (!feof(pMapFile));

	if (feof(pMapFile)) printf ("Whoops, 1st EOF\n");

	// Found start of object file listing, now read func offsets
	
	do {
		char pS1[255]; unsigned int offset; int n;
		ReadLine (pMapFile, pS1);
		n = sscanf (pS1, "%x", &offset);
		if (n!=1) break;
		pOff[numfuncs].offset = offset;
		pOff[numfuncs].pStr = strdup(pS1);
		numfuncs++;
	} while (!feof(pMapFile));

	if (feof(pMapFile)) printf ("Whoops, 2nd EOF\n");

	fclose (pMapFile);

	// Sort items and write the lot

	qsort (pOff, numfuncs, 8, &CompareOffsets);

	pOutFile = fopen (argv[3], "wt");
	if (pOutFile == NULL) {
		printf ("Couldn't open file %s\n", argv[3]);
		return 0;
	}

	for (i=0; i<numfuncs; i++) fprintf (pOutFile, "%s\n", pOff[i].pStr);
	fclose (pOutFile);
	return 0;
}
