#include "cache.h"


// Funcao que inicializa toda a cache, conjuntos, etc
Cache *initializeCache(int numSets, int blockSize, int associativity, char policyChar)
{
    Cache *cache = malloc(sizeof(Cache));
    srand(time(NULL)); // Inicializa o gerador de números aleatórios
    if (cache == NULL)
    {
        printf("ERRO: Falha na alocação de memória para a cache\n");
        return NULL;
    }

    cache->numSets = numSets;
    cache->blockSize = blockSize;
    cache->associativity = associativity;
    cache->sets = malloc(numSets * sizeof(CacheSet));
    if (cache->sets == NULL)
    {
        printf("ERRO: Falha na alocação de memória para os conjuntos\n");
        free(cache);
        return NULL;
    }

    for (int i = 0; i < numSets; i++)
    {
        cache->sets[i].blocks = malloc(associativity * sizeof(CacheBlock));
        cache->sets[i].numBlocks = associativity;
        if (cache->sets[i].blocks == NULL)
        {
            printf("ERRO: Falha na alocação de memória para os blocos\n");
            for (int j = 0; j < i; j++)
            {
                free(cache->sets[j].blocks);
            }
            free(cache->sets);
            free(cache);
            return NULL;
        }
        for (int j = 0; j < associativity; j++)
        {
            // inicializa os blocos
            cache->sets[i].blocks[j].valid = false;
            cache->sets[i].blocks[j].tag = 0;
            cache->sets[i].blocks[j].lastAccess = 0;
            cache->sets[i].blocks[j].insertionTime = 0;
        }
    }

    cache->stats = (CacheStats){0, 0, 0, 0, 0, 0};

    switch (policyChar)
    {
    case 'L':
        cache->replacementPolicy = getLRUIndex;
        break;
    case 'F':
        cache->replacementPolicy = getFIFOIndex;
        break;
    case 'R':
        cache->replacementPolicy = getRandomIndex;
        break;
    default:
        printf("ERRO: Política de substituição inválida\n");
        freeCache(cache);
        return NULL;
    }

    return cache;
}

// Func pra dar free na cache
void freeCache(Cache *cache)
{
    if (cache != NULL)
    {
        for (int i = 0; i < cache->numSets; i++)
        {
            free(cache->sets[i].blocks);
        }
        free(cache->sets);
        free(cache);
    }
}

// le o arquivo e processa os enderecos
void readFile(Cache *cache, const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("ERRO: Não foi possível abrir o arquivo %s\n", filename);
        return;
    }

    uint32_t address;
    while (fread(&address, sizeof(uint32_t), 1, file) == 1)
    {
        address = __builtin_bswap32(address); // converte para big endian
        processAddress(cache, address);       // processa o endereco
    }

    fclose(file);
}

// Func que processa o endereco
void processAddress(Cache *cache, uint32_t address)
{
    int offsetBits = log2(cache->blockSize);
    int indexBits = log2(cache->numSets);
    uint32_t tag = address >> (offsetBits + indexBits);
    uint32_t index = (address >> offsetBits) & ((1 << indexBits) - 1);

    CacheSet *set = &cache->sets[index];
    bool hit = false;
    int emptySlot = -1;

    // Procura o bloco na cache
    for (int i = 0; i < cache->associativity; i++)
    {
        if (set->blocks[i].valid && set->blocks[i].tag == tag) // se for valido e a tag igual eh hit
        {
            hit = true;
            set->blocks[i].lastAccess = cache->stats.totalAccesses;
            break;
        }
        if (!set->blocks[i].valid && emptySlot == -1)
            emptySlot = i;
    }

    if (hit) // se for hit incrementa o contador dos hits
        cache->stats.hits++;
    else // se nao, ve qual o tipo do miss 
    {
        cache->stats.misses++; // incrementa o contador total de misses 
        int replaceIndex;

        if (emptySlot != -1) // se tiver slot vazio, eh miss compulsorio
        {
            replaceIndex = emptySlot;
            cache->stats.compulsory++;
        }
        else // se nao, ve qual o outro tipo de miss
        {
            replaceIndex = cache->replacementPolicy(set, cache->stats.totalAccesses, cache->associativity);

            if (cacheIsFull(cache)) // se estiver full eh miss de capacidade
                cache->stats.capacity++;
            else                    // se nao eh miss de conflito
                cache->stats.conflict++;
        }

        // substitui o bloco
        set->blocks[replaceIndex].valid = true;
        set->blocks[replaceIndex].tag = tag;
        set->blocks[replaceIndex].lastAccess = cache->stats.totalAccesses;
        set->blocks[replaceIndex].insertionTime = cache->stats.totalAccesses;
    }

    cache->stats.totalAccesses++; // incrementa o total de acessos
}

