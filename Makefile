# cory and nova use gcc version 3.3.2

TARGET=tritontalk

CC = gcc
DEBUG = -g #-v

LDFLAGS = -lresolv -lnsl -lpthread -lm

OS = LINUX

CCFLAGS = -Wall $(DEBUG) -D$(OS)

# add object file names here
OBJS = main.o util.o input.o communicate.o sender.o receiver.o crc.o

all: tritontalk

%.o : %.c
	$(CC) -c $(CCFLAGS) $<

%.o : %.cc
	$(CC) -c $(CCFLAGS) $<

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CCFLAGS) $(LDFLAGS) 

clean:
	rm -f $(TARGET) core *.o *~ 

submit: clean
	rm -f project1.tgz; tar czvf project1.tgz *; turnin project1.tgz -c cs123f -p project1
