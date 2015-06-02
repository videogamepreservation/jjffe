#include "ffeapi.h"
#include "win32api.h"
#include "ffecfg.h"
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <windows.h>
#include <winbase.h>	// QueryPerformanceCounter
#include <math.h>
#include <io.h>		// _access()

#define DIRECTSOUND_VERSION 0x300
#include <dsound.h>
#include <dmusicc.h>
#include <dmusici.h>
#include <dmerror.h>

#include <stdio.h>
static FILE *pLog = NULL;


typedef struct {
	char pName[16];
	int loopcount;
	int pitchmod;
	int voice;
	int flags;
} FFESample;

const int soundNumSamples = 0x28;
#define SOUNDNUMSAMPLES 0x28

FFESample pSoundSampleDescs[SOUNDNUMSAMPLES] = {
	{ "nowt", 0, 0, -1, 0 },
	{ "nowt", 0, 0, -1, 0 },
	{ "nowt", 0, 0, -1, 0 },
	{ "nowt", 0, 0, -1, 0 },
	{ "nowt", 0, 0, -1, 0 },		// 0x4
	{ "shipexp", 0, 0, -1, 0 },
	{ "missexpl", 0, 0, -1, 0 },
	{ "nowt", 0, 0, -1, 0 },
	{ "nowt", 0, 0, -1, 0 },	// 0x8
	{ "nowt", 0, 0, -1, 0 },
	{ "laser1", 0, 0, -1, 0 },
	{ "laser2", 0, 0, -1, 0 },
	{ "airopen", 0, 0, -1, 0 },		// 0xc
	{ "airopen", 0, 0, -1, 0 },
	{ "wind", 0, -0x4000, 0, 0 },
	{ "station", 0x32, 0, 1, 0 },
	{ "station", 0x32, 0x1000, 2, 0 },	// 0x10
	{ "nowt", 0, 0, -1, 0 },
	{ "airopen", 0, 0, -1, 0 },
	{ "airopen", 0, 0, -1, 0 },
	{ "bip", 0, 0, -1, 0 },			// 0x14
	{ "bip", 0, 0, -1, 0 },
	{ "bong", 0, 0, -1, 0 },
	{ "chime", 0, 0, -1, 0 },
	{ "nowt", 0, 0, -1, 0 },		// 0x18
	{ "ecm", 2, 0, 3, 0 },
	{ "necm", 2, 0, 4, 0 },
	{ "shipexp", 0, 0, -1, 0 },
	{ "missexpl", 0, 0, -1, 0 },	// 0x1c
	{ "damage", 0, 0, -1, 0 },
	{ "siren", 3, 0, 5, 0 },
	{ "boom", 0, 0, 6, 0 },
	{ "launch", 0, 0, -1, 0 },		// 0x20
	{ "hype", 0, 0, -1, 0 },
	{ "nowt", 0, 0, -1, 0 },
	{ "laser3", 0, 0, -1, 0 },
	{ "laser4", 0, 0, -1, 0 },		// 0x24
	{ "wind", 0, -0x4000, 7, 0 },
	{ "nowt", 0, 0, -1, 0 },
	{ "camera", 0, 0, -1, 0 }
	};

const int soundNumSongs = 17;
#define SOUNDNUMSONGS 17

char ppSoundSongNames[SOUNDNUMSONGS][16] = {
	{ "qqfront" },
	{ "qqatmosp" },
	{ "qqtravel" },
	{ "qqdrama" },
	{ "qqsuspen" },
	{ "qqrock" },
	{ "qqescape" },
	{ "qqparadi" },
	{ "qqfront2" },
	{ "hallking" },
	{ "intro" },
	{ "babayaga" },
	{ "barem" },
	{ "jupalt" },
	{ "valkries" },
	{ "bluedan" },
	{ "ggofkiev" }
	};


// Digital sound vars

#define NUMCHANNELS 16

