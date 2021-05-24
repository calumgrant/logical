project (persist)
cmake_minimum_required(VERSION 3.20)

add_library(persist persist.cpp persist_unix.cpp ../include/persist.h ../include/persist_unix.h)

include_directories(../include)
