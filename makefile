all: wiki

wiki: WikiSort.cpp WikiSort.c WikiSort.java
	g++ WikiSort.cpp -o gen/WikiSort_cpp.x
	gcc WikiSort.c -lm -o gen/WikiSort_c.x
	javac -d gen WikiSort.java 

java: gen/WikiSort.class
	java -classpath gen WikiSort

gen/WikiSort.class:
	javac -d gen WikiSort.java

c: gen/WikiSort_c.x
	gen/WikiSort_c.x

gen/WikiSort_c.x: WikiSort.c
	gcc WikiSort.c -lm -o gen/WikiSort_c.x

cpp: gen/WikiSort_cpp.x
	gen/WikiSort_cpp.x

gen/WikiSort_cpp.x: WikiSort.cpp
	g++ WikiSort.cpp -o gen/WikiSort_cpp.x

clean:
	rm -rf gen/*