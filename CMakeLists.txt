cmake_minimum_required(VERSION 3.0)

project(CompilerDetect C CXX)

file(WRITE "${PROJECT_BINARY_DIR}/cmake_try_run.c" "int main(int argc, char **argv) {return 0;}")
try_run(_C_COMPILER_RUN_CODE _C_COMPILER_COMPILES
    "${PROJECT_BINARY_DIR}" "${PROJECT_BINARY_DIR}/cmake_try_run.c"
    )
string(TOLOWER "${_C_COMPILER_COMPILES}" C_COMPILER_COMPILES)
if(_C_COMPILER_RUN_CODE)
    set(C_COMPILER_RUNS false)
else()
    set(C_COMPILER_RUNS true)
endif()

file(WRITE "${PROJECT_BINARY_DIR}/cmake_try_run.cpp" "int main(int argc, char **argv) {return 0;}")
try_run(_CXX_COMPILER_RUN_CODE _CXX_COMPILER_COMPILES
    "${PROJECT_BINARY_DIR}" "${PROJECT_BINARY_DIR}/cmake_try_run.cpp"
    )
string(TOLOWER "${_CXX_COMPILER_COMPILES}" CXX_COMPILER_COMPILES)
if(_CXX_COMPILER_RUN_CODE)
    set(CXX_COMPILER_RUNS false)
else()
    set(CXX_COMPILER_RUNS true)
endif()

configure_file("${PROJECT_SOURCE_DIR}/version.json.in" "${PROJECT_BINARY_DIR}/version.json" @ONLY)
