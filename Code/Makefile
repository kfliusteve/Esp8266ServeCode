CC= gcc
CFLAGS= -Wall -g -O0
SRC=main.c 
OBJS=myapp

all:
	$(CC) $(CFLAGS) -o $(OBJS)  $(SRC) -lsqlite3 -ljson-c
clean:
	@$(RM)  $(OBJS)  .*.sw? 


