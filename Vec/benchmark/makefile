CC = gcc
CFLAGS = -std=c11 \
	 -O3 \
	 -DNDEBUG \

CXX = g++
CXXFLAGS = -std=c++11 \
	   -O3 \
	   -DNDEBUG \

C_VEC = Vec
C_VEC_DIR = ../
BENCH_CODE = benchmark
BENCH_EXE = benchmark.exe

.PHONY: benchmark clean

benchmark:
	$(MAKE) $(BENCH_EXE)

clean:
	rm -f *.o
	rm -f *.exe

# Link C and C++ objects together.
$(BENCH_EXE): $(BENCH_CODE).o $(C_VEC).o
	${CXX} $(BENCH_CODE).o $(C_VEC).o -o $(BENCH_EXE)

# Compile C++ code.
$(BENCH_CODE).o: $(BENCH_CODE).cpp $(C_VEC_DIR)/$(C_VEC).h
	${CXX} ${CXXFLAGS} -c $(BENCH_CODE).cpp -o $(BENCH_CODE).o

# Compile C code.
$(C_VEC).o: $(C_VEC_DIR)/$(C_VEC).c $(C_VEC_DIR)/$(C_VEC).h
	${CC} ${CFLAGS} -c $(C_VEC_DIR)/$(C_VEC).c -o $(C_VEC).o
