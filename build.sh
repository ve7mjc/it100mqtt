#!/bin/bash

mkdir -p build
cd build
qmake "CONFIG += warn_off debug" ../src

make

cd ../

ln -sTf build/it100mqtt it100mqtt
cp -n settings-example.cfg settings.cfg

