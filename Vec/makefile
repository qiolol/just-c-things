CC = gcc
CFLAGS = -std=c11 \
	 -Wall \
	 -Wextra \
	 -Werror \
	 -pedantic \
	 -pedantic-errors \

DIR_DEBUG = debug
DIR_RELEASE = release

EXAMPLE_EXE = example.exe
TESTS_EXE = tests.exe

MAKE_DIR=@mkdir -p ${@D}

################################
# Non-file targets             #
################################
.PHONY: all clean debug release

# Builds test and example executables in both build modes
all: debug release

# Deletes artifacts
clean:
	rm -f ${DIR_DEBUG}/*.o
	rm -f ${DIR_DEBUG}/*.exe

	rm -f ${DIR_RELEASE}/*.o
	rm -f ${DIR_RELEASE}/*.exe

	rm -f ${DIR_BENCHMARK}/*.o
	rm -f ${DIR_BENCHMARK}/*.exe

# Debug build
debug: export DIR = ${DIR_DEBUG}
debug: CFLAGS += -Og -g -fsanitize=undefined
debug:
	@echo "################################"
	@echo "# MAKING DEBUG BUILD"
	${MAKE} ${DIR}/${EXAMPLE_EXE} CFLAGS="${CFLAGS}" DIR="${DIR}"
	${MAKE} ${DIR}/${TESTS_EXE} CFLAGS="${CFLAGS}" DIR="${DIR}"

# Release build
release: export DIR = ${DIR_RELEASE}
release: CFLAGS += -O3 -DNDEBUG
release:
	@echo "################################"
	@echo "# MAKING RELEASE BUILD"
	${MAKE} ${DIR}/${EXAMPLE_EXE} CFLAGS="${CFLAGS}" DIR="${DIR}"

################################
# File targets                 #
################################
# Builds the usage demo
${DIR}/${EXAMPLE_EXE}: ${DIR}/example.o ${DIR}/Vec.o
	${MAKE_DIR}
	@echo "# # # # # # # # # # # # # # # # "
	@echo "# Building ${DIR}/${EXAMPLE_EXE}"
	${CC} ${CFLAGS} -lm "${DIR}"/example.o \
	                    "${DIR}"/Vec.o \
	                    -o "${DIR}/${EXAMPLE_EXE}"

${DIR}/example.o: example.c Vec.h
	${MAKE_DIR}
	${CC} ${CFLAGS} -c example.c \
	                -o "${DIR}"/example.o

${DIR}/Vec.o: Vec.c Vec.h
	${MAKE_DIR}
	${CC} ${CFLAGS} -c Vec.c \
	                -o "${DIR}"/Vec.o

# Builds the unit tests
#
# For this, we don't want the vector's asserts to fire. We only want the tests'
# asserts to fire so the tests can verify whether the vector code handles
# assert-offending cases gracefully when vector asserts are disabled (like when
# the vector code's built in release mode).
#
# So, for the test executable, we use a testing-specific version of the vector
# code object that has asserts disabled via `-DNDEBUG`.
${DIR}/${TESTS_EXE}: ${DIR}/tests.o ${DIR}/Vec_test.o
	${MAKE_DIR}
	@echo "# # # # # # # # # # # # # # # # "
	@echo "# Building ${DIR}/${TESTS_EXE}"
	${CC} ${CFLAGS} -lm "${DIR}"/tests.o \
	                    "${DIR}"/Vec_test.o \
	                    -o "${DIR}/${TESTS_EXE}"

${DIR}/tests.o: tests.c Vec.h
	${MAKE_DIR}
	${CC} ${CFLAGS} -c tests.c \
	                -o "${DIR}"/tests.o

# Test-specific version of the vector code object with asserts disabled via
# `-DNDEBUG`
${DIR}/Vec_test.o: Vec.c Vec.h
	${MAKE_DIR}
	${CC} ${CFLAGS} -DNDEBUG -c Vec.c \
	                         -o "${DIR}"/Vec_test.o
