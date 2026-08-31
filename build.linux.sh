# 1:01 AM 8/28/2026
# Rajib Chy

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_HTTP_DAY_STATUS=OFF ..
sudo make install


mkdir build-debug
cd build-debug
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DUSE_HTTP_DAY_STATUS=OFF ..
ninja

cd ..
mkdir build-ninja
cd build-ninja
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
ninja