LPDIRECTSOUND pDSound = NULL;
WAVEFORMATEX waveformat;
LPDIRECTSOUNDBUFFER ppSampleBuffer[SOUNDNUMSAMPLES];
UCHAR *ppSampleData[soundNumSamples];
int pSampleLen[soundNumSamples];

LPDIRECTSOUNDBUFFER ppChanBuffer[NUMCHANNELS];
int pChannelUsage[NUMCHANNELS];
int usedchannels = 0;

// Music vars

UINT midiDeviceID = -1;

static int softsynth = 0;
static int forcemci = 0;
static int win2khack = 0;

IDirectMusic *pDMusic = NULL;
IDirectMusicPerformance *pDMPerf = NULL;
IDirectMusicPort *pDMPort = NULL;
IDirectMusicLoader *pDMLoader = NULL;
IDirectMusicSegment *pDMSegment = NULL;
static ULONG timeout = 0;

int DMInit()
{
	GUID synthGUID;
	HRESULT result;
    DMUS_PORTPARAMS dmpp;
	DMUS_PORTCAPS dmpc;
	DWORD i;
    WCHAR pwDir[256];
	char pDir[256];

	if (FAILED(CoInitialize(NULL))) return 0;

    result = CoCreateInstance (CLSID_DirectMusicPerformance, NULL,
		CLSCTX_INPROC, IID_IDirectMusicPerformance, (void**)(&pDMPerf));
    if (FAILED(result)) return 0;
    result = pDMPerf->Init (&pDMusic, pDSound, NULL);
    if (FAILED(result)) return 0;

	memset (&dmpp, 0, sizeof(dmpp));
	dmpp.dwSize = sizeof(dmpp);
	synthGUID = GUID_NULL;

	if (!softsynth) for (i=0; ; i++)
	{
		memset(&dmpc, 0, sizeof(dmpc));
		dmpc.dwSize = sizeof(dmpc);
		result = pDMusic->EnumPort (i, &dmpc);
		if (result != S_OK) break;

char pBuf[256];
wcstombs (pBuf, dmpc.wszDescription, 256);
fprintf (pLog, "Port name = %s\n", pBuf);
if (dmpc.dwClass == DMUS_PC_OUTPUTCLASS) fprintf (pLog, "\tClass: Output\n");
else fprintf (pLog, "\tClass: Input\n");
if (dmpc.dwFlags & DMUS_PC_EXTERNAL) fprintf (pLog, "\tFlags: External\n");
if (dmpc.dwFlags & DMUS_PC_SOFTWARESYNTH) fprintf (pLog, "\tFlags: Softsynth\n");
if (dmpc.dwType == DMUS_PORT_WINMM_DRIVER) fprintf (pLog, "\tType: WinMM\n");
else if (dmpc.dwType == DMUS_PORT_KERNEL_MODE) fprintf (pLog, "\tType: Kernel\n");
else fprintf (pLog, "\tType: User softsynth\n");

		if ((dmpc.dwClass != DMUS_PC_OUTPUTCLASS) ||
			(dmpc.dwFlags & DMUS_PC_EXTERNAL) ||
			(dmpc.dwFlags & DMUS_PC_SOFTWARESYNTH)) continue;
		synthGUID = dmpc.guidPort;
		break;
    }

	result = pDMusic->CreatePort (synthGUID, &dmpp, &pDMPort, NULL);
	if (FAILED(result)) return 0;

	pDMPort->Activate (true);
	result = pDMPerf->AddPort (pDMPort);
	if (FAILED(result)) return 0;
	result = pDMPerf->AssignPChannelBlock (0, pDMPort, 1);
	if (FAILED(result)) return 0;

	result = CoCreateInstance (CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, 
		IID_IDirectMusicLoader, (void**)(&pDMLoader));
	if (FAILED(result)) return 0;

	mbstowcs (pwDir, DirMakeSongName (pDir, ""), 256);
	result = pDMLoader->SetSearchDirectory (GUID_DirectMusicAllTypes,
		pwDir, false);
    if (FAILED(result)) return 0;
	pDMLoader->EnableCache (GUID_DirectMusicAllTypes, true);//do not touch
	pDMLoader->ScanDirectory (GUID_DirectMusicAllTypes, L"*", NULL);

	result = pDMusic->Activate (true);
    if (FAILED(result)) return 0;

	return 1;
}

