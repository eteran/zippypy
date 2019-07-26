cmake_minimum_required (VERSION 3.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)


option(ZIPPYPY_USE_BOOST          "Use Boost Data Structures")
option(ZIPPYPY_USE_CPYTHON        "Use CPython")

add_library(PyVM 
    BufferAccess.cpp
    CodeDefinition.cpp
    instruction.cpp
    objects.cpp
    PyCompile.cpp
    PyVM.cpp
    utils.cpp

    include/PyVM/baseObject.h
    include/PyVM/BufferAccess.h
    include/PyVM/cfunc.h
    include/PyVM/CodeDefinition.h
    include/PyVM/defs.h
    include/PyVM/except.h
    include/PyVM/gen_string_method_names.h
    include/PyVM/log.h
    include/PyVM/objects.h
    include/PyVM/ObjPool.h
    include/PyVM/opcodes_def.h
    include/PyVM/opcodes.h
    include/PyVM/OpImp.h
    include/PyVM/PyCompile.h
    include/PyVM/PyVM.h
    include/PyVM/utils.h
    include/PyVM/VarArray.h

)

target_include_directories(PyVM
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(ZIPPYPY_USE_BOOST)
	target_compile_definitions(PyVM PUBLIC -DUSE_BOOST)
endif()

if(ZIPPYPY_USE_CPYTHON)
	find_package (Python2 COMPONENTS Development)
	target_compile_definitions(PyVM PUBLIC -DUSE_CPYTHON)
	target_link_libraries(PyVM PUBLIC Python2::Python)
endif()

set_property(TARGET PyVM PROPERTY CXX_STANDARD 14)
set_property(TARGET PyVM PROPERTY CXX_EXTENSIONS OFF)
