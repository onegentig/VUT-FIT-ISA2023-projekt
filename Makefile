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
#   - `make zip` to create a zip archive of the project
#   - `make help` to show Makefile usage
#
###############################################################################

CLIENT_TARGET          = tftp-client
SERVER_TARGET          = tftp-server
ZIPNAME                = xkrame00.tar

CLIENT_SRC             = src/client
SERVER_SRC             = src/server
OBJ_DIR                = obj
BIN_DIR                = bin

CPP                    = g++
CPPFLAGS               = -std=c++20
EXTRA_CPPFLAGS         = -Wall -Wextra -Werror -pedantic \
                    -fdata-sections -ffunction-sections
RELEASE_CPPFLAGS       = -DNDEBUG -O2 -march=native
DEBUG_CPPFLAGS         = -g -Og -fsanitize=undefined
LINT_FLAGS             = --format-style=file --fix \
    -checks="bugprone-*,google-*,performance-*,readability-*"
RM                     = rm -f

CLIENT_SRCS            = $(wildcard $(CLIENT_SRC)/*.cpp)
SERVER_SRCS            = $(wildcard $(SERVER_SRC)/*.cpp)
CLIENT_OBJS            = $(CLIENT_SRCS:.cpp=.o)
SERVER_OBJS            = $(SERVER_SRCS:.cpp=.o)

###############################################################################

.PHONY: all release debug help clean zip lint format

all: release

release: EXTRA_CPPFLAGS += ${RELEASE_CPPFLAGS}
release: $(CLIENT_TARGET) $(SERVER_TARGET)

debug: EXTRA_CPPFLAGS += ${DEBUG_CPPFLAGS}
debug: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) $^ -o $(BIN_DIR)/$@

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) $^ -o $(BIN_DIR)/$@

# generic rule for .cpp -> .o
%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(EXTRA_CPPFLAGS) -c $< -o $@

help:
	@echo "tftp-client & tftp-server Makefile"
	@echo "@author Filip J. Kramec <xkrame00@vutbr.cz>"
	@echo ""
	@echo "Usage: make [TARGET]"
	@echo "TARGETs:"
	@echo "  all     compile and link the project (default)"
	@echo "  debug   compile and link the project with debug flags"
	@echo "  clean   clean objects and executables"
	@echo "  format  run formatter"
	@echo "  lint    run linter"
	@echo "  zip     create a .zip archive with the source files"
	@echo "  help    print this message"

clean:
	$(RM) $(CLIENT_OBJS) $(SERVER_OBJS)
	$(RM) $(BIN_DIR)/$(CLIENT_TARGET) $(BIN_DIR)/$(SERVER_TARGET)

zip:
	zip -q -r $(ZIPNAME) src include obj Makefile

format:
	clang-format -i *.cpp *.hpp

lint:
	clang-tidy ${CLIENT_SRCS} ${LINT_FLAGS} -- ${CPPFLAGS} ${EXTRA_CPPFLAGS}
	clang-tidy ${SERVER_SRCS} ${LINT_FLAGS} -- ${CPPFLAGS} ${EXTRA_CPPFLAGS}
