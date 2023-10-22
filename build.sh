#!/bin/bash

mkdir -p build
cd build
qmake "CONFIG += warn_off debug" ../src

make

cd ../

ln -sTf build/it100mqtt it100mqtt

# copy example settings if no local settings present
cp -n settings-example.cfg settings.cfg

#echo
#echo restarting it100mqtt service..
#sudo systemctl restart it100mqtt

# fini
echo
echo done
echo
