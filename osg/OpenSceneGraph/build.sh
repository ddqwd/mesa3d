#!bin/sh -e 
#rm -rf build 
cmake -Bbuild \
 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DBUILD_OSG_EXAMPLES=ON \
 -DCMAKE_BUILD_TYPE=Debug  
cd build 
make 
cmake --install . --prefix=`pwd`/install
