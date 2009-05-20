#!/usr/bin/bash

#############################################################
# gen-asm.sh - opcode test assembly generator
#
# Copyright (C) MMIV Ray A. Arachelian, All Rights Reserved
#         Released under the GNU GPL 2.0 License.
#
#############################################################


INIT

 for d in r l;
 do
  for s in l w b;
  do
   WRITE "ro${d}${s}" "d0,d1"
  done
 done


FINISH
