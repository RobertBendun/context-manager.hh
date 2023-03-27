all: example

%: %.cc *.hh
	g++ -std=c++20 -Wall -Wextra -o $@ $< -ggdb3

.PHONY: all
