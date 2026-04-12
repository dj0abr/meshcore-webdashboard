# Makefile - build meshcore_api from all .cpp files in this directory

CXX      := g++
TARGET   := meshcore_api

CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic -pthread -Wno-psabi
LDFLAGS  := -pthread -lmariadb -lssl -lcrypto -lcurl -ltinyxml2

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Compile .cpp -> .o and generate dependency .d files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)