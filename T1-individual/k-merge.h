// k-merge.h

#ifndef K_MERGE_H
#define K_MERGE_H

#include <stdio.h>
#include <stdlib.h>

#define K_MERGE 10 // Grau de intercalação (K)
#define MAX_RUNS 50// Máximo de runs que o main.c pode gerar ou manipular.
#define RECORDS_PER_RUN 1000

#ifndef MAX
#define MAX 256
#endif

typedef struct {
    char nome[MAX];
    int limiar;
    int largura, altura;
    long posicao;
    int removido;
} Indice;


/**
 * Intercala runs binários (com Min-Heap) em um arquivo de saída de texto.
 * Assume que os arquivos de entrada jรก ESTÃO ordenados.
 * @param nomes_runs_entrada Array com os nomes dos runs (arquivos temporários) binários.
 * @param num_runs O número total de runs a serem mesclados.
 * @param nome_arquivo_saida Nome do arquivo final ordenado (banco.idx.tmp).
 * @param k_merge O grau de intercalação (K).
 * @return 0 em sucesso, -1 em falha.
 */

int k_way_merge(const char *nomes_runs_entrada[], int num_runs, const char *nome_arquivo_saida, int k_merge);

#endif // K_MERGE_H