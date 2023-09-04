CC = gcc
CFLAGS = -std=c17 \
	 -Wall \
	 -Wextra \
	 -Werror \
	 -pedantic \
	 -pedantic-errors

.PHONY: buildmode all clean

EXAMPLE_EXE = example.exe
TESTS_EXE = tests.exe

# Apply build mode
BUILD_DIR = $(mode)

ifeq ($(mode), debug)
CFLAGS += -Og \
	  -g3 \
	  -fsanitize=undefined
else ifeq ($(mode), release)
CFLAGS += -O3
endif

# Handy shorthand for making directories if they don't exist (Unix-only!)
MAKE_DIR=@mkdir -p $(@D)

# Builds all executables
all: buildmode $(BUILD_DIR)/$(EXAMPLE_EXE) $(BUILD_DIR)/$(TESTS_EXE)

# Deletes artifacts
clean:
	@echo "🧹  ..."
	rm -f debug/*.o
	rm -f debug/*.exe

	rm -f release/*.o
	rm -f release/*.exe

# Checks and applies the build mode
buildmode:
# Check for valid build mode
ifneq ($(mode), debug)
ifneq ($(mode), release)
	@echo "Valid build mode needed."
	@echo "Run \`make\` with \`mode=debug\` or \`mode=release\`."
	@exit 1
endif
endif

# Build the executable of the usage demo.
$(BUILD_DIR)/$(EXAMPLE_EXE): $(BUILD_DIR)/example.o $(BUILD_DIR)/Vec.o
	$(MAKE_DIR)
	@echo "🏗️  Building $(BUILD_DIR)/$(EXAMPLE_EXE)"
	${CC} ${CFLAGS} -lm $(BUILD_DIR)/example.o \
	                    $(BUILD_DIR)/Vec.o -o \
	                    $(BUILD_DIR)/$(EXAMPLE_EXE)

$(BUILD_DIR)/example.o: example.c Vec.h
	$(MAKE_DIR)
	${CC} ${CFLAGS} -c example.c \
	                -o $(BUILD_DIR)/example.o

$(BUILD_DIR)/Vec.o: Vec.c Vec.h
	$(MAKE_DIR)
	${CC} ${CFLAGS} -c Vec.c \
	                -o $(BUILD_DIR)/Vec.o

# Build the executable of the unit tests.
#
# For this, we don't want the vector's asserts to fire. We only want the tests'
# asserts to fire. The tests verify whether the vector code handles
# assert-offending cases gracefully when vector asserts are disabled.
#
# So the test executable has to use a testing-specific version of the vector
# code object -- one with asserts disabled via `-DNDEBUG`.
$(BUILD_DIR)/$(TESTS_EXE): $(BUILD_DIR)/tests.o $(BUILD_DIR)/Vec_test.o
	$(MAKE_DIR)
	@echo "🏗️  Building $(BUILD_DIR)/$(TESTS_EXE)"
	${CC} ${CFLAGS} -lm $(BUILD_DIR)/tests.o \
	                    $(BUILD_DIR)/Vec_test.o \
	                    -o $(BUILD_DIR)/$(TESTS_EXE)

$(BUILD_DIR)/tests.o: tests.c Vec.h
	$(MAKE_DIR)
	${CC} ${CFLAGS} -c tests.c \
	                -o $(BUILD_DIR)/tests.o

# Test-specific version of the vector code object with asserts disabled via
# `-DNDEBUG`
$(BUILD_DIR)/Vec_test.o: Vec.c Vec.h
	$(MAKE_DIR)
	${CC} ${CFLAGS} -DNDEBUG -c Vec.c \
	                         -o $(BUILD_DIR)/Vec_test.o
