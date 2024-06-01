#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

#define MAX_X 25
#define MAX_Y 25
#define START_X 15
#define START_Y 15
#define MY_SNAKE_PAIR 1
#define COMP_SNAKE_PAIR 2
#define EAT_PAIR 3
#define CODE_KEY_END 27
#define CODE_0 48

enum {LEFT = 1, RIGHT, UP, DOWN, STOP_GAME = KEY_END, PAUSE_PLAY = 'p'};
enum {MAX_TAIL_SIZE = MAX_X - 2, START_TAIL_SIZE = 2, SPEED_LEVEL = 5
	, MAX_FOOD_SIZE = 5, FOOD_EXPIRE_SECONDS = 15};

char *START_MENU = {"\nMenu:\n"
		"KEY_END - exiting the game\n"
		"p - pause in game\n"
		"Choice speed mode:\n"
		"1 - beginner\n"
		"2 - advanced\n"
		"3 - very hard\n"};

char *DESCRIPTION = {"Game Snake\n"
		"Your snake is red.\n"
		"To set the direction of movement, press the keys UP, DOWN, RIGHT and LEFT.\n"
		"To choice speed mode, press the keys 1, 2 or 3.\n"
		"To exit the game, press END key.\n"};


int showStartMenu() {
	int ret;
	mvprintw(0, 0, "%s", DESCRIPTION);
	mvprintw(5, 0, "%s", START_MENU);
	ret = getch();
	return ret;
}

struct control_buttons {
	int down;
	int up;
	int right;
	int left;
} control_buttons;

struct control_buttons default_controls = {KEY_DOWN, KEY_UP, KEY_RIGHT, KEY_LEFT};

typedef struct tail_t {
	uint8_t x;
	uint8_t y;
} tail_t;

typedef struct snake_t {
	uint8_t x;
	uint8_t y;
	uint8_t direction;
	size_t tsize;
	struct tail_t *tail;
	struct control_buttons controls;
} snake_t;

typedef struct food_t {
	uint8_t x;
	uint8_t y;
    time_t put_time;
    char point;
    uint8_t enable;
} food_t;

void initTail(struct tail_t t[], size_t size, uint8_t x, uint8_t y) {
	for (size_t i = 0; i < size; ++i) {
		t[i].x = x + i + 1;
		t[i].y = y;
	}
}

void initHead(struct snake_t *head, uint8_t x, uint8_t y) {
	head->x = x;
	head->y = y;
	head->direction = LEFT;
}

void initSnake(struct snake_t *head, size_t size, uint8_t x, uint8_t y) {
	tail_t* tail = (tail_t*) malloc(MAX_TAIL_SIZE * sizeof(tail_t));
	initTail(tail, size, x, y);
	initHead(head, x, y);
	head->tail = tail;
	head->tsize = size;
	head->controls = default_controls;
}

void initFood(struct food_t f[], size_t size) {
    food_t init = {0,0,0,0,0};
    for(size_t i=0; i<size; i++) {
        f[i] = init;
    }
}

/*
Обновить/разместить текущее зерно на поле
*/
void putFoodSeed(struct food_t *fp) {
//    int max_x=0, max_y=0;
//    getmaxyx(stdscr, max_y, max_x);
	char spoint[2] = {0};
	attron(COLOR_PAIR(EAT_PAIR));
    mvprintw(fp->y, fp->x, " ");
    fp->x = rand() % (MAX_X - 1);
    fp->y = rand() % (MAX_Y - 2) + 1; //Не занимаем верхнюю строку
    fp->put_time = time(NULL);
    fp->point = '$';
    fp->enable = 1;
    spoint[0] = fp->point;
    mvprintw(fp->y, fp->x, "%s", spoint);
    attroff(COLOR_PAIR(EAT_PAIR));
}

/*
Разместить еду на поле
*/
void putFood(struct food_t f[], size_t number_seeds) {
    for(size_t i = 0; i < number_seeds; i++) {
        putFoodSeed(&f[i]);
    }
}

void refreshFood(struct food_t f[], int nfood) {
    for(size_t i = 0; i < nfood; i++) {
        if( f[i].put_time ) {
            if( !f[i].enable || (time(NULL) - f[i].put_time) > FOOD_EXPIRE_SECONDS ) {
                putFoodSeed(&f[i]);
            }
        }
    }
}

