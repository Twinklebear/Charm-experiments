CHARMDIR?=~/libs/charm/mpi-linux-x86_64-smp-mpicxx
CHARMC=$(CHARMDIR)/bin/charmc $(OPTS)

all: aobench.out

aobench.out: main.o aobench_tile.o scene.o
	$(CHARMC) -optimize -module CommonLBs -balance workstealing -language charm++ -c++-option -std=c++11 $^ -o $@

aobench_tile.o: aobench_tile.cpp aobench_tile.h aobench_tile.decl.h aobench_tile.def.h main.decl.h scene.h
	$(CHARMC) -balance workstealing -optimize -c++-option -std=c++11 $< -o $@

main.o: main.cpp main.h main.decl.h main.def.h aobench_tile.decl.h
	$(CHARMC) -balance workstealing -optimize -c++-option -std=c++11 $< -o $@

# The Charm++ generated code seems to produce a lot of warnings at this high a warning level,
# makes it kind of hard to find warnings about my code.
#$(CHARMC) -c++-option -std=c++11 -c++-option -Wall -c++-option -Wextra -c++-option -pedantic $< -o $@
%.o: %.cpp %.h %.decl.h %.def.h
	$(CHARMC) -balance workstealing -optimize -c++-option -std=c++11 $< -o $@

%.o: %.cpp %.h
	$(CHARMC) -balance workstealing -optimize -c++-option -std=c++11 $< -o $@

%.decl.h %.def.h: %.ci
	$(CHARMC) -optimize $^

# Cancel Make's default cpp compilation rule
%.o: %.cpp

.PHONY: clean
clean:
	rm -f *.decl.h *.def.h *.o hello charmrun

