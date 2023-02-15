memory.o: memory.h memory.cc
	g++ -c memory.cc -g

list.o: list.cc memory.o common.h
	g++ -c list.cc memory.o -g

value.o: value.h value.cc list.o object.o common.h
	g++ -c value.cc list.o object.o -g

chunk.o: chunk.h chunk.cc list.o value.o common.h
	g++ -c chunk.cc list.o value.o -g

debug.o: debug.h debug.cc chunk.o value.o common.h
	g++ -c debug.cc chunk.o value.o -g

vm.o: vm.h vm.cc chunk.o value.o debug.o compiler.o object.o table.o common.h
	g++ -c vm.cc chunk.o value.o debug.o compiler.o object.o table.o -g

scanner.o: scanner.cc scanner.h chunk.o value.o
	g++ -c scanner.cc chunk.o value.o -g

compiler/compiler.o: compiler/compiler.cc compiler/compiler.h chunk.o value.o scanner.o object.o common.h
	g++ -c compiler/compiler.cc chunk.o value.o object.o -g

compiler/expression.o: compiler/expression.cc compiler/compiler.h chunk.o value.o scanner.o object.o common.h
	g++ -c compiler/expression.cc chunk.o value.o object.o -g

compiler/flow.o: compiler/flow.cc compiler/compiler.h chunk.o value.o scanner.o object.o common.h
	g++ -c compiler/flow.cc chunk.o value.o object.o -g

compiler/util.o: compiler/util.cc compiler/compiler.h chunk.o value.o scanner.o object.o common.h
	g++ -c compiler/util.cc chunk.o value.o object.o -g

object.o: object.cc object.h
	g++ -c object.cc -g

table.o: table.cc table.h
	g++ -c table.cc -g

tests: tests/tests.cc compiler/util.o compiler/compiler.o compiler/expression.o compiler/flow.o chunk.o debug.o vm.o compiler.o scanner.o object.o table.o common.h
	g++ -o out/clox_tests tests/tests.cc util.o compiler.o expression.o flow.o list.o chunk.o list.o memory.o value.o debug.o vm.o scanner.o object.o table.o -g

main: main.cc compiler/util.o compiler/compiler.o compiler/expression.o compiler/flow.o chunk.o debug.o vm.o compiler.o scanner.o object.o table.o common.h
	g++ -o out/clox main.cc chunk.o util.o compiler.o expression.o flow.o list.o memory.o value.o debug.o vm.o scanner.o object.o table.o -g

clean:
	rm -r *.o
	rm -r out
	mkdir "out"

all: main tests
