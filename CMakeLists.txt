project(MemAxes C CXX)

# target_compile_features (for C++11) requires 3.1.0
cmake_minimum_required(VERSION 3.22.0)

# Qt5 + Modules
find_package(Qt5 REQUIRED Core Widgets OpenGL Xml)

# OpenGL
find_package(OpenGL)

# sys-sage
find_package(sys-sage REQUIRED)
include_directories(${sys-sage_INCLUDE_DIRS})
link_libraries(${sys-sage_LIBRARIES})
#libxml2 temporary
find_package(LibXml2 REQUIRED) # Use pkg-config via the LibXml2 find-module
execute_process(COMMAND xml2-config --cflags OUTPUT_VARIABLE LIBXML_F OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND xml2-config --libs OUTPUT_VARIABLE LIBXML_L OUTPUT_STRIP_TRAILING_WHITESPACE)
include_directories(${LIBXML_F})
link_libraries(${LIBXML_L})
include_directories(${LIBXML2_INCLUDE_DIR})
include_directories(${LIBXML2_INCLUDE_DIRS})
link_libraries(${LIBXML2_LIBRARY})
link_libraries(${LIBXML2_LIBRARIES})

# Top-level build just includes subdirectories.
add_subdirectory(src)
add_subdirectory(example_data)
