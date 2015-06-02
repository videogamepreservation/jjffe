/*
 *  Nasty little decompiler by John Jordan
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <malloc.h>

extern "C" {
	#include "insns.h"
	#include "nasm.h"
	#include "nasmlib.h"
	#include "disasm.h"
	#include "sync.h"
}

typedef unsigned long UINT;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;

#define VCODEOFF 0x10000
#define VDATAOFF 0x80000
#define VCODESIZE 0x6A000
#define VDATASIZE 0x25B000
#define FCODEOFF 0xE00
#define FDATAOFF 0x6A000
#define FCODESIZE 0x69200
#define FDATASIZE 0x18C000
#define RELOCOFF 0x1F6800
#define RELOCSIZE 0x9A00


struct Label { UINT pos; int num; int type; };
struct From { UINT pos; Label *pLab; };
struct Reloc { UINT pos; UINT dest; };

static Label **ppCLabel;
static Label **ppDLabel;
static Label *pCLabel;
static Label *pDLabel;
static int numDLabs, numCLabs;
static int firstDLab, firstCLab;

static Label *InsertCLabel (UINT pos, int type)
{
	static int last = 0;
	while (ppCLabel[last]->pos < pos) last ++;
	while (ppCLabel[last-1]->pos >= pos) last --;
	if (ppCLabel[last]->pos == pos) return ppCLabel[last];

	int i; for (i=numCLabs; i>last; i--) ppCLabel[i] = ppCLabel[i-1];

	ppCLabel[last] = pCLabel + numCLabs;
	pCLabel[numCLabs].pos = pos;
	pCLabel[numCLabs].type = type;

	numCLabs++;
	return ppCLabel[last];
}

static Label *InsertDLabel (UINT pos, int type)
{
	static int last = 0;
	while (ppDLabel[last]->pos < pos) last ++;
	while (ppDLabel[last-1]->pos >= pos) last --;
	if (ppDLabel[last]->pos == pos) return ppDLabel[last];

	int i; for (i=numDLabs; i>last; i--) ppDLabel[i] = ppDLabel[i-1];

	ppDLabel[last] = pDLabel + numDLabs;
	pDLabel[numDLabs].pos = pos;
	pDLabel[numDLabs].type = type;

	numDLabs++;
	return ppDLabel[last];
}

static int RelocCmp (const void *e1, const void *e2)
{
	Reloc *r1 = (Reloc *)e1;
	Reloc *r2 = (Reloc *)e2;
	return (int(r1->pos) - int(r2->pos));
}



static void WriteData (FILE *pOut, UCHAR *pImage, 
	int offset, int end, From *&pNF)
{
	if (pImage[end-1] == 0x0 && end <= pNF->pos) 
	{
		int i, j, s = 0, n = 0;
		for (i=offset; i<end; i++)
		{
			if (i == end-1)
			{
				if ((3*n)/2 < end-offset) break;
				i = offset;
				if (pImage[i]>=0x20 && pImage[i]<0x7e)
					{ fprintf (pOut, "\tdb '%c", pImage[i++]); s=1; }
				else fprintf (pOut, "\tdb 0x%x", pImage[i++]);

				while (i<end)
				{
					if (pImage[i]>=0x20 && pImage[i]<0x7e 
						&& pImage[i] != 0x27)
					{
						if (s==0) { fprintf (pOut, ", '"); s=1; }
						fputc (int(pImage[i++]), pOut);
					}
					else {
						if (s==1) { fprintf (pOut, "'"); s=0; }
						fprintf (pOut, ", 0x%x", pImage[i++]);
					}
				}
				fprintf (pOut, "\n");
				return;
			}
			if (pImage[i] == 0xff || pImage[i] == 0xfd
				|| pImage[i] == 0xfe || pImage[i] == 0x92) continue;
			if (pImage[i]>=0x20 && pImage[i]<0x7e) n++;
			if (pImage[i] > 0x7F) break;

		}
	}

	int bytes = 0;
	while (offset < end)
	{
		if (offset == pNF->pos) 
		{
			if (bytes) { fprintf (pOut, "\n"); bytes = 0; }
			switch (pNF->pLab->type) {
				case 0: fprintf (pOut, "\tdd JUMP_"); break;
				case 1: fprintf (pOut, "\tdd DATA_"); break;
				case 2: fprintf (pOut, "\tdd FUNC_"); break;
			}
			fprintf (pOut, "%06i\n", pNF->pLab->num);
			offset += 4; pNF ++; continue;
		}

		if (bytes == 8) { fprintf (pOut, "\n"); bytes = 0; }
		if (!bytes) fprintf (pOut, "\tdb 0x%x", pImage[offset]);
		else fprintf (pOut, ", 0x%x", pImage[offset]);
		bytes ++; offset ++;
	}
	if (bytes) fprintf (pOut, "\n");
}


int main(int argc, char **argv)
{
	int i, j, k;
	init_sync();

	if (argc!=4) {
		fprintf (stderr, "Wrong number of arguments\n");
		fprintf (stderr, "Syntax: ffedis [logfile] [textfile] [datafile]\n");
		return 0;
	}

	FILE *pFile = fopen ("firstenc.exe", "rb");
	if (pFile==NULL) {
		fprintf (stderr, "File firstenc.exe not found\n");
		return 0;
	}
	FILE *pLog = fopen (argv[1], "wt");
	if (pLog==NULL) {
		fprintf (stderr, "Couldn't open file %s\n", argv[1]);
		return 0;
	}
	FILE *pOut = fopen (argv[2], "wt");
	if (pOut==NULL) {
		fprintf (stderr, "Couldn't open file %s\n", argv[2]);
		return 0;
	}
	FILE *pOut2 = fopen (argv[3], "wt");
	if (pOut2==NULL) {
		fprintf (stderr, "Couldn't open file %s\n", argv[3]);
		return 0;
	}
	

	// malloc image/reloc space and load from file

	UCHAR *pImage = (UCHAR *)malloc (VCODESIZE+VDATASIZE);
	memset (pImage, 0, VCODESIZE+VDATASIZE);

	fseek (pFile, FCODEOFF, SEEK_SET);
	fread (pImage+VCODEOFF, 1, FCODESIZE, pFile);

	fseek (pFile, FDATAOFF, SEEK_SET);
	fread (pImage+VDATAOFF, 1, FDATASIZE, pFile);


	// malloc and prepare label space

	pCLabel = (Label *)malloc(400000*12);	// Jumps + numRelocs
	pDLabel = (Label *)malloc(200000*12);	// Can't be greater than numRelocs?
	ppCLabel = (Label **)malloc(400000*4);	// Jumps + calls + numRelocs
	ppDLabel = (Label **)malloc(200000*4);	// Jumps + calls + numRelocs

	pCLabel[0].pos = VCODEOFF; pCLabel[0].type = 2;
	pCLabel[1].pos = 0x7fffffff; pCLabel[1].type = 2;
	ppCLabel[0] = pCLabel; ppCLabel[1] = pCLabel+1;
	numCLabs = 2;

	pDLabel[0].pos = VDATAOFF; pDLabel[0].type = 1;
	pDLabel[1].pos = 0x7fffffff; pDLabel[1].type = 1;
	ppDLabel[0] = pDLabel; ppDLabel[1] = pDLabel+1;
	numDLabs = 2;


	Reloc *pReloc = (Reloc *)malloc(20000*sizeof(Reloc));	// Actually 80000-odd
	int numRelocs = 0, nextReloc = 0;

	From *pFrom = (From *)malloc(800000*sizeof(From));		// Approx 2*CLabels?
	int numFroms = 0, nextFrom = 0;


	// Now for the relocs

	fprintf (stderr, "Applying relocs...\n");

	struct { UINT rva; UINT size; } relocHead;
	USHORT *pTempReloc = (USHORT *)malloc (0xa000);
	fseek (pFile, RELOCOFF, SEEK_SET);

	while (ftell(pFile) < RELOCOFF+RELOCSIZE)
	{
		// Fetch reloc block

		fread (&relocHead, 2, 4, pFile);
		if (relocHead.size == 0) break;
		fread (pTempReloc, 1, (relocHead.size+3-8)&-4, pFile);
		int n = (relocHead.size-8)>>1;

		for (i=0; i<n; i++)
		{
			if ((pTempReloc[i] >> 12) != 3) continue;

			// Generate pReloc entry

			UINT pos = (UINT)(pTempReloc[i] & 0xfff) + relocHead.rva;
			*(UINT *)(pImage + pos) -= 0x400000;

			pReloc[numRelocs].pos = pos;
			pReloc[numRelocs].dest = *(UINT *)(pImage + pos);
			numRelocs ++;

			fprintf (pLog, "Applied reloc to offset %x: value = %x\n", 
				pos, pReloc[numRelocs-1].dest);
		}
	}
	pReloc[numRelocs].pos = 0x7fffffff;		// Terminate reloc list
	fprintf (pLog, "NumRelocs = %i\n", numRelocs);

	qsort (pReloc, numRelocs, 8, &RelocCmp);

	// Bung in preset data labels
/*
	InsertCLabel (0x17c44, 1);
	InsertCLabel (0x17c48, 1);
	InsertCLabel (0x17c4c, 1);
	InsertCLabel (0x17c54, 1);
	InsertCLabel (0x17c5c, 1);
	InsertCLabel (0x17c64, 1);

	InsertCLabel (0x18694, 1);
	InsertCLabel (0x1869c, 1);

	InsertCLabel (0x6f834, 1);

	InsertCLabel (0x72cc5, 1);
	InsertCLabel (0x72cdc, 1);
	InsertCLabel (0x72cf8, 1);
	InsertCLabel (0x72d14, 1);

	InsertCLabel (0x77388, 1);
	InsertCLabel (0x774ec, 1);
	InsertCLabel (0x774f8, 1);
	InsertCLabel (0x77504, 1);
*/

	// First disassembly pass

	fprintf (stderr, "Starting first disassembly pass...\n");
	char pBuf[256];
	UINT offset = VCODEOFF;
	UINT lastprint = 0x0;
	UINT vtable = 0x0;

	while (offset < VCODEOFF+FCODESIZE)
	{
		if ((lastprint&0x1000) != (offset&0x1000)) {
			fprintf (stderr, "0x%x bytes processed\n", offset-VCODEOFF);
			lastprint = offset;
		}
		UINT thisJump = 0; int type = 0;	// jump

		// Data section skips
/*
		if (offset >= 0x17c44 && offset < 0x17c6c) offset = 0x17c6c;
		if (offset >= 0x18694 && offset < 0x186a4) offset = 0x186a4;
		if (offset >= 0x6f834 && offset < 0x6f838) offset = 0x6f838;
		if (offset >= 0x72cc5 && offset < 0x72d5c) offset = 0x72d5c;
		if (offset >= 0x77388 && offset < 0x77390) offset = 0x77390;
		if (offset >= 0x774ec && offset < 0x77508) offset = 0x77508;
*/
		if (vtable != 0)
		{
			fprintf (pLog, "VTable found at %x\n", vtable);
			UINT lastReloc = pReloc[nextReloc--].pos - 4;
			while (pReloc[++nextReloc].pos == lastReloc + 4)
			{
				Label *pLabel = InsertCLabel (pReloc[nextReloc].dest, 0);
				lastReloc += 4;
			}
			offset = lastReloc + 4;
			vtable = 0; continue;
		}

		int lendis = disasm (pImage+offset, pBuf, 32, offset, 1, 0);
		if (!lendis) lendis = eatbyte (pImage+offset, pBuf);



		if (lendis+offset > pReloc[nextReloc].pos) 
		{
			thisJump = pReloc[nextReloc++].dest;
			if (thisJump >= VDATAOFF) type = 1;
			else {
				if (pImage[offset] == 0xFF && pImage[offset+1] == 0x24) {
					vtable = thisJump; type = 5;
				} else {
					if (pImage[offset] == 0x8A && (pImage[offset+1] == 0x80
						|| pImage[offset+1] == 0x92)) type = 1;
					else type = 2;
				}
			}
		}
		else if (pImage[offset] == 0xE8) {
			thisJump = offset + 5 + *(int *)(pImage+offset+1);
			type = 2;
		}	
		else if (pImage[offset] == 0xEB) {
			thisJump = offset + 2 + *(char *)(pImage+offset+1);	
		}
		else if (pImage[offset] == 0xE9) {
			thisJump = offset + 5 + *(int *)(pImage+offset+1);
		}
		else if (pImage[offset] >= 0x70 && pImage[offset] <= 0x7f) {
			thisJump = offset + 2 + *(char *)(pImage+offset+1);
		}
		else if (pImage[offset] == 0x0F && pImage[offset+1] >= 0x80 &&
			pImage[offset+1] <= 0x8f) {
			thisJump = offset + 6 + *(int *)(pImage+offset+2);
		}

		while (thisJump != 0)
		{
			if (thisJump >= VCODEOFF && thisJump < VCODEOFF+FCODESIZE) {
				Label *pLabel = InsertCLabel (thisJump, type);
			}
			else if ((type&1) && thisJump >= VDATAOFF && thisJump < VDATASIZE+VDATAOFF) {
				Label *pLabel = InsertDLabel (thisJump, type);
			}

			// Possible second reloc...

			if (lendis+offset > pReloc[nextReloc].pos)
			{
				thisJump = pReloc[nextReloc++].dest;
				if (thisJump >= VDATAOFF) type = 1;
				else type = 2;
			}
			else break;
		}
		offset += lendis;
	}
	InsertCLabel (VCODEOFF+FCODESIZE, 2);		// Terminate code label list

	// Data processing

	while (nextReloc < numRelocs)
	{
		offset = pReloc[nextReloc].pos;
		if ((lastprint&0x1000) != (offset&0x1000)) {
			fprintf (stderr, "0x%x bytes processed\n", offset-VCODEOFF);
			lastprint = offset;
		}
		
		if (pReloc[nextReloc].dest < VDATAOFF)
			Label *pLabel = InsertCLabel (pReloc[nextReloc].dest, 2);
		else Label *pLabel = InsertDLabel (pReloc[nextReloc].dest, 1);
		nextReloc++;
	}


	// Pass 1.5 :-)

	fprintf (stderr, "Starting second disassembly pass...\n");
	nextReloc = 0; 

	Label **ppTempLabel = (Label **)malloc (numCLabs*4);
	memcpy (ppTempLabel, ppCLabel, numCLabs*4);
	int numTempLabs = numCLabs;

	for (i=0; i<numTempLabs; i++)
	{
		Label *pCurLab = ppTempLabel[i];
		Label *pNextLab = ppTempLabel[i+1];
		offset = pCurLab->pos;
		if (offset >= VCODEOFF+FCODESIZE) break;
		if ((lastprint&0x1000) != (offset&0x1000)) {
			fprintf (stderr, "0x%x bytes processed\n", offset-VCODEOFF);
			lastprint = offset;
		}

		if (pCurLab->type == 1 || pCurLab->type == 5) {
			while (pReloc[nextReloc].pos < pNextLab->pos)
			{
				Label *pLabel = InsertCLabel (pReloc[nextReloc].dest, 0);
				pFrom[numFroms].pos = pReloc[nextReloc].pos;
				pFrom[numFroms++].pLab = pLabel;
				pReloc[nextReloc++].dest = 0;
			}
			continue;
		}

		while (1)
		{
			int lendis = disasm (pImage+offset, pBuf, 32, offset, 1, 0);
			if (!lendis) lendis = eatbyte (pImage+offset, pBuf);
			if (lendis+offset > pNextLab->pos) break;

			UINT thisJump = 0; int type = 0;	// jump

			if (lendis+offset > pReloc[nextReloc].pos) {
				thisJump = pReloc[nextReloc++].dest;
				pReloc[nextReloc-1].dest = 0; type = 1;
			}
			else if (pImage[offset] == 0xE8) {
				thisJump = offset + 5 + *(int *)(pImage+offset+1);
				type = 2;
			}
			else if (pImage[offset] == 0xEB || pImage[offset] == 0xE2
				|| pImage[offset] == 0xE3)
				thisJump = offset + 2 + *(char *)(pImage+offset+1);	
			else if (pImage[offset] == 0xE9)
				thisJump = offset + 5 + *(int *)(pImage+offset+1);
			else if (pImage[offset] >= 0x70 && pImage[offset] <= 0x7f)
				thisJump = offset + 2 + *(char *)(pImage+offset+1);
			else if (pImage[offset] == 0x0F && pImage[offset+1] >= 0x80 &&
				pImage[offset+1] <= 0x8f)
				thisJump = offset + 6 + *(int *)(pImage+offset+2);


			while (thisJump != 0)
			{
				if (thisJump >= VCODEOFF && thisJump < VCODEOFF+FCODESIZE) 
				{
					Label *pLabel = InsertCLabel (thisJump, type);
					pFrom[numFroms].pos = offset;
					pFrom[numFroms].pLab = pLabel;
					numFroms++;
				}
				else if (thisJump >= VDATAOFF && thisJump < VDATAOFF+VDATASIZE)
				{
					Label *pLabel = InsertDLabel (thisJump, type);
					pFrom[numFroms].pos = offset;
					pFrom[numFroms].pLab = pLabel;
					numFroms++;
				}

				// Possible second reloc...

				if (lendis+offset > pReloc[nextReloc].pos)
				{
					thisJump = pReloc[nextReloc++].dest;
					pReloc[nextReloc-1].dest = 0;
					if (thisJump >= VDATAOFF) type = 1;
					else type = 2;
				}
				else break;
			}
			offset += lendis;
		}
	}
	InsertCLabel (VCODEOFF+FCODESIZE, 2);		// Terminate code label list

	// Data processing

	while (nextReloc < numRelocs)
	{
		offset = pReloc[nextReloc].pos;
		if ((lastprint&0x1000) != (offset&0x1000)) {
			fprintf (stderr, "0x%x bytes processed\n", offset-VCODEOFF);
			lastprint = offset;
		}
		
		if (pReloc[nextReloc].dest < VDATAOFF)
		{
			Label *pLabel = InsertCLabel (pReloc[nextReloc].dest, 2);
			pFrom[numFroms].pos = offset;
			pFrom[numFroms].pLab = pLabel;
			numFroms++;
		}
		else {
			Label *pLabel = InsertDLabel (pReloc[nextReloc].dest, 1);
			pFrom[numFroms].pos = offset;
			pFrom[numFroms].pLab = pLabel;
			numFroms++;
		}
		pReloc[nextReloc].dest = 0;

		nextReloc++;
	}
	InsertDLabel (VDATAOFF+VDATASIZE, 1);



	// Numerify the label lists...

	fprintf (pLog, "\n\nFunction offsets:\n");
	int d; for (i=0, j=0, k=0, d=0; i<numCLabs; i++) {
		switch (ppCLabel[i]->type&3) {
			case 0: ppCLabel[i]->num = k++; break;
			case 1: ppCLabel[i]->num = d++; break;
			case 2: ppCLabel[i]->num = j++; break;
		}
	}
	for (i=0; i<numDLabs; i++) ppDLabel[i]->num = d++;

	// Print out unused relocs up to offset

	fprintf (pLog, "\n\nUnused relocs:\n");
	for (i=0; i<numRelocs; i++) {
		if (pReloc[i].pos > offset) break;
		if (pReloc[i].dest == 0) continue;
		fprintf (pLog, "\tReloc for offset %x unused\n", pReloc[i].pos);
	}


	fprintf (pLog, "\n\nCode labels:\n");
	for (i=0; i<numCLabs; i++)
		fprintf (pLog, "\tCode label %i: position %x, type %i\n", 
			i, ppCLabel[i]->pos, ppCLabel[i]->type);

	fprintf (pLog, "\n\nData labels:\n");
	for (i=0; i<numDLabs; i++)
		fprintf (pLog, "\tData label %i: position %x, type %i\n", 
			i, ppDLabel[i]->pos, ppDLabel[i]->type);

	fprintf (pLog, "\n\nJumps and calls:\n");
	for (i=0; i<numFroms; i++)
		fprintf (pLog, "\tFrom %i: pos %x, label pos %x, label type %i\n", 
			i, pFrom[i].pos, pFrom[i].pLab->pos, pFrom[i].pLab->type);




	// Second disassembly pass

	fprintf (stderr, "Starting third disassembly pass...\n");
	From *pNF = pFrom;
	int lasttype = 1, ret = 1, jmp = 0;

	for (i=0; i<numCLabs; i++)
	{
		Label *pCurLab = ppCLabel[i];
		Label *pNextLab = ppCLabel[i+1];
		offset = pCurLab->pos;
		if (offset >= VCODEOFF+FCODESIZE) break;

		switch (pCurLab->type) {
			case 0: {	// Jump
				if (lasttype&1) fprintf (pOut, "\n\nSECTION .text\n");
				fprintf (pOut, "\tJUMP_%06i:\t\t\t; Pos = %x\n", 
					pCurLab->num, offset);
				lasttype = 0; break;
			}
			case 2: {	// Function
				if (!ret && !jmp)
					fprintf (pLog, "Warning (unret) at %x\n", offset);
				if (lasttype&1) fprintf (pOut, "\n\nSECTION .text\n");
				fprintf (pOut, "\n\n\nFUNC_%06i:\t\t\t; Pos = %x\n\n",
					pCurLab->num, offset);
				fprintf (stderr, "Processing func %i\n", pCurLab->num);
				lasttype = 2; ret = 0; break;
			}
			case 1: 
			case 5:
			{
				if (!(lasttype&1)) fprintf (pOut, "\n\nSECTION .data\n");
				fprintf (pOut, "\nDATA_%06i:\t\t\t; Pos = %x\n",
					pCurLab->num, offset);
				WriteData (pOut, pImage, offset, pNextLab->pos, pNF);
				lasttype = 1; continue;
			}
		}



		while (1)
		{
			int lendis = disasm (pImage+offset, pBuf, 32, offset, 0, 0);
			if (!lendis) lendis = eatbyte (pImage+offset, pBuf);
			if (offset+lendis > pNextLab->pos) break;

			char *pNum, *pEnd = pBuf;
			char pBuf2[255];

			if (strstr (pBuf, "ret")) ret = 1;
			if (strstr (pBuf, "jmp")) jmp = 1;
			else jmp = 0;

/*			if (int(offset) - int(pNF->pos) > 20) {
				fprintf (pLog, "Warning: From not found: pos = %x\n", offset);
				pNF++;
			}
*/
			while ((pNum = strchr (pEnd, '0')) != NULL)
			{
				UINT jumpoff = strtol (pNum, &pEnd, 16);
				if (jumpoff < VCODEOFF || jumpoff >= VDATAOFF+VDATASIZE) continue;

				if (jumpoff != pNF->pLab->pos) {
					if (jumpoff == (pNF+1)->pLab->pos) {
						fprintf (pLog, "Warning (pff) at %x\n", offset);
						pNF++;
					}
					else if (jumpoff == (pNF+2)->pLab->pos) {
						fprintf (pLog, "Warning (pff2) at %x\n", offset);
						pNF+=2;
					}
				}

				if (jumpoff == pNF->pLab->pos)
				{
					strcpy (pBuf2, pEnd);
					switch (pNF->pLab->type&3) {
						case 0: sprintf (pNum, "JUMP"); break;
						case 1: sprintf (pNum, "DATA"); break;
						case 2: sprintf (pNum, "FUNC"); break;
					}
					sprintf (pNum+4, "_%06i%s", pNF->pLab->num, pBuf2);
					pNF ++; continue;
				}
				else fprintf (pLog, "Warning (puf) at %x\n", offset);
			}

			fprintf (pOut, "\t\t%s\n", pBuf);		// Write opcode
			offset += lendis;		// Add opcode size
		}
	}

	fprintf (stderr, "Writing data file...\n");
	fprintf (pOut2, "SECTION .data\n\n");

	for (i=0; i<numDLabs; i++)
	{
		Label *pCurLab = ppDLabel[i];
		Label *pNextLab = ppDLabel[i+1];
		offset = pCurLab->pos;
		if (offset >= VDATAOFF+FDATASIZE) break;

		if ((lastprint&0x1000) != (offset&0x1000)) {
			fprintf (stderr, "0x%x bytes processed\n", offset-VDATAOFF);
			lastprint = offset;
		}

		fprintf (pOut2, "DATA_%06i:\n", pCurLab->num);
		WriteData (pOut2, pImage, offset, pNextLab->pos, pNF);
	}

	fprintf (pOut2, "\n\n\nSECTION .bss\n\n");

	for (i=i; i<numDLabs; i++)
	{
		Label *pCurLab = ppDLabel[i];
		Label *pNextLab = ppDLabel[i+1];
		int offset = pCurLab->pos;
		if (offset >= VDATAOFF+VDATASIZE) break;

		if ((lastprint&0x1000) != (offset&0x1000)) {
			fprintf (stderr, "0x%x bytes processed\n", offset-VDATAOFF);
			lastprint = offset;
		}

		fprintf (pOut2, "DATA_%06i:\n", pCurLab->num);
		fprintf (pOut2, "\tresb %i\n", pNextLab->pos - offset);
	}

	free (pImage);
	free (ppCLabel);
	free (ppDLabel);
	free (pCLabel);
	free (pDLabel);
	free (pFrom);
	free (pReloc);
	free (pTempReloc);

	fclose(pFile);
	return 0;
}

