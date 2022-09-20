%.o: %.c
	gcc -c -Iinclude -g $< -o $@

all: kerncc kernccd

kerncc: kerncc.o utils.o
	gcc $^ -o $@

kernccd: kernccd.o utils.o
	gcc $^ -o $@

clean:
	rm -f *.o kerncc kernccd
