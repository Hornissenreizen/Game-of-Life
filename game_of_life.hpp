#ifndef GAME_OF_LIFE_HPP
#define GAME_OF_LIFE_HPP

#include <fstream>
#include <iostream>
#include <vector>


inline int MOD(int a, int b) {
    return (a%b+b) % b;
}


class Grid {
    unsigned char* grid = nullptr;
    size_t rows, cols, element_count, _byte_count;

    inline bool _get_at_bit_index(size_t _index) const {
        return (grid[_index>>3] & (0x1 << (_index & 0x7))) != 0; // != 0 not strictly necessary, but it ensures that the bits are canonically set for bools.
    }

    inline void _set_at_bit_index(size_t _index, bool _val) {
        grid[_index>>3] &= ~(0x1 << (_index & 0x7));
        grid[_index>>3] |= ((_val != 0) & 0x1) << (_index & 0x7);
    }

    inline size_t _to_index(int _row, int _col) const {
        return MOD(_row, rows)*cols + MOD(_col, cols);
    }

public:
    Grid(size_t rows, size_t cols)
        : rows(rows), cols(cols) , element_count(rows * cols) {
        _byte_count = 1 + (element_count / 8);
        grid = new unsigned char[_byte_count];
        for (size_t i = 0; i < _byte_count; i++) {
            grid[i] = 0;
        }
    }
    Grid(size_t rows, size_t cols, const unsigned char* data) // Be very careful with this constructor, it does not check if the data is valid and also does no copying. _byte_count is also left uninitialized.
        : rows(rows), cols(cols), element_count(rows * cols), grid(const_cast<unsigned char*>(data)) {}
    Grid() {}
    ~Grid() {
        if (grid) delete[] grid; // grid might be null because of the default constructor
    }

    Grid(const Grid& other) : rows(other.rows), cols(other.cols), element_count(other.element_count), _byte_count(other._byte_count) {
        grid = new unsigned char[_byte_count];
        for (size_t i = 0; i < _byte_count; i++) {
            grid[i] = other.grid[i];
        }
    }

    Grid(Grid&& other) : rows(other.rows), cols(other.cols), element_count(other.element_count), _byte_count(other._byte_count) {
        grid = other.grid;
        other.grid = nullptr;
    }

    Grid& operator=(const Grid& other) {
        if (this == &other) return *this;
        if (grid) delete[] grid;
        rows = other.rows;
        cols = other.cols;
        element_count = other.element_count;
        _byte_count = other._byte_count;
        grid = new unsigned char[_byte_count];
        for (size_t i = 0; i < _byte_count; i++) {
            grid[i] = other.grid[i];
        }
        return *this;
    }

    Grid& operator=(Grid&& other) {
        if (this == &other) return *this;
        if (grid) delete[] grid;
        rows = other.rows;
        cols = other.cols;
        element_count = other.element_count;
        _byte_count = other._byte_count;
        grid = other.grid;
        other.grid = nullptr;
        return *this;
    }

     bool get(int row, int col) const {
        return _get_at_bit_index(_to_index(row, col));
    }

     void set(int row, int col, bool val) {
        _set_at_bit_index(_to_index(row, col), val);
    }

    size_t no_neighbors(int row, int col) const {
        size_t count = 0;
        const std::vector<int> d({-1,0,1});
        for (auto& dx : d) for (auto& dy : d) {
            count += get(row + dx, col + dy) & 1;
        }
        return count - (get(row, col) & 1); // middle cell is not a neighbor
    }

    void print() const {
        for (size_t row = 0; row < rows; ++row) {
            for (size_t col = 0; col < cols; ++col) {
                std::cout << get(row, col);
                if (col < cols - 1) {
                    std::cout << " ";  // Space between columns
                }
            }
            std::cout << std::endl;  // Newline after each row
        }
    }

    const unsigned char* data() const {
        return grid;
    }

    size_t size() const {
        return _byte_count;
    }

    void _nullify() { grid = nullptr; } // Call this function to prevent the destructor from deleting the data

    Grid subgrid(int start_row, int start_col, int end_row, int end_col) const {
        Grid sub(MOD(end_row - start_row, rows), MOD(end_col - start_col, cols));
        for (int i = 0; i < sub.rows; i++) {
            for (int j = 0; j < sub.cols; j++) {
                sub.set(i, j, get(i + start_row, j + start_col));
            }
        }
        return sub;
    }

    void set_subgrid(int start_row, int start_col, const Grid& subgrid) {
        for (int i = 0; i < subgrid.rows; i++) {
            for (int j = 0; j < subgrid.cols; j++) {
                set(start_row + i, start_col + j, subgrid.get(i, j));
            }
        }
    }

    
    // getting and setting rows and columns

