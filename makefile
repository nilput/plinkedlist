CFLAGS := 
.PHONY: all clean debug rel
all: rel

rel: CFLAGS := -O2 -DNDEBUG
rel: test bench

rel_lto: CFLAGS := -O2 -DNDEBUG -flto
rel_lto: test bench

debug: CFLAGS := -O0 -g3 -fsanitize=address,undefined
debug: LDLIBS := -lasan -lubsan
debug: test bench

test: util.o test.o
bench: util.o bench.o

clean:
	rm *.o test bench
