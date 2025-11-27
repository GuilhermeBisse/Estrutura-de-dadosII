#ifndef BTREE_H
#define BTREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORDEM 3
#define MAX_CHAVES (ORDEM - 1)

#define ARQUIVO_ARVORE "banco.btree"
#define ARQUIVO_DADOS "banco.dat"

typedef struct {
    char nome[256];
    int limiar;
    int largura;
    int altura;
    long posicao_dados;
    int removido;        // 0 = Ativo, 1 = Removido (Lazy Deletion)
} Registro;

typedef struct {
    int num_chaves;
    int folha;
    Registro chaves[MAX_CHAVES];
    long filhos[ORDEM];
    long meu_offset;
} Pagina;

extern Pagina *raiz_virtual; 
extern int raiz_modificada;

void inicializar_arvore();
void finalizar_arvore();

void inserir_chave(Registro reg);
int buscar_chave(char *nome, int limiar, Registro *reg_retorno);
int remover_chave(char *nome, int limiar); 

void percorrer_in_order();
void reconstruir_banco_completo(); // Nova função de compactação

#endif