void DMStopSong()
{
	if (!pDMSegment) return;
	pDMPerf->Stop (NULL, NULL, 0, 0);
	pDMSegment->Release();
	pDMSegment = NULL;
}

int DMSongDone()
{
	HRESULT result;
	if (!pDMSegment) return 1;
	if (timeout > TimerGetTimeStamp()) return 0;

	result = pDMPerf->IsPlaying (pDMSegment, NULL);
	if (result == S_OK) return 0;
	DMStopSong ();
	return 1;
}


extern "C" void *load_hmp (void *buf, unsigned long insize, unsigned long *outsize);

static int ConvertHMP (char *pHmpName, char *pMidiName)
{
	FILE *pHmp, *pMidi;
	void *pHmpImage, *pMidiImage;
	unsigned long hmpsize, midisize;

fprintf (pLog, "%s not found, converting from HMP version\n", pMidiName);
	pHmp = fopen (pHmpName, "rb");
	if (pHmp == 0) {
fprintf (pLog, "%s not found, can't play music\n", pHmpName);
		return 0;
	}
	pMidi = fopen (pMidiName, "wb");
	if (pMidi == 0) {
fprintf (pLog, "%s couldn't be opened for writing\n", pMidiName);
		fclose (pHmp);
		return 0;
	}
	

	fseek (pHmp, 0, SEEK_END);
	hmpsize = ftell (pHmp);
	fseek (pHmp, 0, SEEK_SET);
	pHmpImage = malloc (hmpsize);
	fread (pHmpImage, 1, hmpsize, pHmp);

	pMidiImage = load_hmp (pHmpImage, hmpsize, &midisize);
	fwrite (pMidiImage, 1, midisize, pMidi);

	fclose (pHmp);
	fclose (pMidi);
	free (pHmpImage);
	free (pMidiImage);
	return 1;
}

void DMPlaySong (int index)
{
    DMUS_OBJECTDESC dmod;
	HRESULT result;
    IDirectMusicObject *pDMObject;
	char pBuf[256], pBuf2[256], pBuf3[256];

	if (!DMSongDone()) return;

	strcpy (pBuf, ppSoundSongNames[index]);
	strcat (pBuf, ".mid");
	DirMakeSongName (pBuf3, ppSoundSongNames[index]);
	strcat (pBuf3, ".mid");
	if (_access (pBuf3, 4) != 0)		// Check for existence of .mid version
	{
		DirMakeSongName (pBuf2, ppSoundSongNames[index]);
		strcat (pBuf2, ".hmp");
		if (!ConvertHMP (pBuf2, pBuf3)) return;
	}

	memset (&dmod, 0, sizeof(dmod));
	dmod.dwSize = sizeof(dmod);
	dmod.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;
	dmod.guidClass = CLSID_DirectMusicSegment;
	mbstowcs (dmod.wszFileName, pBuf, DMUS_MAX_FILENAME);	

	result = pDMLoader->GetObject (&dmod, IID_IDirectMusicSegment,
		(void**)(&pDMObject));
	if (FAILED(result)) return;
    pDMObject->QueryInterface (IID_IDirectMusicSegment, (void**)(&pDMSegment));
    pDMObject->Release();

	pDMSegment->SetParam (GUID_StandardMIDIFile, -1, 0, 0, (void*)pDMPerf);
 	pDMSegment->SetParam (GUID_Download, -1, 0, 0, (void*)pDMPerf);
	result = pDMPerf->PlaySegment (pDMSegment, DMUS_SEGF_DEFAULT, 0, NULL);
    if (FAILED(result)) DMStopSong();

	timeout = TimerGetTimeStamp() + 200;
}


