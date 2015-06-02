#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "ffeapi.h"
#include "ffecfg.h"

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
	{ "wind", 0, -0x4000, 8, 0 },
	{ "station", 0x32, 0, 9, 0 },
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
	{ "ggofkiev" } };





struct SoundChannel
{
	int index;
	void *pData;
	int len;
	int vol;

	int loop;
	int pos;			
	int step;			// Both 24.8 fixed point
};

#define NUMCHANNELS 16

static void *ppSampleData[SOUNDNUMSAMPLES];	// Data (22050, S16)
static int pSampleLen[SOUNDNUMSAMPLES];		// length in samples (*2)

// 0 reserved for music, 1 reserved for video
static SoundChannel pChannel[NUMCHANNELS];

static int soundInit = 0;
static FILE *pLog = 0;
static int buffersize = 4096;

void FillSound (void *udata, Uint8 *stream, int len)
{
	int i, n;
//	int pAccum[BUFFERSIZE];
	int *pAccum = (int *)stream;
	len /= 4;							// conv to bytes

/*
#ifdef WIN32
	static int priority = 0;
	if (priority == 0) {
		SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL);
	}
#endif
*/
	// Mix channels into accumulation buffer

	for (i=0; i<len; i++) pAccum[i] = 0;

	for (n=0; n<NUMCHANNELS; n++)
	{
		SoundChannel *pChan = pChannel + n;
		if (pChan->pData == 0) continue;
		Sint16 *pData = (Sint16 *)pChan->pData;

		for (i=0; i<len; i++)
		{
			if (pChan->vol > 250) pAccum[i] += pData[pChan->pos>>8];
			else pAccum[i] += (pData[pChan->pos>>8] * pChan->vol) >> 8;
			pChan->pos += pChan->step;

			if (pChan->pos >= pChan->len) {
				pChan->pos -= pChan->len;
				if (pChan->loop-- == 0) {
					pChan->pData = 0;
					break;
				}
			}
		}
	}

	// Clip to output

	for (i=0; i<len; i++)
	{
		if (pAccum[i] < -32768) pAccum[i] = -2147450880;
		else if (pAccum[i] > 0x7fff) pAccum[i] = 0x7fff7fff;
		else pAccum[i] = (pAccum[i] & 0xffff) | (pAccum[i] << 16);
	}
}

extern "C" void SoundCheckInit (long *pAll, long *pDigi, long *pMidi)
{
	*pAll = soundInit;
	*pDigi = soundInit;
	*pMidi = 0;
}

extern "C" void SoundStopAllSamples (void)
{
	int i;
	SDL_LockAudio ();
	for (i=1; i<NUMCHANNELS; i++) pChannel[i].pData = 0;
	SDL_UnlockAudio ();
}

extern "C" void SoundPlaySample (long index, long vol, long pitch)
{
	int fixednum, i;

	if (!soundInit) return;
	if (index >= soundNumSamples) return;
	if (ppSampleData[index] == NULL) return;

	if (pitch==-1) pitch = 22050;
	pitch += pSoundSampleDescs[index].pitchmod;
	if (pitch > 100000) pitch = 100000;
	if (pitch < 100) pitch = 100;

	// Find channel to play on

	fixednum = pSoundSampleDescs[index].voice;
	if (fixednum != -1)
	{
		SoundChannel *pChan = pChannel + fixednum;
		if (pChan->pData == 0) i = fixednum;
		else if (index != 16) return;
		else {
			// engine noise - modify frequency
			pChan->vol = vol >> 7;
			pChan->step = (pitch << 8) / 22050;
			pChan->loop = 0x32;
			return;
		}
	}
	else		// Find free or beam-laser channel
	{
		for (i=10; i<NUMCHANNELS; i++) {
			if (pChannel[i].pData == 0) break;
		}
		if (i == NUMCHANNELS) for (i=10; i<NUMCHANNELS; i++) {
			if (pChannel[i].index == 0xb) break;
		}
		if (i == NUMCHANNELS) return;		// no free slots found
	}

	// Play sample (index) in channel i

	SDL_LockAudio ();

	SoundChannel *pChan = pChannel + i;
	pChan->index = index;
	pChan->len = pSampleLen[index] << 8;
	pChan->loop = pSoundSampleDescs[index].loopcount;
	pChan->pos = 0;
	pChan->step = (pitch << 8) / 22050;
	pChan->vol = vol >> 7;
	pChan->pData = ppSampleData[index];

	SDL_UnlockAudio ();

//	fprintf (pLog, "Sample %i, channel %i, len %i, vol %i, pitch %i\n",
//		index, i, pSampleLen[index], vol >> 7, pitch);
//	fflush (pLog);
}


