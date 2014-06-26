if [ ! -d build ]; then
	mkdir build
fi

cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-powerpc.cmake ..
make -j2
cd ..
