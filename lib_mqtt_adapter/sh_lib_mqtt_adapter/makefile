
all:
	mkdir -p ../__cmake/Debug
	cmake -DCMAKE_BUILD_TYPE=Debug -H. -B../__cmake/Debug
	make -C../__cmake/Debug -j8 VERBOSE=1 all
	mkdir -p ../__cmake/Release
	cmake -DCMAKE_BUILD_TYPE=Release -H. -B../__cmake/Release
	make -C../__cmake/Release -j8 VERBOSE=1 all


clean:
	make -C../__cmake/Debug clean
	make -C../__cmake/Release clean
	rm ../__cmake/* -rf 