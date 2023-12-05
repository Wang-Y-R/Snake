#include <SDL2/SDL.h>
#include <SDL2//SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define MAP_SIZE 27 //地图大小
int GRID_SIZE = 25;
int EDGE_SIZE = 1;
#define ACTION_BAR_HIGH (GRID_SIZE)*5

int W;//= ((MAP_SIZE)*(GRID_SIZE+EDGE_SIZE)+EDGE_SIZE);//窗口大小
int H;//= ((MAP_SIZE)*(GRID_SIZE+EDGE_SIZE)+EDGE_SIZE+ACTION_BAR_HIGH);//窗口大小
#define FONT_SIZE 72//字体分配空间


SDL_Texture *texture = NULL; //渲染用
SDL_Surface *surface = NULL; //渲染用
SDL_Window *window; //窗口
SDL_Renderer *renderer;//渲染器
TTF_Font *font;//字体

int frame = 160;//刷新率75ms一次

int level = 1;

typedef struct SnakeNode {       //蛇的身体信息链表
    int x;//在map上坐标 head中方向
    int y;//在map上坐标 head中数量
    struct SnakeNode *next;
    struct SnakeNode *front;
} SnakeNode;
SnakeNode *head;
SnakeNode *tail;

struct Map {
    int type;
    int x;
    int y;
    char *texture;
    void *data;
} map[MAP_SIZE + 1][MAP_SIZE + 1];

enum { //四个方向
    UP, DOWN, LEFT, RIGHT
};

enum { //地图元素
    NOTHING, SNAKE, FOOD, BARRIER
};

//音乐文件
Mix_Chunk *music_eatFood;
Mix_Chunk *music_win;
Mix_Chunk *music_gameover;
Mix_Music *music_journey;

void Load();

void InitLevel();//初始化游戏

void Clear();//清除所有参数（重开）

void ReadMap(FILE *fp);

SnakeNode *HeadInsert(int x, int y);//在蛇头部插入节点

void DetectInput(SDL_Event *);

void Move();//计算蛇头下一次移动位置

bool Check();//检查当前场面

void EventControl();//核心循环

void Draw();//绘图

void DeathEvent(SDL_Event *);//死亡事件

void PrintText(char *str, Uint8 r, Uint8 g, Uint8 b, Uint8 a, int x, int y, int w, int h);//渲染文字

void PrintImage(char *imgae_path, int x, int y, int w, int h);//画图片

void CreateFood(int count);//随机生成食物

void Quit();//退出，销毁

#undef main

int main(void) {
    Load();
    EventControl();
    Quit();
    return 1;
}

void Load() {
    //
    W = ((MAP_SIZE) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE);//窗口大小
    H = ((MAP_SIZE) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE + ACTION_BAR_HIGH);//窗口大小
    //加载窗口和渲染器
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_InitSubSystem failed: %s", SDL_GetError());
    }
    window = SDL_CreateWindow("Snake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H,
                              SDL_FALSE);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    //加载字体
    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
    }
    font = TTF_OpenFont("fonts\\STXINWEI.TTF", FONT_SIZE);
    if (!font) {
        SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
    }
    IMG_Init(IMG_INIT_PNG);
    //打开音乐
    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) == -1)
        SDL_Log("%s", SDL_GetError());
    music_eatFood = Mix_LoadWAV("music\\getfood.wav");
    music_gameover = Mix_LoadWAV("music\\gameover.wav");
    music_journey = Mix_LoadMUS("music\\journey.wav");

}

void Clear() {
    for (int i = 0; i <= MAP_SIZE; ++i) {
        for (int j = 0; j <= MAP_SIZE; ++j) {
            map[i][j].type = NOTHING;
        }
    }
    if (head->next != NULL) {
        for (SnakeNode *current = head->next; current != tail;) {
            current = current->next;
            free(current->front);
        }
    }
}

void InitLevel() {
    //清空所有数据

    //小蛇
    head = (SnakeNode *) malloc(sizeof *head);
    head->x = LEFT;
    head->y = 0;
    head->next = NULL;
    head->front = NULL;
    tail = HeadInsert(MAP_SIZE - 1, MAP_SIZE / 2 + 1);
    HeadInsert(MAP_SIZE - 2, MAP_SIZE / 2 + 1);
    HeadInsert(MAP_SIZE - 3, MAP_SIZE / 2 + 1);

    //读入地图
    FILE *fp = NULL;
    ReadMap(fp);

    //食物
    srand(time(NULL));
    CreateFood(3);
    //
    Mix_PlayMusic(music_journey, -1);
    //
    frame = 150;
}

