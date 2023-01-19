
# SMP RP2040 Free RTOS

Symmetric Multiprocessing with FreeRTOS and RP2040. Demonstration project with a small test of RTOS api calling and jlink support. 


For installation on OSX you need these steps:

$ mkdir pico-rtos
$ cd pico-rtos
$ git clone https://github.com/RaspberryPi/pico-sdk --recurse-submodules
$ git clone -b smp https://github.com/FreeRTOS/FreeRTOS-Kernel --recurse-submodules
$ export PICO_SDK_PATH=$PWD/pico-sdk
$ export FREERTOS_KERNEL_PATH=$PWD/FreeRTOS-Kernel

The FreeRTOSConfig.h configuration header is attached. For jlink debugging, configNUM_CORES = 1. Allocation scheme 4 is set in CMakeList.txt.

Note allocation:

heap_1 - the very simplest, does not permit memory to be freed.
heap_2 - permits memory to be freed, but does not coalescence adjacent free blocks.
heap_3 - simply wraps the standard malloc() and free() for thread safety.
heap_4 - coalescences adjacent free blocks to avoid fragmentation. Includes absolute address placement option.
heap_5 - as per heap_4, with the ability to span the heap across multiple non-adjacent memory areas.


# OSX ARM Toolchain

$ brew install cmake
$ brew tap ArmMbed/homebrew-formulae
$ brew install arm-none-eabi-gcc

# Visual Code

GCC 10.3.1. arm-none-eabi

Visual Code
PlatformIO IDE
CMake
CMake Tools
Cortex-Debug
GNU debugger
C/C++, C/C++ Extension Pack

