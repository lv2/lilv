CFLAGS = -Wall -I../../../include -fPIC

all: amp.so

amp.so: amp.o
	$(LD) amp.o -o amp.so -shared
	rm amp.o

clean:
	rm *.o amp.so
