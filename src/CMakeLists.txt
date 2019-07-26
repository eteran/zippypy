cmake_minimum_required (VERSION 3.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

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

set_property(TARGET zippypy PROPERTY CXX_STANDARD 14)
set_property(TARGET zippypy PROPERTY CXX_EXTENSIONS OFF)
