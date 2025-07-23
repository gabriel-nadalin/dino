#include <stdio.h>
#include <stdlib.h>
#include "address_map_arm.h"

/* VGA control functions */
void video_text(int x, int y, char *text_ptr);
void video_box(int x1, int y1, int x2, int y2, short pixel_color);
void video_clear_screen(short color);
void draw_sprite(int x, int y, int width, int height, short color);
int resample_rgb(int num_bits, int color);
int get_data_bits(int mode);

/* Game constants */
#define STANDARD_X 320
#define STANDARD_Y 240
#define GROUND_Y 200
#define DINO_X 50
#define DINO_WIDTH 16
#define DINO_HEIGHT 16
#define CACTUS_WIDTH 12
#define CACTUS_HEIGHT 20
#define CLOUD_WIDTH 20
#define CLOUD_HEIGHT 8
#define MAX_OBSTACLES 3
#define MAX_CLOUDS 6

/* Colors */
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define BROWN 0x7920
#define GRAY 0x8410

/* Game structures */
typedef struct {
    int x, y;
    int isJumping;
    int jumpVelocity;
    int groundY;
} Dino;

typedef struct {
    int x, y;
    int active;
} Obstacle;

typedef struct {
    int x, y;
    int active;
} Cloud;

/* Global variables */
int screen_x, screen_y;
int res_offset, col_offset;
int score = 0;
int gameSpeed = 2;
int gameOver = 0;
Dino player;
Obstacle obstacles[MAX_OBSTACLES];
Cloud clouds[MAX_CLOUDS];

/* Simple random number generator */
static unsigned long seed = 1;
int simple_rand() {
    seed = seed * 1103515245 + 12345;
    return (seed / 65536) % 32768;
}

/* Input handling - usando botões da FPGA */
volatile int * KEY_ptr = (int *) KEY_BASE;
volatile int * SW_ptr = (int *) SW_BASE;

int get_key_press() {
    static int last_key = 0;
    int current_key = *KEY_ptr & 0xF;
    int key_pressed = 0;
    
    // Detecta borda de descida (botão pressionado)
    if ((last_key & 0x1) && !(current_key & 0x1)) key_pressed = 1;
    if ((last_key & 0x2) && !(current_key & 0x2)) key_pressed = 1;
    if ((last_key & 0x4) && !(current_key & 0x4)) key_pressed = 1;
    if ((last_key & 0x8) && !(current_key & 0x8)) key_pressed = 1;
    
    last_key = current_key;
    return key_pressed;
}

/* VGA Functions */
void video_clear_screen(short color) {
    video_box(0, 0, screen_x - 1, screen_y - 1, color);
}

void video_text_clear(int y1, int y2) {
    int i;
    for (i = y1; i < y2; i++) {
        video_text(0, i, "                                                                                                 ");
    }
}

void draw_sprite(int x, int y, int width, int height, short color) {
    video_box(x, y, x + width - 1, y + height - 1, color);
}

void draw_dino(Dino* dino) {
    // Limpa posição anterior
    video_box(dino->x - 2, dino->groundY - (DINO_HEIGHT * 4), 
              dino->x + DINO_WIDTH + 2, dino->groundY + 5, BLACK);
    
    // Desenha o dinossauro
    short color = gameOver ? GRAY : GREEN;
    
    // Cabeça
    draw_sprite(dino->x + 5, dino->y, 9, 5, color);
    draw_sprite(dino->x + 11, dino->y + 3, 3, 1, BLACK);

    // Corpo
    draw_sprite(dino->x + 3, dino->y + 5, 7, 5, color);

    // Cauda
    draw_sprite(dino->x, dino->y + 3, 2, 7, color);
    
    // Pernas (animação simples)
    if ((score / 5) % 2 == 0) {
        draw_sprite(dino->x + 2, dino->y + DINO_HEIGHT - 6, 4, 4, color);
        draw_sprite(dino->x + 10, dino->y + DINO_HEIGHT - 4, 4, 4, color);
    } else {
        draw_sprite(dino->x + 4, dino->y + DINO_HEIGHT - 4, 4, 4, color);
        draw_sprite(dino->x + 8, dino->y + DINO_HEIGHT - 6, 4, 4, color);
    }
}