_Bool haveEat(struct snake_t *head, struct food_t f[]) {
    for(size_t i = 0; i < MAX_FOOD_SIZE; i++) {
        if( f[i].enable && head->x == f[i].x && head->y == f[i].y ) {
            f[i].enable = 0;
            return 1;
        }
    }
    return 0;
}

/*
Увеличение хвоста на 1 элемент
*/
_Bool addTail(struct snake_t *head) {
    if(head == NULL || head->tsize >= MAX_TAIL_SIZE) {
        mvprintw(0, 0, "Can't add tail");
        return 0;
    }
    head->tsize++;
    return 1;
}

void addSpeed(struct snake_t *snake, uint8_t max_speed, uint8_t *current_speed, uint8_t mode_speed) {
	if (snake->tsize % SPEED_LEVEL == 0 && max_speed - *current_speed + mode_speed > 1) {
		nocbreak();
		(*current_speed)++;
		halfdelay(max_speed - *current_speed + mode_speed);
	}
}

void move_snake(struct snake_t* snake, int pair) {
	char ch = '@', ct = '+';
	size_t size = snake->tsize;
	uint8_t temp_x, x = snake->x, temp_y, y = snake->y;
	attron(COLOR_PAIR(pair));
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
	attroff(COLOR_PAIR(pair));
}

void autoMotionShake(snake_t *snake, struct food_t f[], int nfood, int pair) {
	// find nearer food
	size_t index = 0;
	uint32_t temp, min_distance = (f[0].x - snake->x)*(f[0].x - snake->x) + (f[0].y - snake->y)*(f[0].y - snake->y);
	for(size_t i = 1; i < nfood; i++) {
		temp = (f[i].x - snake->x)*(f[i].x - snake->x) + (f[i].y - snake->y)*(f[i].y - snake->y);
		if (min_distance > temp) {
			min_distance = temp;
			index = i;
		}
	}
	// done step and return
	if (f[index].y == snake->y && f[index].x < snake->x) {
		snake->direction = LEFT;
		move_snake(snake, pair);
		return;
	} else if (f[index].y == snake->y && f[index].x > snake->x) {
		snake->direction = RIGHT;
		move_snake(snake, pair);
		return;
	} else if (f[index].x == snake->x && f[index].y < snake->y) {
		snake->direction = UP;
		move_snake(snake, pair);
		return;
	} else if (f[index].x == snake->x && f[index].y > snake->y) {
		snake->direction = DOWN;
		move_snake(snake, pair);
		return;
	}
	// choice direction when snake reverses
	// then done step and return
	switch (snake->direction) {
		case UP:
			if (f[index].y > snake->y && f[index].x < snake->x) {
				snake->direction = LEFT;
				move_snake(snake, pair);
				snake->direction = DOWN;
				return;
			} else if (f[index].y > snake->y && f[index].x > snake->x) {
				snake->direction = RIGHT;
				move_snake(snake, pair);
				snake->direction = DOWN;
				return;
			}
			break;
		case DOWN:
			if (f[index].y < snake->y && f[index].x < snake->x) {
				snake->direction = LEFT;
				move_snake(snake, pair);
				snake->direction = UP;
				return;
			} else if (f[index].y < snake->y && f[index].x > snake->x) {
				snake->direction = RIGHT;
				move_snake(snake, pair);
				snake->direction = UP;
				return;
			}
			break;
		case RIGHT:
			if (f[index].x < snake->x && f[index].y < snake->y) {
				snake->direction = UP;
				move_snake(snake, pair);
				snake->direction = LEFT;
				return;
			} else if (f[index].x < snake->x && f[index].y > snake->y) {
				snake->direction = DOWN;
				move_snake(snake, pair);
				snake->direction = LEFT;
				return;
			}
			break;
		case LEFT:
			if (f[index].x > snake->x && f[index].y < snake->y) {
				snake->direction = UP;
				move_snake(snake, pair);
				snake->direction = RIGHT;
				return;
			} else if (f[index].x > snake->x && f[index].y > snake->y) {
				snake->direction = DOWN;
				move_snake(snake, pair);
				snake->direction = RIGHT;
				return;
			}
			break;
		default:
			break;
	}
	// if nothing worked
	move_snake(snake, pair);
}

