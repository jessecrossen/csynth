#!/bin/bash

cd lib
for F in *.h; do
  sed -n -e 's/.*\/\/\/\(.*\)/\1/p' "$F" > "../docs/$F.md"
done
cd -

