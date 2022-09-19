CC	:= gcc

OBJS	:= kerncc.o kernccd.o utils.o

CFLAGS	:= -Iinclude -g

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

all: $(OBJS)
	gcc kerncc.o utils.o -o kerncc
	gcc kernccd.o utils.o -o kernccd

clean:
	rm -f *.o kerncc kernccd
