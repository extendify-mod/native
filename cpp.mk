CXXSRC=$(shell find src -name '*.cpp')

%.o: %.cpp
	@mkdir -p $(shell dirname $(patsubst src/%, dist/%, $@))
ifeq ($(OS),Windows_NT)
		$(CXX) -MJ $(patsubst src/%, dist/%, $(<:.cpp=.json)) -c $(FLAGS) $(CXXFLAGS) $< -o $(patsubst src/%, dist/%, $@)
else
		$(CXX) -c $(FLAGS) $(CXXFLAGS) $< -o $(patsubst src/%, dist/%, $@)
endif

all: $(CXXSRC:.cpp=.o)
