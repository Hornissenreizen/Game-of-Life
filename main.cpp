#include "game_of_life_mpi.hpp"

// some global constants
const size_t NO_TICKS = 44;
const size_t PROC_ROWS = 2;
const size_t PROC_COLS = 2;

const int ROOT = 0;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    MPIProcess mpi_proc("../init.pgm", PROC_ROWS, PROC_COLS, ROOT);

    for (size_t i = 0; i < NO_TICKS; i++) {
        mpi_proc.exchange();
        mpi_proc.tick();
    }

    mpi_proc.to_pgm("result.pgm");

    MPI_Finalize();
    return 0;
}