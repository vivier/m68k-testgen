#!/usr/bin/bash

#############################################################
# gen-asm.sh - opcode test assembly generator
#
# Copyright (C) MMIV Ray A. Arachelian, All Rights Reserved
#         Released under the GNU GPL 2.0 License.
#
#############################################################

source ./gen-asm-include.sh

INIT

#byte only - single parameter
for o in nbcd scc scs seq sge sgt shi sle sls slt smi sne spl svc svs tas
do
   WRITE "${o}" "d1"
done

FINISH
