all:
	mkdir -p build && cd build && mpic++ ../main.cpp && mpirun -np 4 ./a.out > output.txt

debug:
	rm -f ./a.out && mpic++ ./main.cpp && mpirun -np 4 ./a.out > output.

test_serial:
	mkdir -p build/tests_serial && \
	cd build && g++ ../tests.cpp -o tests_serial/tests && ./tests_serial/tests -s > tests_serial/output.txt

test_mpi:
	mkdir -p build/tests_mpi && \
	cd build && mpic++ ../tests_mpi.cpp -o tests_mpi/tests && mpirun -np 4 ./tests_mpi/tests -s -r compact > tests_mpi/output.txt