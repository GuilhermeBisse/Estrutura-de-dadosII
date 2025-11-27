#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 256
#include "btree.h"


void liberar_matriz(int **matriz, int altura) {
    if (matriz == NULL) return;
    for (int i = 0; i < altura; i++) {
        if (matriz[i] != NULL) {
            free(matriz[i]);
        }
    }
    free(matriz);
}

int **lerPGM(char *arquivo, int *largura, int *altura, int *maxval) {
    FILE *f = fopen(arquivo, "r");
    if (!f) {
        perror("Erro ao abrir arquivo PGM");
        return NULL;
    }

    char tipo[3];
    if (fscanf(f, "%2s", tipo) != 1 || strcmp(tipo, "P2") != 0) {
        fprintf(stderr, "Tipo de arquivo PGM inválido ou não é P2.\n");
        fclose(f);
        return NULL;
    }

    if (fscanf(f, "%d %d %d", largura, altura, maxval) != 3) {
        fprintf(stderr, "Erro ao ler largura, altura ou maxval.\n");
        fclose(f);
        return NULL;
    }

    int **img = malloc((*altura) * sizeof(int *));
    if (!img) {
        perror("Erro de alocação de memória (linhas)");
        fclose(f);
        return NULL;
    }

    for (int i = 0; i < *altura; i++) {
        img[i] = malloc((*largura) * sizeof(int));
        if (!img[i]) {
            perror("Erro de alocação de memória (colunas)");
            liberar_matriz(img, i);
            fclose(f);
            return NULL;
        }

        for (int j = 0; j < *largura; j++) {
            if (fscanf(f, "%d", &img[i][j]) != 1) {
                fprintf(stderr, "Erro de leitura de pixel em linha %d, coluna %d.\n", i, j);
                liberar_matriz(img, *altura);
                fclose(f);
                return NULL;
            }
        }
    }

    fclose(f);
    return img;
}

void salvarPGM(char *arquivo, int **img, int largura, int altura) {
    FILE *f = fopen(arquivo, "w");
    if (!f) {
        perror("Erro ao criar arquivo de saída PGM");
        return;
    }

    fprintf(f, "P2\n%d %d\n255\n", largura, altura);

    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++)
            fprintf(f, "%d ", img[i][j] * 255);
        fprintf(f, "\n");
    }

    fclose(f);
}

void binarizar(int **img, int largura, int altura, int limiar) {
    for (int i = 0; i < altura; i++)
        for (int j = 0; j < largura; j++)
            img[i][j] = (img[i][j] > limiar) ? 1 : 0;
}

void comprimir(int **img, int largura, int altura, FILE *out) {
    int atual = img[0][0];
    long cont = 0; 
    
    fprintf(out, "%d ", atual); 

    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++) {
            if (img[i][j] == atual) {
                cont++;
            } else {
                fprintf(out, "%ld ", cont); 
                cont = 1; 
                atual = img[i][j]; 
            }
        }
    }
    fprintf(out, "%ld\n", cont); 
}

