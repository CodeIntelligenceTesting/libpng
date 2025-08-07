$!------------------------------------------------------------------------------
$! make "CI: The Definitive Guide" demo programs (for X) under OpenVMS
$!
$! Script created by Martin Zinser for libci; modified by Greg Roelofs
$! for standalone cibook source distribution.
$!
$!
$!    Set locations where zlib and libci sources live.
$!
$ zpath   = ""
$ cipath = ""
$!
$ if f$search("[---.zlib]zlib.h").nes."" then zpath = "[---.zlib]"
$ if f$search("[--]ci.h").nes."" then cipath = "[--]"
$!
$ if f$search("[-.zlib]zlib.h").nes."" then zpath = "[-.zlib]"
$ if f$search("[-.libci]ci.h").nes."" then cipath = "[-.libci]"
$!
$ if zpath .eqs. ""
$ then
$   write sys$output "zlib include not found. Exiting..."
$   exit 2
$ endif
$!
$ if cipath .eqs. ""
$ then
$   write sys$output "libci include not found. Exiting..."
$   exit 2
$ endif
$!
$!    Look for the compiler used.
$!
$ ccopt="/include=(''zpath',''cipath')"
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
$ open/write lopt lib.opt
$ write lopt "''cipath'libci.olb/lib"
$ write lopt "''zpath'libz.olb/lib"
$ close lopt
$ open/write xopt x11.opt
$ write xopt "sys$library:decw$xlibshr.exe/share"
$ close xopt
$!
$!    Build 'em.
$!
$ write sys$output "Compiling CI book programs ..."
$   CALL MAKE readci.OBJ "cc ''CCOPT' readci" -
	readci.c readci.h
$   CALL MAKE readci2.OBJ "cc ''CCOPT' readci2" -
	readci2.c readci2.h
$   CALL MAKE writeci.OBJ "cc ''CCOPT' writeci" -
	writeci.c writeci.h
$   write sys$output "Building rci-x..."
$   CALL MAKE rci-x.OBJ "cc ''CCOPT' rci-x" -
	rci-x.c readci.h
$   call make rci-x.exe -
	"LINK rci-x,readci,lib.opt/opt,x11.opt/opt" -
	rci-x.obj readci.obj
$   write sys$output "Building rci2-x..."
$   CALL MAKE rci2-x.OBJ "cc ''CCOPT' rci2-x" -
	rci2-x.c readci2.h
$   call make rci2-x.exe -
	"LINK rci2-x,readci2,lib.opt/opt,x11.opt/opt" -
	rci2-x.obj readci2.obj
$   write sys$output "Building wci..."
$   CALL MAKE wci.OBJ "cc ''CCOPT' wci" -
	wci.c writeci.h
$   call make wci.exe -
	"LINK wci,writeci,lib.opt/opt" -
	wci.obj writeci.obj
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