void DMCleanup()
{
	if (pDMusic)
	{
		DMStopSong();
		pDMusic->Activate (false);
		if (pDMLoader) { pDMLoader->Release(); pDMLoader = NULL; }
		if (pDMPort) {
			pDMPort->Activate (false);
			pDMPerf->Stop (NULL, NULL, 0, 0);
			pDMPerf->RemovePort (pDMPort);
			pDMPort->Release();
			pDMPort = NULL;
		}
		if (pDMPerf) {
			pDMPerf->CloseDown();
			pDMPerf->Release();
			pDMPerf = NULL;
		}
		pDMusic->Release();
		pDMusic = NULL;
	}
}

//static char mci_temp[MAX_PATH] = {0};

extern "C" void SoundPlaySong (long index)
{
	char pBuf[256], pBuf2[255];
	int rval;

	if (pDMusic) { DMPlaySong (index); return; }

	MCI_OPEN_PARMS mciOpenParms;
	MCI_PLAY_PARMS mciPlayParms;
	ZeroMemory(&mciPlayParms,sizeof(mciPlayParms));
	ZeroMemory(&mciOpenParms,sizeof(mciOpenParms));
	
	SoundStopSong();
	DirMakeSongName (pBuf, ppSoundSongNames[index]);
	strcat (pBuf, ".mid");
	if (_access (pBuf, 4) != 0)
	{
		DirMakeSongName (pBuf2, ppSoundSongNames[index]);
		strcat (pBuf2, ".hmp");
		ConvertHMP (pBuf2, pBuf);
	}

	mciOpenParms.lpstrDeviceType = (char*)(DWORD)MCI_DEVTYPE_SEQUENCER;
	mciOpenParms.lpstrElementName = pBuf;
	rval = mciSendCommand (NULL, MCI_OPEN, MCI_WAIT | MCI_OPEN_ELEMENT 
		| MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID,
		(DWORD)(LPVOID)&mciOpenParms);
	if (rval != 0) return;
	midiDeviceID = mciOpenParms.wDeviceID;

    rval = mciSendCommand (midiDeviceID, MCI_PLAY, 0,
		(DWORD)(LPVOID)&mciPlayParms);
	if (rval != 0) {
        mciSendCommand (midiDeviceID, MCI_CLOSE, MCI_WAIT, NULL);
		midiDeviceID = -1; return;
    }
	
}

extern "C" void SoundStopSong (void)
{
	if (pDMusic) { DMStopSong (); return; }

	if (midiDeviceID == -1) return;
	mciSendCommand (midiDeviceID, MCI_CLOSE, MCI_WAIT, NULL);
}

extern "C" long SoundSongDone (void)
{
	int rval;
	MCI_STATUS_PARMS mciStatusParms;

	if (pDMusic) { return DMSongDone (); }

	if (midiDeviceID == -1) return 1;
	mciStatusParms.dwItem = MCI_STATUS_MODE;
	rval = mciSendCommand (midiDeviceID, MCI_STATUS, MCI_STATUS_ITEM,
		(DWORD)(LPVOID) &mciStatusParms);

	if (rval != 0 || mciStatusParms.dwReturn == MCI_MODE_STOP) {
        mciSendCommand(midiDeviceID, MCI_CLOSE, MCI_WAIT, NULL);
		midiDeviceID = -1; return 1;
	}
	else return 0;
}




extern "C" void SoundStopAllSamples (void)
{
	int i;
	for (i=0; i<NUMCHANNELS; i++) {
		if (ppChanBuffer[i] == NULL) continue;
		ppChanBuffer[i]->Stop();
		ppChanBuffer[i]->Release();
		ppChanBuffer[i] = NULL;
	}
	usedchannels = 0;
}

