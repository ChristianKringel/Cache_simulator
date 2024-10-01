#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <ctype.h>

// cache_simulator <nsets> <bsize> <assoc> <substituição> <flag_saida> arquivo_de_entrada


// Estruturas utilizadas
typedef struct
{
    unsigned int tag;
    unsigned int index;
    unsigned int offset;
    unsigned int itWasAdded;
    bool validade;
} CacheAddress;

typedef struct
{
    int totalMisses;
    int capacity;
    int compulsory;
    int conflict;
    int hits;
} CacheMisses;

typedef struct
{
    unsigned int associativity; // associatividade
    unsigned int block_size;    // tam do bloco
    unsigned int nsets;         // numero de conjuntos
    char substitution_policy;   // politica de substituicao
    CacheAddress *cache_address;
    CacheMisses *misses;
    unsigned int totalAccesses;
} Cache;

//bool debug = true;

// Declaracao das funcoes utilizadas
int LRU(Cache *cache, int index);
void readFile(Cache *cache, const char *file);
bool setIsFull(Cache *cache);
void processAddress(Cache *cache, int address);
Cache *initializeCache(Cache *cache, unsigned int associativity, unsigned int block_size, unsigned int nsets, char substitution_policy);
CacheAddress *freeCacheAddress(CacheAddress *cache_address);
int indexForReplace(Cache *cache, int index);
static bool powerOfTwo(const unsigned int value);
int LRU(Cache *cache, int index);
int FIFO(Cache *cache, int index);
int RANDOM(Cache *cache, int index);
void printFlag0(Cache *cache);
void printFlag1(Cache *cache);
unsigned int getIndexForCheckFull(unsigned int set, unsigned int way, unsigned int associativity);

#endif