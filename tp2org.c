#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TAM_PALAVRA 4

typedef struct {
    int cache1_hits;
    int cache1_misses;
    int cache2_hits;
    int cache2_misses;
    int cache3_hits;
    int cache3_misses;
} EstatisticasCache;
// Estrutura que representa um bloco de memória ou linha de cache

typedef struct {
    int palavras[TAM_PALAVRA]; // 4 palavras por bloco/linha
    int tag;                   // Tag para identificar o bloco/linha
    int contador_acessos;       // Usado para LFU
    int ultima_vez_acessada;    // Usado para LRU
} BlocoMemoria;

// Estruturas para as caches de 3 níveis e a memória RAM
typedef struct {
    BlocoMemoria *linhas;
    int tamanho;                // Número de linhas na cache
} Cache;

typedef struct {
    BlocoMemoria *blocos;
    int tamanho;                // Número de blocos na memória
} MemoriaRAM;

// Função para inicializar uma cache
Cache inicializa_cache(int tamanho_cache) {
    Cache cache;
    cache.tamanho = tamanho_cache;
    cache.linhas = (BlocoMemoria *)malloc(tamanho_cache * sizeof(BlocoMemoria));

    // Inicializando cada linha da cache
    for (int i = 0; i < tamanho_cache; i++) {
        cache.linhas[i].tag = -1;  // Tag inválida para representar linha vazia
        cache.linhas[i].contador_acessos = 0;
        cache.linhas[i].ultima_vez_acessada = 0;
    }

    return cache;
}

// Função para inicializar a memória RAM
MemoriaRAM inicializa_ram(int tamanho_ram) {
    MemoriaRAM ram;
    ram.tamanho = tamanho_ram;
    ram.blocos = (BlocoMemoria *)malloc(tamanho_ram * sizeof(BlocoMemoria));

    // Inicializando os blocos da RAM
    for (int i = 0; i < tamanho_ram; i++) {
        ram.blocos[i].tag = i; // Cada bloco na RAM possui um tag único (índice)
    }

    return ram;
}

// Função para buscar um bloco na cache
int busca_na_cache(Cache *cache, int tag, int tempo_atual) {
    for (int i = 0; i < cache->tamanho; i++) {
        if (cache->linhas[i].tag == tag) {
            cache->linhas[i].ultima_vez_acessada = tempo_atual;  // Atualiza para LRU
            return i;  // Encontrou o bloco
        }
    }
    return -1;  // Não encontrou
}

// Função para substituir um bloco na cache usando LRU
void substitui_na_cache_lru(Cache *cache, BlocoMemoria bloco, int tempo_atual) {
    int indice_lru = 0;
    int menor_tempo = cache->linhas[0].ultima_vez_acessada;

    // Encontra a linha menos recentemente usada (LRU)
    for (int i = 1; i < cache->tamanho; i++) {
        if (cache->linhas[i].ultima_vez_acessada < menor_tempo) {
            menor_tempo = cache->linhas[i].ultima_vez_acessada;
            indice_lru = i;
        }
    }

    // Substitui o bloco na cache
    cache->linhas[indice_lru] = bloco;
    cache->linhas[indice_lru].ultima_vez_acessada = tempo_atual;
}

int UCM(Cache *cache1, Cache *cache2, Cache *cache3, MemoriaRAM *ram, int endereco, int tempo_atual, EstatisticasCache *estatisticas) {
    int tag = endereco / TAM_PALAVRA;  // Calcula a tag baseada no endereço

    // Busca na cache nível 1
    int indice = busca_na_cache(cache1, tag, tempo_atual);
    if (indice != -1) {
        estatisticas->cache1_hits++;
        return cache1->linhas[indice].palavras[endereco % TAM_PALAVRA];  // Cache hit
    }
    estatisticas->cache1_misses++;

    // Busca na cache nível 2
    indice = busca_na_cache(cache2, tag, tempo_atual);
    if (indice != -1) {
        estatisticas->cache2_hits++;
        // Substitui na cache nível 1
        substitui_na_cache_lru(cache1, cache2->linhas[indice], tempo_atual);
        return cache2->linhas[indice].palavras[endereco % TAM_PALAVRA];  // Cache hit
    }
    estatisticas->cache2_misses++;

    // Busca na cache nível 3
    indice = busca_na_cache(cache3, tag, tempo_atual);
    if (indice != -1) {
        estatisticas->cache3_hits++;
        // Substitui na cache nível 2
        substitui_na_cache_lru(cache2, cache3->linhas[indice], tempo_atual);
        // E na cache nível 1
        substitui_na_cache_lru(cache1, cache3->linhas[indice], tempo_atual);
        return cache3->linhas[indice].palavras[endereco % TAM_PALAVRA];  // Cache hit
    }
    estatisticas->cache3_misses++;

    // Busca na memória RAM (cache miss em todas as caches)
    BlocoMemoria bloco_ram = ram->blocos[tag];
    substitui_na_cache_lru(cache3, bloco_ram, tempo_atual);
    substitui_na_cache_lru(cache2, bloco_ram, tempo_atual);
    substitui_na_cache_lru(cache1, bloco_ram, tempo_atual);

    return bloco_ram.palavras[endereco % TAM_PALAVRA];  // Retorna o dado da RAM
}

void simular(Cache *cache1, Cache *cache2, Cache *cache3, MemoriaRAM *ram, int num_acessos, int max_endereco) {
    EstatisticasCache estatisticas = {0, 0, 0, 0, 0, 0};  // Inicializa as estatísticas

    srand(time(NULL));  // Seed para geração de números aleatórios

    for (int tempo_atual = 0; tempo_atual < num_acessos; tempo_atual++) {
        int endereco = rand() % max_endereco;  // Gera um endereço aleatório
        UCM(cache1, cache2, cache3, ram, endereco, tempo_atual, &estatisticas);
    }

    // Exibe os resultados
    printf("Resultados da Simulação:\n");
    printf("Cache Nível 1 - Hits: %d, Misses: %d\n", estatisticas.cache1_hits, estatisticas.cache1_misses);
    printf("Cache Nível 2 - Hits: %d, Misses: %d\n", estatisticas.cache2_hits, estatisticas.cache2_misses);
    printf("Cache Nível 3 - Hits: %d, Misses: %d\n", estatisticas.cache3_hits, estatisticas.cache3_misses);
}


int main() {
    // Parâmetros de simulação
    int tamanho_ram = 1024;  // Tamanho da RAM em blocos
    int tamanho_cache1 = 16;  // Tamanho da Cache Nível 1
    int tamanho_cache2 = 32;  // Tamanho da Cache Nível 2
    int tamanho_cache3 = 64;  // Tamanho da Cache Nível 3
    int num_acessos = 1000;   // Número de acessos simulados
    int max_endereco = tamanho_ram * TAM_PALAVRA;  // Máximo de endereços

    // Inicializa as memórias
    MemoriaRAM ram = inicializa_ram(tamanho_ram);
    Cache cache1 = inicializa_cache(tamanho_cache1);
    Cache cache2 = inicializa_cache(tamanho_cache2);
    Cache cache3 = inicializa_cache(tamanho_cache3);

    // Simula os acessos à memória
    simular(&cache1, &cache2, &cache3, &ram, num_acessos, max_endereco);

    // Libera a memória alocada
    free(ram.blocos);
    free(cache1.linhas);
    free(cache2.linhas);
    free(cache3.linhas);

    return 0;
}
