# Host build: the renderer (docs/screenshots) and the unit tests, from the same common/ sources the console builds use.
CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter
OUT := ../build/host
COMMON := $(wildcard ../common/*.cpp)
OBJS := $(patsubst ../common/%.cpp,$(OUT)/obj/%.o,$(COMMON))
HEADERS := $(wildcard ../common/*.h)

all: $(OUT)/render $(OUT)/tests

$(OUT)/obj/%.o: ../common/%.cpp $(HEADERS)
	@mkdir -p $(OUT)/obj
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT)/render: render.cpp png.h $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ render.cpp $(OBJS)

$(OUT)/tests: tests.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ tests.cpp $(OBJS)

clean:
	rm -rf $(OUT)
.PHONY: all clean
