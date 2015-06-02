#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ffecfg.h"

#ifdef WIN32
#define strcasecmp stricmp
#endif

// Initialises file filename into cfg
// Returns 1 on success, 0 on failure
int CfgOpen(CfgStruct *cfg,char *filename)
{
	FILE *fd=fopen(filename,"r");

	memset(cfg,0,sizeof(CfgStruct));

	if(fd)
	{
		char buf[512];
		char *t;
		char *p;

		while(fgets(buf,512,fd))
		{
			if(!(t=strpbrk(buf,"=["))) continue;

			while ((p = strrchr(buf,'\n')) != NULL) *p = 0; // Fixed by AlexFA
			while ((p = strrchr(buf,'\r')) != NULL) *p = 0;

			if(*t=='[')
			{
				if(!strtok(++t,"]")) continue;

				cfg->sections=realloc(cfg->sections,sizeof(CfgSection)*(++cfg->sectioncount));
				cfg->currentsection=&cfg->sections[cfg->sectioncount-1];
				cfg->currentsection->keycount=0;
				cfg->currentsection->name=strdup(t);
				cfg->currentsection->keys=NULL;
			}
			else if(*t=='=')
			{
				if(!cfg->currentsection) continue;

				*(t++)=0;
				t+=strspn(t," ");
				for(p=t+strlen(t)-1;*p==' ';--p);
				*(p+1)=0;

				cfg->currentsection->keys=realloc(cfg->currentsection->keys,sizeof(CfgKey)*(++cfg->currentsection->keycount));
				cfg->currentsection->keys[cfg->currentsection->keycount-1].data=strdup(t);

				t=buf+strspn(buf," ");
				if((p=strchr(t,' '))) *p=0;

				cfg->currentsection->keys[cfg->currentsection->keycount-1].name=strdup(t);
			}
		}

		fclose(fd);

		cfg->currentsection=NULL;
		cfg->filename=strdup(filename);

		return 1;
	}
	return 0;
}

// Closes config file - saves if modified
// Returns 1 on success, 0 on failure
int CfgClose(CfgStruct *cfg)
{
	int i,j;

	if(!cfg->filename) return 0;
	
	for(i=0;i<cfg->sectioncount;++i)
	{
		cfg->currentsection=&cfg->sections[i];
		for(j=0;j<cfg->currentsection->keycount;++j)
		{
			free(cfg->currentsection->keys[j].data);
			free(cfg->currentsection->keys[j].name);
		}
		free(cfg->currentsection->name);
		free(cfg->currentsection->keys);
	}

	free(cfg->filename);
	free(cfg->sections);

	memset(cfg,0,sizeof(CfgStruct));

	return 1;
}

// Sets offsets in cfg to position of [sectname]
// Returns 1 if found, 0 on failure
int CfgFindSection(CfgStruct *cfg,char *sectname)
{
	int i;

	if(!cfg->filename) return 0;

	for(i=0;i<cfg->sectioncount;++i)
	{
		if(!strcasecmp(sectname,cfg->sections[i].name))
		{
			cfg->currentsection=&cfg->sections[i];
			return 1;
		}
	}
	
	return 0;
}

// Reads the value of key keyname as an integer
// Returns 1 on success, 0 on failure
int CfgGetKeyVal(CfgStruct *cfg,char *keyname,int *value)
{
	int i;

	if(!cfg->filename) return 0;
	if(!cfg->currentsection) return 0;

	for(i=0;i<cfg->currentsection->keycount;++i)
	{
		if(!strcasecmp(keyname,cfg->currentsection->keys[i].name))
		{
			*value=strtol(cfg->currentsection->keys[i].data,NULL,0);
			return 1;
		}
	}
	
	return 0;
}

int CfgGetKeyStr(CfgStruct *cfg,char *keyname,char *value,int buflen)
{
	int i;

	if(!cfg->filename) return 0;
	if(!cfg->currentsection) return 0;

	for(i=0;i<cfg->currentsection->keycount;++i)
	{
		if(!strcasecmp(keyname,cfg->currentsection->keys[i].name))
		{
			strncpy (value,cfg->currentsection->keys[i].data,buflen);
			return 1;
		}
	}
	
	return 0;
}

int CfgGetKeyValDef(CfgStruct *cfg,char *keyname,int *value,int def)
{
	int i;

	if(!cfg->filename)
	{
		*value=def;
		return 0;
	}

	if(!cfg->currentsection)
	{
		*value=def;
		return 0;
	}

	for(i=0;i<cfg->currentsection->keycount;++i)
	{
		if(!strcasecmp(keyname,cfg->currentsection->keys[i].name))
		{
			*value=strtol(cfg->currentsection->keys[i].data,NULL,0);
			return 1;
		}
	}

	*value=def;	
	return 0;
}