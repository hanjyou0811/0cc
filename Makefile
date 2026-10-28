CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

0cc: $(OBJS)
	$(CC) -o 0cc $(OBJS) $(LDFLAGS)

$(OBJS): 0cc.h

test: 0cc
	./0cc test/test.c > tmp.s
	cc -o tmp tmp.s
	./tmp > tmp.out; test $$? -eq 42
	diff tmp.out test/test.ok
	sh test/int-width.sh

clean:
	rm -f 0cc *.o *~ tmp*

.PHONY: test clean
