all:
	mkdir -p build && cd build && g++ ../game_of_life.cpp && ./a.out > output.txt