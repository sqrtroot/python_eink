find_package(pybind11 CONFIG REQUIRED)
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/dist" CACHE PATH "default install path" FORCE )
endif()

include_directories(${pybind11_INCLUDE_DIR})

pybind11_add_module(IT8951 MODULE PythonBindings.cpp)
target_link_libraries(IT8951 PUBLIC IT8951_LIB)
