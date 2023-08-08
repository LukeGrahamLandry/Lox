CXX=g++ $(CXXFLAGS)
CXXFLAGS=-g -std=c++17

memory.o: memory.h memory.cc
	$(CXX) -c memory.cc -g

list.o: list.cc memory.o common.h
	$(CXX) -c list.cc memory.o -g

value.o: value.h value.cc list.o object.o common.h
	$(CXX) -c value.cc list.o object.o -g

chunk.o: chunk.h chunk.cc list.o value.o common.h
	$(CXX) -c chunk.cc list.o value.o -g

debug.o: debug.h debug.cc chunk.o value.o common.h
	$(CXX) -c debug.cc chunk.o value.o -g

vm.o: vm.h vm.cc chunk.o value.o debug.o compiler/compiler.o object.o table.o common.h natives.o
	$(CXX) -c vm.cc chunk.o value.o debug.o compiler.o object.o table.o natives.o -g

scanner.o: scanner.cc scanner.h chunk.o value.o
	$(CXX) -c scanner.cc chunk.o value.o -g

compiler/compiler.o: compiler/compiler.cc compiler/compiler.h chunk.o value.o scanner.o object.o common.h
	$(CXX) -c compiler/compiler.cc chunk.o value.o object.o -g

compiler/expression.o: compiler/expression.cc compiler/compiler.h chunk.o value.o scanner.o object.o common.h
	$(CXX) -c compiler/expression.cc chunk.o value.o object.o -g

compiler/flow.o: compiler/flow.cc compiler/compiler.h chunk.o value.o scanner.o object.o common.h
	$(CXX) -c compiler/flow.cc chunk.o value.o object.o -g

compiler/util.o: compiler/util.cc compiler/compiler.h chunk.o value.o scanner.o object.o common.h
	$(CXX) -c compiler/util.cc chunk.o value.o object.o -g

object.o: object.cc object.h
	$(CXX) -c object.cc -g

natives.o: lib/natives.cc value.cc value.h
	$(CXX) -c lib/natives.cc -g

table.o: table.cc table.h
	$(CXX) -c table.cc -g

tests: tests/tests.cc compiler/util.o compiler/compiler.o compiler/expression.o compiler/flow.o chunk.o debug.o vm.o scanner.o object.o table.o common.h
	$(CXX) -o out/clox_tests tests/tests.cc util.o compiler.o expression.o flow.o list.o chunk.o list.o memory.o value.o debug.o vm.o scanner.o object.o table.o -g

main: main.cc compiler/util.o compiler/compiler.o compiler/expression.o compiler/flow.o chunk.o debug.o vm.o scanner.o object.o table.o common.h
	$(CXX) -o out/clox main.cc chunk.o util.o compiler.o expression.o flow.o list.o memory.o value.o debug.o vm.o scanner.o object.o table.o -g

clean:
	rm -r *.o
	rm -r out
	mkdir "out"

all: main tests
