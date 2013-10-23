#!/bin/sh

#check if we have a repository
git ls-remote > /dev/null
if [ $? -ne 0 ];then
   echo 'Not using a git version'
   exit 0
fi

file=./client/version-revno.inc
tmpFile="$file.tmp"
git=`git describe --abbrev=4 --dirty=-d`
IFS='-'

set -- $git
echo "#define GIT_TAG $1" > $tmpFile
echo "#define GIT_COMMIT $2" >> $tmpFile

echo $4
if [ -z "$4" ]; then
   echo "#define GIT_HASH \"$3\"" >> $tmpFile
else
   echo 'Local modifications found'
   echo "#define GIT_HASH \"$3-$4\"" >> $tmpFile
fi

echo "#define GIT_COMMIT_COUNT `git rev-list HEAD --count`" >> $tmpFile

if diff -q "$file" "$tmpFile" > /dev/null; then
    : # files are the same
    rm "$tmpFile"
    echo 'No commit changes detected, using the old version file'
else
    : # files are different
    mv "$tmpFile" "$file"
    echo 'Version file generated'
fi
