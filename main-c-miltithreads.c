#include "main.h"
//#include "debug.h"

void *png_in_yuv_Multi(void *arg) {
    /* uint8_t *img_rgb, uint8_t *img_y, uint8_t *img_u, uint8_t *img_v, int64_t length */
    sem_post(&sem);
    int32_t i,j,l=0;
    yuv pix;
    /* arg_multi arg_loc = * ((arg_multi*)arg); - либо, можно сделать через локальную переменную */

    for(i=0,j=0,l=0; j < ((arg_multi*)arg)->length; i+=3,j++) {
        pix.y = 0.299 * ((arg_multi*)arg)->img_rgb[i] + 0.587 * ((arg_multi*)arg)->img_rgb[i+1] + 0.114 * ((arg_multi*)arg)->img_rgb[i+2];
        pix.u = -0.169 * ((arg_multi*)arg)->img_rgb[i] - 0.331 * ((arg_multi*)arg)->img_rgb[i+1] + 0.500 * ((arg_multi*)arg)->img_rgb[i+2] + 128;
        pix.v = 0.500 * ((arg_multi*)arg)->img_rgb[i] - 0.419 * ((arg_multi*)arg)->img_rgb[i+1] - 0.081 * ((arg_multi*)arg)->img_rgb[i+2] + 128;

        ((arg_multi*)arg)->img_y[j] = pix.y;
        if ((j % 2) == 0) {
            ((arg_multi*)arg)->img_u[l] = pix.u;
            ((arg_multi*)arg)->img_v[l] = pix.v;
            l++;
        }
    }
}

/**
 * Функция преобразования RGB в YUV формат
 *
 */
int8_t rgb_in_yuv(uint8_t *img_rgb, uint8_t *img_yuv, int32_t width, int32_t height) {
    int32_t i, result,j,k=0;
    arg_multi arg;
    int32_t count_thread=1;
    pthread_t t[count_thread];

    sem_init(&sem, 0, 0);

    /**
      * Преобразование картинки из RGB в YUV
      */
    for(i=0; i<(int32_t)(height/6); i++) {
        arg.img_rgb = img_rgb+width*3*6*i;
        arg.img_y = img_yuv+width*6*i;
        arg.img_u = img_yuv+width*height+(width/2)*3*i;
        arg.img_v = img_yuv+width*height+(width*height)/4+(width/2)*3*i;
        arg.length = width*6;

        result = pthread_create(&t[k], NULL, png_in_yuv_Multi, &arg);
        if (result != 0) {
            printf("Ошибка при создании потока !\n");
            sem_destroy(&sem);
            return 1;
        }
        k++;
        if (k>=count_thread) {
            k = 0;
            for(j=0; j<count_thread; j++) {
                result = pthread_join(t[j], NULL);
                if (result != 0) {
                    printf("Ошибка в ожидании потока !\n");
                    sem_destroy(&sem);
                    return 1;
                }
            }

        }
        sem_wait(&sem);
    }

    if (height%6 != 0) {
        arg.img_rgb = img_rgb+width*3*6*i;
        arg.img_y = img_yuv+width*6*i;
        arg.img_u = img_yuv+width*height+(width/2)*3*i;
        arg.img_v = img_yuv+width*height+(width*height)/4+(width/2)*3*i;
        arg.length = width*(height%6);

        result = pthread_create(&t[k], NULL, png_in_yuv_Multi, &arg);
        if (result != 0) {
            printf("Ошибка при создании потока !\n");
            sem_destroy(&sem);
            return 1;
        }
        k++;
    }

    for(j=0; j<k; j++) {
        result = pthread_join(t[j], NULL);
        if (result != 0) {
            printf("Ошибка в ожидании потока !\n");
            sem_destroy(&sem);
            return 1;
        }
    }

    sem_destroy(&sem);

    return 0;
}

/**
 * Функция загрузки картинки формата bmp в память
 *
 * @return 0
 */
