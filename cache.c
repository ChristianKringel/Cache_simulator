#include "cache.h"
// Codigo .c que contempla as funcoes declaradas no arquivo cache.h

/**
 * Funcao que inicializa toda a cache, conjuntos, etc
 * Tambem seta as politicas de substituicao
 */
Cache *createCacheWithPolicy(int numSets, int blockSize, int associativity, char policyChar)
{
    Cache *cache = malloc(sizeof(Cache));
    srand(time(NULL)); // Inicializa o gerador de números aleatórios
    if (cache == NULL)
    {
        printf("ERRO: Falha na alocação de memória para a cache\n");
        return NULL;
    }

    // seta os dados da cache na struct
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
        cache->sets[i].blocks = malloc(associativity * sizeof(CacheBlock)); // aloca os blocos de cada conjunto com base na associatividade
        cache->sets[i].numBlocks = associativity; // seta o numero de blocos
        if (cache->sets[i].blocks == NULL)
        {
            printf("ERRO: Falha na alocação de memória para os blocos\n");
            for (int j = 0; j < i; j++)
            {
                free(cache->sets[j].blocks);
            }
            // se teve algum erro pra alocar os blocos, da free em tudo e retorna NULL
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
    // ve qual a politica de substituicao e seta a funcao correspondente
    // faz uso do ponteiro pra funcao
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

/**
 * Func pra dar free na cache
 */
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

/**  
 * le o arquivo e processa os enderecos
 */ 
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

/** 
 * Func que processa o endereco
 */
void processAddress(Cache *cache, const uint32_t address)
{
    // calculo dos bits de offset e index com log2
    int offsetBits = log2(cache->blockSize);
    int indexBits = log2(cache->numSets);
    // Calculo da tag com base no pseudo codigo fornecido
    uint32_t tag = address >> (offsetBits + indexBits);
    uint32_t index = (address >> offsetBits) & ((1 << indexBits) - 1);

    int emptySlot = -1;

    if (isHit(cache, tag, index, &emptySlot) ) // se for hit incrementa o contador dos hits
        cache->stats.hits++;
    else // se nao, trata a falta
        handleCacheMiss(cache, tag, index, &emptySlot);

    cache->stats.totalAccesses++; // incrementa o total de acessos
}

/**
 * Funcao para testar se o endereco eh um hit
 * Retorna true caso seja, e false caso contrario 
 */
bool isHit(Cache *cache, uint32_t tag, uint32_t index, int *emptySlot)
{
    CacheSet *set = &cache->sets[index]; // Pega o conjunto correspondente, com base no indice

    for (int i = 0; i < cache->associativity; i++)
    {
        if (set->blocks[i].valid && set->blocks[i].tag == tag) // compara pra ver se eh valido e a tag eh igual a tag calculada
        {
            set->blocks[i].lastAccess = cache->stats.totalAccesses; // Atualiza o tempo de acesso do bloco (LRU)
            return true;                                            // Retorna true se for hit
        }
        if (!set->blocks[i].valid && (*emptySlot) == -1)
            *emptySlot = i;  // Atualiza o indice do bloco invalido
    }

    return false; // Se nao achou nenhum bloco valido com a tag, entao nao eh hit
}

/** 
 * Funcao que trata o miss
*/
void handleCacheMiss(Cache *cache, uint32_t tag, uint32_t index, int *emptySlot)
{
    cache->stats.misses++;  // Incrementa o contador total de misses

    CacheSet *set = &cache->sets[index]; // Pega o conjunto correspondente, com base no indice
    int replaceIndex; // Indice do bloco que sera substituido

    if (*emptySlot != -1) // Se tiver um bloco invalido, usa ele (foi calculado no isHit)
    {
        replaceIndex = *emptySlot; // entao o bloco a ser substituido eh o bloco invalido
        cache->stats.compulsory++; // Incrementa o contador de misses compulsorios
    }
    else
    {
        replaceIndex = cache->replacementPolicy(set, cache->associativity); // Calcula o indice do bloco a ser substituido

        if (cacheIsFull(cache)) // Se a cache estiver cheia, eh um miss de capacidade
            cache->stats.capacity++;
        else                    // Se nao, eh um miss de conflito
            cache->stats.conflict++;
    }

    // Atualiza o bloco que sera substituido
    set->blocks[replaceIndex].valid = true;
    set->blocks[replaceIndex].tag = tag;
    set->blocks[replaceIndex].lastAccess = cache->stats.totalAccesses;
    set->blocks[replaceIndex].insertionTime = cache->stats.totalAccesses;
}

/**
 * Func pra calcular o indice pro LRU
 */
int getLRUIndex(CacheSet *set, int associativity)
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

/**
 * Func pra calcular o indice pro FIFO
 */
int getFIFOIndex(CacheSet *set, int associativity)
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

/** 
 * Func pra calcular o indice pro RANDOM
 */
int getRandomIndex(CacheSet *set, int associativity)
{ return rand() % associativity; }

/** 
 * Func pra printar os stats conforme a flag 
 */
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
        printf("Taxa de misses compulsorios: %.2f\n", compulsoryRate);
        printf("Taxa de misses de capacidade: %.2f\n", capacityRate);
        printf("Taxa de misses de conflito: %.2f\n", conflictRate);
    }
    else if (flag == 1)
    {
        printf("%d %.4f %.4f %.2f %.2f %.2f\n", cache->stats.totalAccesses, hitRate, missRate, compulsoryRate, capacityRate, conflictRate);
    }
}

/** 
 * func pra ver se eh potencia de dois 
 */
bool isPowerOfTwo(const unsigned int value)
{ return (value != 0) && ((value & (value - 1)) == 0); }

/** 
 * Func que percorre a cache pra ver se esta cheia 
 */
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