#include "asteroids.h"
#include "game_master.h"
#include "ili9341.h"
#include <stdlib.h>

#define MPU6050_ADDR 0xD0
#define NUM_ASTEROIDS 5

extern I2C_HandleTypeDef hi2c1;

// Variables del acelerómetro
static int16_t Accel_X_RAW = 0;
static int16_t Accel_Y_RAW = 0;
static uint8_t rec_data[6];

// Estructura de Objetos
typedef struct {
    float x, y;
    float old_x, old_y;
    float vx, vy;
    int size;
    int active;
} SpaceObj;

static SpaceObj player;
static SpaceObj asteroids[NUM_ASTEROIDS];

// Estado del juego
static uint32_t game_start_time;
static uint8_t juego_iniciado = 0; // 0 = Primera vez, 1 = Jugando

// --- PROTOTIPOS LOCALES ---
static void MPU6050_Init_Sensor(void);
static void MPU6050_Read_Accel(void);
static void SpawnAsteroid(int index);
static void ResetGame(void);
static int CheckCollision(SpaceObj* a, SpaceObj* b);

void Asteroids_Init(void) {

    MPU6050_Init_Sensor();

    // Resetear variables lógicas
    juego_iniciado = 0;
}

void Asteroids_Loop(void) {

    // Si la cara no está activa, salimos
    if (bomb.faceState[FACE_GYRO] == 0) {
        juego_iniciado = 0;
        return;
    }

    // INICIO DE PARTIDA
    if (juego_iniciado == 0) {
        ResetGame();
        juego_iniciado = 1;

        // Mensaje de bienvenida
        ILI9341_FillScreen(ILI9341_BLACK);
        ILI9341_WriteString(80, 100, "PILOTAR NAVE", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
        HAL_Delay(1000);
        ILI9341_FillScreen(ILI9341_BLACK);

        game_start_time = HAL_GetTick(); // Reiniciar reloj
    }

    // --- LÓGICA DEL JUEGO ---

    // CONTROL DE TIEMPO (GANAR)
    uint32_t current_time = HAL_GetTick();
    uint32_t time_elapsed = current_time - game_start_time;

    // Meta: Sobrevivir 20 segundos
    if (time_elapsed > 20000) {
        ILI9341_FillScreen(ILI9341_GREEN);
        ILI9341_WriteString(100, 110, "VICTORIA!", Font_11x18, ILI9341_BLACK, ILI9341_GREEN);

        // AVISAR A LA BOMBA
        Game_RegisterWin(FACE_GYRO);

        juego_iniciado = 0; // Resetear
        HAL_Delay(1000);
        return;
    }

    // FÍSICA Y MOVIMIENTO
    MPU6050_Read_Accel();

    // Mover NAVE (Invertir ejes según orientación física)
    player.vx = -(Accel_Y_RAW / 600.0f);
    player.vy = -(Accel_X_RAW / 600.0f);
    player.x += player.vx;
    player.y += player.vy;

    // Límites NAVE (320x240)
    if (player.x < 0) player.x = 0;
    if (player.x > 320 - player.size) player.x = 320 - player.size;
    if (player.y < 0) player.y = 0;
    if (player.y > 220 - player.size) player.y = 220 - player.size;

    // Mover ASTEROIDES
    for(int i=0; i<NUM_ASTEROIDS; i++) {
        asteroids[i].x += asteroids[i].vx;
        asteroids[i].y += asteroids[i].vy;

        // Respawn al salir por abajo
        if (asteroids[i].y > 240) {
            ILI9341_FillRectangle((int)asteroids[i].old_x, (int)asteroids[i].old_y, asteroids[i].size, asteroids[i].size, ILI9341_BLACK);
            SpawnAsteroid(i);
        }
        // Rebote lateral
        if (asteroids[i].x <= 0 || asteroids[i].x >= (320 - asteroids[i].size)) {
            asteroids[i].vx *= -1;
        }

        // DETECCIÓN DE CHOQUE (PERDER)
        if (CheckCollision(&player, &asteroids[i])) {
            ILI9341_FillScreen(ILI9341_RED);
            ILI9341_WriteString(90, 110, "IMPACTO!!", Font_11x18, ILI9341_WHITE, ILI9341_RED);

            // PENALIZACIÓN BOMBA
            Game_RegisterMistake();

            HAL_Delay(2000);
            ResetGame(); // Reiniciar partida desde 0
            game_start_time = HAL_GetTick(); // Resetear tiempo
            return; // Salir del ciclo para refrescar
        }
    }

    // DIBUJADO
    // Borrar viejos
    ILI9341_FillRectangle((int)player.old_x, (int)player.old_y, player.size, player.size, ILI9341_BLACK);
    for(int i=0; i<NUM_ASTEROIDS; i++) {
        ILI9341_FillRectangle((int)asteroids[i].old_x, (int)asteroids[i].old_y, asteroids[i].size, asteroids[i].size, ILI9341_BLACK);
    }

    // Pintar nuevos
    ILI9341_FillRectangle((int)player.x, (int)player.y, player.size, player.size, ILI9341_GREEN);
    for(int i=0; i<NUM_ASTEROIDS; i++) {
        ILI9341_FillRectangle((int)asteroids[i].x, (int)asteroids[i].y, asteroids[i].size, asteroids[i].size, ILI9341_RED);
        // Guardar pos anterior
        asteroids[i].old_x = asteroids[i].x;
        asteroids[i].old_y = asteroids[i].y;
    }
    player.old_x = player.x;
    player.old_y = player.y;

    // UI (Barra de progreso)
    int bar_width = (time_elapsed * 320) / 20000;
    if (bar_width > 320) bar_width = 320;
    ILI9341_FillRectangle(0, 230, bar_width, 10, ILI9341_BLUE);
    ILI9341_FillRectangle(bar_width, 230, 320 - bar_width, 10, ILI9341_BLACK);

    HAL_Delay(10); // Control de FPS
}

// --- FUNCIONES AUXILIARES ---

static void MPU6050_Init_Sensor(void) {
    uint8_t check;
    uint8_t Data;
    // WHO_AM_I
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, 0x75, 1, &check, 1, 1000);
    if (check == 0x68) {
        // Power Management 1 (Despertar)
        Data = 0;
        HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, 0x6B, 1, &Data, 1, 1000);
    }
}

