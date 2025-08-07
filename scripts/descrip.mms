cc_defs = /inc=$(ZLIBSRC)
c_deb =

.ifdef __DECC__
pref = /prefix=all
.endif

OBJS = ci.obj, cierror.obj, ciget.obj, cimem.obj, cipread.obj,\
       ciread.obj, cirio.obj, cirtran.obj, cirutil.obj, ciset.obj,\
       citrans.obj, ciwio.obj, ciwrite.obj, ciwtran.obj, ciwutil.obj

CFLAGS = $(C_DEB) $(CC_DEFS) $(PREF)

all : citest.exe libci.olb
	@ write sys$output " citest available"

libci.olb : libci.olb($(OBJS))
	@ write sys$output " libci available"

citest.exe : citest.obj libci.olb
	link citest,libci.olb/lib,$(ZLIBSRC)libz.olb/lib

test : citest.exe
	run citest

clean :
	delete *.obj;*,*.exe;

# Other dependencies.
ci.obj :      ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
cierror.obj : ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
ciget.obj :   ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
cimem.obj :   ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
cipread.obj : ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
ciread.obj :  ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
cirio.obj :   ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
cirtran.obj : ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
cirutil.obj : ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
ciset.obj :   ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
citrans.obj : ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
ciwio.obj :   ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
ciwrite.obj : ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
ciwtran.obj : ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h
ciwutil.obj : ci.h, ciconf.h, cilibconf.h, cipriv.h, cistruct.h,ciinfo.h, cidebug.h

citest.obj :  ci.h, ciconf.h, cilibconf.h
