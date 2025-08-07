Copyright (c) 2017 Glenn Randers-Pehrson

This code is released under the libci license.
For conditions of distribution and use, see the disclaimer
and license in ci.h

Files in this directory are used by the oss-fuzz project
(https://github.com/google/oss-fuzz/tree/master/projects/libci).
for "fuzzing" libci.

They were licensed by Google Inc, using the BSD-like Chromium license,
which may be found at https://cs.chromium.org/chromium/src/LICENSE, or, if
noted in the source, under the Apache-2.0 license, which may
be found at http://www.apache.org/licenses/LICENSE-2.0 .
If they have been modified, the derivatives are copyright Glenn Randers-Pehrson
and are released under the same licenses as the originals.  Several of
the original files (libci_read_fuzzer.options, ci.dict, project.yaml)
had no licensing information; we assumed that these were under the Chromium
license. Any new files are released under the libci license (see ci.h).

The files are
                            Original
 Filename                   or derived   Copyright          License
 =========================  ==========   ================   ==========
 Dockerfile*                derived      2017, Glenn R-P    Apache 2.0
 build.sh                   derived      2017, Glenn R-P    Apache 2.0
 libci_read_fuzzer.cc      derived      2017, Glenn R-P    Chromium
 libci_read_fuzzer.options original     2015, Chrome Devs  Chromium
 ci.dict                   original     2015, Chrome Devs  Chromium
 README.txt (this file)     original     2017, Glenn R-P    libci

 * Dockerfile is a copy of the file used by oss-fuzz. build.sh,
   ci.dict and libci_read_fuzzer.* are the actual files used by oss-fuzz,
   which retrieves them from the libci repository at Github.

To do: exercise the progressive reader and the ci encoder.
