add_executable(dbase
    dbase.c
    dbase.h)
target_link_libraries(dbase libdbase::libdbase)
