CC     = gcc
TARGET = vehicle_ecu
SRCS   = src/main.c src/input.c src/mode.c src/control.c \
         src/fault.c src/state.c src/log.c
OBJS   = $(SRCS:.c=.o)

CFLAGS = -Wall -Wextra -Werror -Wpedantic -std=c11 \
         -Iinclude                                  \
         -Wshadow -Wundef -Wcast-qual               \
         -Wstrict-prototypes -Wmissing-prototypes

.PHONY: all clean run run_interactive

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo ""
	@echo "  Build OK: ./$(TARGET)"
	@echo "  Run:  ./$(TARGET)      (scenario mode)"
	@echo "  Run:  ./$(TARGET) -i   (interactive mode)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

run_interactive: all
	./$(TARGET) -i

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "  Clean done"
