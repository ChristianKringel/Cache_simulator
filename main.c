#include <stdio.h>
#include <stdbool.h> // bool lib
#include <stdlib.h>

// cache_simulator <nsets> <bsize> <assoc> <substituição> <flag_saida> arquivo_de_entrada
// 2 5 6 r 1 trace.txt
typedef struct {
    unsigned int associativity; // associatividade
    unsigned int block_size;    // tam do bloco
    unsigned int nsets;         // numero de conjuntos
    char *substitution_policy;   // politica de substituicao
    }Cache;
typedef struct {
    unsigned int tag;
    unsigned int index;
    unsigned int offset;
} CacheAddress;

CacheAddress *createCacheAddress(int tag, int index, int offset);
Cache *initializeCache(Cache *cache, const unsigned int associativity, 
const unsigned int block_size, const unsigned int nsets, const char *substitution_policy);

static bool powerOfTwo(const unsigned int value);

int main(int argc, char *argv[])
{
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
    return 0;
}

// func para testar se esta em potencia de dois
static bool powerOfTwo(const unsigned int value)
{
    return (value & (value - 1)) == 0;
}

Cache *initializeCache(Cache *cache, const unsigned int associativity, 
const unsigned int block_size, const unsigned int nsets, const char *substitution_policy)
{
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    if (cache == NULL)
    {
        printf("ERRO: Problema ao alocar memoria para a cache\n");
        return NULL;
    }
    
    if(!powerOfTwo(nsets))
    {
        printf("ERRO: o numero de conjuntos deve ser uma potencia de 2\n");
        return NULL;
    }

    if(!powerOfTwo(associativity) || (!associativity == 1))
    {
        printf("ERRO: a associatividade deve ser uma potencia de 2\n");
        return NULL;
    }

    if(!powerOfTwo(block_size))
    {
        printf("ERRO: tamanho do bloco deve ser uma potencia de 2\n");
        return NULL;
    }
    

    cache->associativity = associativity;
    cache->block_size = block_size;
    cache->nsets = nsets;
    cache->substitution_policy = substitution_policy;

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

int calculateTagBits(int indice, int offset) { return 32 - (indice + offset); }
int calculateIndexBits(int nsets) { return log2(nsets); }
int calculateOffsetBits(int block_size) { return log2(block_size); }

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