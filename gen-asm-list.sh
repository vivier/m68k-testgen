###########################
# 68040 tests

###########################
# modify 68000-tester.c to not support d2


 mkdir /mnt/next
 
 #RUNTEST "bfchg"   "d0{d1:d2}"
 #RUNTEST "bfclr"   "d0{d1:d2}"
 #RUNTEST "bfexts"  "d0{d1:d2}"
 #RUNTEST "bfextu"  "d0{d1:d2}"
 #RUNTEST "bfffo"   "d0{d1:d2}"
 #RUNTEST "bfins"   "d0{d1:d2}"
 #RUNTEST "bfset"   "d0{d1:d2}"
 #RUNTEST "bftst"   "d0{d1:d2}"

# RUNTEST "rorl" "d0,d1"
# RUNTEST "rorw" "d0,d1"
# RUNTEST "rorb" "d0,d1"
# RUNTEST "roll" "d0,d1"
# RUNTEST "rolw" "d0,d1"
# RUNTEST "rolb" "d0,d1"
 RUNTEST "roxrl" "d0,d1"
 RUNTEST "roxrw" "d0,d1"
 RUNTEST "roxrb" "d0,d1"
 RUNTEST "roxll" "d0,d1"
 RUNTEST "roxlw" "d0,d1"
 RUNTEST "roxlb" "d0,d1"

## modify 68000-tester.c to support d2
#
# RUNTEST "mulul" "d2,d0:d1"
# RUNTEST "mulsl" "d2,d0:d1"
# RUNTEST "divul" "d0,d2:d1"
# RUNTEST "divsl" "d0,d2:d1"
#
## modify 68000-tester.c to support writing d2 over #0
#
# RUNTEST "pack d0,d1,#0"
# RUNTEST "unpk d0,d1,#0"
#
#
############################
## standard 68000 opcodes.
############################
#
## modify 68000-tester.c to not support d2
#
#  RUNTEST "addl" "d0,d1"
#  RUNTEST "divs" "d0,d1"
#  RUNTEST "divu" "d0,d1"
#  RUNTEST "bchg" "d0,d1"
#  RUNTEST "bclr" "d0,d1"
#  RUNTEST "bset" "d0,d1"
#  RUNTEST "abcd" "d0,d1"
#  RUNTEST "sbcd" "d0,d1"
#  RUNTEST "addl" "d0,d1"
#  RUNTEST "addw" "d0,d1"
#  RUNTEST "addb" "d0,d1"
#  RUNTEST "addxl" "d0,d1"
#  RUNTEST "addxw" "d0,d1"
#  RUNTEST "addxb" "d0,d1"
#  RUNTEST "andl" "d0,d1"
#  RUNTEST "andw" "d0,d1"
#  RUNTEST "andb" "d0,d1"
#  RUNTEST "cmpl" "d0,d1"
#  RUNTEST "cmpw" "d0,d1"
#  RUNTEST "cmpb" "d0,d1"
#  RUNTEST "eorl" "d0,d1"
#  RUNTEST "eorw" "d0,d1"
#  RUNTEST "eorb" "d0,d1"
#  RUNTEST "orl" "d0,d1"
#  RUNTEST "orw" "d0,d1"
#  RUNTEST "orb" "d0,d1"
#  RUNTEST "subl" "d0,d1"
#  RUNTEST "subw" "d0,d1"
#  RUNTEST "subb" "d0,d1"
#  RUNTEST "subxl" "d0,d1"
#  RUNTEST "subxw" "d0,d1"
#  RUNTEST "subxb" "d0,d1"
#  RUNTEST "lsrl" "d0,d1"
#  RUNTEST "lsrw" "d0,d1"
#  RUNTEST "lsrb" "d0,d1"
#  RUNTEST "lsll" "d0,d1"
#  RUNTEST "lslw" "d0,d1"
#  RUNTEST "lslb" "d0,d1"
#  RUNTEST "asrl" "d0,d1"
#  RUNTEST "asrw" "d0,d1"
#  RUNTEST "asrb" "d0,d1"
#  RUNTEST "asll" "d0,d1"
#  RUNTEST "aslw" "d0,d1"
#  RUNTEST "aslb" "d0,d1"
#
## modify 68000-test.c to only use d1.
#
#RUNTEST "extw" "d1"
#RUNTEST "extl" "d1"
#RUNTEST "clrl" "d1"
#RUNTEST "clrw" "d1"
#RUNTEST "clrb" "d1"
#RUNTEST "negl" "d1"
#RUNTEST "negw" "d1"
#RUNTEST "negb" "d1"
#RUNTEST "negxl" "d1"
#RUNTEST "negxw" "d1"
#RUNTEST "negxb" "d1"
#RUNTEST "notl" "d1"
#RUNTEST "notw" "d1"
#RUNTEST "notb" "d1"
#RUNTEST "nbcd" "d1"
#RUNTEST "scc" "d1"
#RUNTEST "scs" "d1"
#RUNTEST "seq" "d1"
#RUNTEST "sge" "d1"
#RUNTEST "sgt" "d1"
#RUNTEST "shi" "d1"
#RUNTEST "sle" "d1"
#RUNTEST "sls" "d1"
#RUNTEST "slt" "d1"
#RUNTEST "smi" "d1"
#RUNTEST "sne" "d1"
#RUNTEST "spl" "d1"
#RUNTEST "svc" "d1"
#RUNTEST "svs" "d1"
#RUNTEST "tas" "d1"