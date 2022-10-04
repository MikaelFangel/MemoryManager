CC = gcc
CCOPTS = -c -g -Wall
LINKOPTS = -g -lrt 

EXEC=mem
OBJECTS=testrunner.o mymem.o memorytests.o

all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) $(LINKOPTS) -o $@ $^

%.o:%.c
	$(CC) $(CCOPTS) -o $@ $^

clean:
	- $(RM) $(EXEC)
	- $(RM) $(OBJECTS)
	- $(RM) *~
	- $(RM) core.*

test: mem
	mem -test -f0 all all

stage1-test: mem
	mem -test -f0 all first

pretty: 
	indent *.c *.h -kr