void draw_obstacle(Obstacle* obs) {
    if (!obs->active) return;
    
    // Limpa posição anterior
    video_box(obs->x + gameSpeed, obs->y, 
              obs->x + gameSpeed + CACTUS_WIDTH + 2, obs->y + CACTUS_HEIGHT + 2, BLACK);
    
    // Desenha o cacto
    draw_sprite(obs->x, obs->y + 2, 3, 6, GREEN);
    draw_sprite(obs->x + 4, obs->y + 1, 3, 19, GREEN);
    draw_sprite(obs->x + 9, obs->y, 3, 8, GREEN);
    draw_sprite(obs->x, obs->y + 8, 12, 3, GREEN);
}

void draw_cloud(Cloud* cloud) {
    if (!cloud->active) return;
    
    // Limpa posição anterior
    video_box(cloud->x + gameSpeed/2, cloud->y - 2, 
              cloud->x + gameSpeed/2 + CLOUD_WIDTH + 2, cloud->y + CLOUD_HEIGHT + 2, BLACK);
    
    // Desenha a nuvem
    draw_sprite(cloud->x, cloud->y, CLOUD_WIDTH, CLOUD_HEIGHT, WHITE);
    draw_sprite(cloud->x + 4, cloud->y - 2, 12, 4, WHITE);
}

void draw_ground() {
    // Linha do chão
    video_box(0, GROUND_Y, screen_x - 1, GROUND_Y + 2, GRAY);
    
    // Pequenos detalhes no chão
    int i;
    for (i = 0; i < screen_x; i += 20) {
        if ((i + score) % 40 < 20) {
            draw_sprite(i, GROUND_Y + 3, 4, 2, GRAY);
        }
    }
}

void draw_score() {
    char score_text[20];
    sprintf(score_text, "Score: %05d", score);
    video_text(1, 1, score_text);
    
    if (gameOver) {
        video_text(35, 10, "GAME OVER");
        video_text(30, 12, "Press any key to restart");
    }
}

/* Game Logic */
void init_game() {
    int i;
    
    score = 0;
    gameSpeed = 5;
    gameOver = 0;
    
    // Inicializar dinossauro
    player.x = DINO_X;
    player.groundY = GROUND_Y;
    player.y = player.groundY - DINO_HEIGHT;
    player.isJumping = 0;
    player.jumpVelocity = 0;
    
    // Inicializar obstáculos
    for (i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].x = screen_x + (i * 150) + simple_rand() % 100;
        obstacles[i].y = GROUND_Y - CACTUS_HEIGHT;
        obstacles[i].active = (obstacles[i].x <= screen_x) ? 1 : 0;
    }
    
    // Inicializar nuvens
    for (i = 0; i < MAX_CLOUDS; i++) {
        clouds[i].x = screen_x + (i * 200) + simple_rand() % 150;
        clouds[i].y = 50 + simple_rand() % 50;
        clouds[i].active = (clouds[i].x <= screen_x) ? 1 : 0;
    }
    video_text_clear(10, 16);
    video_clear_screen(BLACK);
}

void update_dino() {
    // Processar pulo
    if (get_key_press() && !player.isJumping && !gameOver) {
        player.isJumping = 1;
        player.jumpVelocity = -8; // Velocidade inicial para cima
    }
    
    if (player.isJumping) {
        player.y += player.jumpVelocity;
        player.jumpVelocity += 1; // Gravidade
        
        // Verificar se tocou o chão
        if (player.y >= player.groundY - DINO_HEIGHT) {
            player.y = player.groundY - DINO_HEIGHT;
            player.isJumping = 0;
            player.jumpVelocity = 0;
        }
    }
}

void update_obstacles() {
    int i;
    
    for (i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].x -= gameSpeed;

        if (obstacles[i].active) {
            
            // Remover obstáculo que saiu da tela
            if (obstacles[i].x + 2 * CACTUS_WIDTH < 0) {
                obstacles[i].x = screen_x + simple_rand() % 400 + 100;
                obstacles[i].active = 0;
            }
        } else {

            if (obstacles[i].x <= screen_x) {
                obstacles[i].active = 1;
            }
        }
    }
}

void update_clouds() {
    int i;
    
    for (i = 0; i < MAX_CLOUDS; i++) {
        clouds[i].x -= gameSpeed / 2;

        if (clouds[i].active) {
            
            if (clouds[i].x + 2 * CLOUD_WIDTH < 0) {
                clouds[i].x = screen_x + simple_rand() % 500 + 150;
                clouds[i].y = 50 + simple_rand() % 50;
                clouds[i].active = 0;
            }
        } else {

            if (clouds[i].x <= screen_x) {
                clouds[i].active = 1;
            }
        }
    }
}

