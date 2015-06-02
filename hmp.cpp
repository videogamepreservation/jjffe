//(c) 2000-2001 Peter Pawlowski
//http://www.blorp.com/~peter/

//based on HMP converter from the MIDI plug-in

#include <memory.h>
#include <malloc.h>

//windoze defines
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef int BOOL;
typedef unsigned short WORD;

#define rev16(X) (((X)&0xFF)<<8)|(((X)>>8)&0xFF)

#define _rv(X) ((((DWORD)(X)&0xFF)<<24)|(((DWORD)(X)&0xFF00)<<8)|(((DWORD)(X)&0xFF0000)>>8)|(((DWORD)(X)&0xFF000000)>>24))


#define _MThd 'dhTM'
#define _MTrk 'krTM'

#ifndef __GNUC__
#pragma pack(push)
#pragma pack(1)
typedef struct
{
	WORD fmt,trax,dtx;
} MIDIHEADER;
#pragma pack(pop)
#else
typedef struct
{
	WORD fmt __attribute__ ((packed));
	WORD trax __attribute__ ((packed));
	WORD dtx __attribute__ ((packed));
} MIDIHEADER;
#endif

//from MIDI plug-in utils
class CWriteBuf		//handle write-to-memory operations
{
public:
	BYTE* buf;
	UINT ptr,sz;

	void* finish(UINT* s);
	void freebuf();
	BOOL write(void* p,UINT s);
	BOOL writeoffs(void* p,UINT s,UINT ofs);
	BOOL writedwordoffs(DWORD dw,UINT ofs);
	BOOL writedword(DWORD d);
	UINT get_offs() {return ptr;}
	BOOL writebyte(BYTE b);
	void setptr(UINT p) {ptr=p;}
};

BOOL CWriteBuf::writedwordoffs(DWORD dw,UINT ofs)
{
	return writeoffs(&dw,4,ofs);
}

void* CWriteBuf::finish(UINT* s)
{
	void* r=0;
	r=realloc(buf,ptr);
	*s=ptr;
	return r;
}

void CWriteBuf::freebuf()
{
	free(buf);
}

BOOL CWriteBuf::write(void* p,UINT s)
{
	UINT np=ptr+s;
	if (np>sz)
	{
		do {
			sz<<=1;
		} while(np>sz);
		buf=(BYTE*)realloc(buf,sz);
	}
	if (p) memcpy(buf+ptr,p,s);
	ptr=np;
	return buf ? 1 : 0;
}


CWriteBuf* wb_create()
{
	CWriteBuf* r=new CWriteBuf;
	if (r)
	{
		r->sz=0x1000;
		r->ptr=0;
		if (!(r->buf=(BYTE*)malloc(r->sz)))
		{
			delete r;
			r=0;
		}
	}
	return r;
}

BOOL CWriteBuf::writedword(DWORD dw)
{
	return write(&dw,4);
}

BOOL CWriteBuf::writeoffs(void* p,UINT s,UINT ofs)
{
	if (ofs+s > ptr) {ptr=ofs;return write(p,s);}
	if (!buf) return 0;
	if (p) memcpy(buf+ofs,p,s);
	return 1;
}

BOOL CWriteBuf::writebyte(BYTE b)
{
	if (!buf) {ptr++;return 0;}
	if (ptr==sz) return write(&b,1);
	buf[ptr++]=b;
	return 1;
}


CWriteBuf* wb_create();

static DWORD ProcessTrack(BYTE* track,CWriteBuf* buf,int size)
{
	UINT s_sz=buf->get_offs();
	BYTE *pt = track;
	BYTE lc1 = 0,lastcom = 0;
	DWORD t=0,d;
	bool run = 0;
	int n1,n2;
	while(track < pt + size)
	{		
		if (track[0]&0x80)
		{
			BYTE b=track[0]&0x7F;
			buf->writebyte(b);			
			t+=b;
		}
		else
		{
			d = (track[0])&0x7F;
			n1 = 0;
			while((track[n1]&0x80)==0)
			{
				n1++;
				d+=(track[n1]&0x7F)<<(n1*7);
			}
			t+=d;
			
			n1 = 1;
			while((track[n1]&0x80)==0)
			{
				n1++;
				if (n1==4) return 0;
			}
			for(n2=0;n2<=n1;n2++)
			{
				BYTE b=track[n1-n2]&0x7F;
				
				if (n2!=n1) b|=0x80;
				buf->writebyte(b);
			}
			track+=n1;
		}
		track++;
		if (*track == 0xFF)//meta
		{
			buf->write(track,3+track[2]);
			if (track[1]==0x2F) break;
		}
		else 
		{
			lc1=track[0];
			if ((lc1&0x80) == 0) return 0;
			switch(lc1&0xF0)
			{
			case 0x80:
			case 0x90:
			case 0xA0:
			case 0xB0:
			case 0xE0:
				if (lc1!=lastcom)
				{
					buf->writebyte(lc1);
				}
				buf->write(track+1,2);
				track+=3;
				break;
			case 0xC0:
			case 0xD0:
				if (lc1!=lastcom)
				{
					buf->writebyte(lc1);
				}
				buf->writebyte(track[1]);
				track+=2;
				break;
			default:
				return 0;
			}
			lastcom=lc1;
		}
	}
	return buf->get_offs()-s_sz;
}

#define FixHeader(H) {(H).fmt=rev16((H).fmt);(H).trax=rev16((H).trax);(H).dtx=rev16((H).dtx);}

BYTE hmp_track0[19]={'M','T','r','k',0,0,0,11,0,0xFF,0x51,0x03,0x18,0x80,0x00,0,0xFF,0x2F,0};

//buf = HMP image, br = HMP image size, out_size = output MIDI image size
extern "C" void* load_hmp(BYTE* buf,DWORD br,DWORD* out_size)
{
	MIDIHEADER mhd = {1,0,0xC0};
	BYTE * max = buf+br;
	BYTE* ptr = buf;
	CWriteBuf* dst=wb_create();
	DWORD n1,n2;
	dst->writedword(_rv('MThd'));
	dst->writedword(_rv(6));
	dst->write(0,sizeof(mhd));
	ptr = buf+0x30;
	mhd.trax = *ptr;
	dst->write(hmp_track0,sizeof(hmp_track0));

	while(*(WORD*)ptr != 0x2FFF && ptr < max - 4-7) ptr++;
	ptr+=7;
	if (ptr == max-4) { dst->freebuf(); delete dst; return 0; }
	UINT n;

	for(n=1;n<mhd.trax;n++)
	{
		dst->writedword(_rv('MTrk'));
		n1 = *(DWORD*)ptr - 12;
		if (ptr + n1 > max) { dst->freebuf(); delete dst; return 0; }
		ptr += 8;
		
		UINT ts_ofs=dst->get_offs();
		dst->writedword(0);
		if (!(n2=ProcessTrack(ptr,dst,n1)))
			{ dst->freebuf(); delete dst; return 0; }
		
		dst->writedwordoffs(_rv(n2),ts_ofs);
		ptr += n1 + 4;
	}
	FixHeader(mhd);
	dst->writeoffs(&mhd,sizeof(mhd),8);


	UINT f_sz;
	BYTE* r=(BYTE*)dst->finish(&f_sz);
	delete dst;
	if (r)
	{
		if (out_size) *out_size=f_sz;
		return r;
	}
	else
	{
		return 0;
	}
}
