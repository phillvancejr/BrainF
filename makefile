default: all
all: yasm tcc bf

.PHONY: yasm tcc
# build cmake statically
cmake_flags= -D BUILD_SHARED_LIBS=OFF -D YASM_BUILD_TESTS=OFF -D CMAKE_BUILD_TYPE=MinSizeRel
# run the tcc task once for each platform
yasm:
	cd yasm && \
	mkdir -p build && \
	cd build && \
	cmake $(cmake_flags) .. && \
	cmake --build . && \
	cd ../../ && \
	mkdir -p backends && \
	cp yasm/build/yasm backends


# run the tcc task once for each platform
tcc:
	cd tinycc && \
	./configure && \
	make && \
	cd ../ && \
	mkdir -p backends && \
	cp tinycc/libtcc1.a backends

bf: bf.cc
	clang++ -std=c++17 bf.cc -o bf -O3 -Ltinycc -ltcc

clean:
	rm -rf backends && \
	cd tinycc && make clean && \
	cd ../yasm && rm -rf build
asm:
	yasm/build/yasm -f macho64 bf_mac.s && \
	ld -macosx_version_min 10.10 bf_mac.o -o bf_mac -lSystem
