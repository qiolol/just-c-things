CC = gcc
CFLAGS = -std=c17 \
	 -Og \
	 -g3 \
	 -Wall \
	 -Wextra \
	 -Werror \
	 -pedantic \
	 -pedantic-errors \
	 -fsanitize=undefined

.PHONY: all clean

EXAMPLE_EXECUTABLE = example.exe
TEST_EXECUTABLE = tests.exe

# Builds all executables
all: ${EXAMPLE_EXECUTABLE} ${TEST_EXECUTABLE}

# Deletes artifacts
clean:
	# "🧹..."
	rm -f *.o
	rm -f *.exe

# This one's a usage demo of the vector.
${EXAMPLE_EXECUTABLE}: example.o Vec.o
	${CC} ${CFLAGS} -lm example.o Vec.o -o ${EXAMPLE_EXECUTABLE}

example.o: example.c Vec.h
	${CC} ${CFLAGS} -c example.c -o example.o

Vec.o: Vec.c Vec.h
	${CC} ${CFLAGS} -c Vec.c -o Vec.o

# This one's for the unit tests.
#
# For the tests, we don't want the vector's asserts to fire. We only want the
# tests' asserts to fire as the tests verify whether the vector code handles
# assert-offending cases gracefully when asserts are disabled.
#
# So, to disable the vector code's asserts, the test executable will use a test
# version of the vector code object with asserts disabled.
${TEST_EXECUTABLE}: tests.o Vec_test.o
	${CC} ${CFLAGS} -lm tests.o Vec_test.o -o ${TEST_EXECUTABLE}

tests.o: tests.c Vec.h
	${CC} ${CFLAGS} -c tests.c -o tests.o

# Test version of the vector code object with asserts disabled via `-DNDEBUG`
Vec_test.o: Vec.c Vec.h
	${CC} ${CFLAGS} -DNDEBUG -c Vec.c -o Vec_test.o
