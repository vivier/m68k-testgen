#!/bin/bash 

source ./gen-asm-include.sh

RUNTEST()
{
   echo Starting test for $1 $2
   echo generating test for $1 $2...

   INIT
   count=0
   WRITE $1 $2
   FINISH

   if ! make m68k-testgen
   then
      exit 1;
   fi
   
   echo Running $1 $2 test...
   ./m68k-testgen --directory=/mnt/next
   echo done $1 $2 test.
   echo 
}

source ./gen-asm-list.sh
