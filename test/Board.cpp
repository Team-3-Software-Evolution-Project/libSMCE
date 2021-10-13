
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

TEST_CASE("Arduino Characters", "[Board]") {
    smce::Toolchain tc{SMCE_PATH};
    REQUIRE(!tc.check_suitable_environment());
    smce::Sketch sk{SKETCHES_PATH "characters", {.fqbn = "arduino:avr:nano"}};
    const auto ec = tc.compile(sk);
    if (ec)
        std::cerr << tc.build_log().second;
    REQUIRE_FALSE(ec);
    smce::Board br{};
    // clang-format off
    smce::BoardConfig bc{
        /* .pins = */{0, 2},
        /* .gpio_drivers = */{
            smce::BoardConfig::GpioDrivers{
                0,
                smce::BoardConfig::GpioDrivers::DigitalDriver{false, true}
            },
            smce::BoardConfig::GpioDrivers{
                2,
                smce::BoardConfig::GpioDrivers::DigitalDriver{false, true}
            },
        }
    };
    // clang-format on
    REQUIRE(br.configure(std::move(bc)));
    REQUIRE(br.attach_sketch(sk));
    REQUIRE(br.start());

    auto bv = br.view();
    auto pin2 = bv.pins[2];
    REQUIRE(pin2.exists());
    auto pin2d = pin2.digital();
    REQUIRE(pin2d.exists());
    REQUIRE_FALSE(pin2d.can_read());
    REQUIRE(pin2d.can_write());

    test_pin_delayable(pin2d, true, 16384, 1ms);

    auto pin0 = bv.pins[0];
    REQUIRE(pin0.exists());
    auto pin0d = pin0.digital();
    REQUIRE(pin0d.exists());
    REQUIRE_FALSE(pin0d.can_read());
    REQUIRE(pin0d.can_write());

    test_pin_delayable(pin0d, true, 16384, 1ms);


}
