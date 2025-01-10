#include <mpi.h>
#include "game_of_life_mpi.hpp"

// some global constants
const size_t NO_TICKS = 44;
const size_t PROC_ROWS = 2;
const size_t PROC_COLS = 2;

const int ROOT = 0;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    GameOfLife game(11, 17);
    game.init({{0,1},{1,2},{2,0},{2,1},{2,2}});

    size_t rows;
    size_t cols;

    if (argc < 3) {
	rows = PROC_ROWS;
	cols = PROC_COLS;
    } else {
    	rows = atoi(argv[1]);
    	cols = atoi(argv[2]);
    }

	   

    MPIProcess mpi_proc(game, rows, cols, ROOT);

    MPI_Barrier(MPI_COMM_WORLD);
    auto start = MPI_Wtime();
    for (size_t i = 0; i < NO_TICKS; i++) {
        mpi_proc.exchange();
        mpi_proc.tick();
    }

    GameOfLife result = mpi_proc.gather_subgrids();
    if (mpi_proc.get_rank() == ROOT) {
        result.print();
	result.to_pgm("Ende.ppm");
    }
    MPI_Barrier(MPI_COMM_WORLD);
    auto end = MPI_Wtime();

    if (mpi_proc.get_rank() == ROOT) {
	    std::cout << "runtime: " << end - start << std::endl;
    }

    MPI_Finalize();
    return 0;
}
