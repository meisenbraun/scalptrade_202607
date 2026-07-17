TARGET = scalptrade_202607
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = $(wildcard *.h)
# debug config
#CPPFLAGS = -g -O0 -Wall -std=c++14 # C++ 14 per the exercise instructions
# release config
CPPFLAGS = -O3 -march=native -DNDEBUG -Wall -std=c++14

CPP = g++

.PHONY: default all clean
default: $(TARGET)

all: default

%.o: %.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CPP) $(CPPFLAGS) -o $(TARGET) $(OBJECTS)

clean:
	-rm -f $(OBJECTS) $(TARGET)