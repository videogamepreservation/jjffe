#ifndef __FFEAPI__H__
#define __FFEAPI__H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __FFELNXSDL__
#define __CONFIGFILE__ "ffelnxsdl.cfg"
#endif
#ifdef __FFEWIN__
#define __CONFIGFILE__ "ffewin.cfg"
#endif
#ifdef __FFEWINSDL__
#define __CONFIGFILE__ "ffewinsdl.cfg"
#endif

#ifndef __CONFIGFILE__
#error Unknown version: Config file not set
#endif


typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;

void SystemInit (void);
void SystemCleanup (void);

void VideoBlit (UCHAR *pData, long x, long y, long w, long h, long jump);
void VideoMaskedBlit (UCHAR *pData, long x, long y, long w, long h, long jump);
void VideoReverseBlit (UCHAR *pData, long x, long y, long w, long h, long jump);
void VideoGetPalValue (long palindex, UCHAR *pVal);
void VideoSetPalValue (long palindex, UCHAR *pVal);
long VideoPointerExclusive (void);
void VideoPointerEnable (void);
void VideoPointerDisable (void);

long InputMouseReadButtons (void);
void InputMouseReadMickeys (long *pXMick, long *pYMick);
void InputMouseReadPos (long *pXPos, long *pYPos);

void InputJoyReadPos (long *pXPos, long *pYPos);
long InputJoyReadButtons (void);

void InputKeybReadStates (UCHAR *pKeyArray);
long InputKeybGetLastKey (void);
void InputKeybSetLastKey (long);

ULONG TimerGetTimeStamp (void);
void TimerSleep (void);
void TimerFrameUpdate (void);

void SoundCheckInit (long *pAll, long *pDigi, long *pMidi);
void SoundPlaySong (long index);
void SoundStopSong (void);
long SoundSongDone (void);
void SoundStopAllSamples (void);
void SoundPlaySample (long index, long vol, long pitch);

long SoundStreamGetUsedBlocks (void);
void SoundStreamReset (void);
void SoundStreamAddBlock (void *pData, long size);
void SoundStreamStart (void);
void SoundStreamWait (void);
void SoundStreamStop (void);

typedef struct {
	char pName[16];
	int type;
	int size;
} FileInfo;

char *DirMakeAVIName (char *pBuf, char *pStub);
char *DirMakeSampleName (char *pBuf, char *pFilename);
char *DirMakeSongName (char *pBuf, char *pFilename);
char *DirMakeCmmdrName (char *pBuf, char *pFilename);
char *DirGetCmmdrPath (void);
void DirResetCmmdrPath (void);
void DirNavigateTree (FileInfo *pFile);
int DirFindFirst (FileInfo *pFile);
int DirFindNext (FileInfo *pFile);


#ifdef __cplusplus
}
#endif

#endif /* __FFEAPI_H__ */
