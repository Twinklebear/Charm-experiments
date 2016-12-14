CHARMDIR=~/libs/charm/mpi-linux-x86_64-smp-mpicxx
CHARMC=$(CHARMDIR)/bin/charmc $(OPTS)

all: hello.out

hello.out: main.o
	$(CHARMC) -language charm++ $< -o $@

main.o: main.cpp main.h main.decl.h main.def.h
	$(CHARMC) $< -o $@

%.decl.h %.def.h: %.ci
	$(CHARMC) $^

.PHONY: clean
clean:
	rm -f main.decl.h main.def.h main.o hello charmrun 

