cmake_minimum_required(VERSION 2.8)
project(fftResample LANGUAGES C)
 
add_executable(fftResample main.c)
target_link_libraries(fftResample -lm)