#!/bin/bash

for SCRIPT in $(find . -name build.sh)
do
  cd $(dirname $SCRIPT)
  ./build.sh
  cd -
done
