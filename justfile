# PER|FORMER entry points. `just` lists them.
# The toolchain and CMake setup live in the Makefile; these recipes only call it.

sim_dir := "build/sim/release"
stm32_dir := "build/stm32/release"
arm_path := justfile_directory() / "tools/gcc-arm-none-eabi/bin"
sandbox_dir := "tools/ui-sandbox"

default:
    @just --list

# Build the simulator and start it
sim: sim-build
    cd {{sim_dir}} && ./src/apps/sequencer/sequencer

# Build the simulator (configures CMake on first run)
sim-build:
    @test -f {{sim_dir}}/Makefile || make setup_sim
    make -C {{sim_dir}} -j sequencer

# Build the Python test bindings (testsim)
testsim:
    @test -f {{sim_dir}}/Makefile || make setup_sim
    make -C {{sim_dir}} -j testsim

# Run the headless engine tests: all of them, or one by name (e.g. `just test note_v2`)
test name="": testsim
    #!/usr/bin/env bash
    set -uo pipefail
    if [ -n "{{name}}" ]; then
        exec python3 src/apps/sequencer/tests/ui/{{name}}_test.py
    fi
    failed=()
    for t in src/apps/sequencer/tests/ui/*_test.py; do
        echo "== $t"
        python3 "$t" || failed+=("$t")
    done
    if [ ${#failed[@]} -gt 0 ]; then
        echo "FAILED: ${failed[*]}"
        exit 1
    fi
    echo "all tests passed"

# Build the hardware firmware and reveal UPDATE.DAT in Finder
firmware:
    @test -x {{arm_path}}/arm-none-eabi-gcc || make tools_install
    @test -f {{stm32_dir}}/Makefile || make setup_stm32
    PATH="{{arm_path}}:$PATH" make -C {{stm32_dir}} -j sequencer
    open -R {{stm32_dir}}/src/apps/sequencer/UPDATE.DAT

# Start the UI design sandbox dev server
sandbox:
    @test -d {{sandbox_dir}}/node_modules || npm --prefix {{sandbox_dir}} install
    npm --prefix {{sandbox_dir}} run dev

# Type-check the UI design sandbox
sandbox-check:
    npm --prefix {{sandbox_dir}} run check

# Check agent-docs/ for dead links and paths, and that index.md links every doc
docs-check:
    python3 scripts/check-agent-docs.py
