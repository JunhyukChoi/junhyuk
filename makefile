OBJECTS = twoultra.o
SRCS = $(OBJECTS:.o=.c)

CFLAGS = -g
TARGET = twoultra

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) 

clean :
	rm -f $(OBJECTS) $(TARGET) core

twoultra.o : twoultra_v0.c

