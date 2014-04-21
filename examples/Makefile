LDFLAGS+= -lmarpa
CFLAGS+= -Wall -g

all: ambiguous_grammar

ambiguous_grammar: ambiguous_grammar.o genericStack.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c thin_macros.h stack.h
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -f *.o core

mrproper: clean