int **descomprimir(FILE *in, int largura, int altura) {
    int primeiro;
    if (fscanf(in, "%d", &primeiro) != 1) return NULL; 

    int total = largura * altura;
    int *valores = malloc(total * sizeof(int));
    if (!valores) { perror("Erro de alocação em descomprimir"); return NULL; }

    int pos = 0, atual = primeiro;
    
    while (pos < total) {
        long qtd;
        if (fscanf(in, "%ld", &qtd) != 1) { 
            if (pos < total) fprintf(stderr, "Aviso: Arquivo de dados truncado (pos %d de %d).\n", pos, total);
            break;
        }

        for (long i = 0; i < qtd && pos < total; i++) {
            valores[pos++] = atual;
        }
        
        atual = 1 - atual;
    }

    int **img = malloc(altura * sizeof(int *));
    if (!img) { free(valores); perror("Erro de alocação (matriz)"); return NULL; }
    
    pos = 0;
    for (int i = 0; i < altura; i++) {
        img[i] = malloc(largura * sizeof(int));
        if (!img[i]) { liberar_matriz(img, i); free(valores); perror("Erro de alocação (linhas)"); return NULL; }

        for (int j = 0; j < largura; j++)
            img[i][j] = valores[pos++];
    }

    free(valores);
    return img;
}
void adicionar_multiplos() {
    char arquivo[256], nome[256], linha_limiares[256];
    
    printf("Caminho PGM: "); if (scanf("%s", arquivo) != 1) return;
    printf("Nome base: "); if (scanf("%s", nome) != 1) return;
    getchar(); 
    printf("Limiares (ex: 10 50 128): "); fgets(linha_limiares, sizeof(linha_limiares), stdin);

    int largura, altura, maxval;
    int **img_original = lerPGM(arquivo, &largura, &altura, &maxval);
    if (!img_original) { printf("Erro ao ler PGM.\n"); return; }

    char *token = strtok(linha_limiares, " ");
    while (token != NULL) {
        int limiar = atoi(token);
        if (limiar >= 0 && limiar <= 255) {
            int **img_bin = malloc(altura * sizeof(int*));
            for(int i=0; i<altura; i++) {
                img_bin[i] = malloc(largura * sizeof(int));
                for(int j=0; j<largura; j++) img_bin[i][j] = (img_original[i][j] > limiar) ? 1 : 0;
            }

            FILE *dados = fopen("banco.dat", "ab");
            if (!dados) dados = fopen("banco.dat", "wb");
            long pos = ftell(dados);
            fprintf(dados, "%d %d %d ", largura, altura, limiar);
            comprimir(img_bin, largura, altura, dados);
            fclose(dados);
            liberar_matriz(img_bin, altura);

            Registro reg; strcpy(reg.nome, nome);
            reg.limiar = limiar; reg.largura = largura; reg.altura = altura;
            reg.posicao_dados = pos; reg.removido = 0; 
            inserir_chave(reg);
            printf("Inserido limiar %d.\n", limiar);
        }
        token = strtok(NULL, " ");
    }
    liberar_matriz(img_original, altura);
}

void buscar_extrair() {
    char nome[256], saida[300];
    int limiar;
    printf("Nome: "); scanf("%s", nome);
    printf("Limiar: "); scanf("%d", &limiar);
    
    Registro reg;
    if (buscar_chave(nome, limiar, &reg)) {
        FILE *dados = fopen("banco.dat", "rb");
        fseek(dados, reg.posicao_dados, SEEK_SET);
        int w, h, l;
        fscanf(dados, "%d %d %d", &w, &h, &l);
        int **img = descomprimir(dados, w, h);
        fclose(dados);
        
        sprintf(saida, "%s_%d.pgm", nome, limiar);
        salvarPGM(saida, img, w, h);
        liberar_matriz(img, h);
        printf("Extraído: %s\n", saida);
    } else printf("Não encontrado ou removido.\n");
}

void remover_interface() {
    char nome[256]; int limiar;
    printf("Nome: "); scanf("%s", nome);
    printf("Limiar: "); scanf("%d", &limiar);
    if (remover_chave(nome, limiar)) printf("Marcado como removido. Use compactar para limpar.\n");
    else printf("Não encontrado.\n");
}

int main() {
    inicializar_arvore(); 
    int op;
    do {
        printf("\n1. Adicionar\n2. Listar\n3. Buscar/Extrair\n4. Remover\n5. Compactar (Reconstruir)\n0. Sair\nOp: ");
        if (scanf("%d", &op) != 1) break;
        switch(op) {
            case 1: adicionar_multiplos(); break;
            case 2: percorrer_in_order(); break;
            case 3: buscar_extrair(); break;
            case 4: remover_interface(); break;
            case 5: reconstruir_banco_completo(); break;
        }
    } while(op != 0);
    finalizar_arvore();
    return 0;
}