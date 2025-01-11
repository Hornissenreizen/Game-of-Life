#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "game_of_life.hpp" // Assume the GameOfLife implementation is in this header file

TEST_CASE("Grid basic operations") {
    Grid grid(5, 5);

    SECTION("Set and get cells") {
        grid.set(0, 0, true);
        REQUIRE(grid.get(0, 0) == true);

        grid.set(4, 4, true);
        REQUIRE(grid.get(4, 4) == true);

        grid.set(2, 2, false);
        REQUIRE(grid.get(2, 2) == false);
    }

    SECTION("Check wraparound") {
        grid.set(0, 0, true);
        REQUIRE(grid.get(5, 0) == true); // Wraparound on row
        REQUIRE(grid.get(0, 5) == true); // Wraparound on column
        REQUIRE(grid.get(5, 5) == true); // Wraparound on both
    }

    SECTION("Count neighbors") {
        grid.set(1, 1, true);
        grid.set(1, 2, true);
        grid.set(2, 1, true);

        REQUIRE(grid.no_neighbors(1, 1) == 2);
        REQUIRE(grid.no_neighbors(2, 2) == 3);
    }
}

TEST_CASE("Game of Life rules") {
    GameOfLife game(5, 5);

    SECTION("Initialization works correctly") {
        game.init({{1, 1}, {2, 2}, {3, 3}});
        REQUIRE(game.get(1, 1) == true);
        REQUIRE(game.get(2, 2) == true);
        REQUIRE(game.get(3, 3) == true);
    }

    SECTION("Cells come to life") {
        game.init({{1, 0}, {1, 1}, {1, 2}});
        game.tick();
        REQUIRE(game.get(0, 1) == true);
        REQUIRE(game.get(1, 1) == true);
        REQUIRE(game.get(2, 1) == true);
    }

    SECTION("Over-/ Underpopulation kills cells") {
        game.init({{1, 1}, {1, 2}, {2, 1}, {2, 2}, {3, 1}, {4, 1}});
        game.tick();
        REQUIRE(game.get(1, 1) == true);    // Still alive (3 NB)
        REQUIRE(game.get(2, 2) == false);   // Dies due to overpopulation (4 NB)
        REQUIRE(game.get(1, 2) == true);    // Still alive (3 NB)
        REQUIRE(game.get(4, 1) == false);   // Dies due to underpopulation (1 NB)
    }


    SECTION("Stable configurations remain stable") {
        // Block (stable configuration)
        game.init({{1, 1}, {1, 2}, {2, 1}, {2, 2}});
        game.tick();
        REQUIRE(game.get(1, 1) == true);
        REQUIRE(game.get(1, 2) == true);
        REQUIRE(game.get(2, 1) == true);
        REQUIRE(game.get(2, 2) == true);
    }

    SECTION("Oscillators work correctly") {
        // Blinker (oscillator)
        game.init({{1, 0}, {1, 1}, {1, 2}});
        game.tick();
        REQUIRE(game.get(0, 1) == true);
        REQUIRE(game.get(1, 1) == true);
        REQUIRE(game.get(2, 1) == true);

        game.tick();
        REQUIRE(game.get(1, 0) == true);
        REQUIRE(game.get(1, 1) == true);
        REQUIRE(game.get(1, 2) == true);
    }

    SECTION("Edge wrapping works") {
        game.init({{0, 0}, {0, 1}, {1, 0}, {0, 4}, {4, 0}});
        game.tick();
        REQUIRE(game.get(4, 4) == true); // Wraps around diagonally
        REQUIRE(game.get(0, 0) == false); // Dies of overpopulation
    }
}