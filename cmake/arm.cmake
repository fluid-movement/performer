
# disable compiler checks
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# setup cross compiler toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CROSSCOMPILING 1)

# Use bundled ARM toolchain so CMake picks up the correct ar/ranlib for ARM ELF.
# Without this, CMake defaults to the macOS system ar/ranlib which produce an
# empty archive symbol table and break archive scanning during linking.
get_filename_component(_arm_toolchain_bin "${CMAKE_CURRENT_LIST_DIR}/../tools/gcc-arm-none-eabi/bin" ABSOLUTE)
set(CMAKE_C_COMPILER   ${_arm_toolchain_bin}/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${_arm_toolchain_bin}/arm-none-eabi-g++)
set(CMAKE_AR           ${_arm_toolchain_bin}/arm-none-eabi-ar    CACHE FILEPATH "ARM archiver" FORCE)
set(CMAKE_RANLIB       ${_arm_toolchain_bin}/arm-none-eabi-ranlib CACHE FILEPATH "ARM ranlib"   FORCE)

# Make find_program() search the bundled toolchain so tools like objcopy, ld,
# size, gdb can be located even when the PATH doesn't include the toolchain bin.
list(APPEND CMAKE_PROGRAM_PATH ${_arm_toolchain_bin})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