static int LoadSample (int index)
{
	char pBuf[256];
	FILE *pFile;
	int size = 0;
	void *pSample = NULL;

	DirMakeSampleName (pBuf, pSoundSampleDescs[index].pName);
	strcat (pBuf, ".raw");
	pFile = fopen (pBuf, "rb");
	if (pFile != NULL) 
	{
		fseek (pFile, 0, SEEK_END);
		size = ftell (pFile);
		fseek (pFile, 0, SEEK_SET);

		if (size != 0) {
			pSample = malloc (size);
			fread (pSample, 1, size, pFile);
			fclose (pFile);
		}
	}
	else fprintf (stderr, "Sample %s not found\n", pBuf);

	ppSampleData[index] = pSample;
	pSampleLen[index] = size / 2;
	return 1;
}


void SoundInit ()
{
	CfgStruct cfg;
	CfgOpen (&cfg, __CONFIGFILE__);
	CfgFindSection (&cfg, "SOUND");
	CfgGetKeyVal (&cfg, "buffersize", &buffersize);
	CfgClose (&cfg);

	int i;
	for (i=0; i<SOUNDNUMSAMPLES; i++) LoadSample (i);
	for (i=0; i<NUMCHANNELS; i++) pChannel[i].pData = 0;

	SDL_AudioSpec spec;
	spec.freq = 44100;
	spec.format = AUDIO_S16;
	spec.channels = 1;
	spec.samples = buffersize;
	spec.callback = FillSound;
	spec.userdata = NULL;

	if (SDL_OpenAudio (&spec, 0) != 0) soundInit = 0;
	else { soundInit = 1; SDL_PauseAudio(0); }
}

void SoundCleanup ()
{
	SDL_CloseAudio ();

	SoundStopAllSamples();
	SoundStreamStop();
	SoundStopSong();

	int i;
	for (i=0; i<SOUNDNUMSAMPLES; i++)
	{
		if (ppSampleData[i]) free (ppSampleData[i]);
		ppSampleData[i] = NULL;
		pSampleLen[i] = 0;
	}
}



static UCHAR pStreamData[22050*2*20];
static int pBlockEnd[1000];
static int lastblock = -1;
static int writepos = 0;

extern "C" void SoundStreamReset (void)
{
	if (soundInit == 0) return;
	if (pChannel[1].pData != NULL) SoundStreamStop();
	writepos = 0;
}

extern "C" void SoundStreamAddBlock (void *pData, long size)
{
	if (soundInit == 0) return;
	memcpy (pStreamData+writepos, pData, size);
	writepos += size;
	pBlockEnd[++lastblock] = writepos;
	pChannel[1].len = writepos << 7;
}

extern "C" long SoundStreamGetUsedBlocks (void)
{
	if (soundInit == 0) return 0;
	if (lastblock == -1) return 0;
	int curbuf = 0;
	int curpos = (pChannel[1].pos >> 8) * 2;
	while (pBlockEnd[curbuf] <= curpos) ++curbuf;
	return curbuf;
}

extern "C" void SoundStreamStart (void)
{
	if (soundInit == 0) return;
	pChannel[1].pos = 0;
	pChannel[1].step = 0x100;
	pChannel[1].vol = 255;
	pChannel[1].len = writepos << 7;
	pChannel[1].loop = 0;
	pChannel[1].pData = pStreamData;
}

static void *GetStreamPos ()
{
	return pChannel[1].pData;
}

extern "C" void SoundStreamWait (void)
{
	if (soundInit == 0) return;
	while (GetStreamPos () != NULL);
	SoundStreamStop();
}

extern "C" void SoundStreamStop (void)
{
	if (soundInit == 0) return;
	SDL_LockAudio ();
	pChannel[1].pData = 0;
	lastblock = -1;
	SDL_UnlockAudio ();
}



extern "C" void SoundPlaySong (long index) 
{
}

extern "C" void SoundStopSong (void)
{
}

extern "C" long SoundSongDone (void)
{
	return 0;
}
