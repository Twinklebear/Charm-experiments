CHARMDIR?=~/libs/charm/mpi-linux-x86_64-smp-mpicxx
CHARMC=$(CHARMDIR)/bin/charmc $(OPTS)

all: hello.out

hello.out: main.o hello.o
	$(CHARMC) -language charm++ -c++-option -std=c++14 $^ -o $@

hello.o: hello.cpp hello.h hello.decl.h hello.def.h main.decl.h
	$(CHARMC) $< -o $@

%.o: %.cpp %.h %.decl.h %.def.h hello.decl.h
	$(CHARMC) $< -o $@

%.decl.h %.def.h: %.ci
	$(CHARMC) $^

# Cancel Make's default cpp compilation rule
%.o: %.cpp

.PHONY: clean
clean:
	rm -f *.decl.h *.def.h *.o hello charmrun

