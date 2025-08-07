#!/bin/bash -eu

# Copyright 2017-2018 Glenn Randers-Pehrson
# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Revisions by Glenn Randers-Pehrson, 2017:
# 1. Build only the library, not the tools (changed "make -j$(nproc) all" to
#     "make -j$(nproc) libci16.la").
# 2. Disabled WARNING and WRITE options in cilibconf.dfa.
# 3. Build zlib alongside libci
################################################################################

# Disable logging via library build configuration control.
cat scripts/cilibconf.dfa | \
  sed -e "s/option STDIO/option STDIO disabled/" \
      -e "s/option WARNING /option WARNING disabled/" \
      -e "s/option WRITE enables WRITE_INT_FUNCTIONS/option WRITE disabled/" \
> scripts/cilibconf.dfa.temp
mv scripts/cilibconf.dfa.temp scripts/cilibconf.dfa

# build the libci library.
autoreconf -f -i
./configure --with-libci-prefix=OSS_FUZZ_
make -j$(nproc) clean
make -j$(nproc) libci16.la

# build libci_read_fuzzer.
$CXX $CXXFLAGS -std=c++11 -I. \
     $SRC/libci/contrib/oss-fuzz/libci_read_fuzzer.cc \
     -o $OUT/libci_read_fuzzer \
     -lFuzzingEngine .libs/libci16.a -lz

# add seed corpus.
find $SRC/libci -name "*.ci" | grep -v crashers | \
     xargs zip $OUT/libci_read_fuzzer_seed_corpus.zip

cp $SRC/libci/contrib/oss-fuzz/*.dict \
     $SRC/libci/contrib/oss-fuzz/*.options $OUT/
