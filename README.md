# efim

Multithreaded C++20 implementation of EFIM (Zida et al., *EFIM: a fast and memory
efficient algorithm for high-utility itemset mining*, Knowl Inf Syst 51, 2017),
tuned for Apple silicon.

## Build

    cmake -S . -B build && cmake --build build -j
    ctest --test-dir build

spdlog and CLI11 are fetched by CMake. Options: `EFIM_FETCH_DEPS`, `EFIM_NATIVE`,
`EFIM_SANITIZE`, `EFIM_LTO`.

## Use

    build/efim <input> <output> <minutil> [-s tab|space|<literal>] [-t threads] [-v]

Input: one transaction per line, `items:total:utilities`, e.g. `a c d:8:5 1 2`.
Output: `item<TAB>item...:utility` per pattern (line order is nondeterministic with
more than one thread). Patterns are streamed to disk as found, so memory stays small.

## Layout

    include/efim/{core,io,mine,util}   headers
    src/{core,io,mine,util}, main.cpp  implementation and CLI
    tests/                             example input/expected output
