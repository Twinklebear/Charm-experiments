GLM_DIR?=/usr/include/
CXXFLAGS=-Wall -Wextra -pedantic -O3 -std=c++11 -DGLM_FORCE_RADIANS -I$(GLM_DIR)
SRCS=ray.cpp
HEADERS=$(wildcard *.h)
OBJS=$(SRCS:.cpp=.o)

all: libscivis.a

libscivis.a: $(OBJS)
	ar rcs $@ $^

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.a

