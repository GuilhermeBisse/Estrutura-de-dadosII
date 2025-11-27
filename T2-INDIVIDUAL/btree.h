#ifndef BTREE_H
#define BTREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Definições de estrutura da Árvore B */
#define ORDEM 3                 // Grau da árvore: cada página tem no máximo 3 filhos
#define MAX_CHAVES (ORDEM - 1)  // Cada página armazena até 2 chaves

/* Persistência */
#define ARQUIVO_ARVORE "banco.btree" // Arquivo binário contendo a estrutura da árvore (índice)
#define ARQUIVO_DADOS "banco.dat"    // Arquivo contendo dados brutos (se houver separação)

/* Payload: Dados armazenados na árvore */
typedef struct {
    char nome[256];
    int limiar;
    int largura;
    int altura;
    long posicao_dados;      // Link para o arquivo de dados brutos (se aplicável)
    int removido;            // 0 = Ativo, 1 = Removido (Remoção Lógica/Lazy Deletion: o dado persiste até a reconstrução)
} Registro;

/* Nó da Árvore (Página) para manipulação em disco */
typedef struct {
    int num_chaves;          // Quantidade atual de chaves na página
    int folha;               // Flag: 1 se for folha, 0 se for nó interno
    Registro chaves[MAX_CHAVES];
    long filhos[ORDEM];      // Armazena OFFSETS (posições no arquivo), não ponteiros de memória
    long meu_offset;         // Posição desta própria página no arquivo (facilita atualizações)
} Pagina;

/* Variáveis globais para controle do cache em memória */
extern Pagina *raiz_virtual; 
extern int raiz_modificada;

/* Gerenciamento de Arquivo */
void inicializar_arvore();
void finalizar_arvore();

/* Operações CRUD */
void inserir_chave(Registro reg);
// Busca baseada em chave composta (nome + limiar)
int buscar_chave(char *nome, int limiar, Registro *reg_retorno);
int remover_chave(char *nome, int limiar); 

/* Utilitários e Manutenção */
void percorrer_in_order();
void reconstruir_banco_completo(); // Realiza a "coleta de lixo": remove fisicamente registros marcados como excluídos

#endif
