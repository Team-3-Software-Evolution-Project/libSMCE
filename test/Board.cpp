
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <catch2/catch_test_macros.hpp>
#include "SMCE/Board.hpp"
#include "SMCE/Sketch.hpp"
#include "SMCE/Toolchain.hpp"
#include "defs.hpp"

using namespace std::literals;

TEST_CASE("Board contracts", "[Board]") {
    smce::Toolchain tc{SMCE_PATH};
    REQUIRE(!tc.check_suitable_environment());
    smce::Sketch sk{SKETCHES_PATH "noop", {.fqbn = "arduino:avr:nano"}};
    const auto ec = tc.compile(sk);
    if (ec)
        std::cerr << tc.build_log().second;
    REQUIRE_FALSE(ec);
    REQUIRE(sk.is_compiled());
    smce::Board br{};
    REQUIRE(br.status() == smce::Board::Status::clean);
    REQUIRE_FALSE(br.view().valid());
    REQUIRE(br.configure({}));
    REQUIRE(br.status() == smce::Board::Status::configured);
    REQUIRE_FALSE(br.view().valid());
    REQUIRE(br.attach_sketch(sk));
    REQUIRE_FALSE(br.view().valid());
    REQUIRE(br.start());
    REQUIRE(br.status() == smce::Board::Status::running);
    REQUIRE(br.view().valid());
    REQUIRE(br.suspend());
    REQUIRE(br.status() == smce::Board::Status::suspended);
    REQUIRE(br.view().valid());
    REQUIRE(br.resume());
    REQUIRE(br.status() == smce::Board::Status::running);
    REQUIRE(br.view().valid());
    REQUIRE(br.stop());
    REQUIRE(br.status() == smce::Board::Status::stopped);
    REQUIRE_FALSE(br.view().valid());
    REQUIRE(br.reset());
    REQUIRE(br.status() == smce::Board::Status::clean);
    REQUIRE_FALSE(br.view().valid());
}

TEST_CASE("Board exit_notify", "[Board]") {
    smce::Toolchain tc{SMCE_PATH};
    REQUIRE(!tc.check_suitable_environment());
    smce::Sketch sk{SKETCHES_PATH "uncaught", {.fqbn = "arduino:avr:nano"}};
    const auto ec = tc.compile(sk);
    if (ec)
        std::cerr << tc.build_log().second;
    REQUIRE_FALSE(ec);
    std::promise<int> ex;
    smce::Board br{[&](int ec) { ex.set_value(ec); }};
    REQUIRE(br.configure({}));
    REQUIRE(br.attach_sketch(sk));
    REQUIRE(br.start());
    auto exfut = ex.get_future();
    int ticks = 0;
    while (ticks++ < 5 && exfut.wait_for(0ms) != std::future_status::ready) {
        exfut.wait_for(1s);
        br.tick();
    }
    REQUIRE(exfut.wait_for(0ms) == std::future_status::ready);
    REQUIRE(exfut.get() != 0);
}

TEST_CASE("Mixed INO/C++ sources", "[Board]") {
    smce::Toolchain tc{SMCE_PATH};
    REQUIRE(!tc.check_suitable_environment());
    smce::Sketch sk{SKETCHES_PATH "with_cxx", {.fqbn = "arduino:avr:nano"}};
    const auto ec = tc.compile(sk);
    if (ec)
        std::cerr << tc.build_log().second;
    REQUIRE_FALSE(ec);
}

/**
 * Test that attaching sketches to the board and resetting of the board only works
 * when the board is not running and not suspended.
 */