static void MPU6050_Read_Accel(void) {
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, 0x3B, 1, rec_data, 6, 1000);
    Accel_X_RAW = (int16_t)(rec_data[0] << 8 | rec_data[1]);
    Accel_Y_RAW = (int16_t)(rec_data[2] << 8 | rec_data[3]);
    // Accel_Z_RAW no lo usamos para movernos
}

static void SpawnAsteroid(int index) {
    asteroids[index].size = 20 + (rand() % 20);
    asteroids[index].x = rand() % (320 - asteroids[index].size);
    asteroids[index].y = -50;
    asteroids[index].vx = (float)((rand() % 100) - 50) / 20.0f;
    asteroids[index].vy = (float)((rand() % 50) + 20) / 20.0f;
    asteroids[index].active = 1;
    asteroids[index].old_x = asteroids[index].x;
    asteroids[index].old_y = asteroids[index].y;
}

static void ResetGame(void) {
    player.x = 160; player.y = 200;
    player.size = 20;
    player.old_x = 160; player.old_y = 200;

    ILI9341_FillScreen(ILI9341_BLACK);

    for(int i=0; i<NUM_ASTEROIDS; i++) {
        SpawnAsteroid(i);
        asteroids[i].y = -(rand() % 300); // Escalonar salidas
    }
}

static int CheckCollision(SpaceObj* a, SpaceObj* b) {
    if (a->x < b->x + b->size &&
        a->x + a->size > b->x &&
        a->y < b->y + b->size &&
        a->y + a->size > b->y) {
        return 1;
    }
    return 0;
}
