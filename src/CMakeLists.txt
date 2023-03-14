# find_package(PkgConfig REQUIRED)
# pkg_check_modules(libusb REQUIRED IMPORTED_TARGET libusb-1.0)

set(libdbase_SRC
    libdbase.c
    libdbasei.c)

set(libdbase_INC
    libdbase.h
    libdbasei.h)


add_library(libdbase SHARED
    ${libdbase_SRC}
    ${libdbase_INC}
    )

find_package(libusb REQUIRED)
target_link_libraries(libdbase PUBLIC libusb::libusb)

target_include_directories(libdbase
    PUBLIC
    "."
    )
add_library(libdbase::libdbase ALIAS libdbase)

find_file(DIGIBASE_RBF_FULL_PATH digiBase.rbf HINTS ${DIGIBASE_RBF_PATH} . ${PROJECT_SOURCE_DIR})

if (NOT DIGIBASE_RBF_FULL_PATH)
  message(FATAL_ERROR "Unable to find the file \"digiBase.rbf\". Copy it to the project root directory or set its path with the \"DIGIBASE_RBF_PATH\" argument. E.g.: \"cmake .. -DDIGIBASE_RBF_PATH=/path/to/rbf/file/\".")
else()
  message(STATUS "The file \"digiBase.rbf\" was found here: ${DIGIBASE_RBF_FULL_PATH}")
endif ()

target_compile_definitions(libdbase PRIVATE PACK_PATH="${DIGIBASE_RBF_FULL_PATH}")



set(libdbase_cpp_SRC
    DigiBASE.cpp)

set(libdbase_cpp_INC
    DigiBASE.h)

add_library(libdbase_cpp SHARED
    ${libdbase_cpp_SRC}
    ${libdbase_cpp_INC}
    )

target_link_libraries(libdbase_cpp PRIVATE libdbase)

find_package(pybind11 REQUIRED)

pybind11_add_module(pylibdbase pylibdbase.cpp)
target_link_libraries(pylibdbase PUBLIC libusb::libusb libdbase_cpp)

install(TARGETS libdbase libdbase_cpp
    EXPORT graylog_logger-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

set(INSTALL_HEADERS ${libdbase_INC} ${libdbase_cpp_INC})

install(FILES ${INSTALL_HEADERS} DESTINATION include)
