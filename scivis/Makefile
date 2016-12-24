CHARMDIR?=~/libs/charm/multicore-linux64
GLM_DIR?=/usr/include/
CHARMC=$(CHARMDIR)/bin/charmc $(OPTS)

CHARM_SRCS=main.cpp
CHARM_OBJS=$(CHARM_SRCS:.cpp=.o)
CHARM_DECLS=$(CHARM_SRCS:.cpp=.decl.h)

all: scivis.out

scivis.out: $(CHARM_OBJS) sv/libscivis.a
	$(CHARMC) -optimize -module CommonLBs -balance workstealing -language charm++ \
		-c++-option -std=c++11 $^ -o $@ -Lsv/ -lscivis

sv/libscivis.a: $(wildcard sv/*.cpp sv/*.h)
	$(MAKE) -C sv all

# The Charm++ generated code seems to produce a lot of warnings at this high a warning level,
# makes it kind of hard to find warnings about my code.
#$(CHARMC) -c++-option -std=c++11 -c++-option -Wall -c++-option -Wextra -c++-option -pedantic $< -o $@
%.o: %.cpp %.h %.def.h $(CHARM_DECLS)
	$(CHARMC) -balance workstealing -optimize -c++-option -std=c++11 -c++-option -I$(GLM_DIR) $< -o $@

%.decl.h %.def.h: %.ci
	$(CHARMC) -optimize $^

# Cancel Make's default cpp compilation rule
%.o: %.cpp

.PHONY: clean
clean:
	$(MAKE) -C sv clean
	rm -f *.decl.h *.def.h *.o *.a scivis.out charmrun

