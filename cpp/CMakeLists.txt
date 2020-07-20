cmake_minimum_required(VERSION 3.0)	 
project(main)

add_executable(main
    main.cpp
)

add_subdirectory(oop)

target_link_libraries(main oop)
