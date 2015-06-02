#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ffecfg.h"
#include "ffeapi.h"

#include <unistd.h>
#include <dirent.h>


static char pDirAVIPathDef[] = "data";
static char pDirCmmdrPathDef[] = "commandr";
static char pDirSamplePathDef[] = "fx";
static char pDirSongPathDef[] = "music";

static char pDirAVIPath[256];
static char pDirCmmdrPathSys[256];
static char pDirCmmdrPath[256];
static char pDirSamplePath[256];
static char pDirSongPath[256];

static DIR *pDirHandle;
static int dirFirstFlag = 1;
static int dirDirsFlag = 1;


void LinuxDirFixPath (char *pInPath, char *pOutPath)
{
	int i;
	if (pInPath[0] != '/') {
		getcwd (pOutPath, 255);
		strcat (pOutPath, "/");
		strcat (pOutPath, pInPath);
	}
	i = strlen (pOutPath);	
	if (pOutPath[i-1] != '/') pOutPath[i] = '/';
}

void DirInit (void)
{
	CfgStruct cfg;
	int rv;
	char *pTok;
	char pBuf[256];

	FILE *pFile = fopen ("mission.dat", "rb");
	if (pFile == NULL) chdir ("game");
	else fclose (pFile);

	CfgOpen (&cfg, __CONFIGFILE__);
	CfgFindSection (&cfg, "DIR");

	rv = CfgGetKeyStr (&cfg, "AVIPath", pBuf, 255);
	if (rv != 0) {
		pTok = strtok (pBuf, " \n\t");
		strcpy (pBuf, pTok);
		LinuxDirFixPath (pBuf, pDirAVIPath);
	}
	else LinuxDirFixPath (pDirAVIPathDef, pDirAVIPath);

	rv = CfgGetKeyStr (&cfg, "CmmdrPath", pBuf, 255);
	if (rv != 0) {
		pTok = strtok (pBuf, " \n\t");
		strcpy (pBuf, pTok);
		LinuxDirFixPath (pBuf, pDirCmmdrPath);
	}
	else LinuxDirFixPath (pDirCmmdrPathDef, pDirCmmdrPath);

	rv = CfgGetKeyStr (&cfg, "SamplePath", pBuf, 255);
	if (rv != 0) {
		pTok = strtok (pBuf, " \n\t");
		strcpy (pBuf, pTok);
		LinuxDirFixPath (pBuf, pDirSamplePath);
	}
	else LinuxDirFixPath (pDirSamplePathDef, pDirSamplePath);

	rv = CfgGetKeyStr (&cfg, "SongPath", pBuf, 255);
	if (rv != 0) {
		pTok = strtok (pBuf, " \n\t");
		strcpy (pBuf, pTok);
		LinuxDirFixPath (pBuf, pDirSongPath);
	}
	else LinuxDirFixPath (pDirSongPathDef, pDirSongPath);


	// Test for directory presence

	getcwd (pBuf, 255);
	if (chdir (pDirCmmdrPath) != 0) {
		getcwd (pDirCmmdrPath, 255);
		strcat (pDirCmmdrPath, "/");
	}
	strcpy (pDirCmmdrPathSys, pDirCmmdrPath);
	chdir (pBuf);

	// Read drive letters

	CfgClose (&cfg);
}

void DirCleanup (void)
{
}

char *DirMakeAVIName (char *pBuf, char *pStub)
{
	strcpy (pBuf, pDirAVIPath);
	strcat (pBuf, pStub);
	strcat (pBuf, ".avi");
	return pBuf;
}

char *DirMakeSongName (char *pBuf, char *pFilename)
{
	strcpy (pBuf, pDirSongPath);
	strcat (pBuf, pFilename);
	return pBuf;
}

char *DirMakeSampleName (char *pBuf, char *pFilename)
{
	strcpy (pBuf, pDirSamplePath);
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

void LinuxDirGetFileInfo (FileInfo *pInfo)
{
	char pTemp[256], pTemp2[256];
	getcwd (pTemp, 255);
	strcpy (pTemp2, pDirCmmdrPath);
	strcat (pTemp2, pInfo->pName);
	if (chdir (pTemp2) != 0)
	{
		FILE *pFile = fopen (pTemp2, "rb");
		if (pFile != NULL) {
			fseek (pFile, 0, SEEK_END);
			pInfo->size = ftell (pFile);
			fclose (pFile);
		}
		else pInfo->size = 0;
		pInfo->type = 0;
	}
	else {
		pInfo->type = 2;
		pInfo->size = 0;
	}
	chdir (pTemp);
}

int DirFindFirst (FileInfo *pFile)
{
	dirFirstFlag = 1;
	dirDirsFlag = 1;

	if (!strcmp(pDirCmmdrPath, "/")) {
		return DirFindNext (pFile);
	}
	else {
		strcpy (pFile->pName, "(Parent Dir)/"); 
		pFile->type = 4; pFile->size = 0;
	}
	return 1;
}

int DirFindNext (FileInfo *pFile)
{
	struct dirent *pDirent;

	while (1) {
		if (dirFirstFlag == 1) {
			pDirHandle = opendir (pDirCmmdrPath);
			dirFirstFlag = 0;
		}
		pDirent = readdir (pDirHandle);
		if (pDirent == NULL) {
			closedir (pDirHandle);
			if (dirDirsFlag==0) return 0;
			else { dirDirsFlag = 0;	dirFirstFlag = 1; continue; }
		}
		strncpy (pFile->pName, pDirent->d_name, 15);
		LinuxDirGetFileInfo (pFile);
		if (dirDirsFlag && pFile->type==0) continue;
		if (!dirDirsFlag && pFile->type==2) continue;
		if (pFile->pName[0] == '.') continue;
		break;
	}
	if (pFile->type == 2) strcat (pFile->pName, "/");
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
		pTemp = strrchr (pDirCmmdrPath, '/');
		if (pTemp != 0) pTemp[1] = 0;
		else pDirCmmdrPath[i-1] = '/';
	}
}
