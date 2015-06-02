#ifndef __CFG__H__
#define __CFG__H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	char *name;
	char *data;
} CfgKey;

typedef struct
{
	char *name;
	int keycount;
	CfgKey *keys;
} CfgSection;

typedef struct
{
	char *filename;
	int wasmodified;
	int sectioncount;
	CfgSection *sections;
	CfgSection *currentsection;
} CfgStruct;

// Initialises file filename into cfg
// Returns 1 on success, 0 on failure
int CfgOpen(CfgStruct *cfg,char *filename);

// Closes config file - saves if modified
// Returns 1 on success, 0 on failure
int CfgClose(CfgStruct *cfg);

// Sets offsets in cfg to position of [sectname]
// Returns 1 if found, 0 on failure
int CfgFindSection(CfgStruct *cfg,char *sectname);

// Reads the value of key keyname as an integer
// Returns 1 on success, 0 on failure
int CfgGetKeyVal(CfgStruct *cfg,char *keyname,int *value);

// Reads the value of key keyname as a string
// Returns 1 on success, 0 on failure
int CfgGetKeyStr(CfgStruct *cfg,char *keyname,char *value,int buflen);

// Reads the value of key keyname as an integer
// Puts def int value if no key found
// Returns 1 if key was found, 0 if def was used
// Note: safe to call even if CfgOpen and CfgFindSection failed
int CfgGetKeyValDef(CfgStruct *cfg,char *keyname,int *value,int def);

#ifdef __cplusplus
}
#endif

#endif // __CFG__H__