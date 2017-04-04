CHARMDIR?=~/libs/charm/multicore-linux64
GLM_DIR?=/usr/include/
CHARMC=$(CHARMDIR)/bin/charmc -cc $(CC) -c++ $(CXX) $(OPTS)

CHARM_SRCS=main.cpp image_parallel_tile.cpp data_parallel.cpp
CHARM_OBJS=$(CHARM_SRCS:.cpp=.o)
CHARM_DECLS=$(CHARM_SRCS:.cpp=.decl.h)

all: pathtracer.out

pathtracer.out: $(CHARM_OBJS) pt/libpt.a
	$(CHARMC) -optimize -module CommonLBs -balance workstealing -language charm++ \
		-c++-option -std=c++11 -c++-option -DGLM_FORCE_RADIANS $^ -o $@ -Lpt/ -lpt

pt/libpt.a: $(wildcard pt/*.cpp pt/*.h)
	$(MAKE) -C pt all

# The Charm++ generated code seems to produce a lot of warnings at this high a warning level,
# makes it kind of hard to find warnings about my code.
#$(CHARMC) -c++-option -std=c++11 -c++-option -Wall -c++-option -Wextra -c++-option -pedantic $< -o $@
%.o: %.cpp %.h %.def.h $(CHARM_DECLS)
	$(CHARMC) -balance workstealing -optimize -c++-option -std=c++11 \
		-c++-option -DGLM_FORCE_RADIANS -c++-option -I$(GLM_DIR) $< -o $@

%.decl.h %.def.h: %.ci
	$(CHARMC) -optimize $^

# Cancel Make's default cpp compilation rule
%.o: %.cpp

.PHONY: clean
clean:
	$(MAKE) -C pt clean
	rm -f *.decl.h *.def.h *.o *.a pathtracer.out charmrun

