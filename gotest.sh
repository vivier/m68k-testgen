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

   if ! make mc68000-tester
   then
      exit 1;
   fi
   
   echo Running $1 $2 test...
   ./mc68000-tester
   echo done $1 $2 test.
   echo 
}

source ./gen-asm-list.sh