    std::vector<unsigned char> get_row(int row) const {
        std::vector<unsigned char> row_vec(cols / 8 + 1, 0);
        for (size_t i = 0; i < cols; i++) {
            row_vec[i >> 3] |= (get(row, i) & 1) << (i % 8);
        }
        return row_vec;
    }

    std::vector<unsigned char> get_col(int col) const {
        std::vector<unsigned char> col_vec(rows / 8 + 1, 0);
        for (size_t i = 0; i < rows; i++) {
            col_vec[i >> 3] |= (get(i, col) & 1) << (i % 8);
        }
        return col_vec;
    }

    void set_row(int row, const unsigned char* row_vec) {
        for (size_t i = 0; i < cols; i++) {
            set(row, i, (row_vec[i >> 3] >> (i % 8)) & 1);
        }
    }

    void set_col(int col, const unsigned char* col_vec) {
        for (size_t i = 0; i < rows; i++) {
            set(i, col, (col_vec[i >> 3] >> (i % 8)) & 1);
        }
    }
};


class GameOfLife {
    Grid state, next_state;
    size_t rows, cols, element_count;

public:
    GameOfLife(size_t rows, size_t cols)
        : state(rows, cols), next_state(rows, cols), rows(rows), cols(cols), element_count(rows * cols) {}

    GameOfLife() {}
    ~GameOfLife() = default;

    GameOfLife(const GameOfLife& other) : state(other.state), next_state(other.next_state), rows(other.rows), cols(other.cols), element_count(other.element_count) {}
    GameOfLife(GameOfLife&& other) : state(std::move(other.state)), next_state(std::move(other.next_state)), rows(other.rows), cols(other.cols), element_count(other.element_count) {}

    GameOfLife& operator=(const GameOfLife& other) {
        if (this == &other) return *this;
        state = other.state;
        next_state = other.next_state;
        rows = other.rows;
        cols = other.cols;
        element_count = other.element_count;
        return *this;
    }

    GameOfLife& operator=(GameOfLife&& other) {
        if (this == &other) return *this;
        state = std::move(other.state);
        next_state = std::move(other.next_state);
        rows = other.rows;
        cols = other.cols;
        element_count = other.element_count;
        return *this;
    }

    inline bool becomes_alive(size_t row, size_t col) const {
        size_t no_neighbors = state.no_neighbors(row, col);
        return (no_neighbors == 3) || (no_neighbors == 2 && state.get(row, col));
    }

    inline bool get(size_t row, size_t col) const {
        return state.get(row, col);
    }

    void init(std::initializer_list<std::initializer_list<size_t>>&& l) {
        for (auto& pair : l) {
            state.set(*pair.begin(), *(pair.begin() + 1), true);
        }
    }

    void tick() {
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                next_state.set(i, j, becomes_alive(i, j));
            }
        }
        std::swap(state, next_state); // Swap the two Grid objects
    }

    void to_pgm(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::ios_base::failure("Failed to open file");
        }

        // Write PPM header
        file << "P5\n";
        file << cols << " " << rows << "\n";
        file << "255\n";

        // Write pixel data
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
		    char pixel = state.get(i, j) * 255;
                file << pixel;
            }
        }

        file.close();
    }

    void print() const {
        state.print();
    }

    void print_info() const {
        std::cout << "Rows: " << rows << ", Cols: " << cols << std::endl;
    }

    const unsigned char* data() const {
        return state.data();
    }

    const size_t size() const {
        return state.size();
    }


    // Some getter functions
    size_t get_rows() const { return rows; }
    size_t get_cols() const { return cols; }


    // Some subgrid utilities
    GameOfLife subgame(int start_row, int start_col, int end_row, int end_col) const {
        GameOfLife sub(MOD(end_row - start_row, rows), MOD(end_col - start_col, cols));
        sub.state = state.subgrid(start_row, start_col, end_row, end_col);
        return sub;
    }

    void set_subgame(int start_row, int start_col, const Grid& subgrid) {
        state.set_subgrid(start_row, start_col, subgrid);
    }

    std::vector<unsigned char> get_row(int row) const {
        return state.get_row(row);
    }

    std::vector<unsigned char> get_col(int col) const {
        return state.get_col(col);
    }

    void set_row(int row, const unsigned char* row_vec) {
        state.set_row(row, row_vec);
    }

    void set_col(int col, const unsigned char* col_vec) {
        state.set_col(col, col_vec);
    }
};

#endif
