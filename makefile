default: bf
# run the tcc task once for each platform
tcc:
	cd tinycc && \
	./configure && \
	make && \
	cd ../ && \
	mkdir tcc && \
	cp tinycc/libtcc1.a tcc
bf: bf.cc
	clang++ -std=c++17 bf.cc -o bf -O3 -Ltinycc -ltcc

clean:
	rm -rf tcc && cd tinycc && make clean
