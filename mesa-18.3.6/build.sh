#!bin/sh
rm -rf build
meson setup  build\
	  --native-file llvm.ini \
      --prefix="$PWD/build/install"\
	  --install-umask=0000\
      -Dlibdir=lib \
      -Dbuildtype=debug \
	  -Dplatforms=x11,drm \
      -Ddri3=true  \
      -Ddri-drivers='' \
      -Dgallium-drivers=radeonsi,swrast\
	  -Dgallium-vdpau=false \
	  -Dgallium-omx=disabled\
	  -Dgallium-va=false \
	  -Dgallium-xa=false \
	  -Dgallium-nine=false \
	  -Dgallium-opencl=icd\
      -Dvulkan-drivers="amd" \
      -Dgles1=false \
      -Dgles2=false\
	  -Dopengl=true\
	  -Dgbm=true\
	  -Dglx=auto \
	  -Degl=true\
	  -Dglvnd=false \
      -Dllvm=true \
	  -Dshared-llvm=false \
	  -Dvalgrind=false \
      -Dlibunwind=false  \
	  -Dlmsensors=false \
	  -Dbuild-tests=false \
	  -Dselinux=false \
	  -Dosmesa=none\
	  -Dpower8=false\
	  -Dxlib-lease=auto\
	  -Dglx-direct=true\
	  -Dcpp_args='-fcommon' \
	  -Dtools=glsl \
	  -Dc_args=' -fcommon -z muldefs' \
	  -Dc_link_args=' -fcommon' \
	  -Dcpp_link_args=' -fcommon -z muldefs' 


#meson compile build --ninja-args="-j $(nproc) -t compdb"
#
cd build
ninja -j $(nproc) -t compdb
ninja install

