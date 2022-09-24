%.o: %.c
	gcc -c -Iinclude -g $< -o $@

all: kerncc kernccd

kerncc: kerncc.o utils.o zpipe.o
	gcc $^ -o $@ -lz

kernccd: kernccd.o utils.o zpipe.o
	gcc $^ -o $@ -lz

clean:
	rm -f *.o kerncc kernccd
