CC = clang
CFLAGS = -Wall -Werror -Wextra -pedantic 

all: memory

memory: memory.o
	$(CC) -o memory memory.o

memory.o: memory.c 
	$(CC) $(CFLAGS) -c memory.c

clean:
	rm -f memory *.o

format:
	clang-format -i memory.c
