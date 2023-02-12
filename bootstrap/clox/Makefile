memory.o: memory.h memory.cc
	g++ -c memory.cc -g

list.o: list.cc memory.o
	g++ -c list.cc memory.o -g

value.o: value.h value.cc list.o object.o
	g++ -c value.cc list.o object.o -g

chunk.o: chunk.h chunk.cc list.o value.o
	g++ -c chunk.cc list.o value.o -g

debug.o: debug.h debug.cc chunk.o value.o
	g++ -c debug.cc chunk.o value.o -g

vm.o: vm.h vm.cc chunk.o value.o debug.o compiler.o object.o table.o
	g++ -c vm.cc chunk.o value.o debug.o compiler.o object.o table.o -g

scanner.o: scanner.cc scanner.h chunk.o value.o
	g++ -c scanner.cc chunk.o value.o -g

compiler.o: compiler.cc compiler.h chunk.o value.o scanner.o object.o
	g++ -c compiler.cc chunk.o value.o object.o -g

object.o: object.cc object.h
	g++ -c object.cc -g

table.o: table.cc table.h
	g++ -c table.cc -g

tests: tests/tests.cc chunk.o debug.o vm.o compiler.o scanner.o object.o table.o common.h
	g++ -o out/clox_tests tests/tests.cc chunk.o list.o memory.o value.o debug.o vm.o compiler.o scanner.o object.o table.o -g

main: main.cc chunk.o debug.o vm.o compiler.o scanner.o object.o table.o common.h
	g++ -o out/clox main.cc chunk.o list.o memory.o value.o debug.o vm.o compiler.o scanner.o object.o table.o -g

clean:
	rm -r *.o
	rm -r out
	mkdir "out"

all: main tests
