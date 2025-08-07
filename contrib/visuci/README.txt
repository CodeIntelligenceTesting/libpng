Microsoft Developer Studio Build File, Format Version 6.00 for VisualCi
------------------------------------------------------------------------

Copyright 2000, Willem van Schaik.

This code is released under the libci license.
For conditions of distribution and use, see the disclaimer
and license in ci.h

As a CI .dll demo VisualCi is finished. More features would only hinder
the program's objective. However, further extensions (like support for other
graphics formats) are in development. To get these, or for pre-compiled
binaries, go to "http://www.schaik.com/ci/visualci.html".

------------------------------------------------------------------------

Assumes that

   libci DLLs and LIBs are in ..\..\projects\msvc\win32\libci
   zlib DLLs and LIBs are in   ..\..\projects\msvc\win32\zlib
   libci header files are in  ..\..\..\libci
   zlib header files are in    ..\..\..\zlib
   the cisuite images are in  ..\cisuite

To build:

1) On the main menu Select "Build|Set Active configuration".
   Choose the configuration that corresponds to the library you want to test.
   This library must have been built using the libci MS project located in
   the "..\..\mscv" subdirectory.

2) Select "Build|Clean"

3) Select "Build|Rebuild All"

4) After compiling and linking VisualCi will be started to view an image
   from the CiSuite directory.  Press Ctrl-N (and Ctrl-V) for other images.


To install:

When distributing VisualCi (or a further development) the following options
are available:

1) Build the program with the configuration "Win32 LIB" and you only need to
   include the executable from the ./lib directory in your distribution.

2) Build the program with the configuration "Win32 DLL" and you need to put
   in your distribution the executable from the ./dll directory and the dll's
   libci1.dll, zlib.dll and msvcrt.dll.  These need to be in the user's PATH.


Willem van Schaik
Calgary, June 6th 2000

P.S. VisualCi was written based on preliminary work of:

    - Simon-Pierre Cadieux
    - Glenn Randers-Pehrson
    - Greg Roelofs