int check_collision() {
    int i;
    
    for (i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            // Verificar colisão simples (retângulos)
            if (player.x < obstacles[i].x + CACTUS_WIDTH &&
                player.x + DINO_WIDTH > obstacles[i].x &&
                player.y < obstacles[i].y + CACTUS_HEIGHT &&
                player.y + DINO_HEIGHT > obstacles[i].y) {
                return 1;
            }
        }
    }
    return 0;
}

void game_loop() {
    if (!gameOver) {
        update_dino();
        update_obstacles();
        update_clouds();
                
        score += 1;
        // Aumentar velocidade a cada 150 pontos
        if (score % 500 == 0 && gameSpeed < 10) {
            gameSpeed++;
        }
        
        // Verificar colisão
        if (check_collision()) {
            gameOver = 1;
        }
    } else {
        // Reiniciar jogo se pressionar qualquer tecla
        if (get_key_press()) {
            init_game();
        }
    }
}

void render_game() {
    draw_dino(&player);
    
    int i;
    for (i = 0; i < MAX_OBSTACLES; i++) {
        draw_obstacle(&obstacles[i]);
    }
    
    for (i = 0; i < MAX_CLOUDS; i++) {
        draw_cloud(&clouds[i]);
    }
    
    draw_ground();
    draw_score();
}

/* VGA helper functions */
void video_text(int x, int y, char * text_ptr) {
    int offset;
    volatile char * character_buffer = (char *)FPGA_CHAR_BASE;
    offset = (y << 7) + x;
    while (*(text_ptr)) {
        *(character_buffer + offset) = *(text_ptr);
        ++text_ptr;
        ++offset;
    }
}

void video_box(int x1, int y1, int x2, int y2, short pixel_color) {
    int pixel_buf_ptr = *(int *)PIXEL_BUF_CTRL_BASE;
    int pixel_ptr, row, col;
    int x_factor = 0x1 << (res_offset + col_offset);
    int y_factor = 0x1 << (res_offset);
    
    x1 = x1 / x_factor;
    x2 = x2 / x_factor;
    y1 = y1 / y_factor;
    y2 = y2 / y_factor;
    
    for (row = y1; row <= y2; row++)
        for (col = x1; col <= x2; ++col) {
            pixel_ptr = pixel_buf_ptr + (row << (10 - res_offset - col_offset)) + (col << 1);
            *(short *)pixel_ptr = pixel_color;
        }
}

int resample_rgb(int num_bits, int color) {
    if (num_bits == 8) {
        color = (((color >> 16) & 0x000000E0) | ((color >> 11) & 0x0000001C) |
                ((color >> 6) & 0x00000003));
        color = (color << 8) | color;
    } else if (num_bits == 16) {
        color = (((color >> 8) & 0x0000F800) | ((color >> 5) & 0x000007E0) |
                ((color >> 3) & 0x0000001F));
    }
    return color;
}

int get_data_bits(int mode) {
    switch (mode) {
        case 0x0: return 1;
        case 0x7: return 8;
        case 0x11: return 8;
        case 0x12: return 9;
        case 0x14: return 16;
        case 0x17: return 24;
        case 0x19: return 30;
        case 0x31: return 8;
        case 0x32: return 12;
        case 0x33: return 16;
        case 0x37: return 32;
        case 0x39: return 40;
        default: return 16;
    }
}

/* Main function */
int main() {
    // Configurar VGA
    volatile int * video_resolution = (int *)(PIXEL_BUF_CTRL_BASE + 0x8);
    screen_x = *video_resolution & 0xFFFF;
    screen_y = (*video_resolution >> 16) & 0xFFFF;
    volatile int * rgb_status = (int *)(RGB_RESAMPLER_BASE);
    int db = get_data_bits(*rgb_status & 0x3F);

    res_offset = (screen_x == 160) ? 1 : 0;
    col_offset = (db == 8) ? 1 : 0;

    // Limpar tela
    video_clear_screen(BLACK);
    video_text_clear(0, 16);

    // Tela inicial
    video_text(30, 10, "T-REX GAME");
    video_text(25, 15, "Press any key to start");

    // Esperar input para começar
    while (!get_key_press()) {
        seed++; // Incrementar seed para randomização
    }
    
    // Inicializar jogo
    init_game();
    
    // Loop principal do jogo
    while (1) {
        game_loop();
        render_game();
        
        // Pequeno delay para controlar framerate
        volatile int delay_count;
        for (delay_count = 0; delay_count < 100000; delay_count++);
    }
    
    return 0;
}
