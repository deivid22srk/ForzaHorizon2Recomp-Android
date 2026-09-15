# Root CMake (host/offline helpers only).
# The APK itself is built via Gradle AGP + app/src/main/cpp/CMakeLists.txt.
# This file exists so `cmake -S . -B out` validates the runtime sources on a
# desktop toolchain (syntax-level check, no Android APIs required beyond stubs).
cmake_minimum_required(VERSION 3.25)
project(ForzaHorizon2Recomp_Host NONE)
message(STATUS "FH2 host placeholder — use ./gradlew assembleDebug for the APK (see README).")
