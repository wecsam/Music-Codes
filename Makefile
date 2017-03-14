CC=g++
CFLAGS=-Wall -Werror -std=c++11 -g -fvar-tracking
PARTS=\
	MidiReader\
	Note\

%.o: %.cpp $(foreach part, $(PARTS), $(part).h)
	$(CC) $< -c -o $@ $(CFLAGS)

halfsteps: $(foreach part, $(PARTS), $(part).o) halfsteps.o
	$(CC) $(foreach part, $(PARTS), $(part).o) halfsteps.o -o halfsteps $(CFLAGS)
