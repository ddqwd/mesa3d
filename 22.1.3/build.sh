#!bin/bash

rm -rf build
meson build \
    -Dprefix="$PWD"/install \
    -Dbuildtype=debug \
    -Dgallium-drivers=swrast,radeonsi \
    -Dbuild-tests=true \
    -Dvulkan-drivers=   \   
    -Dplatforms=x11 
cd build
ninja -j2 

