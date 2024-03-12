#!bin/bash

rm -rf build
meson build \
	-Dprefix="$PWD"/install \
	-Dbuildtype=debug \
    -Dplatforms=x11,wayland \
    -Dgallium-rusticl="false" \
    -Dgallium-opencl="standalone"\
    -Dgallium-drivers="swrast" \
    -Dvulkan-drivers="swrast" \
    -Dvulkan-layers="device-select" \
    -Dcpp_args='-fcommon -ludev' \
    -Dc_args='-DDEBUG_STORE -fcommon -ludev'  \
    -Dc_link_args='-ludev' \
    -Dcpp_link_args='-ludev' \

cd build

ninja -j2  -t compdb

ninja install
