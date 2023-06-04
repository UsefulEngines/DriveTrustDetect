##########################    PROJECT MAKEFILE  #############################
#
#   NOTES:
#   	1.  Build targets:
#				Just build (default all):>	nmake 	
#				Build all targets :>		nmake all
#				clean :>			nmake clean     
#				debug :>			nmake DEBUG=1 all
#
#	Phil Pennington, philpenn@microsoft.com
#	Copyright Microsoft Corporation, 2008, for illustration purposes only.
######################################################################

MACHINE = X64

!ifndef SDK_LIB_PATH
SDK_LIB_PATH = "$(MSSDK)\lib\x64"
!endif

!ifdef DEBUG
OUTDIR = .\DEBUG64
!else
OUTDIR = .\RELEASE64
!endif

!ifdef DEBUG
CFLAGS = $(CFLAGS) /W4 /WX /Zi /D _CRT_SECURE_NO_DEPRECATE -D WIN32_LEAN_AND_MEAN -D UNICODE -D _UNICODE /EHsc /I "$(MSSDK)\Include" /I ".\WDK.H"
LFLAGS = $(LFLAGS) /DEBUG /MACHINE:$(MACHINE) /LIBPATH:$(SDK_LIB_PATH)
!else 
CFLAGS = $(CFLAGS) /W4 /WX /O2 /GL /D _CRT_SECURE_NO_DEPRECATE -D WIN32_LEAN_AND_MEAN -D UNICODE -D _UNICODE /EHsc /I "$(MSSDK)\Include" /I ".\WDK.H"
LFLAGS = $(LFLAGS) /LTCG /MACHINE:$(MACHINE) /LIBPATH:$(SDK_LIB_PATH)
!endif

####################################################################### 

TARGETNAME=DiskInfo

OBJS = \
	$(OUTDIR)\stdafx.obj \
	$(OUTDIR)\DiskInfo.obj
	
LIBS = \
    kernel32.lib \
    user32.lib 

default: all

all: $(OUTDIR) $(OUTDIR)\$(TARGETNAME).exe

"$(OUTDIR)":
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

$(OUTDIR)\stdafx.obj: stdafx.cpp
    $(CC) $(CFLAGS) /Fo"$(OUTDIR)\\" /c $**

$(OUTDIR)\DiskInfo.obj: DiskInfo.cpp
    $(CC) $(CFLAGS) /Fo"$(OUTDIR)\\" /c $**

$(OUTDIR)\$(TARGETNAME).exe: $(OBJS)
    LINK $(LFLAGS) $(LIBS) /PDB:$(OUTDIR)\$(TARGETNAME).PDB -out:$(OUTDIR)\$(TARGETNAME).exe $**

clean:
	@del $(OUTDIR)\*.* /Q

clean2:
	@del $(OUTDIR)\*.obj /Q
	
# HEADER DEPENDENCIES
stdafx.cpp:	stdafx.h targetver.h
DiskInfo.cpp: DiskDrive.h AtaInterface.h AtaIdentifySector.h UsbInterface.h
	
########################################################################
