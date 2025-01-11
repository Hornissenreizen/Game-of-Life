#ifndef GAME_OF_LIFE_MPI_HPP
#define GAME_OF_LIFE_MPI_HPP

#include "game_of_life.hpp"
#include <stdexcept>
#include <sstream>
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
    MPIProcess(const std::string& filename, size_t proc_rows, size_t proc_cols, int root);
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
        MPI_Recv(bottom_row_recv, bottom_row_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[1], 0, MPI_COMM_WORLD, &statuses[1]);
        MPI_Recv(top_row_recv, top_row_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[0], 0, MPI_COMM_WORLD, &statuses[0]);

        subgame.set_row(0, top_row_recv);
        subgame.set_row(-1, bottom_row_recv);
        std::vector<unsigned char> left_col_send = subgame.get_col(1);
        std::vector<unsigned char> right_col_send = subgame.get_col(-2);
        
        // Send and receive the border columns
        MPI_Isend(left_col_send.data(), left_col_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[3], 0, MPI_COMM_WORLD, &requests[2]);
        MPI_Isend(right_col_send.data(), right_col_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[2], 0, MPI_COMM_WORLD, &requests[3]);
        MPI_Recv(right_col_recv, right_col_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[2], 0, MPI_COMM_WORLD, &statuses[3]);
        MPI_Recv(left_col_recv, left_col_send.size(), MPI_UNSIGNED_CHAR, neighbor_ranks[3], 0, MPI_COMM_WORLD, &statuses[2]);

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

    void to_pgm(const std::string&) const;
    void initialize_from_pgm(const std::string&);

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


void MPIProcess::to_pgm(const std::string& filename) const {
    MPI_File file;
    MPI_Status status;

    // Open the file for writing
    MPI_File_open(MPI_COMM_WORLD, filename.c_str(), MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &file);

    int header_size = 0;
    if (rank == root) {
        // Construct and write the header on the root process
        std::ostringstream header;
        header << "P5\n" << grid_cols << " " << grid_rows << "\n1\n";
        std::string header_str = header.str();

        header_size = header_str.size();
        MPI_File_write(file, header_str.c_str(), header_size, MPI_CHAR, &status);
    }

    // Broadcast the size of the header to all processes
    MPI_Bcast(&header_size, 1, MPI_INT, root, MPI_COMM_WORLD);

    // Synchronize before writing data
    MPI_Barrier(MPI_COMM_WORLD);

    // Get the subgame data for this process, excluding the ghost border
    GameOfLife subgame_without_border = subgame.subgame(1, 1, -1, -1);

    // Serialize the subgrid into a linear buffer of bytes
    std::vector<unsigned char> local_data(subgame_without_border.get_rows() * subgame_without_border.get_cols());
    for (size_t i = 0; i < subgame_without_border.get_rows(); ++i) {
        for (size_t j = 0; j < subgame_without_border.get_cols(); ++j) {
            local_data[i * subgame_without_border.get_cols() + j] = subgame_without_border.get(i, j) ? 1 : 0;
        }
    }

    // Calculate the offset for each process's data in the global file
    MPI_Offset offset = header_size +
                        starting_row * grid_cols +  // Offset to the starting row
                        starting_col;              // Offset to the starting column in the row

    // Write the subgrid data for this process
    for (size_t i = 0; i < subgrid_rows; ++i) {
        MPI_File_write_at(file, offset + i * grid_cols, &local_data[i * subgrid_cols], subgrid_cols, MPI_UNSIGNED_CHAR, &status);
    }

    // Close the file
    MPI_File_close(&file);
}

MPIProcess::MPIProcess(const std::string& filename, size_t proc_rows, size_t proc_cols, int root = 0)
    : proc_rows(proc_rows), proc_cols(proc_cols), root(root) {
    // Initialize MPI
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size != proc_rows * proc_cols) {
        throw std::runtime_error("Number of processes must be equal to proc_rows * proc_cols");
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    rank_to_coords(rank, proc_row, proc_col);

    // File header variables
    size_t global_rows, global_cols;
    MPI_Offset header_offset = 0;

    if (rank == root) {
        // Root reads the file header to determine global dimensions
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open the file");
        }

        std::string magic;
        file >> magic;
        if (magic != "P5") {
            throw std::runtime_error("Unsupported file format (only PGM P5 is supported)");
        }

        file >> global_cols >> global_rows; // Read width and height
        file.ignore(); // Skip the line with max intensity value
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Skip any extra newlines or carriage returns after header
        header_offset = file.tellg(); // Start of pixel data
    }

    // Broadcast the global dimensions and the header offset
    MPI_Bcast(&global_rows, 1, MPI_UNSIGNED_LONG, root, MPI_COMM_WORLD);
    MPI_Bcast(&global_cols, 1, MPI_UNSIGNED_LONG, root, MPI_COMM_WORLD);
    MPI_Bcast(&header_offset, sizeof(header_offset), MPI_BYTE, root, MPI_COMM_WORLD);

    grid_rows = global_rows;
    grid_cols = global_cols;

    // Compute the dimensions of the subgrid for this process
    subgrid_rows = grid_rows / proc_rows;
    subgrid_cols = grid_cols / proc_cols;
    starting_row = proc_row * subgrid_rows;
    starting_col = proc_col * subgrid_cols;
    ending_row = (proc_row == proc_rows - 1) ? grid_rows : starting_row + subgrid_rows;
    ending_col = (proc_col == proc_cols - 1) ? grid_cols : starting_col + subgrid_cols;
    subgrid_rows = ending_row - starting_row;
    subgrid_cols = ending_col - starting_col;

    // Allocate space for the local subgame (including border)
    subgame = GameOfLife(subgrid_rows + 2, subgrid_cols + 2); // +2 for borders

    // Allocate local data buffer for subgrid
    unsigned char* local_data = new unsigned char[subgrid_rows * subgrid_cols];

    // Read the subgrid data using MPI I/O
    MPI_File mpi_file;
    MPI_File_open(MPI_COMM_WORLD, filename.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &mpi_file);

    for (size_t i = 0; i < subgrid_rows; i++) {
        // Calculate the exact offset in the global PGM data for this row
        MPI_Offset file_offset = header_offset + (starting_row + i) * grid_cols + starting_col;
        MPI_File_read_at(mpi_file, file_offset, local_data + i * subgrid_cols, subgrid_cols, MPI_UNSIGNED_CHAR, MPI_STATUS_IGNORE);
    }

    // Close the MPI file
    MPI_File_close(&mpi_file);

    // Populate the subgame grid with the data from the local buffer
    for (size_t i = 0; i < subgrid_rows; i++) {
        for (size_t j = 0; j < subgrid_cols; j++) {
            subgame.set(i + 1, j + 1, local_data[i * subgrid_cols + j]); // Offset for borders
        }
    }

    delete[] local_data;

    // Calculate the ranks of the neighboring processes
    neighbor_ranks[0] = coords_to_rank(proc_row - 1, proc_col);  // North
    neighbor_ranks[1] = coords_to_rank(proc_row + 1, proc_col);  // South
    neighbor_ranks[2] = coords_to_rank(proc_row, proc_col + 1);  // East
    neighbor_ranks[3] = coords_to_rank(proc_row, proc_col - 1);  // West

    // Allocate buffers for exchanging border data
    top_row_recv = new unsigned char[subgame.get_cols() / 8 + 1];
    bottom_row_recv = new unsigned char[subgame.get_cols() / 8 + 1];
    left_col_recv = new unsigned char[subgame.get_rows() / 8 + 1];
    right_col_recv = new unsigned char[subgame.get_rows() / 8 + 1];
}

#endif