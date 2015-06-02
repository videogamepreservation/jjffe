# Microsoft Developer Studio Project File - Name="jjffew32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=jjffew32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "jjffew32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "jjffew32.mak" CFG="jjffew32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "jjffew32 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "jjffew32 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "jjffew32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir "win32"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__FFEWIN__" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 dxguid.lib ddraw.lib dinput.lib dsound.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /map /machine:I386 /out:"ffewin.exe"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "jjffew32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "win32db"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__FFEWIN__" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dxguid.lib ddraw.lib dinput.lib dsound.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386 /out:"ffewindb.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "jjffew32 - Win32 Release"
# Name "jjffew32 - Win32 Debug"
# Begin Group "win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\win32\w32dir.cpp
# End Source File
# Begin Source File

SOURCE=.\win32\w32input.cpp
# End Source File
# Begin Source File

SOURCE=.\win32\w32sound.cpp
# End Source File
# Begin Source File

SOURCE=.\win32\w32time.cpp
# End Source File
# Begin Source File

SOURCE=.\win32\w32video.cpp
# End Source File
# Begin Source File

SOURCE=.\win32\win32api.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ffe.asm

!IF  "$(CFG)" == "jjffew32 - Win32 Release"

USERDEP__FFE_A="ffedat.asm"	
# Begin Custom Build - Assembling ffe.asm...
IntDir=.\win32
InputPath=.\ffe.asm

"$(IntDir)\ffe.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o $(IntDir)\ffe.obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "jjffew32 - Win32 Debug"

# Begin Custom Build - Assembling ffe.asm...
IntDir=.\win32db
InputPath=.\ffe.asm

"$(IntDir)\ffe.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o $(IntDir)\ffe.obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ffeapi.h
# End Source File
# Begin Source File

SOURCE=.\ffebmp.asm

!IF  "$(CFG)" == "jjffew32 - Win32 Release"

# Begin Custom Build - Assembling ffebmp.asm...
IntDir=.\win32
InputPath=.\ffebmp.asm

"$(IntDir)\ffebmp.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o $(IntDir)\ffebmp.obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "jjffew32 - Win32 Debug"

# Begin Custom Build - Assembling ffebmp.asm...
IntDir=.\win32db
InputPath=.\ffebmp.asm

"$(IntDir)\ffebmp.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o $(IntDir)\ffebmp.obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ffecfg.c
# End Source File
# Begin Source File

SOURCE=.\ffecfg.h
# End Source File
# Begin Source File

SOURCE=.\ffedat.asm
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\ffemisc.c
# End Source File
# Begin Source File

SOURCE=.\hmp.cpp
# End Source File
# End Target
# End Project
