cmake_minimum_required(VERSION 3.1)
project(Memory_Simulator VERSION 1.0
                  DESCRIPTION "A simple memory management simulation"
                  LANGUAGES CXX)

add_executable(mem_sim mem_sim.cpp mem_sim.h)