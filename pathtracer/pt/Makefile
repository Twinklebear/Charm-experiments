GLM_DIR?=/usr/include/
CXXFLAGS=-Wall -Wextra -pedantic -std=c++14 \
		 -DGLM_FORCE_RADIANS -I$(GLM_DIR) -O2 -g

SRCS=ray.cpp camera.cpp sphere.cpp stb_image.cpp hit_integrator.cpp \
	 normal_integrator.cpp whitted_integrator.cpp scene.cpp \
	 light.cpp plane.cpp brdf.cpp mc.cpp diff_geom.cpp \
	 geometry.cpp path_integrator.cpp bvh.cpp distributed_region.cpp \
	 integrator.cpp

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