int8_t load_bmp(uint8_t *img_rgb, char *pic_name, int32_t width, int32_t height) {
    /* Открывает файл */
    FILE *f;
    f = fopen(pic_name, "rb");
    if (!f) {
        printf("Ошибка при открытия файла !\n");
        return 1;
    }

    /* Считываем заголовок файла */
    BMPheader bmPheader;
    fread(&bmPheader.bfType,1, 2, f);
    fread(&bmPheader.byte16,1, 8, f);
    fread(&bmPheader.byte16,1, 8, f);
    fread(&bmPheader.biWidth,1, 4, f);
    fread(&bmPheader.biHeight,1, 4, f);
    fread(&bmPheader.byte32,1, 8, f);
    fread(&bmPheader.byte32,1, 8, f);
    fread(&bmPheader.byte32,1, 8, f);
    fread(&bmPheader.byte32,1, 8, f);

    /* Проверяем сигнатуру файла */
    if( bmPheader.bfType!=0x4d42 && bmPheader.bfType!=0x4349 && bmPheader.bfType!=0x5450 ) {
        printf("Файл не bmp формата !\n");
        fclose(f);
        return 1;
    }

    /* Проверяем соответствует ли размер картинки размеру видео */
    if ((bmPheader.biWidth != width) || (bmPheader.biHeight != height)) {
        printf("Размер картинки не соответствует размеру видео !\n");
        fclose(f);
        return 1;
    }

    /* Считывает (пропускаем) таблицу цветов */
    uint8_t *color_header = (uint8_t *)malloc(66);
    if (!color_header) {
        printf("Ошибка при выделении памяти !\n");
        fclose(f);
        f = NULL;
        return 1;
    }
    fread(color_header, 1, 66,f);

    /* После всех действий проверим зиписалось ли что нибудь */
    *img_rgb = '\0';

    /* Считываем картинку, преобразовываем в rgb и переворачиваем */
    int64_t i,j;
    uint8_t *tmp_rbg = (uint8_t *)malloc(sizeof(uint8_t)*3);
    if (!tmp_rbg) {
        printf("Ошибка при выделении памяти !\n");
        return 1;
    }

    for (i=bmPheader.biHeight-1; i >= 0; i--)
        for (j=0; j < bmPheader.biWidth*3; j+=3) {
            fread(tmp_rbg, 1, 3, f);
            img_rgb[j+bmPheader.biWidth*i*3] = tmp_rbg[0];
            img_rgb[(j + 1)+bmPheader.biWidth*i*3] = tmp_rbg[2];
            img_rgb[(j + 2)+bmPheader.biWidth*i*3] = tmp_rbg[1];
        }

    free(tmp_rbg);
    tmp_rbg = NULL;
    fclose(f);
    f = NULL;
    free(color_header);
    color_header = NULL;

    /* Проверяем на запись */
    if (*img_rgb == '\0') {
        printf("Ошибка записи !\n");
        return 1;
    }

    return 0;
}

/**
 * Вставка картинки в видео
 *
 */
int8_t img_insert_video(uint8_t *img_yuv, uint8_t *img_rgb, char *input_name_steam, char *output_name_stream, int32_t width, int32_t height) {
    /**
     * Открытие входного и создание выходного потока
     */
    FILE *input_stream;
    FILE *output_stream;
    input_stream = fopen(input_name_steam,"rb");
    if (!input_stream) {
        printf("Ошибка при открытия файла !\n");
        return 1;
    }

    output_stream = fopen(output_name_stream,"wb");
    if (!output_stream) {
        printf("Ошибка при создании файла !\n");
        fclose(input_stream);
        input_stream = NULL;
        return 1;
    }

    /**
     * Переменная для кадра
     */
    uint8_t *frame = (uint8_t *)malloc((size_t)(width*height + width*height/2));
    if (!frame) {
        printf("Ошибка при выделении памяти !\n");
        fclose(input_stream);
        input_stream = NULL;
        fclose(output_stream);
        output_stream = NULL;

        return 1;
    }

    while (1) {
        int32_t i, j;

        /**
         * Считывание кадра
         */
        fread(frame, (size_t) (width * height + width*height/2), 1, input_stream);

        /**
         * Проверка на конец файла (проверка делается не в while, что бы не добавлялся лишний кадр)
         */
        if (feof(input_stream)) break;

        /**
         * Наложение изображении
         */
        int32_t l,k;
        for (i=0,k=0; i < height; i++) {
            for (j=0, l=0; j < width; j++) {
                if ((height/3 > i)&&(width/3 > j)) {
                    /**
                     * Записываем цвет пикселя из RGB картинки
                     */
                    frame[j+i*width] = img_yuv[j+i*width];

                    frame[width*height + l + k*(width/2)] = img_yuv[width*height + l + k*(width/2)];
                    frame[width*height + (width*height)/4 + l + k*(width/2)] =
                            img_yuv[width*height + (width*height)/4 + l + k*(width/2)];
                    if ((j % 2) == 0) l++;
                }
                /**
                 * Остаётся исходный цвет кадра
                 */
            }
            if ((i % 2) == 0) k++;
        }

        /**
         * Сохранение кадра
         */
        for (i = 0; i < (int64_t)(height*width*1.5); i++) {
            fwrite(&frame[i], 1, 1, output_stream);
        }
    }

    free(frame);
    frame = NULL;
    fclose(input_stream);
    input_stream = NULL;
    fclose(output_stream);
    output_stream = NULL;
    return 0;
}

