# Microsoft Developer Studio Generated NMAKE File, Based on Tag.dsp
!IF "$(CFG)" == ""
CFG=Tag - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Tag - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Tag - Win32 Release" && "$(CFG)" != "Tag - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Tag.mak" CFG="Tag - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Tag - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Tag - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "Tag - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\Tag.exe"


CLEAN :
	-@erase "$(INTDIR)\find.obj"
	-@erase "$(INTDIR)\guess.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\mp3.obj"
	-@erase "$(INTDIR)\mpc.obj"
	-@erase "$(INTDIR)\ogg.obj"
	-@erase "$(INTDIR)\Tag.obj"
	-@erase "$(INTDIR)\tagread.obj"
	-@erase "$(INTDIR)\tags.obj"
	-@erase "$(INTDIR)\tagwrite.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vcedit.obj"
	-@erase "$(OUTDIR)\Tag.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\Tag.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Tag.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\Tag.pdb" /machine:I386 /nodefaultlib:"libcmt" /out:"$(OUTDIR)\Tag.exe" 
LINK32_OBJS= \
	"$(INTDIR)\find.obj" \
	"$(INTDIR)\guess.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\mp3.obj" \
	"$(INTDIR)\mpc.obj" \
	"$(INTDIR)\ogg.obj" \
	"$(INTDIR)\Tag.obj" \
	"$(INTDIR)\tagread.obj" \
	"$(INTDIR)\tags.obj" \
	"$(INTDIR)\tagwrite.obj" \
	"$(INTDIR)\vcedit.obj" \
	"..\..\id3lib-3.8.0pre2.1\prj\Release\id3lib.lib" \
	"..\..\vorbis-sdk-1.0rc3\lib\vorbisfile_static.lib" \
	"..\..\vorbis-sdk-1.0rc3\lib\ogg_static.lib" \
	"..\..\vorbis-sdk-1.0rc3\lib\vorbis_static.lib"

"$(OUTDIR)\Tag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Tag - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\Tag.exe"


CLEAN :
	-@erase "$(INTDIR)\find.obj"
	-@erase "$(INTDIR)\guess.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\mp3.obj"
	-@erase "$(INTDIR)\mpc.obj"
	-@erase "$(INTDIR)\ogg.obj"
	-@erase "$(INTDIR)\Tag.obj"
	-@erase "$(INTDIR)\tagread.obj"
	-@erase "$(INTDIR)\tags.obj"
	-@erase "$(INTDIR)\tagwrite.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vcedit.obj"
	-@erase "$(OUTDIR)\Tag.exe"
	-@erase "$(OUTDIR)\Tag.ilk"
	-@erase "$(OUTDIR)\Tag.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\Tag.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Tag.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\Tag.pdb" /debug /machine:I386 /out:"$(OUTDIR)\Tag.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\find.obj" \
	"$(INTDIR)\guess.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\mp3.obj" \
	"$(INTDIR)\mpc.obj" \
	"$(INTDIR)\ogg.obj" \
	"$(INTDIR)\Tag.obj" \
	"$(INTDIR)\tagread.obj" \
	"$(INTDIR)\tags.obj" \
	"$(INTDIR)\tagwrite.obj" \
	"$(INTDIR)\vcedit.obj" \
	"..\..\id3lib-3.8.0pre2.1\prj\Release\id3lib.lib" \
	"..\..\vorbis-sdk-1.0rc3\lib\vorbisfile_static.lib" \
	"..\..\vorbis-sdk-1.0rc3\lib\ogg_static.lib" \
	"..\..\vorbis-sdk-1.0rc3\lib\vorbis_static.lib"

"$(OUTDIR)\Tag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("Tag.dep")
!INCLUDE "Tag.dep"
!ELSE 
!MESSAGE Warning: cannot find "Tag.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Tag - Win32 Release" || "$(CFG)" == "Tag - Win32 Debug"
SOURCE=.\find.c

"$(INTDIR)\find.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\guess.c

"$(INTDIR)\guess.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\misc.c

"$(INTDIR)\misc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mp3.c

"$(INTDIR)\mp3.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mpc.c

"$(INTDIR)\mpc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ogg.c

"$(INTDIR)\ogg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Tag.c

"$(INTDIR)\Tag.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tagread.c

"$(INTDIR)\tagread.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tags.c

"$(INTDIR)\tags.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tagwrite.c

"$(INTDIR)\tagwrite.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\vcedit.c

"$(INTDIR)\vcedit.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

