CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -O2
INCLUDES := -Iinclude
BUILD_DIR := build

LIB_SOURCES := src/domain/order.cpp src/fix/fix_parser.cpp src/engine/matching_engine.cpp src/book/order_book.cpp
APP_SOURCES := $(LIB_SOURCES) app/main.cpp
TEST_SOURCES := $(LIB_SOURCES) tests/order_book_tests.cpp

APP_TARGET := $(BUILD_DIR)/matching_engine.exe
TEST_TARGET := $(BUILD_DIR)/order_book_tests.exe

.PHONY: all test clean

all: $(APP_TARGET)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(APP_TARGET): $(APP_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(APP_SOURCES) -o $(APP_TARGET)

$(TEST_TARGET): $(TEST_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SOURCES) -o $(TEST_TARGET)

test: $(TEST_TARGET)
	$(TEST_TARGET)

clean:
	rmdir /S /Q $(BUILD_DIR)
