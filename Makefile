CC = gcc
CFLAGS = -Wall -ansi -pedantic -g
MAIN = mytar
OBJS = mytar.o 
all: $(MAIN)

$(MAIN) : $(OBJS) mytar.h
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

mytar.o : mytar.c mytar.h
	$(CC) $(CFLAGS) -c mytar.c

clean :
	rm *.o $(MAIN) 

