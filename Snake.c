#include <SDL2/SDL.h>
#include <SDL2//SDL_ttf.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

#define MAP_SIZE 27 //地图大小
#define W ((MAP_SIZE)*20+1)
#define H ((MAP_SIZE)*20+1+100)
#define FONT_SIZE 72
#define FRAME 75

SDL_Texture *texture = NULL;
SDL_Surface *surface = NULL;
SDL_Window *window; //窗口
SDL_Renderer *renderer;//渲染
TTF_Font *font;//字体

int map[MAP_SIZE + 1][MAP_SIZE + 1];

typedef struct SnakeNode {
    int x;//在map上坐标 head中方向
    int y;//在map上坐标 head中数量
    struct SnakeNode *next;
    struct SnakeNode *front;
} SnakeNode;
SnakeNode *head, *tail;

enum {
    UP, DOWN, LEFT, RIGHT
};

enum {
    NOTHING, SNAKE, FOOD, BARRIER
};

bool InitGame();

void Clear();

SnakeNode *HeadInsert(int x, int y);

void Move();

bool Check();

void EventControl();

void Draw();

void PrintText(char *str, int r, int g, int b, int a, int x, int y, int w, int h);

void CreateFood(int count);

void Quit();

#undef main

int main(void) {
    //加载窗口和渲染器
    if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_InitSubSystem failed: %s", SDL_GetError());
        return false;
    }
    window = SDL_CreateWindow("Snake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H,
                              SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    //加载字体
    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
        return false;
    }
    font = TTF_OpenFont("C:\\Windows\\Fonts\\CENTURY.TTF", FONT_SIZE);
    if (!font) {
        SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
        return false;
    }
    EventControl();
    Quit();
    return 1;
}

void Clear() {
    for (int i = 0; i <= MAP_SIZE; ++i) {
        for (int j = 0; j <= MAP_SIZE; ++j) {
            map[i][j] = NOTHING;
        }
    }
    if (head->next != NULL) {
        for (SnakeNode *current = head->next; current != tail;) {
            current = current->next;
            free(current->front);
        }
    }
}

bool InitGame() {
    //清空所有数据

    //小蛇
    head = (SnakeNode *) malloc(sizeof *head);
    head->x = LEFT;
    head->y = 0;
    head->next = NULL;
    head->front = NULL;
    tail = HeadInsert(MAP_SIZE / 2+1, MAP_SIZE / 2);
    HeadInsert(MAP_SIZE / 2+1, MAP_SIZE / 2 - 1);
    HeadInsert(MAP_SIZE / 2+1, MAP_SIZE / 2 - 2);
    //边界
    for (int i = 1; i <= MAP_SIZE; ++i) {
        map[i][MAP_SIZE] = BARRIER;
        map[MAP_SIZE][i] = BARRIER;
        map[i][1] = BARRIER;
        map[1][i] = BARRIER;
    }
    for (int i = 9; i <= 19 ; ++i) {
        if (13<=i && i<= 15) continue;
        map[i][19] = BARRIER;
        map[19][i] = BARRIER;
        map[i][9] = BARRIER;
        map[9][i] = BARRIER;
    }

    //食物
    srand(time(NULL));
    CreateFood(3);
    //
    return true;
}

void Quit() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void EventControl() {
    if (!InitGame()) return;
    SDL_Event event;
    Draw();
    while (true) {
        unsigned long long start = SDL_GetTicks64();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return;
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        if (head->x != DOWN) head->x = UP;
                        break;
                    case SDLK_DOWN:
                        if (head->x != UP)head->x = DOWN;
                        break;
                    case SDLK_LEFT:
                        if (head->x != RIGHT)head->x = LEFT;
                        break;
                    case SDLK_RIGHT:
                        if (head->x != LEFT) head->x = RIGHT;
                        break;
                    case SDLK_ESCAPE:
                        return;
                }
                break;
            }
        }
        Move();
        if (!Check()) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            PrintText("You lose.Press any button to replay", 0, 150, 255, 255, MAP_SIZE * 10 - 250, MAP_SIZE * 10, 500, 100);
            SDL_RenderPresent(renderer);
            while (true) {
                while (SDL_PollEvent(&event)) {
                    switch (event.type) {
                        case SDL_QUIT:
                            return;
                        case SDL_KEYDOWN:
                            Clear();
                            EventControl();
                            return;
                    }
                }
            }
        }
        Draw();
        unsigned long long end = SDL_GetTicks64();
        if (FRAME > (end - start)) SDL_Delay(FRAME - (end - start));
    }
}