TEST_CASE("Board - Check conditions for attach_sketch and reset", "[Board]") {
    smce::Toolchain tc{SMCE_PATH};
    REQUIRE(!tc.check_suitable_environment());

    smce::Sketch sk{SKETCHES_PATH "noop", {.fqbn = "arduino:avr:nano"}};
    tc.compile(sk);
    REQUIRE(sk.is_compiled());

    smce::Sketch sk2{SKETCHES_PATH "noop", {.fqbn = "arduino:avr:nano"}};
    tc.compile(sk2);
    REQUIRE(sk2.is_compiled());

    // Board is clean, so not running yet => Attaching sketches and resetting possible
    smce::Board br{};
    REQUIRE(br.status() == smce::Board::Status::clean);
    REQUIRE(br.attach_sketch(sk));
    REQUIRE(br.reset());

    // Board is configured, so not running yet => Attaching sketches and resetting possible
    REQUIRE(br.configure({}));
    REQUIRE(br.status() == smce::Board::Status::configured);
    REQUIRE(br.attach_sketch(sk));
    REQUIRE(br.reset());

    // Board is running => Attaching sketches and resetting NOT possible
    REQUIRE(br.configure({}));
    REQUIRE(br.attach_sketch(sk));
    REQUIRE(br.start());
    REQUIRE(br.status() == smce::Board::Status::running);
    REQUIRE_FALSE(br.attach_sketch(sk2));
    REQUIRE_FALSE(br.reset());

    // Board is suspended => Attaching sketches and resetting NOT possible
    REQUIRE(br.suspend());
    REQUIRE(br.status() == smce::Board::Status::suspended);
    REQUIRE_FALSE(br.attach_sketch(sk2));
    REQUIRE_FALSE(br.reset());

    // Board is stopped => Attaching sketches and resetting possible
    REQUIRE(br.stop());
    REQUIRE(br.status() == smce::Board::Status::stopped);
    REQUIRE(br.attach_sketch(sk2));
    REQUIRE(br.reset());
}

/**
 * Test that configuring the board only works
 * when the board is clean or already configured.
 */
TEST_CASE("Board - Check conditions for configure", "[Board]") {
    smce::Toolchain tc{SMCE_PATH};
    REQUIRE(!tc.check_suitable_environment());

    smce::Sketch sk{SKETCHES_PATH "noop", {.fqbn = "arduino:avr:nano"}};
    tc.compile(sk);
    REQUIRE(sk.is_compiled());
    smce::Board br{};

    // Board is both clean and configured => Configuration possible
    REQUIRE(br.status() == smce::Board::Status::clean);
    REQUIRE_FALSE(br.status() == smce::Board::Status::configured);
    REQUIRE(br.configure({}));

    // Board is not clean, but configured => Configuration possible
    REQUIRE_FALSE(br.status() == smce::Board::Status::clean);
    REQUIRE(br.status() == smce::Board::Status::configured);
    REQUIRE(br.configure({}));

    // Starting and stopping of board
    REQUIRE(br.attach_sketch(sk));
    REQUIRE(br.start());
    REQUIRE(br.status() == smce::Board::Status::running);
    REQUIRE(br.stop());
    REQUIRE(br.status() == smce::Board::Status::stopped);

    // Board is neither clean nor configured => Configuration NOT possible
    REQUIRE_FALSE(br.status() == smce::Board::Status::clean);
    REQUIRE_FALSE(br.status() == smce::Board::Status::configured);
    REQUIRE_FALSE(br.configure({}));
}

/**
 * Test that starting the board only works
 * when a compiled sketch is attached and when the board is not already running.
 */
TEST_CASE("Board - Check conditions for start", "[Board]") {
    smce::Toolchain tc{SMCE_PATH};
    REQUIRE(!tc.check_suitable_environment());

    smce::Sketch sk{SKETCHES_PATH "noop", {.fqbn = "arduino:avr:nano"}};
    smce::Board br{};
    REQUIRE(br.configure({}));

    // Board has no attached sketch => Starting NOT possible
    REQUIRE_FALSE(br.start());

    // Board has attached but not compiled sketch => Starting NOT possible
    REQUIRE(br.attach_sketch(sk));
    REQUIRE_FALSE(br.start());

    // Board has attached and compiled sketch => Starting possible
    tc.compile(sk);
    REQUIRE(sk.is_compiled());
    REQUIRE(br.attach_sketch(sk));
    REQUIRE(br.start());
    REQUIRE(br.status() == smce::Board::Status::running);

    // Board is already started => Starting NOT possible
    REQUIRE_FALSE(br.start());
}