int PlaySampleInChannel (long index, long chan, long vol, long pitch)
{
	int rval;

	LPDIRECTSOUNDBUFFER pBuf = ppSampleBuffer[index];
	rval = pDSound->DuplicateSoundBuffer (pBuf, &ppChanBuffer[chan]);
	if (rval != DS_OK) return 0;

	ppChanBuffer[chan]->SetVolume (vol);
	ppChanBuffer[chan]->SetFrequency (pitch);
	ppChanBuffer[chan]->Play (0, 0, 0);

	pChannelUsage[chan] = index;
	usedchannels++;
	return 1;
}

extern "C" void SoundPlaySample (long index, long vol, long pitch)
{
	int fixednum, i;
	DWORD bufstatus;

	if (!pDSound) return;
	if (index >= soundNumSamples) return;
	if (ppSampleBuffer[index] == NULL) return;

	if (pitch==-1) pitch = 22050;
	pitch += pSoundSampleDescs[index].pitchmod;
	if (pitch > DSBFREQUENCY_MAX) pitch = DSBFREQUENCY_MAX;
	if (pitch < DSBFREQUENCY_MIN) pitch = DSBFREQUENCY_MIN;

	fixednum = pSoundSampleDescs[index].voice;
	vol = (long)(2217.0 * log10((double)vol));
	vol = vol + 15 & -16; vol -= 10000;

	// Clear stopped sounds

	if (usedchannels == NUMCHANNELS)
		for (i=0; i<NUMCHANNELS; i++)
	{
		if (ppChanBuffer[i] == NULL) continue;
		ppChanBuffer[i]->GetStatus (&bufstatus);
		if (bufstatus & DSBSTATUS_PLAYING) continue;
		ppChanBuffer[i]->Release();
		ppChanBuffer[i] = NULL;
		usedchannels--;
		break;
	}

	// Find channel to play on

	for (i=0; i<NUMCHANNELS; i++)
	{
		if (ppChanBuffer[i] == NULL) continue;
		if (pChannelUsage[i] != index) continue;

		ppChanBuffer[i]->GetStatus (&bufstatus);
		if (!(bufstatus & DSBSTATUS_PLAYING)) {
			ppChanBuffer[i]->SetVolume (vol);
			ppChanBuffer[i]->SetFrequency (pitch);
			ppChanBuffer[i]->Play (0, 0, 0);
			return;
		}
		if (index == 16) {
			ppChanBuffer[i]->SetVolume (vol);
			ppChanBuffer[i]->SetFrequency (pitch);
			return;
		}
		else if (fixednum != -1) return;
	}

	if (i == NUMCHANNELS) for (i=0; i<NUMCHANNELS; i++) {
		if (ppChanBuffer[i] == NULL) break;
	}

	if (i == NUMCHANNELS) for (i=0; i<NUMCHANNELS; i++) {
		int ci = pChannelUsage[i];
		if (ci != 0xb) continue;
		ppChanBuffer[i]->Release();
		ppChanBuffer[i] = NULL;
		usedchannels--;
		break;
	}

	if (i == NUMCHANNELS) return;
	PlaySampleInChannel (index, i, vol, pitch);
}




static LPDIRECTSOUNDBUFFER pStreamBuf = NULL;
static void *pStreamBackBuf = NULL;
static DWORD pBlockEnd[1000];
static int lastblock = -1;
static DWORD readpos = 0, writepos = 0;
static LARGE_INTEGER playtime;

void Win32StreamUpdate ()
{
	DWORD size1, size2;
	int rval;
	void *pBufData;

	if (pStreamBuf == NULL) return;
	if (readpos == writepos) return;

	rval = pStreamBuf->Lock (readpos, writepos-readpos, 
		&pBufData, &size1, NULL, &size2, 0);
	if (rval != DS_OK) return;
	memcpy (pBufData, (char *)pStreamBackBuf+readpos, writepos-readpos);
	pStreamBuf->Unlock (pBufData, size1, NULL, size2);

	readpos = writepos;
}

