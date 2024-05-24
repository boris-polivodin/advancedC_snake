#include <ncurses.h>
#include <stdlib.h>


#define MAX_X 20
#define MAX_Y 20
#define START_X 10
#define START_Y 10

enum {LEFT = 1, RIGHT, UP, DOWN, STOP_GAME = KEY_END};
enum {MAX_TAIL_SIZE = 100, START_TAIL_SIZE = 6};

struct control_buttons {
	int down;
	int up;
	int right;
	int left;
} control_buttons;

struct control_buttons default_controls = {KEY_DOWN, KEY_UP, KEY_RIGHT, KEY_LEFT};

typedef struct tail_t {
	int x;
	int y;
} tail_t;

typedef struct snake_t {
	int x;
	int y;
	int direction;
	size_t tsize;
	struct tail_t *tail;
	struct control_buttons controls;
} snake_t;

void initTail(struct tail_t t[], size_t size, int x, int y) {
	for (size_t i = 0; i < size; ++i) {
		t[i].x = x + i + 1;
		t[i].y = y;
	}
}

void initHead(struct snake_t *head, int x, int y) {
	head->x = x;
	head->y = y;
	head->direction = LEFT;
}

void initSnake(struct snake_t *head, size_t size, int x, int y) {
	tail_t* tail = (tail_t*) malloc(MAX_TAIL_SIZE * sizeof(tail_t));
	initTail(tail, size, x, y);
	initHead(head, x, y);
	head->tail = tail;
	head->tsize = size;
	head->controls = default_controls;
}

void move_snake(struct snake_t* snake) {
	char ch = '@', ct = '+';
	size_t size = snake->tsize;
	int temp_x, x = snake->x, temp_y, y = snake->y;
	switch (snake->direction) {
		case LEFT:
			if(snake->x <= 0)
				snake->x = MAX_X;
			mvprintw(snake->y, --(snake->x), "%c", ch);
			break;
		case RIGHT:
			if (snake->x >= MAX_X)
				snake->x = 0;
			mvprintw(snake->y, ++(snake->x), "%c", ch);
			break;
		case UP:
			if(snake->y <= 0)
				snake->y = MAX_Y;
			mvprintw(--(snake->y), snake->x, "%c", ch);
			break;
		case DOWN:
			if(snake->y >= MAX_Y)
				snake->y = 0;
			mvprintw(++(snake->y), snake->x, "%c", ch);
			break;
		default:
			break;
	}

	tail_t* tail = snake->tail;
	for (size_t i = 0; i < size; ++i) {
		temp_x = tail[i].x;
		temp_y = tail[i].y;

		tail[i].x = x;
		tail[i].y = y;

		mvprintw(y, x, "%c", ct);

		x = temp_x;
		y = temp_y;
	}
	mvprintw(y, x, " ");
}

void changeDirection(snake_t *snake, const int32_t key) {
	if (key == snake->controls.down)
		snake->direction = DOWN;
	else if (key == snake->controls.up)
		snake->direction = UP;
	else if (key == snake->controls.right)
		snake->direction = RIGHT;
	else if (key == snake->controls.left)
		snake->direction = LEFT;
}

int checkDirection(snake_t *snake, int32_t key) {
	if (key == snake->controls.down && snake->direction == UP) {
		return 0;
	} else if (key == snake->controls.up && snake->direction == DOWN) {
		return 0;
	} else if (key == snake->controls.right && snake->direction == LEFT) {
		return 0;
	} else if (key == snake->controls.left && snake->direction == RIGHT) {
		return 0;
	} else {
		return 1;
	}
}

int main() {
	initscr();
	int key_pressed;
	raw();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(0);
	snake_t* snake = (snake_t*) malloc(sizeof(snake_t));
	initSnake(snake, START_TAIL_SIZE, START_X, START_Y);
	move_snake(snake);
	while ((key_pressed = getch()) != STOP_GAME) {
		move(0, 0);
		if (checkDirection(snake, key_pressed)) {
			changeDirection(snake, key_pressed);
			move_snake(snake);
		}

	}
	refresh();
	getch();
	free(snake->tail);
	free(snake);
	endwin();
	return 0;
}
