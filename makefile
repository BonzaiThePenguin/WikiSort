CPP = g++
GCC = gcc
JAVAC = javac
JAVA = java

GEN = gen

CPPFILE = WikiSort.cpp
CFILE = WikiSort.c
JAVAFILE = WikiSort.java
JAVACLASS = WikiSort.class

CPPX = $(GEN)/WikiSort_cpp.x
CX = $(GEN)/WikiSort_c.x
JAVAX = WikiSort

CPPFLAGS = -o $(CPPX)
CFLAGS = -lm -o $(CX)
JAVACFLAGS = -d gen
JAVAFLAGS = -classpath $(GEN)

all: wiki

wiki: $(CPPFILE) $(CFILE) $(JAVAFILE)
	$(CPP) $(CPPFLAGS) $(CPPFILE)
	$(GCC) $(CFLAGS) $(CFILE)
	$(JAVAC) $(JAVACFLAGS) $(JAVAFILE)

java: $(JAVACLASS)
	$(JAVA) $(JAVAFLAGS) $(JAVAX)

$(JAVACLASS): $(JAVAFILE)
	$(JAVAC) $(JAVACFLAGS) $(JAVAFILE)

c: $(CX)
	$(CX)

$(CX): $(CFILE)
	$(GCC) $(CFILE) $(CFLAGS)

cpp: $(CPPX)
	$(CPPX)

$(CPPX): $(CPPFILE)
	$(CPP) $(CPPFLAGS) $(CPPFILE)

clean:
	rm -rf $(GEN)/*