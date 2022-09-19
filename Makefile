CC	:= gcc

OBJS	:= kerncc.o kerncc-server.o utils.o

CFLAGS	:= -Iinclude -g

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

all: $(OBJS)
	gcc kerncc.o utils.o -o kerncc
	gcc kerncc-server.o utils.o -o kerncc-server

clean:
	rm -f *.o kerncc kerncc-server 
