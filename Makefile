bin=HttpServer
cc=g++
arg=text.cpp
$(bin):$(arg)
	@$(cc) -o $@ $^ -std=c++11  -pthread 

.PHONY:clean
clean:
	@rm $(bin)