extern "C" void SoundStreamReset (void)
{
	DSBUFFERDESC dsbd;

	// Create 20-second buffer

	if (pDSound == NULL) return;
	if (pStreamBuf != NULL) SoundStreamStop();

	memset (&dsbd, 0, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_LOCSOFTWARE;
	dsbd.dwBufferBytes = 22050 * 2 * 20;
	dsbd.lpwfxFormat = &waveformat;
	pDSound->CreateSoundBuffer (&dsbd, &pStreamBuf, NULL);

	pStreamBackBuf = malloc (22050 * 2 * 20);
	readpos = writepos = 0;
}

extern "C" void SoundStreamAddBlock (void *pData, long size)
{
	// Add at nextpos

	if (pStreamBuf == NULL) return;
	memcpy ((char *)pStreamBackBuf+writepos, pData, size);
	writepos += size;
	pBlockEnd[++lastblock] = writepos;
}

extern "C" long SoundStreamGetUsedBlocks (void)
{
	DWORD curpos;
	DWORD curbuf = 0;
	LARGE_INTEGER curtime, freq;

	if (pStreamBuf == NULL) return 0;
	if (lastblock == -1) return 0;

	QueryPerformanceCounter (&curtime);
	QueryPerformanceFrequency (&freq);
	curpos = (DWORD)(((curtime.QuadPart - playtime.QuadPart) * 44100)
		/ freq.QuadPart);

	while (pBlockEnd[curbuf] <= curpos) ++curbuf;
	return curbuf;
}

extern "C" void SoundStreamStart (void)
{
	if (pStreamBuf == NULL) return;
	Win32StreamUpdate ();
	pStreamBuf->Play (0, 0, 0);
	QueryPerformanceCounter (&playtime);
}

extern "C" void SoundStreamWait (void)
{
	if (pStreamBuf == NULL) return;

	DWORD curpos = 0;
	while (curpos < writepos)
		pStreamBuf->GetCurrentPosition (&curpos, NULL);
	SoundStreamStop();
}

extern "C" void SoundStreamStop (void)
{
	if (pStreamBuf == NULL) return;

	pStreamBuf->Stop();
	pStreamBuf->Release();
	pStreamBuf = NULL;
	lastblock = -1;
	free (pStreamBackBuf);
}




extern "C" void SoundCheckInit (long *pAll, long *pDigi, long *pMidi)
{
	if (pDSound) { *pDigi = *pAll = 1; }
	else { *pDigi = *pAll = 0; }
	*pMidi = 1;
}

static int LoadSample (int index)
{
	char pBuf[256];
	FILE *pFile;
	int size=0, i, loopcount, rval;
	DSBUFFERDESC dsbd;
	UCHAR *pData = NULL;

	ppSampleBuffer[index] = NULL;
	ppSampleData[index] = NULL;
	pSampleLen[index] = 0;

	loopcount = pSoundSampleDescs[index].loopcount;
	if (loopcount == 0) loopcount = 1;

	DirMakeSampleName (pBuf, pSoundSampleDescs[index].pName);
	strcat (pBuf, ".raw");
	pFile = fopen (pBuf, "rb");
	if (pFile == NULL) {
fprintf (pLog, "Sample %s not found\n", pBuf);
		return 0;
	}

	fseek (pFile, 0, SEEK_END);
	size = ftell (pFile);
	fseek (pFile, 0, SEEK_SET);
	if (size == 0) return 0;

	pData = (UCHAR *)malloc (size*loopcount);
	fread (pData, 1, size, pFile);
	for (i=1; i<loopcount; i++)
		memcpy (pData+i*size, pData, size);
	fclose (pFile);

	ppSampleData[index] = pData;
	pSampleLen[index] = size*loopcount;

	memset (&dsbd, 0, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME
		| DSBCAPS_STATIC | DSBCAPS_LOCSOFTWARE;
	dsbd.dwBufferBytes = pSampleLen[index];
	dsbd.lpwfxFormat = &waveformat;
	rval = pDSound->CreateSoundBuffer (&dsbd, &ppSampleBuffer[index], NULL);
	if (rval != DS_OK) return 0;

	return 1;
}

static int FillSampleBuffer (int index)
{
	int rval;
	DWORD size1, size2;
	void *pBufData;

	if (ppSampleBuffer[index] == NULL) return 0;

	rval = ppSampleBuffer[index]->Lock (0, 0, &pBufData, &size1,
		NULL, &size2, DSBLOCK_ENTIREBUFFER);
	if (rval != DS_OK) {
		ppSampleBuffer[index]->Release();
		ppSampleBuffer[index] = NULL;
		return 0;
	}
	memcpy (pBufData, ppSampleData[index], size1);
	ppSampleBuffer[index]->Unlock (pBufData, size1, NULL, size2);
	return 1;
}


void Win32SoundReacquire (void)
{
	int i;
	if (!pDSound) return;

	for (i=0; i<SOUNDNUMSAMPLES; i++) {
		if (ppSampleBuffer[i] != NULL) ppSampleBuffer[i]->Restore ();
		FillSampleBuffer (i);
	}
	for (i=0; i<NUMCHANNELS; i++) {
		if (ppChanBuffer[i] != NULL) ppChanBuffer[i]->Restore ();
	}
}


void DSoundInit (void)
{
	int rval, i;
	if (pDSound != NULL) return;

	// Initialise DirectSound

	rval = DirectSoundCreate (NULL, &pDSound, NULL);
	if (rval != DS_OK) return;
	rval = pDSound->SetCooperativeLevel (Win32GetWnd(), DSSCL_PRIORITY);
	if (rval != DS_OK) return;

	// Set up wave format

	memset(&waveformat, 0, sizeof(PCMWAVEFORMAT));
	waveformat.wFormatTag = WAVE_FORMAT_PCM;
	waveformat.nChannels = 1;
	waveformat.nSamplesPerSec = 22050;
	waveformat.nBlockAlign = 2;
	waveformat.nAvgBytesPerSec = 44100;
	waveformat.wBitsPerSample = 16;

	DSBUFFERDESC dsbd;
	LPDIRECTSOUNDBUFFER pDSPri;

	memset (&dsbd, 0, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0; dsbd.lpwfxFormat = NULL;
	rval = pDSound->CreateSoundBuffer (&dsbd, &pDSPri, NULL);
	if (rval != DS_OK) return;
	rval = pDSPri->SetFormat (&waveformat);

	// Create buffer for each sample

	for (i=0; i<SOUNDNUMSAMPLES; i++) {
		LoadSample (i);
		FillSampleBuffer (i);
	}

	// Clear channels

	for (i=0; i<NUMCHANNELS; i++) ppChanBuffer[i] = NULL;
}

void SoundInit (void)
{
	CfgStruct cfg;

	if (!pLog) pLog = fopen ("ffesound.log", "wt");

	CfgOpen (&cfg, __CONFIGFILE__);
	CfgFindSection (&cfg, "SOUND");
	CfgGetKeyVal (&cfg, "softsynth", &softsynth);
	CfgGetKeyVal (&cfg, "forcemci", &forcemci);
	CfgGetKeyVal (&cfg, "win2khack", &win2khack);
	CfgClose (&cfg);

	DSoundInit();
	if (forcemci) return;
	if (!DMInit()) DMCleanup();
}

void SoundCleanup (void)
{
	DMCleanup();
	SoundStopAllSamples();
	SoundStreamStop();
	SoundStopSong();

	int i;
	for (i=0; i<SOUNDNUMSAMPLES; i++)
	{
		if (ppSampleBuffer[i]) ppSampleBuffer[i]->Release ();
		if (ppSampleData[i]) free (ppSampleData[i]);
		ppSampleBuffer[i] = NULL;
		ppSampleData[i] = NULL;
	}

	if (pDSound) { pDSound->Release(); pDSound = NULL; }

	if (pLog) fclose (pLog);
}