#include "k-merge.h"
#include <string.h>

// Estrutura para a Heap de Mínimos (Fila de Prioridade)
typedef struct {
    Indice registro;          // O registro atual lido do arquivo
    int indice_arquivo;       // Índice do arquivo
    FILE* ponteiro_arquivo;   // Ponteiro para o arquivo de onde o registro foi lido
} HeapNode;

// função para manter a propriedade da Heap de ter sempre o menor elemento na raiz. Isso faz com que nós possamos ter sempre o controle do menor elemento entre os 'K elementos' atuais.
void min_heapify(HeapNode heap[], int i, int tamanho_heap) {
    int menor = i;
    int esquerda = 2 * i + 1;
    int direita = 2 * i + 2;

    if (esquerda < tamanho_heap && heap[esquerda].registro.limiar < heap[menor].registro.limiar) {
        menor = esquerda;
    }
    if (direita < tamanho_heap && heap[direita].registro.limiar < heap[menor].registro.limiar) {
        menor = direita;
    }
    if (menor != i) {
        HeapNode temp = heap[i];
        heap[i] = heap[menor];
        heap[menor] = temp;
        min_heapify(heap, menor, tamanho_heap);
    }
}

// Função principal de K-Way Merge, que usa a função min_heapify para manter a ordem dos elementos na heap e escrever os registros ordenados no arquivo de saída.
int k_way_merge(const char *nomes_runs_entrada[], int num_runs, const char *nome_arquivo_saida, int k_merge) {
    
    int runs_para_mesclar = (num_runs < k_merge) ? num_runs : k_merge;
    HeapNode *heap = (HeapNode *)malloc(runs_para_mesclar * sizeof(HeapNode));
    if (heap == NULL) {
        perror("Erro de alocação de memória para a Heap");
        return -1;
    }
    
    FILE *fp_saida = NULL;
    int tamanho_heap = 0;

    // 1. Inicialização da Heap
    for (int i = 0; i < runs_para_mesclar; i++) {
        FILE *fp_entrada = fopen(nomes_runs_entrada[i], "rb"); 
        if (fp_entrada == NULL) continue;

        Indice reg_lido;
        if (fread(&reg_lido, sizeof(Indice), 1, fp_entrada) == 1) { 
            heap[tamanho_heap].registro = reg_lido;
            heap[tamanho_heap].indice_arquivo = i;
            heap[tamanho_heap].ponteiro_arquivo = fp_entrada;
            tamanho_heap++;
        } else {
            fclose(fp_entrada);
        }
    }

    for (int i = tamanho_heap / 2 - 1; i >= 0; i--) {
        min_heapify(heap, i, tamanho_heap);
    }

    fp_saida = fopen(nome_arquivo_saida, "w"); 
    if (fp_saida == NULL) {
        perror("Erro ao abrir arquivo de saída");
        free(heap);
        return -1;
    }

    // 2. Intercalação Principal
    while (tamanho_heap > 0) {
        HeapNode menor_no = heap[0];

        // Escreve o registro textual
        fprintf(fp_saida, "%s %d %d %d %ld %d\n",
                menor_no.registro.nome, menor_no.registro.limiar, menor_no.registro.largura, 
                menor_no.registro.altura, menor_no.registro.posicao, menor_no.registro.removido);

        // Tenta ler o próximo registro binário
        Indice proximo_reg;
        if (fread(&proximo_reg, sizeof(Indice), 1, menor_no.ponteiro_arquivo) == 1) {
            heap[0].registro = proximo_reg;
        } else {
            fclose(menor_no.ponteiro_arquivo);
            tamanho_heap--;
            if (tamanho_heap > 0) {
                heap[0] = heap[tamanho_heap];
            }
        }

        if (tamanho_heap > 0) {
            min_heapify(heap, 0, tamanho_heap);
        }
    }

    fclose(fp_saida);
    free(heap);

    return 0;
}