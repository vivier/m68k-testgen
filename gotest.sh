#!/bin/bash 

source ./gen-asm-include.sh

DIR="/mnt/next"

mkdir "$DIR"

RUNTEST()
{
   OPCODE="$1"
   PARAMS="$2"

   echo Starting test for $OPCODE $PARAMS
   echo generating test for $OPCODE $PARAMS...

   INIT
   count=0
   WRITE $OPCODE $PARAMS
   FINISH

   if ! make m68k-testgen > /dev/null
   then
      exit 1;
   fi
   
   echo Running $1 $2 test...
   ./m68k-testgen --directory="$DIR" --compress="gzip -1" --registers="$PARAMS"
   echo done $OPCODE $PARAMS test.
   echo 
}

source ./gen-asm-list.sh
