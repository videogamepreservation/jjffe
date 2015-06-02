#include <stdio.h>
#include <string.h>
#include <malloc.h>

int main (int argc, char **argv)
{
	char pLineBuf[1024];
	char *ppExtern[1000];
	int pExtLen[1000];
	int numExts = 0;
	int i, size, offset, lastoffset;
	char *pImage;
	FILE *pFile;

	int numChecks = 0;
	int numReplace = 0;

	if (argc != 3) {
		printf ("Format: lnxconv asmfile lnxasmfile\n");
		return 0;
	}
	printf ("Reading file %s...\n", argv[1]);
	pFile = fopen (argv[1], "rb");
	if (pFile == NULL) {
		printf ("File %s not found\n", argv[1]);
		return 0;
	}

	while (!feof (pFile))
	{
		char *pTok;
		pTok = fgets (pLineBuf, 1024, pFile);
		if (pTok == NULL) break;
		pTok = strtok (pLineBuf, " \t\r\n");
		if (pTok == NULL) continue;
		if (strcmp (pTok, "EXTERN")	&& strcmp (pTok, "GLOBAL")) continue;
		pTok = strtok (NULL, " \t\r\n");
		if (pTok == NULL) continue;
		if (pTok[0] != '_') continue;
		ppExtern[numExts++] = strdup (pTok);
	}

	for (i=0; i<numExts; i++) pExtLen[i] = strlen (ppExtern[i]);

	lastoffset = offset = 0;
	fseek (pFile, 0, SEEK_END);
	size = ftell (pFile);
	fseek (pFile, 0, SEEK_SET);
	pImage = (char *)malloc (size+1);
	fread (pImage, 1, size, pFile);
	pImage[size] = 0;
	offset += strspn (pImage+offset, "[] \t\r\n");

	fclose (pFile);
	printf ("Writing file %s...\n", argv[2]);
	pFile = fopen (argv[2],"wb");
	if (pFile == NULL) {
		printf ("File %s could not be opened\n", argv[2]);
		return 0;
	}

	while (offset<size)
	{
		if (pImage[offset] == '_')
		{
			for (i=0; i<numExts; i++) {
				numChecks++;
				if (!strncmp (pImage+offset, ppExtern[i], pExtLen[i])) break;
			}
			if (i != numExts) {
				numReplace++;
				fwrite (pImage+lastoffset, 1, offset-lastoffset, pFile);
				lastoffset = offset + 1;
			}
		}
		offset += strcspn (pImage+offset, "[] \t\r\n");
		if (offset>=size) break;
		offset += strspn (pImage+offset, "[] \t\r\n");
	}
	fwrite (pImage+lastoffset, 1, offset-lastoffset, pFile);
	fclose (pFile);

	printf ("numfuncs = %i, numcheck = %i, numreplace = %i\n",
		numExts, numChecks, numReplace);

	return 0;
}

	