cmake_minimum_required (VERSION 3.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)



option(ZIPPYPY_PYVM_COUNT_OBJECTS "Count Objects")
option(ZIPPYPY_USE_BOOST          "Use Boost Data Structures")
option(ZIPPYPY_USE_CPYTHON        "Use CPython")

add_library(zippypy 
	baseObject.h
	BufferAccess.cpp
	BufferAccess.h
	cfunc.h
	CodeDefinition.cpp
	CodeDefinition.h
	defs.h
	except.h
	gen_string_method_names.h
	instruction.cpp
	log.h
	objects.cpp
	objects.h
	ObjPool.h
	opcodes_def.h
	opcodes.h
	OpImp.h
	PyCompile.cpp
	PyCompile.h
	PyVM.cpp
	PyVM.h
	utils.cpp
	utils.h
	VarArray.h
)

if(ZIPPYPY_PYVM_COUNT_OBJECTS)
	target_compile_definitions(zippypy PUBLIC -DPYVM_COUNT_OBJECTS)
endif()

if(ZIPPYPY_USE_BOOST)
	target_compile_definitions(zippypy PUBLIC -DUSE_BOOST)
endif()

if(ZIPPYPY_USE_CPYTHON)
	find_package (Python2 COMPONENTS Development)
	target_compile_definitions(zippypy PUBLIC -DUSE_CPYTHON)
	target_link_libraries(zippypy PUBLIC Python2::Python)
endif()

set_property(TARGET zippypy PROPERTY CXX_STANDARD 14)
set_property(TARGET zippypy PROPERTY CXX_EXTENSIONS OFF)
