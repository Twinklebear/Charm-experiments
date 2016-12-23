CXXFLAGS=-Wall -Wextra -pedantic -O3 -std=c++11
LIBSCIVIS_SRCS=read_raw_volume.cpp
LIBSCIVIS_OBJS=$(LIBSCIVIS_SRCS:.cpp=.o)

all: libscivis.a

libscivis.a: $(LIBSCIVIS_OBJS)
	ar rcs $@ $^

.PHONY: clean
clean:
	rm -f *.o *.a

