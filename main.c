#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

#define MAX_X 20
#define MAX_Y 20
#define START_X 10
#define START_Y 10

enum {LEFT = 1, RIGHT, UP, DOWN, STOP_GAME = KEY_END};
enum {MAX_TAIL_SIZE = MAX_X - 2, START_TAIL_SIZE = 2, SPEED_LEVEL = 4
	, MAX_FOOD_SIZE = 5, FOOD_EXPIRE_SECONDS = 15};

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

typedef struct food_t {
    int x;
    int y;
    time_t put_time;
    char point;
    uint8_t enable;
} food_t;

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
    mvprintw(fp->y, fp->x, " ");
    fp->x = rand() % (MAX_X - 1);
    fp->y = rand() % (MAX_Y - 2) + 1; //Не занимаем верхнюю строку
    fp->put_time = time(NULL);
    fp->point = '$';
    fp->enable = 1;
    spoint[0] = fp->point;
    mvprintw(fp->y, fp->x, "%s", spoint);
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
	uint8_t max_speed = 6, current_speed = 1;
	raw();
	keypad(stdscr, TRUE);   //Включаем режим чтения функциональных клавиш
	noecho();               //Выключаем отображение вводимых символов, нужно для getch()
	halfdelay(max_speed - current_speed);           //Устанавливаем ограничение по времени ожидания getch() в 0.3 сек
	curs_set(0);
	snake_t* snake = (snake_t*) malloc(sizeof(snake_t));
	initSnake(snake, START_TAIL_SIZE, START_X, START_Y);

	food_t* food = (food_t*) malloc(MAX_FOOD_SIZE * sizeof(food_t));
	initFood(food, MAX_FOOD_SIZE);
	putFood(food, MAX_FOOD_SIZE);

	time_t start_time = time(NULL);
	move_snake(snake);
	while ((key_pressed = getch()) != STOP_GAME) {
		move(0, 0);
		if (checkDirection(snake, key_pressed)) {
			changeDirection(snake, key_pressed);
			move_snake(snake);
			if (haveEat(snake, food)) {
				if (!addTail(snake)) {
					break;
				}
				if (snake->tsize % SPEED_LEVEL == 0 && max_speed - current_speed > 1) {
					nocbreak();
					current_speed++;
					halfdelay(max_speed - current_speed);
					mvprintw(MAX_Y + 1, 0, "Speed level = %d", current_speed);
				}
			}
		}
		refreshFood(food, MAX_FOOD_SIZE);
	}
	mvprintw(1, 0, "Playing time = %ld", time(NULL) - start_time);
	mvprintw(2, 0, "Amount of food eaten = %lu", snake->tsize - START_TAIL_SIZE);
	refresh();
	nocbreak();
	getch();
	free(snake->tail);
	free(snake);
	free(food);
	endwin();
	return 0;
}
