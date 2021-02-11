CC     = gcc
CFLAGS = -I./ -g -Wall -Werror
INCS   = infrared.h
LIBS   = -lwiringPi
TARGET = IR_RECV
OBJS   = \
	recv.o \
	infrared.o \

%.o: %.c $(INCS)
	$(CC) $(CFLAGS) -c $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm $(TARGET) *.o
