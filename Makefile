all:
	mkdir -p build && cd build && mpic++ ../main.cpp && mpirun -np 4 ./a.out > output.txt

debug:
	rm -f ./a.out && mpic++ ./main.cpp && mpirun -np 4 ./a.out > output.

test:
	mkdir -p build/tests_non_mpi && \
	cd build && g++ ../tests.cpp -o tests_non_mpi/tests && ./tests_non_mpi/tests -s > tests_non_mpi/output.txt

test_mpi:
	mkdir -p build/tests_mpi && \
	cd build && mpic++ ../tests_mpi.cpp -o tests_mpi/tests && mpirun -np 4 ./tests_mpi/tests -s -r compact > tests_mpi/output.txt