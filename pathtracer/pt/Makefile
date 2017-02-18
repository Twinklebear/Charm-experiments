GLM_DIR?=/usr/include/
CXXFLAGS=-Wall -Wextra -pedantic -O3 -std=c++11 -DGLM_FORCE_RADIANS -I$(GLM_DIR)

SRCS=ray.cpp camera.cpp sphere.cpp stb_image.cpp hit_integrator.cpp \
	 normal_integrator.cpp whitted_integrator.cpp scene.cpp \
	 light.cpp
HEADERS=$(wildcard *.h)
OBJS=$(SRCS:.cpp=.o)

all: libpt.a

libpt.a: $(OBJS)
	ar rcs $@ $^

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.a

