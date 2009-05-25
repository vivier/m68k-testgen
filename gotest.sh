#!/bin/bash 

source ./gen-asm-include.sh

DIR="/mnt/next"

mkdir "$DIR"

RUNTEST()
{
   OPCODE="$1"
   PARAMS="$2"

   echo Starting test for $OPCODE $PARAMS

   INIT
   count=0
   WRITE $OPCODE $PARAMS
   FINISH

   if ! make m68k-testgen > /dev/null
   then
      exit 1;
   fi
   
   ./m68k-testgen -v --registers="$PARAMS" | \
      gzip -1 -c > "${DIR}/m68040-opcode-${OPCODE}.txt.gz"
   if [ ! $? ]
   then
      echo failing $OPCODE $PARAMS test.
      exit 1
   fi

   echo done $OPCODE $PARAMS test.
   echo 
}

source ./gen-asm-list.sh
