all: lab1

lab1: lab1.cpp log.cpp ppm.cpp
	g++ lab1.cpp log.cpp ppm.cpp -o lab1 -Wall -Wextra -lX11 -lm

clean:
	rm -f lab1

