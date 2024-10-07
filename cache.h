#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <ctype.h>


// cache_simulator <nsets> <bsize> <assoc> <substituição> <flag_saida> arquivo_de_entrada

// Estruturas utilizadas

// Estrutura para um bloco de cache
typedef struct {
    bool valid;
    uint32_t tag;
    //int *data;
    int lastAccess;
    int insertionTime;
} CacheBlock;

// Estrutura para um conjunto de cache
typedef struct {
    CacheBlock *blocks; // cada conjunto tem (ou pode ter) varios blocos
    int numBlocks;
} CacheSet;

// Estrutura para estatisticas da cache
typedef struct {
    int hits;
    int misses;
    int compulsory;
    int capacity;
    int conflict;
    int totalAccesses;
} CacheStats;

// Estrutura principal da cache
typedef struct {
    CacheSet *sets;
    int numSets;
    int blockSize;
    int associativity;
    CacheStats stats;
    int (*replacementPolicy)(CacheSet *set, int currentTime, int associativity); // isso eh um ponteiro para uma funcao
    // a funcao retorna um inteiro que eh o indice do bloco que sera substituido
    // quando precisamos determinar qual bloco sera substituido, chamamos essa funcao apontada por replacementPolicy
} Cache;
//bool debug = true;

// Declaracao das funcoes utilizadas
Cache* initializeCache(int numSets, int blockSize, int associativity, char policyChar);
void freeCache(Cache *cache);
void processAddress(Cache *cache, uint32_t address);
int getLRUIndex(CacheSet *set, int currentTime, int associativity);
int getFIFOIndex(CacheSet *set, int currentTime, int associativity);
int getRandomIndex(CacheSet *set, int currentTime, int associativity);
void readFile(Cache *cache, const char *filename);
void printStats(Cache *cache, int flag);
bool isPowerOfTwo(unsigned int value);
bool cacheIsFull(Cache *cache);

#endif