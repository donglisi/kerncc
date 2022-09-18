CC	:= gcc

OBJS	:= kerndcc.o kerndcc-server.o utils.o

CFLAGS	:= -nostdinc -I/usr/x86_64-linux-musl/include -Iinclude -g

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

all: $(OBJS)
	ld -nostdlib -L/usr/x86_64-linux-musl/lib64 -lc -lpthread -rpath /usr/x86_64-linux-musl/lib64/ \
		-dynamic-linker /lib/ld-musl-x86_64.so.1 /usr/x86_64-linux-musl/lib64/crt1.o kerndcc-server.o utils.o -o kerndcc-server
	ld -nostdlib -L/usr/x86_64-linux-musl/lib64 -lc -rpath /usr/x86_64-linux-musl/lib64/ \
		-dynamic-linker /lib/ld-musl-x86_64.so.1 /usr/x86_64-linux-musl/lib64/crt1.o kerndcc.o utils.o -o kerndcc

clean:
	rm -f *.o kerndcc kerndcc-server 
