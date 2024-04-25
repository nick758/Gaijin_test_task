cmake_minimum_required(VERSION 3.0)
project(Gaijin_test_task)

set(CMAKE_CXX_STANDARD 17)

file(GLOB sources_server main.cpp
          Server.cpp Server.h
          Storage.cpp Storage.h
          )

file(GLOB sources_client client.cpp)

add_executable(Server ${sources_server})
target_compile_options(Server PUBLIC -Wall -Wextra -Wpedantic -Werror)

add_executable(Client ${sources_client})
target_compile_options(Client PUBLIC -Wall -Wextra -Wpedantic -Werror)

find_package(Boost 1.81.0 COMPONENTS log program_options system REQUIRED)

target_link_libraries(Server PUBLIC ${Boost_LIBRARIES})

target_link_libraries(Client PUBLIC ${Boost_LIBRARIES})

install(TARGETS Server Client)

install(FILES testdata/config.txt DESTINATION share/Server/examples)
install(FILES readme.txt DESTINATION share/Server)
