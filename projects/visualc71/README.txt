Microsoft Developer Studio Project File, Format Version 7.10 for libci.

Copyright (C) 2004 Simon-Pierre Cadieux.

This code is released under the libci license.
For conditions of distribution and use, see copyright notice in ci.h

NOTE: This project will be removed from libci-1.5.0.  It has
been replaced with the "vstudio" project.

Assumptions:
* The libci source files are in ..\..
* The zlib source files are in ..\..\..\zlib
* The zlib project file is in . /* Warning: This is until the zlib project
  files get integrated into the next zlib release. The final zlib project
  directory will then be ..\..\..\zlib\projects\visualc71. */

To use:

1) On the main menu, select "File | Open Solution".
   Open "libci.sln".

2) Display the Solution Explorer view (Ctrl+Alt+L)

3) Set one of the project as the StartUp project. If you just want to build the
   binaries set "libci" as the startup project (Select "libci" tree view
   item + Project | Set as StartUp project). If you want to build and test the
   binaries set it to "citest" (Select "citest" tree view item +
   Project | Set as StartUp project)

4) Select "Build | Configuration Manager...".
   Choose the configuration you wish to build.

5) Select "Build | Clean Solution".

6) Select "Build | Build Solution (Ctrl-Shift-B)"

This project builds the libci binaries as follows:

* Win32_DLL_Release\libci16.dll      DLL build
* Win32_DLL_Debug\libci16d.dll       DLL build (debug version)
* Win32_DLL_VB\libci16vb.dll         DLL build for Visual Basic, using stdcall
* Win32_LIB_Release\libci.lib        static build
* Win32_LIB_Debug\libcid.lib         static build (debug version)

Notes:

If you change anything in the source files, or select different compiler
settings, please change the DLL name to something different than any of
the above names. Also, make sure that in your "ciusr.h" you define
CI_USER_PRIVATEBUILD and CI_USER_DLLFNAME_POSTFIX according to the
instructions provided in "ciconf.h".

All DLLs built by this project use the Microsoft dynamic C runtime library
MSVCR71.DLL (MSVCR71D.DLL for debug versions).  If you distribute any of the
above mentioned libraries you may have to include this DLL in your package.
For a list of files that are redistributable in Visual Studio see
$(VCINSTALLDIR)\redist.txt.
