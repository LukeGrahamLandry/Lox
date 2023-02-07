memory.o: memory.h memory.cc
	g++ -c memory.cc 

list.o: list.cc memory.o
	g++ -c list.cc memory.o

value.o: value.h value.cc list.o object.o
	g++ -c value.cc list.o object.o

chunk.o: chunk.h chunk.cc list.o value.o 
	g++ -c chunk.cc list.o value.o

debug.o: debug.h debug.cc chunk.o value.o
	g++ -c debug.cc chunk.o value.o

vm.o: vm.h vm.cc chunk.o value.o debug.o compiler.o object.o
	g++ -c vm.cc chunk.o value.o debug.o compiler.o object.o

scanner.o: scanner.cc scanner.h chunk.o value.o
	g++ -c scanner.cc chunk.o value.o

compiler.o: compiler.cc compiler.h chunk.o value.o scanner.o object.o
	g++ -c compiler.cc chunk.o value.o object.o

object.o: object.cc object.h
	g++ -c object.cc

main: main.cc chunk.o debug.o vm.o compiler.o scanner.o object.o
	g++ -o clox main.cc chunk.o list.o memory.o value.o debug.o vm.o compiler.o scanner.o object.o

clean:
	rm -r *.o clox

all: main
