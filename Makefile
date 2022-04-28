code:text.cpp
	g++ -o $@ $^ -std=c++11  -pthread 

.PHONY:clean
clean:
	rm code
