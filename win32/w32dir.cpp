#include <windows.h>	// messagebox
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ffecfg.h"
#include "ffeapi.h"

#include <direct.h>
#include <io.h>

static char pDirAVIPathDef[] = "DATA\\";
static char pDirCmmdrPathDef[] = "COMMANDR\\";
static char pDirSamplePathDef[] = "FX\\";
static char pDirSongPathDef[] = "MUSIC\\";

static char pDirAVIPath[256];
static char pDirCmmdrPathSys[256];
static char pDirCmmdrPath[256];
static char pDirSamplePath[256];
static char pDirSongPath[256];

static struct _finddata_t DirFFBlk;
static long dirFFHandle;
static FileInfo pDirFFDrives[26];
static int dirFFNumDrives = 0;
static int dirFFCurDrive = 0;
static int dirFirstFlag = 1;
static int dirDirsFlag = 1;

static FILE *pLog = NULL;

void WinDirAddDrive (char *pName, int type)
{
	FileInfo *pCur = pDirFFDrives+dirFFNumDrives;
	strcpy (pCur->pName, pName);
	pCur->type = type;
	pCur->size = 0;
	dirFFNumDrives++;
}

void WinDirFixPath (char *pInPath, char *pOutPath)
{
	char pBuf[256], *pTok;
	if (pInPath[1] != ':') {
		_getcwd (pBuf, 255);
		strcat (pBuf, "\\");
		strcat (pBuf, pInPath);
	}
	else strcpy (pBuf, pInPath);

	strcpy (pOutPath, strtok (pBuf, "\\/"));
	strcat (pOutPath, "\\");

	while ((pTok = strtok (NULL, "\\/")) != NULL) {
		strcat (pOutPath, pTok);
		strcat (pOutPath, "\\");
	}
}

extern "C" void DirInit (void)
{
	CfgStruct cfg;
	int rv;
	char *pTok;
	char pBuf[256];
	char pBuf2[256];

	// Working directory hack for shareware version

	if (_access ("mission.dat", 4) != 0) {
		MessageBox (NULL, "Warning - JJFFE not installed correctly",
			"WinFFE startup error", MB_OK | MB_ICONWARNING);
		_chdir ("game");
	}

	CfgOpen (&cfg, __CONFIGFILE__);
	CfgFindSection (&cfg, "DIR");

	rv = CfgGetKeyStr (&cfg, "AVIPath", pBuf, 255);
	if (rv != 0) {
		pTok = strtok (pBuf, " \n\t");
		strcpy (pBuf, pTok);
		WinDirFixPath (pBuf, pDirAVIPath);
	}
	else WinDirFixPath (pDirAVIPathDef, pDirAVIPath);

	rv = CfgGetKeyStr (&cfg, "CmmdrPath", pBuf, 255);
	if (rv != 0) {
		pTok = strtok (pBuf, " \n\t");
		strcpy (pBuf, pTok);
		WinDirFixPath (pBuf, pDirCmmdrPath);
	}
	else WinDirFixPath (pDirCmmdrPathDef, pDirCmmdrPath);

	rv = CfgGetKeyStr (&cfg, "SamplePath", pBuf, 255);
	if (rv != 0) {
		pTok = strtok (pBuf, " \n\t");
		strcpy (pBuf, pTok);
		WinDirFixPath (pBuf, pDirSamplePath);
	}
	else WinDirFixPath (pDirSamplePathDef, pDirSamplePath);

	rv = CfgGetKeyStr (&cfg, "SongPath", pBuf, 255);
	if (rv != 0) {
		pTok = strtok (pBuf, " \n\t");
		strcpy (pBuf, pTok);
		WinDirFixPath (pBuf, pDirSongPath);
	}
	else WinDirFixPath (pDirSongPathDef, pDirSongPath);

	CfgClose (&cfg);

	// Test for directory presence

	_getcwd (pBuf, 255);
	if (_chdir (pDirCmmdrPath) != 0) {
		WinDirFixPath (pBuf, pDirCmmdrPath);
	}
	strcpy (pDirCmmdrPathSys, pDirCmmdrPath);

	// Read drive letters

	WinDirAddDrive ("A:\\", 6);
	strcpy (pBuf2, "C:\\");
	while (_chdir (pBuf2) == 0) {
		WinDirAddDrive (pBuf2, 6);
		pBuf2[0]++;
	}
	_chdir (pBuf);

}

