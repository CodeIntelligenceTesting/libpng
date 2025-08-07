$! make libci under VMS
$!
$!
$! Check for MMK/MMS
$!
$! This procedure accepts one parameter (contrib), which causes it to build
$! the programs from the contrib directory instead of libci.
$!
$ p1 = f$edit(p1,"UPCASE")
$ if p1 .eqs. "CONTRIB"
$ then
$   set def [.contrib.gregbook]
$   @makevms
$   set def [-.ciminus]
$   @makevms
$   set def [--]
$   exit
$ endif
$ Make = ""
$ If F$Search ("Sys$System:MMS.EXE") .nes. "" Then Make = "MMS"
$ If F$Type (MMK) .eqs. "STRING" Then Make = "MMK"
$!
$! Look for the compiler used
$!
$ zlibsrc = "[-.zlib]"
$ ccopt="/include=''zlibsrc'"
$ if f$getsyi("HW_MODEL").ge.1024
$ then
$  ccopt = "/prefix=all"+ccopt
$  comp  = "__decc__=1"
$  if f$trnlnm("SYS").eqs."" then define sys sys$library:
$ else
$  if f$search("SYS$SYSTEM:DECC$COMPILER.EXE").eqs.""
$   then
$    if f$trnlnm("SYS").eqs."" then define sys sys$library:
$    if f$search("SYS$SYSTEM:VAXC.EXE").eqs.""
$     then
$      comp  = "__gcc__=1"
$      CC :== GCC
$     else
$      comp = "__vaxc__=1"
$     endif
$   else
$    if f$trnlnm("SYS").eqs."" then define sys decc$library_include:
$    ccopt = "/decc/prefix=all"+ccopt
$    comp  = "__decc__=1"
$  endif
$ endif
$!
$! Build the thing plain or with mms/mmk
$!
$ write sys$output "Compiling Libci sources ..."
$ if make.eqs.""
$  then
$   dele citest.obj;*
$   CALL MAKE ci.OBJ "cc ''CCOPT' ci" -
	ci.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE ciset.OBJ "cc ''CCOPT' ciset" -
	ciset.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE ciget.OBJ "cc ''CCOPT' ciget" -
	ciget.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE ciread.OBJ "cc ''CCOPT' ciread" -
	ciread.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE cipread.OBJ "cc ''CCOPT' cipread" -
	cipread.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE cirtran.OBJ "cc ''CCOPT' cirtran" -
	cirtran.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE cirutil.OBJ "cc ''CCOPT' cirutil" -
	cirutil.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE cierror.OBJ "cc ''CCOPT' cierror" -
	cierror.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE cimem.OBJ "cc ''CCOPT' cimem" -
	cimem.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE cirio.OBJ "cc ''CCOPT' cirio" -
	cirio.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE ciwio.OBJ "cc ''CCOPT' ciwio" -
	ciwio.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE citrans.OBJ "cc ''CCOPT' citrans" -
	citrans.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE ciwrite.OBJ "cc ''CCOPT' ciwrite" -
	ciwrite.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE ciwtran.OBJ "cc ''CCOPT' ciwtran" -
	ciwtran.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   CALL MAKE ciwutil.OBJ "cc ''CCOPT' ciwutil" -
	ciwutil.c ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
$   write sys$output "Building Libci ..."
$   CALL MAKE libci.OLB "lib/crea libci.olb *.obj" *.OBJ
$   write sys$output "Building citest..."
$   CALL MAKE citest.OBJ "cc ''CCOPT' citest" -
	citest.c ci.h ciconf.h cilibconf.h
$   call make citest.exe -
	"LINK citest,libci.olb/lib,''zlibsrc'libz.olb/lib" -
	citest.obj libci.olb
$   write sys$output "Testing Libci..."
$   run citest
$  else
$   if f$search("DESCRIP.MMS") .eqs. "" then copy/nolog [.SCRIPTS]DESCRIP.MMS []
$   'make'/macro=('comp',zlibsrc='zlibsrc')
$  endif
$ write sys$output "Libci build completed"
$ exit
$!
$!
$MAKE: SUBROUTINE   !SUBROUTINE TO CHECK DEPENDENCIES
$ V = 'F$Verify(0)
$! P1 = What we are trying to make
$! P2 = Command to make it
$! P3 - P8  What it depends on
$
$ If F$Search(P1) .Eqs. "" Then Goto Makeit
$ Time = F$CvTime(F$File(P1,"RDT"))
$arg=3
$Loop:
$       Argument = P'arg
$       If Argument .Eqs. "" Then Goto Exit
$       El=0
$Loop2:
$       File = F$Element(El," ",Argument)
$       If File .Eqs. " " Then Goto Endl
$       AFile = ""
$Loop3:
$       OFile = AFile
$       AFile = F$Search(File)
$       If AFile .Eqs. "" .Or. AFile .Eqs. OFile Then Goto NextEl
$       If F$CvTime(F$File(AFile,"RDT")) .Ges. Time Then Goto Makeit
$       Goto Loop3
$NextEL:
$       El = El + 1
$       Goto Loop2
$EndL:
$ arg=arg+1
$ If arg .Le. 8 Then Goto Loop
$ Goto Exit
$
$Makeit:
$ VV=F$VERIFY(0)
$ write sys$output P2
$ 'P2
$ VV='F$Verify(VV)
$Exit:
$ If V Then Set Verify
$ENDSUBROUTINE
