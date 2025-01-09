#ifndef GAME_OF_LIFE_MPI_HPP
#define GAME_OF_LIFE_MPI_HPP

#include "game_of_life.hpp"
#include <stdexcept>
#include <mpi.h>

class MPIProcess {
    size_t proc_rows, proc_cols;        // Number of rows and columns in the MPI grid
    int proc_row, proc_col;             // Process coordinates in the grid
    int rank;                           // MPI rank of the process
    int root;                           // Root process rank
    size_t grid_rows, grid_cols;        // Dimensions of the global grid
    size_t subgrid_rows, subgrid_cols;  // Dimensions of this process's subgrid (IMPORTANT: Note that each dimension is two less than the actual dimensions of subgame because subgame has a border of 1 cell at each side)
    int starting_row, starting_col;     // Starting coordinates of the subgrid
    int ending_row, ending_col;         // Ending coordinates of the subgrid

    GameOfLife subgame;

    size_t neighbor_ranks[4];           // Ranks of the neighboring processes: N, S, E, W

    unsigned char* top_row_recv = nullptr;
    unsigned char* bottom_row_recv = nullptr;
    unsigned char* left_col_recv = nullptr;
    unsigned char* right_col_recv = nullptr;

public:
    MPIProcess(const GameOfLife& game, size_t proc_rows, size_t proc_cols, int root)
        : proc_rows(proc_rows), proc_cols(proc_cols), root(root), grid_rows(game.get_rows()), grid_cols(game.get_cols())
        {
        // Initialize MPI
        int size;
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        if (size != proc_rows * proc_cols) {
            throw std::runtime_error("Number of processes must be equal to proc_rows * proc_cols");
        }

        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        rank_to_coords(rank, proc_row, proc_col);

        // Calculate subgrid dimensions and positions
        subgrid_rows = grid_rows / proc_rows; // only temporary
        subgrid_cols = grid_cols / proc_cols; // same here
        starting_row = proc_row * subgrid_rows;
        starting_col = proc_col * subgrid_cols;
        ending_row = (proc_row == proc_rows - 1) ? grid_rows : starting_row + subgrid_rows;
        ending_col = (proc_col == proc_cols - 1) ? grid_cols : starting_col + subgrid_cols;
        subgrid_rows = ending_row - starting_row;
        subgrid_cols = ending_col - starting_col;

        subgame = game.subgame(starting_row - 1, starting_col - 1, ending_row + 1, ending_col + 1);

        // Calculate ranks of the neighboring processes
        neighbor_ranks[0] = coords_to_rank(proc_row - 1, proc_col);
        neighbor_ranks[1] = coords_to_rank(proc_row + 1, proc_col);
        neighbor_ranks[2] = coords_to_rank(proc_row, proc_col + 1);
        neighbor_ranks[3] = coords_to_rank(proc_row, proc_col - 1);

        top_row_recv = new unsigned char[subgame.get_cols() / 8 + 1];
        bottom_row_recv = new unsigned char[subgame.get_cols() / 8 + 1];
        left_col_recv = new unsigned char[subgame.get_rows() / 8 + 1];
        right_col_recv = new unsigned char[subgame.get_rows() / 8 + 1];
    }
    
    ~MPIProcess() {
        delete[] top_row_recv;
        delete[] bottom_row_recv;
        delete[] left_col_recv;
        delete[] right_col_recv;
    }

    // Maps a rank to row and column coordinates in the process grid
    inline void rank_to_coords(int rank, int& row, int& col) const {
        rank = MOD(rank, proc_rows * proc_cols);
        row = rank / proc_cols;
        col = rank % proc_cols;
    }

    inline size_t coords_to_rank(int row, int col) const {
        return MOD(row, proc_rows) * proc_cols + MOD(col, proc_cols);
    }

    inline void tick() {
        subgame.tick();
    }