extern "C" void DirCleanup (void)
{
}

extern "C" {

char *DirMakeAVIName (char *pBuf, char *pStub)
{
	strcpy (pBuf, pDirAVIPath);
	strcat (pBuf, pStub);
	strcat (pBuf, ".avi");
	return pBuf;
}

char *DirMakeSampleName (char *pBuf, char *pFilename)
{
	strcpy (pBuf, pDirSamplePath);
	strcat (pBuf, pFilename);
	return pBuf;
}

char *DirMakeSongName (char *pBuf, char *pFilename)
{
	strcpy (pBuf, pDirSongPath);
	strcat (pBuf, pFilename);
	return pBuf;
}

char *DirMakeCmmdrName (char *pBuf, char *pFilename)
{
	strcpy (pBuf, pDirCmmdrPath);
	strcat (pBuf, pFilename);
	return pBuf;
}



char *DirGetCmmdrPath (void)
{
	return pDirCmmdrPath;
}

void DirResetCmmdrPath (void)
{
	strcpy (pDirCmmdrPath, pDirCmmdrPathSys);
}

int DirFindFirst (FileInfo *pFile)
{
	*pFile = pDirFFDrives[0];
	dirFFCurDrive = 1;
	dirFirstFlag = 1;
	dirDirsFlag = 1;
	return 1;
}

int DirFindNext (FileInfo *pFile)
{
	int rv;

	if (dirFFCurDrive < dirFFNumDrives) {
		*pFile = pDirFFDrives[dirFFCurDrive++];
		return 1;
	}

	while (1) {
		if (dirFirstFlag == 1) {
			char pTemp[256];
			strcpy (pTemp, pDirCmmdrPath);
			strcat (pTemp, "*.*");
			rv = _findfirst (pTemp, &DirFFBlk);
			dirFFHandle = rv;
			dirFirstFlag = 0;
		}
		else {
			rv = _findnext (dirFFHandle, &DirFFBlk);
		}
		if (rv == -1) {
			if (dirFFHandle!=-1) _findclose (dirFFHandle);
			if (dirDirsFlag==0) return 0;
			else { dirDirsFlag = 0; dirFirstFlag = 1; continue; }
			continue;
		}
		if (dirDirsFlag && !(DirFFBlk.attrib & _A_SUBDIR)) continue;
		if (!dirDirsFlag && (DirFFBlk.attrib & _A_SUBDIR)) continue;
		if (!strcmp (DirFFBlk.name, ".")) continue;
		break;
	} 

	strncpy (pFile->pName, DirFFBlk.name, 16);
	pFile->size = DirFFBlk.size;
	if (DirFFBlk.attrib & _A_SUBDIR) {
		pFile->type = 2;
		strcat (pFile->pName, "\\");
	}
	else pFile->type = 0;

	if (!strcmp (pFile->pName, "..\\")) {
		strcpy (pFile->pName, "(Parent Dir)\\");
		pFile->type = 4;
	}

	return 1;
}

void DirNavigateTree (FileInfo *pFile)
{
	if (pFile->type == 2) {
		strcat (pDirCmmdrPath, pFile->pName);
	}
	if (pFile->type == 4) {
		char *pTemp;
		int i = strlen (pDirCmmdrPath);
		pDirCmmdrPath[i-1] = 0;
		pTemp = strrchr (pDirCmmdrPath, '\\');
		if (pTemp != 0) pTemp[1] = 0;
		else pDirCmmdrPath[i-1] = '\\';
	}
	if (pFile->type == 6) {
		strcpy (pDirCmmdrPath, pFile->pName);
	}
}

}