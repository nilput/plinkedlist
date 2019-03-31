CFLAGS := 
.PHONY: all clean debug rel
all: rel

rel: CFLAGS := -O3 -DNDEBUG
rel: test

debug: CFLAGS := -O0 -g3 -fsanitize=address,undefined
debug: LDLIBS := -lasan -lubsan
debug: test

test: util.o test.o

clean:
	rm *.o test