void Draw() {
    //绘制网格
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (int i = 0; i < MAP_SIZE; ++i) {//横线
        SDL_RenderDrawLine(renderer, 0, i * 20, W, i * 20);
    }
    SDL_RenderDrawLine(renderer, 0, (MAP_SIZE) * 20 + 1, W, (MAP_SIZE) * 20 + 1);
    for (int i = 0; i < MAP_SIZE; ++i) {//竖线
        SDL_RenderDrawLine(renderer, i * 20, 0, i * 20, (MAP_SIZE) * 20 + 1);
    }
    SDL_RenderDrawLine(renderer, W, 0, W, H);

    //填充地图元素
    for (int i = 1; i <= MAP_SIZE; ++i) {
        for (int j = 1; j <= MAP_SIZE; ++j) {
            if (map[i][j] == SNAKE) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_Rect rect = {(j - 1) * 20 + 1, (i - 1) * 20 + 1, 19, 19};
                SDL_RenderFillRect(renderer, &rect);
            } else if (map[i][j] == FOOD) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                SDL_Rect rect = {(j - 1) * 20 + 1, (i - 1) * 20 + 1, 19, 19};
                SDL_RenderFillRect(renderer, &rect);
            } else if (map[i][j] == BARRIER) {
                SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
                SDL_Rect rect = {(j - 1) * 20 + 1, (i - 1) * 20 + 1, 19, 19};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    //显示下方信息：
    char str[10];
    PrintText("Score: ", 0, 0, 0, 255, 0, MAP_SIZE * 20 + 1, 100, 40);
    PrintText(itoa(head->y, str, 10), 0, 0, 0, 255, 100, MAP_SIZE * 20 + 4, 30, 37);
    SDL_RenderPresent(renderer);
}

SnakeNode *HeadInsert(int x, int y) {
    SnakeNode *node = (SnakeNode *) malloc(sizeof(*node));
    node->next = head->next;
    head->next = node;
    node->front = head;
    if (node->next != NULL) {
        node->next->front = node;
    }
    node->x = x;
    node->y = y;
    return node;
}

void Move() {
    switch (head->x) {
        case UP:
            HeadInsert(head->next->x - 1, head->next->y);
            break;
        case DOWN:
            HeadInsert(head->next->x + 1, head->next->y);
            break;
        case LEFT:
            HeadInsert(head->next->x, head->next->y - 1);
            break;
        case RIGHT:
            HeadInsert(head->next->x, head->next->y + 1);
            break;
    }
}

bool Check() {
    //死没死
    if (map[head->next->x][head->next->y] == SNAKE || map[head->next->x][head->next->y] == BARRIER) {
        return false;
    }
    //有没有吃的
    if (map[head->next->x][head->next->y] == FOOD) {
        head->y++;
        CreateFood(1);
    } else {
        map[tail->x][tail->y] = NOTHING;
        tail = tail->front;
        free(tail->next);
        tail->next = NULL;
    }
    map[head->next->x][head->next->y] = SNAKE;
    return true;
}

void CreateFood(int count) {
    while (count > 0) {
        int random1 = rand() % MAP_SIZE + 1, random2 = rand() % MAP_SIZE + 1;
        if (map[random1][random2] == NOTHING) {
            map[random1][random2] = FOOD;
            count--;
        }
    }
}

void PrintText(char *str, int r, int g, int b, int a, int x, int y, int w, int h) {
    SDL_Rect text;
    SDL_Color color = {r, g, b, a};
    surface = TTF_RenderUTF8_Blended(font, str, color);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    text.x = x;
    text.y = y;
    text.w = w;
    text.h = h;
    SDL_RenderCopy(renderer, texture, NULL, &text);
}