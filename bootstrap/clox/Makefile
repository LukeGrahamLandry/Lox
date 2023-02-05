memory.o: memory.h memory.cc
	g++ -c memory.cc 

list.o: list.cc memory.o
	g++ -c list.cc memory.o

value.o: value.h value.cc list.o
	g++ -c value.cc list.o

chunk.o: chunk.h chunk.cc list.o value.o 
	g++ -c chunk.cc list.o value.o

debug.o: debug.h debug.cc chunk.o value.o
	g++ -c debug.cc chunk.o value.o

vm.o: vm.h vm.cc chunk.o value.o debug.o
	g++ -c vm.cc chunk.o value.o debug.o

main: main.cc chunk.o debug.o vm.o
	g++ -o clox main.cc chunk.o list.o memory.o value.o debug.o vm.o

clean:
	rm -r *.o clox

all: main