// Func pra calcular o indice pro LRU
int getLRUIndex(CacheSet *set, int currentTime, int associativity)
{
    int lruIndex = 0;
    int oldestTime = set->blocks[0].lastAccess;

    for (int i = 1; i < associativity; i++)
    {
        if (!set->blocks[i].valid) // se tiver bloco invalido, retorna o indice 
            return i;

        if (set->blocks[i].lastAccess < oldestTime) // se nao, ve qual o bloco usado mais antigamente
        {
            oldestTime = set->blocks[i].lastAccess; // atualiza o tempo
            lruIndex = i;                           // atualiza o indice
        }
    }

    return lruIndex; // retorna o indice do bloco menos usado 
}

// essas duas func fazem basicamente a mesma coisa, mudando apenas o criterio de escolha do bloco

// Func pra calcular o indice pro FIFO
int getFIFOIndex(CacheSet *set, int currentTime, int associativity)
{
    int fifoIndex = 0;
    int oldestTime = set->blocks[0].insertionTime;

    for (int i = 1; i < associativity; i++)
    {
        if (!set->blocks[i].valid) // se tiver bloco invalido, retorna o indices
            return i;

        if (set->blocks[i].insertionTime < oldestTime) // se nao, ve qual o bloco mais antigo
        {
            oldestTime = set->blocks[i].insertionTime;
            fifoIndex = i;
        }
    }

    return fifoIndex; // retorna o indice do bloco mais antigo
}

// Func pra calcular o indice pro RANDOM
int getRandomIndex(CacheSet *set, int currentTime, int associativity)
{ return rand() % associativity; }

// Func pra printar os stats conforme a flag 
void printStats(Cache *cache, int flag)
{
    float hitRate = (float)cache->stats.hits / cache->stats.totalAccesses;
    float missRate = (float)cache->stats.misses / cache->stats.totalAccesses;
    float compulsoryRate = (float)cache->stats.compulsory / cache->stats.misses;
    float capacityRate = (float)cache->stats.capacity / cache->stats.misses;
    float conflictRate = (float)cache->stats.conflict / cache->stats.misses;

    if (flag == 0)
    {
        printf("Total de acessos: %d\n", cache->stats.totalAccesses);
        printf("Total de hits: %d\n", cache->stats.hits);
        printf("Total de misses: %d\n", cache->stats.misses);
        printf("Taxa de hits: %.4f\n", hitRate);
        printf("Taxa de misses: %.4f\n", missRate);
        printf("Taxa de misses compulsórios: %.2f\n", compulsoryRate);
        printf("Taxa de misses de capacidade: %.2f\n", capacityRate);
        printf("Taxa de misses de conflito: %.2f\n", conflictRate);
    }
    else if (flag == 1)
    {
        printf("%d %.4f %.4f %.2f %.2f %.2f\n", cache->stats.totalAccesses, hitRate, missRate, compulsoryRate, capacityRate, conflictRate);
    }
}

// func pra ver se eh potencia de dois 
bool isPowerOfTwo(unsigned int value)
{ return (value != 0) && ((value & (value - 1)) == 0); }

//Func que percorre a cache pra ver se esta cheia 
bool cacheIsFull(Cache *cache)
{
    for (int i = 0; i < cache->numSets; i++) // percorre todos os conjuntos
    {
        for (int j = 0; j < cache->associativity; j++) // percorre todos os blocos de cada conjunto
        {
            if (!cache->sets[i].blocks[j].valid) // se tiver algum bloco invalido, a cache nao esta cheia
                return false;
            
        }
    }
    return true; // se nao achou nenhum bloco invalido, entao todos sao validos, entao esta cheia
}