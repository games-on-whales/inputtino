find_package(SWIG 4.0 REQUIRED COMPONENTS python)
include (UseSWIG)

set_property(SOURCE inputtino.i PROPERTY CPLUSPLUS ON)
set_property(SOURCE inputtino.i PROPERTY SWIG_MODULE_NAME py_inputtino)

swig_add_library(py_inputtino
        TYPE MODULE
        LANGUAGE python
        OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/out/py_inputtino
        SOURCES inputtino.i)
add_library(inputtino::py_inputtino ALIAS py_inputtino)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)

target_include_directories(py_inputtino
        PRIVATE
        ${Python3_INCLUDE_DIRS}
)
set_property(TARGET py_inputtino PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES ON)

target_link_libraries(py_inputtino PRIVATE inputtino::libinputtino)