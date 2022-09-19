CC	:= gcc

OBJS	:= kerndcc.o kerndcc-server.o utils.o

CFLAGS	:= -Iinclude -g

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

all: $(OBJS)
	gcc kerndcc.o utils.o -o kerndcc
	gcc kerndcc-server.o utils.o -o kerndcc-server

clean:
	rm -f *.o kerndcc kerndcc-server 
