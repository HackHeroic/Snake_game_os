CC = gcc
CFLAGS = -Wall -Wextra -Werror -Isrc

# Source files
LIB_SRC = src/lib/math.c src/lib/string.c src/lib/memory.c src/lib/screen.c src/lib/keyboard.c
GAME_SRC = src/game/snake.c src/game/food.c src/game/board.c src/game/score.c src/game/obstacles.c src/game/stats.c src/game/ghost.c src/game/speed_zones.c src/game/replay.c
UI_SRC = src/ui/renderer.c src/ui/screens.c src/ui/themes.c
MAIN_SRC = src/main.c

ALL_SRC = $(LIB_SRC) $(GAME_SRC) $(UI_SRC) $(MAIN_SRC)
ALL_OBJ = $(ALL_SRC:.c=.o)

# Test files
TEST_MATH = tests/test_math
TEST_STRING = tests/test_string
TEST_MEMORY = tests/test_memory

.PHONY: all clean run test

all: snake

snake: $(ALL_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_MATH): tests/test_math.c src/lib/math.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_STRING): tests/test_string.c src/lib/string.c
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_MEMORY): tests/test_memory.c src/lib/memory.c src/lib/math.c
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_MATH) $(TEST_STRING) $(TEST_MEMORY)
	@echo "=== Math Tests ==="
	@./$(TEST_MATH)
	@echo "\n=== String Tests ==="
	@./$(TEST_STRING)
	@echo "\n=== Memory Tests ==="
	@./$(TEST_MEMORY)

run: snake
	./snake

clean:
	rm -f $(ALL_OBJ) snake $(TEST_MATH) $(TEST_STRING) $(TEST_MEMORY)
