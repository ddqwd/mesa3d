#!bin/bash

sudo apt-get  install libxshmfence* libxxf86vm* libxrandr* libelf* libxrandr*         

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

