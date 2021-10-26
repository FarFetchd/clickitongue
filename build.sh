#!/bin/bash

BUILDFILENAME="default.ccbuildfile"
if [ -n "$1" ]; then
  BUILDFILENAME="$1"
fi

OUTPUTFILENAME=`grep "OutputBinaryFilename=" $BUILDFILENAME | sed 's/OutputBinaryFilename=//'`
COMPILECOMMANDPREFIX=`grep "CompileCommandPrefix=" $BUILDFILENAME | sed 's/CompileCommandPrefix=//'`
LIBRARIESTOLINK=`grep "LibrariesToLink=" $BUILDFILENAME | sed 's/LibrariesToLink=//'`

echo "
*******************************************************************************
Doing a full rebuild of $OUTPUTFILENAME. If you expect to compile more than this
one time, consider installing https://github.com/FarFetchd/ccsimplebuild.
It will recompile only what is necessary.
*******************************************************************************"

mkdir obj >/dev/null 2>/dev/null
for cur_fname in `ls *.cc | sed 's/\.cc//'` ; do
  CMDTORUN="$COMPILECOMMANDPREFIX -c -o obj/$cur_fname.o $cur_fname.cc"
  echo "$CMDTORUN"
  $CMDTORUN
done
LINKCOMMAND="$COMPILECOMMANDPREFIX "
for cur_fname in `ls *.cc | sed 's/\.cc//'` ; do
  LINKCOMMAND="$LINKCOMMAND obj/$cur_fname.o "
done
LINKCOMMAND="$LINKCOMMAND -o $OUTPUTFILENAME $LIBRARIESTOLINK"
echo "$LINKCOMMAND"
$LINKCOMMAND
