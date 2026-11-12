CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = cinema.exe
SRCS = main.c file_io.c user_auth.c movies.c booking.c admin.c encryption.c payment.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c headers.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	del /Q *.o $(TARGET) 2>nul || true
