all:
	mkdir -p build && cd build && mpic++ ../main.cpp && mpirun -np 4 ./a.out > output.txt
debug:
	rm -f ./a.out && mpic++ ./main.cpp && mpirun -np 4 ./a.out > output.txt