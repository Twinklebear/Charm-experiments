CHARMDIR?=~/libs/charm/mpi-linux-x86_64-smp-mpicxx
CHARMC=$(CHARMDIR)/bin/charmc $(OPTS)

all: mandelbrot.out

mandelbrot.out: main.o mandel_tile.o
	$(CHARMC) -optimize -module CommonLBs -balance workstealing -language charm++ -c++-option -std=c++14 $^ -o $@

mandel_tile.o: mandel_tile.cpp mandel_tile.h mandel_tile.decl.h mandel_tile.def.h main.decl.h
	$(CHARMC) -balance workstealing -optimize -c++-option -std=c++14 $< -o $@

main.o: main.cpp main.h main.decl.h main.def.h mandel_tile.decl.h
	$(CHARMC) -balance workstealing -optimize -c++-option -std=c++14 $< -o $@

# The Charm++ generated code seems to produce a lot of warnings at this high a warning level,
# makes it kind of hard to find warnings about my code.
#$(CHARMC) -c++-option -std=c++14 -c++-option -Wall -c++-option -Wextra -c++-option -pedantic $< -o $@
%.o: %.cpp %.h %.decl.h %.def.h
	$(CHARMC) -balance workstealing -optimize -c++-option -std=c++14 $< -o $@

%.decl.h %.def.h: %.ci
	$(CHARMC) -optimize $^

# Cancel Make's default cpp compilation rule
%.o: %.cpp

.PHONY: clean
clean:
	rm -f *.decl.h *.def.h *.o hello charmrun