void ReadMap(FILE *fp) {
    switch (level) {
        case 1:
            fp = fopen("level\\1.txt", "r");
            break;
        default:
            break;
    }
    if (fp != NULL) {
        for (int i = 1; i <= MAP_SIZE; ++i) {
            for (int j = 1; j <= MAP_SIZE; ++j) {
                fscanf(fp, "%d", &map[i][j].type);
            }
        }
        fclose(fp);
    } else printf("Map load failed\n");
}

void EventControl() {
    InitLevel();
    SDL_Event event;
    Draw();
    while (true) {
        unsigned long long start = SDL_GetTicks64();
        DetectInput(&event);
        Move();
        if (!Check()) {
            DeathEvent(&event);
            return;
        }
        Draw();
        unsigned long long end = SDL_GetTicks64();
        if (frame > (end - start)) SDL_Delay(frame - (unsigned int) (end - start));
    }
}

void Draw() {
    //绘制网格
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (int i = 0; i < MAP_SIZE; ++i) {//横线
        SDL_RenderDrawLine(renderer, 0, i * (GRID_SIZE + EDGE_SIZE), W, i * (GRID_SIZE + EDGE_SIZE));
    }
    SDL_RenderDrawLine(renderer, 0, MAP_SIZE * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, W,
                       MAP_SIZE * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE);
    for (int i = 0; i < MAP_SIZE; ++i) {//竖线
        SDL_RenderDrawLine(renderer, i * (GRID_SIZE + EDGE_SIZE), 0, i * (GRID_SIZE + EDGE_SIZE),
                           MAP_SIZE * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE);
    }
    SDL_RenderDrawLine(renderer, W, 0, W, H);

    //填充地图元素
    for (int i = 1; i <= MAP_SIZE; ++i) {
        for (int j = 1; j <= MAP_SIZE; ++j) {
            if (map[i][j].type == SNAKE) {
                if (i == head->next->x && j == head->next->y) {
                    switch (head->x) {
                        case UP:
                            PrintImage("picture\\snake_up.png", (j - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE,
                                       (i - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, GRID_SIZE, GRID_SIZE);
                            break;
                        case DOWN:
                            PrintImage("picture\\snake_down.png", (j - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE,
                                       (i - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, GRID_SIZE, GRID_SIZE);
                            break;
                        case LEFT:
                            PrintImage("picture\\snake_left.png", (j - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE,
                                       (i - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, GRID_SIZE, GRID_SIZE);
                            break;
                        case RIGHT:
                            PrintImage("picture\\snake_right.png", (j - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE,
                                       (i - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, GRID_SIZE, GRID_SIZE);
                            break;
                        default:
                            break;
                    }
                } else if (i == tail->x && j == tail->y) {
                    if (tail->front->x > tail->x) {
                        PrintImage("picture\\tail_down.png", (j - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE,
                                   (i - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, GRID_SIZE, GRID_SIZE);
                    } else if (tail->front->x < tail->x) {
                        PrintImage("picture\\tail_up.png", (j - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE,
                                   (i - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, GRID_SIZE, GRID_SIZE);
                    } else if (tail->front->y > tail->y) {
                        PrintImage("picture\\tail_right.png", (j - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE,
                                   (i - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, GRID_SIZE, GRID_SIZE);
                    } else
                        PrintImage("picture\\tail_left.png", (j - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE,
                                   (i - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, GRID_SIZE, GRID_SIZE);
                } else {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    SDL_Rect rect = {(j - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE,
                                     (i - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, GRID_SIZE, GRID_SIZE};
                    SDL_RenderFillRect(renderer, &rect);
                }
            } else if (map[i][j].type == FOOD) {
                PrintImage("picture\\apple.png", (j - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE,
                           (i - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, GRID_SIZE, GRID_SIZE);
            } else if (map[i][j].type == BARRIER) {
                SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
                SDL_Rect rect = {(j - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE,
                                 (i - 1) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, GRID_SIZE, GRID_SIZE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    //显示下方信息：
    char str[10];
    PrintText("Score: ", 0, 0, 0, 255, 0, MAP_SIZE * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, (GRID_SIZE + EDGE_SIZE) * 5,
              (GRID_SIZE + EDGE_SIZE) * 2);
    PrintText(itoa(head->y, str, 10), 255, 125, 0, 255, (GRID_SIZE + EDGE_SIZE) * 5,
              MAP_SIZE * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, (GRID_SIZE + EDGE_SIZE) * 2, (GRID_SIZE + EDGE_SIZE) * 2);
    PrintText("Level: ", 0, 0, 0, 255, 250, MAP_SIZE * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, (GRID_SIZE + EDGE_SIZE) * 5,
              (GRID_SIZE + EDGE_SIZE) * 2);
    PrintText(itoa(level, str, 10), 0, 125, 255, 255, (GRID_SIZE + EDGE_SIZE) * 18,
              MAP_SIZE * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE, (GRID_SIZE + EDGE_SIZE) * 2, (GRID_SIZE + EDGE_SIZE) * 2);
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

void DetectInput(SDL_Event *event) {
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT) {
            return;
        } else if (event->type == SDL_KEYDOWN) {
            switch (event->key.keysym.sym) {
                case SDLK_w:
                    if (head->x != DOWN) head->x = UP;
                    break;
                case SDLK_s:
                    if (head->x != UP)head->x = DOWN;
                    break;
                case SDLK_a:
                    if (head->x != RIGHT)head->x = LEFT;
                    break;
                case SDLK_d:
                    if (head->x != LEFT) head->x = RIGHT;
                    break;
                case SDLK_SPACE:
                    frame = 75;
                    break;
                default :
                    break;
            }
            break;
        } else if (event->type == SDL_KEYUP) {
            switch (event->key.keysym.sym) {
                case SDLK_SPACE:
                    frame = 160;
                    break;
                case SDLK_F1:
                    GRID_SIZE++;
                    W = ((MAP_SIZE) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE);//窗口大小
                    H = ((MAP_SIZE) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE + ACTION_BAR_HIGH);//窗口大小
                    SDL_SetWindowSize(window, W, H);
                    break;
                case SDLK_F2:
                    GRID_SIZE--;
                    W = ((MAP_SIZE) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE);//窗口大小
                    H = ((MAP_SIZE) * (GRID_SIZE + EDGE_SIZE) + EDGE_SIZE + ACTION_BAR_HIGH);//窗口大小
                    SDL_SetWindowSize(window, W, H);
                    break;
                default:
                    break;
            }
        }
    }
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
        default:
            break;
    }
}

bool Check() {
    //死没死
    if (map[head->next->x][head->next->y].type == SNAKE || map[head->next->x][head->next->y].type == BARRIER) {
        return false;
    }
    //有没有吃的
    if (map[head->next->x][head->next->y].type == FOOD) {
        head->y++;
        CreateFood(1);
        Mix_PlayChannel(7, music_eatFood, 0);
    } else {
        map[tail->x][tail->y].type = NOTHING;
        tail = tail->front;
        free(tail->next);
        tail->next = NULL;
    }
    map[head->next->x][head->next->y].type = SNAKE;
    return true;
}

void DeathEvent(SDL_Event *event) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    PrintText("You lose.Press any button to replay", 0, 150, 255, 255, MAP_SIZE * (GRID_SIZE + EDGE_SIZE) / 2 - 250,
              MAP_SIZE * GRID_SIZE + EDGE_SIZE + 50, 500,
              70);
    Mix_PauseMusic();
    Mix_PlayChannel(7, music_gameover, 0);
    SDL_RenderPresent(renderer);
    SDL_Delay(3000);
    while (SDL_PollEvent(event));

    while (true) {
        if (SDL_PollEvent(event)) {
            switch (event->type) {
                case SDL_QUIT:
                    return;
                case SDL_KEYDOWN:
                    Clear();
                    EventControl();
                    return;
                default :
                    break;
            }
        }
    }
}

void CreateFood(int count) {
    while (count > 0) {
        int random1 = rand() % MAP_SIZE + 1;
        int random2 = rand() % MAP_SIZE + 1;
        if (map[random1][random2].type == NOTHING) {
            map[random1][random2].type = FOOD;
            count--;
        }
    }
}

void PrintText(char *str, Uint8 r, Uint8 g, Uint8 b, Uint8 a, int x, int y, int w, int h) {
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

void PrintImage(char *imgae_path, int x, int y, int w, int h) {
    SDL_Rect text;
    surface = IMG_Load(imgae_path);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    text.x = x;
    text.y = y;
    text.w = w;
    text.h = h;
    SDL_RenderCopy(renderer, texture, NULL, &text);
}

void Quit() {
    Mix_Quit();
    IMG_Quit();
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}