int main(int argc, char *argv[]) {

    /**
     * Обработка аргументов запуска
     *
     * Проверка кол-во аргументов
     */
    if ((argc < 7 || argc > 7)&&(argc != 2)) {
        printf("Неверное число аргументов !\n");
        return 0;
    }

    /**
     * Вывод справки
     */
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("Использование: %s [--size=640x360] [--pic=name.bmp] "
                       "-i input_stream.yuv -o output_stream.yuv\n\n"
                       "  --size \tУказать размер видео. Ширина, высота.\n"
                       "  --pic \tУказать картинку которая будет накладываться. Картинка в формате .bmp.\n"
                       "  -i \t\tНазвание входного потока(видео).\n"
                       "  -o \t\tНазвание выходного потока(видео).\n"
                       "  -h --help \tДанная справка.\n", argv[0]);
        return 0;
    }

    /**
     * Парсинг аргументов
     */
    int32_t width = 0, height = 0;
    char *pic_name = NULL,
         *input_name_steam = NULL,
         *output_name_stream = NULL;
    uint8_t i;

    for (i=1; i<argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '-') {
            if (strstr(argv[i],"size")-argv[i] == 2) {
                strtok(argv[i],"=x");
                width = atoi(strtok(NULL,"=x"));
                height = atoi(strtok(NULL,"=x"));
            } else
            if (strstr(argv[i],"pic")-argv[i] == 2) {
                strtok(argv[i], "=x");
                pic_name = strtok(NULL, "=x");

            } else {
                printf("Неверный аргумент !\n");
                return 0;
            }
        } else
        if (argv[i][0] == '-') {
            if (argc == i) {
                printf("Не указано значение ключа !\n");
                return 0;
            }
            if (argv[i][1] == 'i') {
                if (strstr(argv[i+1],".yuv\0") != NULL) {
                    input_name_steam = argv[i+1];
                    i++;
                } else {
                    printf("Неверный аргумент !\nВходной поток должен иметь формат .yuv !");
                    return 0;
                }
            } else
            if (argv[i][1] == 'o') {
                if (strstr(argv[i+1],".yuv\0") != NULL) {
                    output_name_stream = argv[i+1];
                    i++;
                } else {
                    printf("Неверный аргумент !\nВыходной поток должен иметь формат .yuv !");
                    return 0;
                }
            } else {
                printf("Неверный аргумент !\n");
                return 0;
            }
        } else {
            printf("Неверный аргумент !\n");
            return 0;
        }
    }

    /**
     * Загрузка картинки
     */
    /* Выделяем память под картинку в rgb */
    uint8_t *img_rgb = (uint8_t *)malloc((size_t)(width * height * 3));
    if (!img_rgb) {
        printf("Ошибка при выделении памяти !\n");
        return 1;
    }

    int8_t ret_lb = load_bmp(img_rgb, pic_name, width, height);
    if (ret_lb) {
        free(img_rgb);
        img_rgb = NULL;
        return 0;
    }
    else printf("Картинка загружена.\n");

    /**
     * Преобразование картинки в yuv
     */
    /* Выделяем память под картинку в yuv */
    uint8_t *img_yuv = (uint8_t *)malloc((size_t)(width * height * 3));
    if (!img_yuv) {
        printf("Ошибка при выделении памяти !\n");
        free(img_rgb);
        img_rgb = NULL;
        return 0;
    }

    /**
     * Измерение времени выполнения
     */
    struct timespec m1, m2;
    clock_gettime (CLOCK_REALTIME, &m1);

    int8_t ret_riy = rgb_in_yuv(img_rgb, img_yuv, width, height);
    clock_gettime (CLOCK_REALTIME, &m2);
    if (ret_riy) {
        free(img_rgb);
        img_rgb = NULL;
        free(img_yuv);
        img_yuv = NULL;
        return 0;
    }

//    prov_file2(img_yuv, width, height);

    printf("%lf мс\n",(double)(1000000000*(m2.tv_sec - m1.tv_sec)+(m2.tv_nsec - m1.tv_nsec))/1000000);

    /**
     * Обработка видео потока (вставка картинки)
     */
    int8_t ret_iiv = img_insert_video(img_yuv, img_rgb, input_name_steam, output_name_stream, width, height);
    if (ret_iiv) {
        free(img_rgb);
        img_rgb = NULL;
        free(img_yuv);
        img_yuv = NULL;
        return 0;
    }
    else printf("ОК.\n");

    free(img_rgb);
    img_rgb = NULL;
    free(img_yuv);
    img_yuv = NULL;

    return  0;
}
