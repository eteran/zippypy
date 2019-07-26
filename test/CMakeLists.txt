cmake_minimum_required (VERSION 3.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(test-zippypy
    main.cpp
    VarArrayTest.cpp
    PyVMTest.cpp
    myTest.h
)

target_link_libraries(test-zippypy PUBLIC
    PyVM
)

set_property(TARGET test-zippypy PROPERTY CXX_STANDARD 14)
set_property(TARGET test-zippypy PROPERTY CXX_EXTENSIONS OFF)

