###############################################################################
#
# tftp-client & tftp-server Makefile
# @author Onegen Something <xkrame00@vutbr.cz>
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

CPP                    = g++
CPPFLAGS               = -std=c++20 -I$(INCLUDE_DIR)
EXTRA_CPPFLAGS         = -Wall -Wextra -Werror -pedantic \
                    -fdata-sections -ffunction-sections
RELEASE_CPPFLAGS       = -DNDEBUG -O2 -march=native
DEBUG_CPPFLAGS         = -g -Og -fsanitize=undefined
LINT_FLAGS             = --format-style=file --fix \
    -checks="bugprone-*,google-*,performance-*,readability-*"

RM                     = rm -f

###############################################################################

INCLUDES			   := $(wildcard $(INCLUDE_DIR)/*.hpp)
CLIENT_SRCS            := $(wildcard $(CLIENT_SRC)/*.cpp)
SERVER_SRCS            := $(wildcard $(SERVER_SRC)/*.cpp)

CLIENT_OBJS            := $(CLIENT_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
SERVER_OBJS            := $(SERVER_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
OBJS                   := $(CLIENT_OBJS) $(SERVER_OBJS)

###############################################################################

.PHONY: all release debug help clean tar zip lint format

all: release

release: EXTRA_CPPFLAGS += ${RELEASE_CPPFLAGS}
release: $(CLIENT_TARGET) $(SERVER_TARGET)

debug: EXTRA_CPPFLAGS += ${DEBUG_CPPFLAGS}
debug: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) $(CLIENT_OBJS) -o $(CLIENT_TARGET)
	@echo "  tftp-client compiled!"
	@echo "  Run with: ./tftp-client <-h hostname> [-p port] [-f path] <-t path>"

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) $(SERVER_OBJS) -o $(SERVER_TARGET)
	@echo "  tftp-server compiled!"
	@echo "  Run with: ./tftp-server [-p port] <path>"

$(OBJS): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) -c $< -o $@

help:
	@echo "tftp-client & tftp-server Makefile"
	@echo "@author Onegen Something <xkrame00@vutbr.cz>"
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
	clang-format -i ${CLIENT_SRCS} ${SERVER_SRCS} ${INCLUDES}
	@echo "  Formatted!"

lint:
	clang-tidy ${CLIENT_SRCS} ${LINT_FLAGS} -- ${CPPFLAGS} ${EXTRA_CPPFLAGS}
	clang-tidy ${SERVER_SRCS} ${LINT_FLAGS} -- ${CPPFLAGS} ${EXTRA_CPPFLAGS}
