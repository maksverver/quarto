CXXFLAGS=-march=native -Wall -Wextra -Wno-sign-compare -O3 -g -std=c++17

OBJS=quarto.o ai_mcts.o main.o

all: quarto

quarto.o: quarto.cc quarto.h
	$(CXX) $(CXXFLAGS) -c -o $@ quarto.cc

ai_mcts.o: ai_mcts.cc ai_mcts.h ai.h quarto.h
	$(CXX) $(CXXFLAGS) -c -o $@ ai_mcts.cc

main.o: main.cc quarto.h ai.h
	$(CXX) $(CXXFLAGS) -c -o $@ main.cc

quarto: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm -f $(OBJS) quarto

.PHONY: all clean
