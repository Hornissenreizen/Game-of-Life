#include <fstream>
#include <vector>

#include <iostream>


inline int MOD(int a, int b) {
    return (a%b+b) % b;
}


class Grid {
    char* grid;
    size_t rows, cols, size, _byte_count;

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
        : rows(rows), cols(cols) , size(rows * cols) {
        _byte_count = 1 + (size / sizeof(char));
        grid = new char[_byte_count];
        for (size_t i = 0; i < _byte_count; i++) {
            grid[i] = 0;
        }
    }
    Grid() {}
    ~Grid() {
        delete[] grid;
    }

    inline bool get(int row, int col) const {
        return _get_at_bit_index(_to_index(row, col));
    }

    inline void set(int row, int col, bool val) {
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
};


class GameOfLife {
    Grid *state, *next_state;
    size_t rows, cols, size;
public:
    GameOfLife(size_t rows, size_t cols)
        : rows(rows), cols(cols), size(rows * cols) {
            state = new Grid(rows, cols);
            next_state = new Grid(rows, cols);
        }
    ~GameOfLife() {
        delete state;
        delete next_state;
    }

    inline bool becomes_alive(size_t row, size_t col) const {
        size_t no_neighbors = state->no_neighbors(row, col);
        return (no_neighbors == 3) || (no_neighbors == 2 && state->get(row, col));
    }

    inline bool get(size_t row, size_t col) const {
        return state->get(row, col);
    }

    void init(std::initializer_list<std::initializer_list<size_t>>&& l) {
        for (auto& pair : l) {
            state->set(*pair.begin(), *(pair.begin() + 1), true);
        }
    }

    void tick() {
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                next_state->set(i, j, becomes_alive(i, j));
            }
        }
        std::swap(state, next_state);
    }

    void to_pgm(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::ios_base::failure("Failed to open file");
        }

        // Write PPM header
        file << "P2\n";
        file << cols << " " << rows << "\n";
        file << "1\n";

        // Write pixel data
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                file << (state->get(i, j) & 1) << " ";
            }
            file << "\n";
        }

        file.close();
    }
};


int main() {
    GameOfLife game(10, 10);
    game.init({{0,1},{1,2},{2,0},{2,1},{2,2}});

    game.to_pgm("Anfang");

    const size_t NO_TICKS = 44;

    for (size_t i = 0; i < NO_TICKS; i++) {
        game.tick();
    }

    game.to_pgm("Ende");

    return 0;
}