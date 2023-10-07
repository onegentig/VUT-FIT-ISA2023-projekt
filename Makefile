###############################################################################
#
# tftp-client & tftp-server Makefile
# @author Filip J. Kramec <xkrame00@vutbr.cz>
#
# Usage:
#   - `make` or `make all` or `make release` to build the project
#   - `make debug` to build the project with debug flags
#   - `make clean` to remove built binaries
#   - `make format` to format the source code
#   - `make lint` to lint the source code
#   - `make tar` or `make zip` to create a tar archive of the source code
#   - `make help` to show Makefile usage
#
###############################################################################

CLIENT_TARGET          = tftp-client
SERVER_TARGET          = tftp-server
TARNAME                = xkrame00.tar

SRC_DIR                = src
OBJ_DIR                = obj
INCLUDE_DIR            = include
CLIENT_SRC             = $(SRC_DIR)/client
SERVER_SRC             = $(SRC_DIR)/server
PACKET_SRC             = $(SRC_DIR)/packet

CPP                    = g++
CPPFLAGS               = -std=c++20 -I$(INCLUDE_DIR)
EXTRA_CPPFLAGS         = -Wall -Wextra -Werror -pedantic \
                    -fdata-sections -ffunction-sections
RELEASE_CPPFLAGS       = -DNDEBUG -O2 -march=native
DEBUG_CPPFLAGS         = -g -Og -fsanitize=undefined
LINT_FLAGS             = --format-style=file -fix-errors \
    -checks="bugprone-*,google-*,performance-*,readability-*"

TEST_DIR               = test
TEST_TARGET            = tests
CATCH2_VERSION         = 3.4.0
CATCH2_DIR             = $(TEST_DIR)/Catch2

RM                     = rm -f

###############################################################################

INCLUDES               := $(shell find include/ -type f -name '*.hpp')
CLIENT_SRCS            := $(wildcard $(CLIENT_SRC)/*.cpp)
SERVER_SRCS            := $(wildcard $(SERVER_SRC)/*.cpp)
PACKET_SRCS            := $(wildcard $(PACKET_SRC)/*.cpp)

CLIENT_OBJS            := $(CLIENT_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
SERVER_OBJS            := $(SERVER_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
PACKET_OBJS            := $(PACKET_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CATCH2_H_URL           := \
	https://github.com/catchorg/Catch2/releases/download/v$(CATCH2_VERSION)/catch_amalgamated.hpp
CATCH2_C_URL           := \
	https://github.com/catchorg/Catch2/releases/download/v$(CATCH2_VERSION)/catch_amalgamated.cpp
CATCH2_HEADER          := $(CATCH2_DIR)/catch_amalgamated.hpp
CATCH2_SRC             := $(CATCH2_DIR)/catch_amalgamated.cpp
CATCH2_OBJ             := $(OBJ_DIR)/test/catch_amalgamated.o
TEST_SRCS              := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS              := $(TEST_SRCS:$(TEST_DIR)/%.cpp=$(OBJ_DIR)/test/%.o)

SRCS                   := $(CLIENT_SRCS) $(SERVER_SRCS) $(PACKET_SRCS) \
	$(TEST_SRCS)
OBJS                   := $(CLIENT_OBJS) $(SERVER_OBJS) $(PACKET_OBJS)
CLASS_OBJS             := $(filter-out %main.o, $(OBJS))

###############################################################################

.PHONY: all release debug help test clean tar zip lint format

all: release

##### Building primary targets #####

release: EXTRA_CPPFLAGS += ${RELEASE_CPPFLAGS}
release: $(CLIENT_TARGET) $(SERVER_TARGET)

debug: EXTRA_CPPFLAGS += ${DEBUG_CPPFLAGS}
debug: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJS) $(PACKET_OBJS)
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) $(CLIENT_OBJS) $(PACKET_OBJS) -o $(CLIENT_TARGET)
	@echo "  tftp-client compiled!"
	@echo "  Run with: ./tftp-client <-h hostname> [-p port] [-f path] <-t path>"

$(SERVER_TARGET): $(SERVER_OBJS) $(PACKET_OBJS)
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) $(SERVER_OBJS) $(PACKET_OBJS) -o $(SERVER_TARGET)
	@echo "  tftp-server compiled!"
	@echo "  Run with: ./tftp-server [-p port] <path>"

$(OBJS): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) -c $< -o $@

##### Tests #####

test: $(TEST_TARGET)
	@echo "  Running tests..."
	@echo ""
	@./$(TEST_TARGET)

$(TEST_TARGET): $(CATCH2_SRC) $(CATCH2_OBJ) $(TEST_OBJS) $(CLASS_OBJS)
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) -Itest/Catch2 $(TEST_OBJS) \
		$(CLASS_OBJS) $(CATCH2_OBJ) -o $(TEST_TARGET)
	@echo "  Tests compiled!"

$(TEST_OBJS): $(OBJ_DIR)/test/%.o : $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) -Itest/Catch2 -c $< -o $@

$(CATCH2_OBJ): $(CATCH2_SRC)
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) -Itest/Catch2 -c $(CATCH2_SRC) -o $(CATCH2_OBJ)

$(CATCH2_SRC):
	echo "  Catch2 framework not would. Downloading..."
	mkdir -p $(CATCH2_DIR)
	curl -L $(CATCH2_H_URL) -o $(CATCH2_SRC)
	curl -L $(CATCH2_C_URL) -o $(CATCH2_HEADER)

##### Other targets #####

help:
	@echo "tftp-client & tftp-server Makefile"
	@echo "@author Filip J. Kramec <xkrame00@vutbr.cz>"
	@echo ""
	@echo "Usage: make [TARGET]"
	@echo "TARGETs:"
	@echo "  all     compile and link the project (default)"
	@echo "  debug   compile and link the project with debug flags"
	@echo "  clean   clean built objects, executables and archives"
	@echo "  format  run formatter"
	@echo "  lint    run linter"
	@echo "  tar     create a .tar archive with all the source files"
	@echo "  help    print this message"

clean:
	$(RM) $(CLIENT_TARGET) $(SERVER_TARGET) $(TARNAME)
	$(RM) -r $(OBJ_DIR)/*
	@echo "  Cleaned!"

tar: clean
	tar -cf $(TARNAME) src include obj Makefile

zip: tar

format:
	clang-format -i $(INCLUDES) $(SRCS)
	@echo "  Formatted!"

lint:
	clang-tidy $(SRCS) $(INCLUDES) $(LINT_FLAGS) -- $(CPPFLAGS) $(EXTRA_CPPFLAGS)