    void exchange() {
        MPI_Request requests[4];
        MPI_Status statuses[4];

        std::vector<unsigned char> top_row_send = subgame.get_row(1);
        std::vector<unsigned char> bottom_row_send = subgame.get_row(-2);
        
        // Send and receive the border rows
        MPI_Isend(top_row_send.data(), top_row_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[0], 0, MPI_COMM_WORLD, &requests[0]);
        MPI_Isend(bottom_row_send.data(), bottom_row_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[1], 0, MPI_COMM_WORLD, &requests[1]);
        MPI_Recv(top_row_recv, top_row_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[0], 0, MPI_COMM_WORLD, &statuses[0]);
        MPI_Recv(bottom_row_recv, bottom_row_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[1], 0, MPI_COMM_WORLD, &statuses[1]);

        subgame.set_row(0, top_row_recv);
        subgame.set_row(-1, bottom_row_recv);
        std::vector<unsigned char> left_col_send = subgame.get_col(1);
        std::vector<unsigned char> right_col_send = subgame.get_col(-2);
        
        // Send and receive the border columns
        MPI_Isend(left_col_send.data(), left_col_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[3], 0, MPI_COMM_WORLD, &requests[2]);
        MPI_Isend(right_col_send.data(), right_col_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[2], 0, MPI_COMM_WORLD, &requests[3]);
        MPI_Recv(left_col_recv, left_col_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[3], 0, MPI_COMM_WORLD, &statuses[2]);
        MPI_Recv(right_col_recv, right_col_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[2], 0, MPI_COMM_WORLD, &statuses[3]);

        subgame.set_col(0, left_col_recv);
        subgame.set_col(-1, right_col_recv);

        MPI_Waitall(4, requests, statuses);
    }

    GameOfLife gather_subgrids() const {
        int sendcount = ((grid_rows / proc_rows) + MOD(grid_rows, proc_rows)) * ((grid_cols / proc_cols) + MOD(grid_cols, proc_cols)) / 8 + 1;
        unsigned char* recv_buffer = nullptr;
        if (rank == root) {
            recv_buffer = new unsigned char[sendcount * proc_rows * proc_cols];
        }
        GameOfLife send_subgame = subgame.subgame(1, 1, -1, -1);
        unsigned char* send_buffer = new unsigned char[sendcount];
        memcpy(send_buffer, send_subgame.data(), send_subgame.size());
        for (size_t i = send_subgame.size(); i < sendcount; i++) send_buffer[i] = 0;
        
        MPI_Gather(send_buffer, sendcount, MPI_UNSIGNED_CHAR, recv_buffer, sendcount, MPI_UNSIGNED_CHAR, root, MPI_COMM_WORLD);

        if (rank != root) return GameOfLife(0, 0);

        GameOfLife game(grid_rows, grid_cols);
        for (int i = 0; i < proc_rows; i++) {
            for (int j = 0; j < proc_cols; j++) {
                Grid current_sub_grid((i == proc_rows - 1) ? grid_rows - i * subgrid_rows : subgrid_rows,
                                      (j == proc_cols - 1) ? grid_cols - j * subgrid_cols : subgrid_cols,
                                      recv_buffer + (i * proc_cols + j) * sendcount);
                game.set_subgame(i * (grid_rows / proc_rows), j * (grid_cols / proc_cols), current_sub_grid);
                current_sub_grid._nullify();
            }
        }
        delete[] recv_buffer;
        return game;
    }

    // Accessors for the subgrid dimensions
    size_t get_subgrid_rows() const { return subgrid_rows; }
    size_t get_subgrid_cols() const { return subgrid_cols; }
    size_t get_starting_row() const { return starting_row; }
    size_t get_starting_col() const { return starting_col; }
    size_t get_ending_row() const { return ending_row; }
    size_t get_ending_col() const { return ending_col; }
    int get_rank() const { return rank; }
    int get_proc_row() const { return proc_row; }
    int get_proc_col() const { return proc_col; }

    void print() const {
        std::cout << "Rank: " << rank << " (" << proc_row << ", " << proc_col << ")\n";
        subgame.print();
    }

    // Debugging: Print process information
    void print_info() const {
        std::cout << "Rank: " << rank << " (" << proc_row << ", " << proc_col << ")\n";
        std::cout << "Subgrid: rows [" << starting_row << "-" << ending_row
                  << "), cols [" << starting_col << "-" << ending_col << ")\n";
    }
};

#endif