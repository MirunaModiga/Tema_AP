#define _XOPEN_SOURCE 600
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

char *INPUT_FILE;
char *OUT_FILE;
int NUM_THREADS;
int reps, dim, length, height, width;
u_int8_t **Matrix2D;
u_int8_t ***Matrix3D;

pthread_barrier_t barrier;

void getArgs(int argc, char **argv)
{
    if (argc < 4)
    {
        printf("Not enough paramters: ./program \n");
        exit(1);
    }
    INPUT_FILE = strdup(argv[1]);
    OUT_FILE = strdup(argv[2]);
    NUM_THREADS = atoi(argv[3]);
}

void init()
{
    FILE *file_in = fopen(INPUT_FILE, "r");
    fscanf(file_in, "%d", &dim);

    if (dim == 2)
    {
        fscanf(file_in, "%d %d %d", &height, &width, &reps);

        Matrix2D = (u_int8_t **)malloc(height * sizeof(u_int8_t *));
        for (int i = 0; i < height; i++)
        {
            Matrix2D[i] = (u_int8_t *)malloc(width * sizeof(u_int8_t));
            for (int j = 0; j < width; j++)
            {
                fscanf(file_in, "%hhd ", &Matrix2D[i][j]);
            }
        }
    }
    else
    {
        fscanf(file_in, "%d %d %d %d", &length, &height, &width, &reps);

        Matrix3D = (u_int8_t ***)malloc(length * sizeof(u_int8_t **));
        for (int i = 0; i < length; i++)
        {
            Matrix3D[i] = (u_int8_t **)malloc(height * sizeof(u_int8_t *));
            for (int j = 0; j < height; j++)
            {
                Matrix3D[i][j] = (u_int8_t *)malloc(width * sizeof(u_int8_t));
                for (int k = 0; k < width; k++)
                {
                    fscanf(file_in, "%hhd ", &Matrix3D[i][j][k]);
                }
            }
        }
    }
    fclose(file_in);
}

int countNeighbors(int x, int y)
{
    int count = 0;
    for (int i = x - 1; i <= x + 1; i++)
    {
        for (int j = y - 1; j <= y + 1; j++)
        {
            if (i == x && j == y)
                continue;
            if (i < 0 || j < 0 || i >= height || j >= width)
                continue;
            if (Matrix2D[i][j] == 1)
                count++;
        }
    }
    return count;
}

int countNeighbors_3D(int x, int y, int z)
{
    int count = 0;
    for (int i = x - 1; i <= x + 1; i++)
    {
        for (int j = y - 1; j <= y + 1; j++)
        {
            for (int k = z - 1; k <= z + 1; k++)
            {
                if (i == x && j == y && k == z)
                    continue;
                if (i < 0 || j < 0 || k < 0 || i >= length || j >= height || k >= width)
                    continue;
                if (Matrix3D[i][j][k] == 1)
                    count++;
            }
        }
    }
    return count;
}

void *threadFunction2D(void *var)
{
    int thread_id = *(int *)var;

    u_int8_t vector_aux[((height * width) / NUM_THREADS) + 1];

    for (int step = 0; step < reps; step++)
    {
        for (int i = thread_id; i < height * width; i += NUM_THREADS)
        {
            int x = i / width;
            int y = i % width;
            if (Matrix2D[x][y] == 1)
            {
                vector_aux[i / NUM_THREADS] = 0;
            }
            if (Matrix2D[x][y] == 0 && countNeighbors(x, y) == 2)
            {
                vector_aux[i / NUM_THREADS] = 1;
            }
        }
        pthread_barrier_wait(&barrier);

        for (int i = thread_id; i < height * width; i += NUM_THREADS)
        {
            int x = i / width;
            int y = i % width;
            Matrix2D[x][y] = vector_aux[i / NUM_THREADS];
        }
        pthread_barrier_wait(&barrier);
    }
}

void *threadFunction3D(void *var)
{
    int thread_id = *(int *)var;

    u_int8_t vector_aux[((length * height * width) / NUM_THREADS) + 1];

    for (int step = 0; step < reps; step++)
    {
        for (int i = thread_id; i < length * height * width; i += NUM_THREADS)
        {
            int x = i / (height * width);
            int y = (i / width) % height;
            int z = i % width;
            if (Matrix3D[x][y][z] == 1)
            {
                vector_aux[i / NUM_THREADS] = 0;
            }
            if (Matrix3D[x][y][z] == 0 && countNeighbors_3D(x, y, z) == 2)
            {
                vector_aux[i / NUM_THREADS] = 1;
            }
        }
        pthread_barrier_wait(&barrier);

        for (int i = thread_id; i < length * height * width; i += NUM_THREADS)
        {
            int x = i / (height * width);
            int y = (i / width) % height;
            int z = i % width;
            Matrix3D[x][y][z] = vector_aux[i / NUM_THREADS];
        }
        pthread_barrier_wait(&barrier);
    }
}

void writeToFile_2D()
{
    FILE *file_out = fopen(OUT_FILE, "w");
    fprintf(file_out, "%d %d %d\n", dim, height, width);
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            fprintf(file_out, "%d ", Matrix2D[i][j]);
        }
    }
    fclose(file_out);
}

void writeToFile_3D()
{
    FILE *file_out = fopen(OUT_FILE, "w");
    fprintf(file_out, "%d %d %d %d\n", dim, length, height, width);
    for (int i = 0; i < length; i++)
    {
        for (int j = 0; j < height; j++)
        {
            for (int k = 0; k < width; k++)
            {
                fprintf(file_out, "%d ", Matrix3D[i][j][k]);
            }
        }
    }
    fclose(file_out);
}

int main(int argc, char **argv)
{
    getArgs(argc, argv);
    init();

    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    pthread_t threads[NUM_THREADS];
    int tids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++)
        tids[i] = i;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (dim == 2)
            pthread_create(&threads[i], NULL, threadFunction2D, &tids[i]);
        else
            pthread_create(&threads[i], NULL, threadFunction3D, &tids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    pthread_barrier_destroy(&barrier);
    if (dim == 2)
        writeToFile_2D();
    else
        writeToFile_3D();

    return 0;
}