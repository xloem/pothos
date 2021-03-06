#!/bin/bash

BRANCH=$1

COMPONENTS="
widgets
plotters
gui
library
python
java
util
opencl
blocks
comms
serialization
sdr
audio
"

for PREFIX in $COMPONENTS; do
    git subtree push --prefix=${PREFIX} git@github.com:pothosware/pothos-${PREFIX}.git ${BRANCH}
done
