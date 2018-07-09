
#ifndef TS1_1_MAIN_H
#define TS1_1_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
sem_t sem;

/**
 * Структура хранящая значения yuv компонентов
 */
typedef struct {
    uint8_t y;
    uint8_t u;
    uint8_t v;
} yuv;

/**
 * Структура аргумента многопоточной функции
 */
typedef struct {
    uint8_t *img_rgb;
    uint8_t *img_y;
    uint8_t *img_u;
    uint8_t *img_v;
    int64_t length;
    int32_t width;
} arg_multi;

/**
 * Структура заголовка bmp файла
 */
typedef struct {
    uint16_t bfType;
    int64_t  byte16;
    int32_t  biWidth;
    int32_t  biHeight;
    int64_t  byte32;
} BMPheader;

#endif //TS1_1_MAIN_H
