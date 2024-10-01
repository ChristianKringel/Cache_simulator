#include <stdio.h>
#include <stdbool.h> // bool lib
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <ctype.h>

// cache_simulator <nsets> <bsize> <assoc> <substituição> <flag_saida> arquivo_de_entrada
// 2 5 6 r 1 trace.txt

typedef struct
{
    unsigned int tag;
    unsigned int index;
    unsigned int offset;
    unsigned int itWasAdded;
    // unsigned int lastAccessedAddress;
    bool validade;
} CacheAddress;

typedef struct
{
    int totalMisses;
    int capacity;
    int compulsory;
    int conflict;
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

// variaveis globais
unsigned long int cache_accesses = 0;
// unsigned long int cache_hits = 0;
int contadorFifo = 0;
unsigned long int cache_misses = 0;
unsigned long int cache_writebacks = 0;
unsigned long int cache_reads = 0;
unsigned long int cache_writes = 0;
bool debug = true;

void readFile(Cache *cache, const char *file, int *missCount, unsigned long int *totalAcess, int *cache_hits);
int getTag(int address, int block_size, int num_sets);
int getIndex(int address, int block_size, int num_sets);
// void processAddress(Cache *cache, CacheAddress *cacheAddress, int adress, CacheMisses *missCount, int replacment, unsigned long int *totalAcess);
void processAddress(Cache *cache, int address, int *missCount, unsigned long int *totalAcess, int *cache_hits);
Cache *initializeCache(Cache *cache, unsigned int associativity,
                       unsigned int block_size, unsigned int nsets, char substitution_policy);
CacheAddress *freeCacheAddress(CacheAddress *cache_address);
int indexForReplace(Cache *cache, int index);
static bool powerOfTwo(const unsigned int value);
// int LRU(Cache *cache, int index);
int FIFO(Cache *cache, int index);
int RANDOM(Cache *cache, int index);

int main(int argc, char *argv[])
{
    // Cache *cache = (Cache *)malloc(sizeof(Cache));
    //  CacheAddress *cacheAdress = (CacheAddress *)malloc(sizeof(CacheAddress));
    Cache *cache;
    if (argc < 7)
    {
        printf("Uso: %s <nsets> <bsize> <assoc> <subs> <flag_saida> <arquivo_de_entrada>\n", argv[0]);
        return 1;
    }
    int nsets = 0;
    int block_size = 0;
    int associativity = 0;
    char substitution_policy;
    int exit_flag = 0;

    nsets = atoi(argv[1]);
    block_size = atoi(argv[2]);
    associativity = atoi(argv[3]);
    substitution_policy = toupper(argv[4][0]);
    exit_flag = atoi(argv[5]);
    char *trace_file = argv[6];
    /*
    cache->nsets = atoi(argv[1]); // atoi converte string para inteiro
    cache->block_size = atoi(argv[2]);
    cache->associativity = atoi(argv[3]);
    cache->substitution_policy = toupper(argv[4][0]);
    int exit_flag = atoi(argv[5]);
    char *trace_file = argv[6];
    */
    /* testando os parametros passados na chamada do codigo*/
    if (debug)
    {
        printf("nsets: %d\n", nsets);
        printf("bsize: %d\n", block_size);
        printf("assoc: %d\n", associativity);
        printf("sub_policy: %c\n", substitution_policy);
        printf("exit_flag: %d\n", exit_flag);
        printf("trace_file: %s\n", trace_file);
    }
    int missCount = 0;
    int cacheHits = 0;
    unsigned long int totalAcess = 0;
    cache = initializeCache(cache, associativity, block_size, nsets, substitution_policy);

    printf("\n\n");
    // void readFile(Cache *cache, CacheAddress *CacheAddress, const char *file, int *missCount, int replacment, unsigned long int *totalAcess, int *cache_hits)
    readFile(cache, trace_file, &missCount, &totalAcess, &cacheHits);

    float missRate = (float)cache->misses->totalMisses / cache->totalAccesses;
    float hitRate = (float)cacheHits / cache->totalAccesses;
    printf("Total de acessos: %lu\n", cache->totalAccesses);
    printf("Total de hits: %.4f\n", hitRate);
    printf("Total de misses: %.4f\n", missRate);
    return 0;
}

// func para testar se esta em potencia de dois
static bool powerOfTwo(const unsigned int value)
{
    return (value & (value - 1)) == 0;
}

Cache *initializeCache(Cache *cache, unsigned int associativity, unsigned int block_size, unsigned int nsets, char substitution_policy)
{
    cache = (Cache *)malloc(sizeof(Cache));
    if (cache == NULL)
    {
        printf("ERRO: Problema ao alocar memoria para a cache\n");
        return NULL;
    }

    cache->associativity = associativity;
    cache->block_size = block_size;
    cache->nsets = nsets;
    cache->substitution_policy = substitution_policy;
    cache->misses->totalMisses = 0;
    cache->misses->capacity = 0;
    cache->misses->compulsory = 0;

    if (!powerOfTwo(nsets))
    {
        printf("ERRO: o numero de conjuntos deve ser uma potencia de 2\n");
        free(cache); // Liberar memória antes de retornar
        return NULL;
    }

    if (!powerOfTwo(block_size))
    {
        printf("ERRO: tamanho do bloco deve ser uma potencia de 2\n");
        free(cache); // Liberar memória antes de retornar
        return NULL;
    }

    // Alocar memória para cache_address
    cache->cache_address = (CacheAddress *)malloc(nsets * associativity * sizeof(CacheAddress));
    if (cache->cache_address == NULL)
    {
        printf("ERRO: Problema ao alocar memoria para os endereços da cache\n");
        free(cache); // Liberar memória antes de retornar
        return NULL;
    }

    // Inicializar a cache
    for (int i = 0; i < nsets * associativity; i++)
    {
        cache->cache_address[i].tag = 0;
        cache->cache_address[i].validade = false;
        cache->cache_address[i].itWasAdded = 0;
    }

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

void readFile(Cache *cache, const char *file, int *missCount, unsigned long int *totalAcess, int *cache_hits)
{
    FILE *fp = fopen(file, "rb");
    int endereco;

    if (!fp)
    {
        printf("ERRO: Problema ao abrir o arquivo %s\n", file);
        return;
    }

    while (fread(&endereco, sizeof(int), 1, fp) == 1)
    {
        if (fread(&endereco, sizeof(int), 1, fp) != 1)
        {
            printf("ERRO: Não foi possível ler o endereço corretamente\n");
        }

        endereco = __builtin_bswap32(endereco);
        printf("Endereco: %d\n", endereco);
        processAddress(cache, endereco, missCount, totalAcess, cache_hits);
    }
    // printf("\nTotal de acessos: %lu\n", cache_accesses);

    fclose(fp);
}

void processAddress(Cache *cache, int address, int *missCount, unsigned long int *totalAcess, int *cache_hits)
{
    // int index = getIndex(address, cache->block_size, cache->nsets);
    // int tag = getTag(address, cache->block_size, cache->nsets);

    // calculo padrao usando log
    unsigned int offset_bits = log2(cache->block_size);
    unsigned int index_bits = log2(cache->nsets);

    uint32_t tag = address >> (offset_bits + index_bits);
    int medio = pow(2, index_bits);
    uint32_t index = (address >> offset_bits) & (medio - 1);
    // Mascara, fazendo um AND bit a bit
    /*
    unsigned int offset_mask = (1 << offset_bits) - 1;
    unsigned int index_mask = (1 << index_bits) - 1;

    // Formata com base no padrao mostrado no codigo do professor
    uint32_t offset = address & offset_mask;
    uint32_t index = (address >> offset_bits) & index_mask;
    uint32_t tag = address >> (offset_bits + index_bits);
    */
    /*
        tag = endereço >> (n_bits_offset + n_bits_indice);
        indice = (endereço >> n_bits_offset) & (2^n_bits_indice -1)
        */

    // printf("\nTag: %d\n", tag);
    // printf("Index: %d\n", index);
    // printf("Offset: %d\n", offset);
    int lineIndex = index * cache->associativity;
    int indexTest = index % cache->nsets;

    // tag = offset;
    //  inicio da cache
    // CacheAddress *set = &cacheAddress[index * cache->associativity];

    // Ve se eh hit
    bool isHit = false;

    for (int i = lineIndex; i < lineIndex + cache->associativity; i++)
    {
        if (cache->cache_address[i].validade && cache->cache_address[i].tag == tag)
        {
            isHit = true;
            (*cache_hits)++;
            break;
        }
    }

    if (!isHit)
    {
        // se n eh hit conta como miss
        //(*missCount)++;
        cache->misses->totalMisses++;

        // Procura por uma linha vazia no conjunto
        int emptyLine = -1;
        for (int i = lineIndex; i < lineIndex + cache->associativity; i++)
        {
            if (!cache->cache_address[i].validade)
            {
                emptyLine = i;
                break;
            }
        }

        // se nao tiver linha vazia usa o replace
        int indexReplace = (emptyLine != -1) ? emptyLine : indexForReplace(cache, index);
        if (cache->substitution_policy == 'F')
        {
            cache->cache_address[indexReplace].itWasAdded = cache->totalAccesses;
        }

        else if (cache->substitution_policy == 'L')
        {
            // cache->cache_address[indexReplace].counterFifo = contadorFifo;
        }
        // Substitui a linha
        cache->cache_address[indexReplace].tag = tag;
        cache->cache_address[indexReplace].validade = true;
    }

    // Incrementa o total de acessos
    cache->totalAccesses++;
}

int indexForReplace(Cache *cache, int index)
{
    // printf("\n\nPOLITICA DE SUBSTITUICAO:%c\n", cache->substitution_policy);
    if (cache->substitution_policy == 'L')
    {
        // return LRU(cache, index);

        return 0;
    }
    else if (cache->substitution_policy == 'R')
    {
        return RANDOM(cache, index);
    }
    else if (cache->substitution_policy == 'F')
    {
        contadorFifo++;
        return FIFO(cache, index);
        // return 0;
    }
    else
    {
        printf("Politica de substituicao invalida.\n");
        exit(EXIT_FAILURE);
    }
}

/*
int LRU(Cache *cache, int index)
{
    return 0;
}
*/

int RANDOM(Cache *cache, int index)
{
    int indexReplace = index * cache->associativity;
    return indexReplace + rand() % cache->associativity;
}
/**/
int FIFO(Cache *cache, int index)
{
    int indexReplace = index * cache->associativity;
    int indexFifo = indexReplace;
    int menor = cache->cache_address[indexReplace].itWasAdded;
    for (int i = indexReplace; i < indexReplace + cache->associativity; i++)
    {
        if (cache->cache_address[i].itWasAdded < menor)
        {
            menor = cache->cache_address[i].itWasAdded;
            indexFifo = i;
        }
    }
    return indexFifo;
}

void printFlag1(Cache *cache, int missCount, unsigned long int totalAcess, int cache_hits)
{
    printf("Total de acessos: %lu\n", totalAcess);
    printf("Total de hits: %d\n", cache_hits);
    printf("Total de misses: %d\n", missCount);
    printf("Taxa de hits: %.4f\n", (float)cache_hits / totalAcess);
    printf("Taxa de misses: %.4f\n", (float)missCount / totalAcess);
}

void printFlag2(Cache *cache, int missCount, unsigned long int totalAcess, int cache_hits)
{
    printf("%lu %.4f %.4f\n", totalAcess, (float)cache_hits / totalAcess, (float)missCount / totalAcess);
}