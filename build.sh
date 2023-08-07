#!bin/sh
rm -rf build_debug
meson build_debug  --native-file llvm.ini \
      -Dprefix=`pwd`/install_debug \
      -Dlibdir=lib \
      -Dbuildtype=debug \
      -Dlibunwind=false  \
      -Ddri3=true  \
      -Dllvm=true \
      -Dgallium-drivers=radeonsi,swrast \
      -Dglvnd=true \
      -Dglx-direct=true \
      -Dgbm=true \
      -Dlmsensors=true \
      -Dgles1=false \
      -Dgles2=true \
	  -Dshared-llvm=false \
	  -Dcpp_args='-fcommon' \
	  -Dc_args='-fcommon' \
      -Dvulkan-drivers=""

cd build_debug
meson configure
ninja -j3 -v
ninja install

