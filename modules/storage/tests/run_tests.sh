#!/bin/bash

# Compile the test files
g++ -std=c++23 -I../include -I../../types/include -I../../event/include -I../../beacon/include \
    -o storage_tests \
    run_tests.cpp \
    unit/data_models_test.cpp \
    unit/query_interface_test.cpp \
    unit/migration_manager_test.cpp \
    unit/storage_manager_test.cpp \
    ../src/database.cpp \
    ../src/data_models.cpp \
    ../src/query_interface.cpp \
    ../src/migration_manager.cpp \
    ../src/storage_manager.cpp \
    -lsqlite3 -lgtest -lgtest_main -pthread

# Run the tests
./storage_tests
