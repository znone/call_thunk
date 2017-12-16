TARGET= libthunk.a libthunk.so scandir
CC=g++
OBJ=call_thunk.o
CFLAGS=-g -Wall -I/usr/local/include -O2
LDFLAGS= -L/usr/local/lib -ldl  

all : $(TARGET)

call_thunk.o : call_thunk.cpp call_thunk.h
	$(CC) -c $(CFLAGS) -fPIC -o $@ $<

libthunk.so :  call_thunk.o
	gcc  -fPIC -shared -o $@ $^

libthunk.a : call_thunk.o
	ar rc $@ $^

scandir : sample/scandir.cpp call_thunk.h
	 $(CC) $(CFLAGS) -o $@ $<   libthunk.so
	
clean:
	rm $(TARGET) $(OBJ) -f
