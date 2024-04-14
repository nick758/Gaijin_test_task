cmake_minimum_required(VERSION 3.0)
project(Gaijin_test_task)

file(GLOB sources_server main.cpp
          Server.cpp Server.h
          Storage.cpp Storage.h
          )

add_executable(Server ${sources_server})

find_package(Boost 1.36.0 COMPONENTS log program_options REQUIRED)

target_link_libraries(Server PUBLIC
  ${Boost_LIBRARIES}
)
