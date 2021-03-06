# Microsoft Developer Studio Project File - Name="log4cplus_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=log4cplus_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "log4cplus_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "log4cplus_dll.mak" CFG="log4cplus_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "log4cplus_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "log4cplus_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "log4cplus_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LOG4CPLUS_DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LOG4CPLUS_BUILD_DLL" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /dll /machine:I386 /out:"Release/log4cplus.dll"

!ELSEIF  "$(CFG)" == "log4cplus_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LOG4CPLUS_DLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LOG4CPLUS_BUILD_DLL" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /dll /debug /machine:I386 /out:"Debug/log4cplusd.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "log4cplus_dll - Win32 Release"
# Name "log4cplus_dll - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\appender.cxx
# End Source File
# Begin Source File

SOURCE=..\src\appenderattachableimpl.cxx
# End Source File
# Begin Source File

SOURCE=..\src\configurator.cxx
# End Source File
# Begin Source File

SOURCE=..\src\consoleappender.cxx
# End Source File
# Begin Source File

SOURCE=..\src\factory.cxx
# End Source File
# Begin Source File

SOURCE=..\src\fileappender.cxx
# End Source File
# Begin Source File

SOURCE=..\src\filter.cxx
# End Source File
# Begin Source File

SOURCE="..\src\global-init.cxx"
# End Source File
# Begin Source File

SOURCE=..\src\hierarchy.cxx
# End Source File
# Begin Source File

SOURCE=..\src\hierarchylocker.cxx
# End Source File
# Begin Source File

SOURCE=..\src\layout.cxx
# End Source File
# Begin Source File

SOURCE=..\src\logger.cxx
# End Source File
# Begin Source File

SOURCE=..\src\loggerimpl.cxx
# End Source File
# Begin Source File

SOURCE=..\src\loggingevent.cxx
# End Source File
# Begin Source File

SOURCE=..\src\loggingserver.cxx
# End Source File
# Begin Source File

SOURCE=..\src\loglevel.cxx
# End Source File
# Begin Source File

SOURCE=..\src\loglog.cxx
# End Source File
# Begin Source File

SOURCE=..\src\logloguser.cxx
# End Source File
# Begin Source File

SOURCE=..\src\ndc.cxx
# End Source File
# Begin Source File

SOURCE=..\src\nteventlogappender.cxx
# End Source File
# Begin Source File

SOURCE=..\src\nullappender.cxx
# End Source File
# Begin Source File

SOURCE=..\src\objectregistry.cxx
# End Source File
# Begin Source File

SOURCE=..\src\patternlayout.cxx
# End Source File
# Begin Source File

SOURCE=..\src\pointer.cxx
# End Source File
# Begin Source File

SOURCE=..\src\property.cxx
# End Source File
# Begin Source File

SOURCE=..\src\rootlogger.cxx
# End Source File
# Begin Source File

SOURCE=..\src\sleep.cxx
# End Source File
# Begin Source File

SOURCE="..\src\socket-win32.cxx"
# End Source File
# Begin Source File

SOURCE=..\src\socket.cxx
# End Source File
# Begin Source File

SOURCE=..\src\socketappender.cxx
# End Source File
# Begin Source File

SOURCE=..\src\socketbuffer.cxx
# End Source File
# Begin Source File

SOURCE=..\src\stringhelper.cxx
# End Source File
# Begin Source File

SOURCE=..\src\threads.cxx
# End Source File
# Begin Source File

SOURCE=..\src\timehelper.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\log4cplus\appender.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\spi\appenderattachable.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\helpers\appenderattachableimpl.h
# End Source File
# Begin Source File

SOURCE="..\include\log4cplus\config-macosx.h"
# End Source File
# Begin Source File

SOURCE="..\include\log4cplus\config-win32.h"
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\config.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\configurator.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\consoleappender.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\spi\factory.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\fileappender.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\spi\filter.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\fstreams.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\hierarchy.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\hierarchylocker.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\layout.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\logger.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\spi\loggerfactory.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\spi\loggerimpl.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\spi\loggingevent.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\loggingmacros.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\loglevel.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\helpers\loglog.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\helpers\logloguser.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\ndc.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\nteventlogappender.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\nullappender.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\spi\objectregistry.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\helpers\pointer.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\helpers\property.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\spi\rootlogger.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\helpers\sleep.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\helpers\socket.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\socketappender.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\helpers\socketbuffer.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\streams.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\helpers\stringhelper.h
# End Source File
# Begin Source File

SOURCE="..\include\log4cplus\helpers\thread-config.h"
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\helpers\threads.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\helpers\timehelper.h
# End Source File
# Begin Source File

SOURCE=..\include\log4cplus\tstring.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
