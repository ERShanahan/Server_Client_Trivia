CXX      = gcc
CXX_FILE = $(wildcard *.c)
TARGET   = $(patsubst %.c,%,$(CXX_FILE))
CXXFLAGS = -g -std=gnu99 -Wall -Werror -pedantic-errors -fmessage-length=0 -I../driver

all: $(TARGET)

$(TARGET):
	$(CXX) $(CXXFLAGS) $@.c -o $@

clean:
	rm -f $(TARGET) $(TARGET).exe *.o *~ core