uint8_t isCrush(snake_t *snake) {
	tail_t *tail = snake->tail;
	size_t size = snake->tsize;
	uint8_t x = snake->x;
	uint8_t y = snake->y;
	for(size_t i = 0; i < size; i++) {
		if (x == tail[i].x && y == tail[i].y) {
			return 1;
		}
	}
	return 0;
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

void makePause() {
	mvprintw(MAX_Y + 1, 0, "For continue game press \'p\'");
	int button;
	while ((button = getch()) != PAUSE_PLAY) {}
	mvprintw(MAX_Y + 1, 0, "                           ");
}

int main() {
	initscr();
	if (has_colors()) {
		start_color();
		init_pair(MY_SNAKE_PAIR, COLOR_RED, COLOR_BLACK);
		init_pair(COMP_SNAKE_PAIR, COLOR_YELLOW, COLOR_BLACK);
		init_pair(EAT_PAIR, COLOR_GREEN, COLOR_BLACK);
	} else {
		endwin();
		exit(1);
	}

	int res = showStartMenu();
	if (res == CODE_KEY_END) {
		endwin();
		return 0;
	}
	clear();

	int key_pressed;
	uint8_t max_speed = 6, current_speed = 1, mode_speed = 0;
	if (res - CODE_0 == 1) {
		mode_speed = 2;
	} else if (res - CODE_0 == 2) {
		mode_speed = 1;
	}
	raw();
	keypad(stdscr, TRUE);   //Включаем режим чтения функциональных клавиш
	noecho();               //Выключаем отображение вводимых символов, нужно для getch()
	halfdelay(max_speed - current_speed);  //Устанавливаем ограничение по времени ожидания getch() в 0.5 сек
	curs_set(0);
	snake_t* snake = (snake_t*) malloc(sizeof(snake_t));
	initSnake(snake, START_TAIL_SIZE, START_X, START_Y);

	snake_t* snake2 = (snake_t*) malloc(sizeof(snake_t));
	initSnake(snake2, START_TAIL_SIZE, START_X + 3, START_Y - 1);

	food_t* food = (food_t*) malloc(MAX_FOOD_SIZE * sizeof(food_t));
	initFood(food, MAX_FOOD_SIZE);

	time_t ltime = time(NULL);
	srand((unsigned) ltime / 2);
	putFood(food, MAX_FOOD_SIZE);


	time_t start_time = time(NULL);
	move_snake(snake, MY_SNAKE_PAIR);
	autoMotionShake(snake2, food, MAX_FOOD_SIZE, COMP_SNAKE_PAIR);
	mvprintw(MAX_Y + 1, 0, "Speed level = %d", current_speed);
	while ((key_pressed = getch()) != STOP_GAME) {
		move(0, 0);
		if (key_pressed == PAUSE_PLAY) {
			makePause();
			mvprintw(MAX_Y + 1, 0, "Speed level = %d", current_speed);
			continue;
		}
		if (checkDirection(snake, key_pressed)) {
			changeDirection(snake, key_pressed);
			move_snake(snake, MY_SNAKE_PAIR);
//			if (isCrush(snake)) {
//				for(size_t i = START_TAIL_SIZE; i < snake->tsize; i++) {
//					mvprintw(snake->tail[i].y, snake->tail[i].x, " ");
//				}
//				snake->tsize = START_TAIL_SIZE;
//			}
			if (haveEat(snake, food)) {
				if (!addTail(snake)) {
					break;
				}
				addSpeed(snake, max_speed, &current_speed, mode_speed);
			}
		}
		autoMotionShake(snake2, food, MAX_FOOD_SIZE, COMP_SNAKE_PAIR);
//		if (isCrush(snake2)) {
//			for (size_t i = START_TAIL_SIZE; i < snake2->tsize; i++) {
//				mvprintw(snake2->tail[i].y, snake2->tail[i].x, " ");
//			}
//			snake2->tsize = START_TAIL_SIZE;
//		}
		if (haveEat(snake2, food)) {
			if (!addTail(snake2)) {
				break;
			}
			if (!mode_speed) {
				addSpeed(snake2, max_speed, &current_speed, mode_speed);
			}
		}
		refreshFood(food, MAX_FOOD_SIZE);
		mvprintw(MAX_Y + 1, 0, "Speed level = %d", current_speed);
	}
	mvprintw(1, 0, "Playing time = %ld", time(NULL) - start_time);
	mvprintw(2, 0, "Amount of food eaten by you= %lu", snake->tsize - START_TAIL_SIZE);
	mvprintw(3, 0, "Amount of food eaten by snake2 = %lu", snake2->tsize - START_TAIL_SIZE);
	refresh();
	nocbreak();
	getch();
	free(snake->tail);
	free(snake);
	free(food);
	endwin();
	return 0;
}
