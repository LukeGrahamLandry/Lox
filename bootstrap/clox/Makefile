memory.o: memory.h memory.cc
	g++ -c memory.cc 

list.o: list.h memory.o
	g++ -c list.h memory.o

value.o: value.h value.cc list.o
	g++ -c value.cc list.o

chunk.o: chunk.h chunk.cc value.o list.o
	g++ -c chunk.cc list.o value.o

debug.o: debug.h debug.cc chunk.o list.o
	g++ -c debug.cc chunk.o value.o

main: main.cc chunk.o debug.o
	g++ -o clox main.cc chunk.o list.o memory.o value.o debug.o