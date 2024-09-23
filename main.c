#include <stdio.h>
#include <stdbool.h> // bool lib
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

// cache_simulator <nsets> <bsize> <assoc> <substituição> <flag_saida> arquivo_de_entrada
// 2 5 6 r 1 trace.txt
typedef struct
{
    unsigned int associativity; // associatividade
    unsigned int block_size;    // tam do bloco
    unsigned int nsets;         // numero de conjuntos
    char *substitution_policy;  // politica de substituicao
} Cache;
typedef struct
{
    unsigned int tag;
    unsigned int index;
    unsigned int offset;
    bool validade;
} CacheAddress;

typedef struct
{
    int capacity;
    int compulsory;
    int conflict;
} CacheMisses;

// variaveis globais
unsigned long int cache_accesses = 0;
unsigned long int cache_hits = 0;
unsigned long int cache_misses = 0;
unsigned long int cache_writebacks = 0;
unsigned long int cache_reads = 0;
unsigned long int cache_writes = 0;

void readFile(Cache *cache, CacheAddress *CacheAddress, const char *file, CacheMisses *missCount, int replacment, unsigned long int *totalAcess);
void processAddress(Cache *cache, CacheAddress *cacheAddress, int adress, CacheMisses *missCount, int replacment, unsigned long int *totalAcess);
long int calculateCacheSize(int nsets, int associativity, int block_size);
CacheAddress *createCacheAddress(int tag, int index, int offset);
Cache *initializeCache(Cache *cache, unsigned int associativity,
                       unsigned int block_size, unsigned int nsets, char *substitution_policy);
CacheAddress *freeCacheAddress(CacheAddress *cache_address);

static bool powerOfTwo(const unsigned int value);

int main(int argc, char *argv[])
{
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    CacheAddress *cacheAdress = (CacheAddress *)malloc(sizeof(CacheAddress));
    if (argc < 7)
    {
        printf("Uso: %s <nsets> <bsize> <assoc> <subs> <flag_saida> <arquivo_de_entrada>\n", argv[0]);
        return 1;
    }

    int nsets = atoi(argv[1]); // atoi converte string para inteiro
    int bsize = atoi(argv[2]);
    int assoc = atoi(argv[3]);
    char *sub_policy = argv[4];
    int exit_flag = atoi(argv[5]);
    char *trace_file = argv[6];

    /* testando os parametros passados na chamada do codigo*/
    printf("nsets: %d\n", nsets);
    printf("bsize: %d\n", bsize);
    printf("assoc: %d\n", assoc);
    printf("sub_policy: %s\n", sub_policy);
    printf("exit_flag: %d\n", exit_flag);
    printf("trace_file: %s\n", trace_file);

    if (!powerOfTwo(nsets))
    {
        printf("nsets deve ser uma potencia de 2\n");
        return 1;
    }
    printf("\n\n");
    readFile(cache, cacheAdress, trace_file, NULL, 0, NULL);

    printf("Total de acessos: %lu\n", cache_accesses);
    printf("Total de hits: %lu\n", cache_hits);
    printf("Total de misses: %lu\n", cache_misses);
    return 0;
}

// func para testar se esta em potencia de dois
static bool powerOfTwo(const unsigned int value)
{
    return (value & (value - 1)) == 0;
}

Cache *initializeCache(Cache *cache, unsigned int associativity,
                       unsigned int block_size, unsigned int nsets, char *substitution_policy)
{
    cache = (Cache *)malloc(sizeof(Cache));
    if (cache == NULL)
    {
        printf("ERRO: Problema ao alocar memoria para a cache\n");
        return NULL;
    }

    if (!powerOfTwo(nsets))
    {
        printf("ERRO: o numero de conjuntos deve ser uma potencia de 2\n");
        return NULL;
    }

    if (!powerOfTwo(associativity) || (!associativity == 1))
    {
        printf("ERRO: a associatividade deve ser uma potencia de 2\n");
        return NULL;
    }

    if (!powerOfTwo(block_size))
    {
        printf("ERRO: tamanho do bloco deve ser uma potencia de 2\n");
        return NULL;
    }

    cache->associativity = associativity;
    cache->block_size = block_size;
    cache->nsets = nsets;
    strcpy(cache->substitution_policy, substitution_policy);

    return cache;
}

Cache *freeCache(Cache *cache)
{
    if (cache != NULL)
    {
        free(cache);
        cache = NULL;
    }
    return cache;
}

// cria um endereco da cache
CacheAddress *createCacheAddress(int tag, int index, int offset)
{
    CacheAddress *cache_address = (CacheAddress *)malloc(sizeof(CacheAddress));
    if (cache_address == NULL)
    {
        printf("ERRO: Problema ao alocar memoria para o endereco da cache\n");
        return NULL;
    }
    cache_address->tag = tag;
    cache_address->index = index;
    cache_address->offset = offset;
    return cache_address;
}

// libera a memoria alocada para o endereco da cache
CacheAddress *freeCacheAddress(CacheAddress *cache_address)
{
    if (cache_address != NULL)
    {
        free(cache_address);
        cache_address = NULL;
    }
    return cache_address;
}

// calcula o tamanho da cache
long int calculateCacheSize(int nsets, int associativity, int block_size)
{
    return nsets * associativity * block_size;
}

void readFile(Cache *cache, CacheAddress *CacheAddress, const char *file, CacheMisses *missCount, int replacment, unsigned long int *totalAcess)
{
    FILE *fp = fopen(file, "rb");
    uint32_t endereco;

    if (!fp)
    {
        printf("ERRO: Problema ao abrir o arquivo %s\n", file);
        return;
    }

    while (fread(&endereco, sizeof(uint32_t), 1, fp) == 1)
    {
        processAddress(cache, CacheAddress, endereco, missCount, replacment, totalAcess);
    }
    //printf("\nTotal de acessos: %lu\n", cache_accesses);

    fclose(fp);
}

void processAddress(Cache *cache, CacheAddress *cacheAddress, int address, CacheMisses *missCount, int replacment, unsigned long int *totalAcess)
{
    // Calcula o offset, index e tag
    /*
    tag = endereço >> (n_bits_offset + n_bits_indice);
    indice = (endereço >> n_bits_offset) & (2^n_bits_indice -1);

    */

    // Calcula o numero de bits para o offset e index com base no tamanho da cache
    unsigned int offset_bits = log2(cache->block_size);
    unsigned int index_bits = log2(cache->nsets);

    unsigned int offset_mask = (1 << offset_bits) - 1;
    unsigned int index_mask = (1 << index_bits) - 1;
    uint32_t offset = address & offset_mask;                // Pega os bits de offset
    uint32_t index = (address >> offset_bits) & index_mask; // Pega os bits do índice
    uint32_t tag = address >> (offset_bits + index_bits);   // Pega o restante dos bits para a tag

    int lineIndex = index * cache->associativity;
    for (int i = 0; i < 100; i++) // somente teste, mudar dps
    {
        if (cacheAddress[i].validade && cacheAddress[i].tag == tag)
        {
            cache_hits++;
            return;
        }

        else
        {
            cache_misses++;
        }
    }

    cache_accesses++;

    cacheAddress->tag = tag;
    cacheAddress->validade = true;
}