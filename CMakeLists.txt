add_library(gdb_server_kit STATIC
    server.cpp
)
target_include_directories(gdb_server_kit PUBLIC include)
