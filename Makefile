CXXFLAGS=-march=native -Wall -Wextra -Wno-sign-compare -O3 -g -std=c++17

OBJS=quarto.o main.o

all: quarto

quarto.o: quarto.cc quarto.h
	$(CXX) $(CXXFLAGS) -c -o $@ quarto.cc

main.o: main.cc quarto.h
	$(CXX) $(CXXFLAGS) -c -o $@ main.cc

quarto: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm -f $(OBJS) quarto

.PHONY: all clean
