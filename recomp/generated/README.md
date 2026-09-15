# recomp/generated — XenonRecomp C++ output (do NOT edit by hand).
#
# Populated offline by tools/run_recomp.sh:
#   XenonAnalyse default.xex -> recompiled_functions.toml (jump tables)
#   XenonRecomp fh2recomp.toml ppc_context.h -> recomp/generated/*.cpp
#
# The Android APK build does NOT require this directory: if
# fh2_sources.cmake is absent, CMake performs a runtime-only build
# (boot, HUD, lifecycle) so CI stays green without the private XEX.
# When present, fh2_sources.cmake must define FH2_GENERATED_SOURCES with the
# list of recompiled .cpp files (absolute or relative to this dir).
#
# Example fh2_sources.cmake (auto-written by run_recomp.sh):
#   set(FH2_GENERATED_SOURCES
#       "${CMAKE_CURRENT_LIST_DIR}/ppc_recomp.0.cpp"
#       "${CMAKE_CURRENT_LIST_DIR}/ppc_func_mapping.cpp")
