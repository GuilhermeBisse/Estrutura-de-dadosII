#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 256
#include "btree.h"

/* Utilitários de Memória */
void liberar_matriz(int **matriz, int altura) {
    if (matriz == NULL) return;
    for (int i = 0; i < altura; i++) {
        if (matriz[i] != NULL) {
            free(matriz[i]);
        }
    }
    free(matriz);
}

/* Leitor de PGM (Portable Gray Map) 
   Suporta apenas formato P2 (ASCII) 
*/
int **lerPGM(char *arquivo, int *largura, int *altura, int *maxval) {
    FILE *f = fopen(arquivo, "r");
    if (!f) {
        perror("Erro ao abrir arquivo PGM");
        return NULL;
    }

    char tipo[3];
    // Valida cabeçalho P2
    if (fscanf(f, "%2s", tipo) != 1 || strcmp(tipo, "P2") != 0) {
        fprintf(stderr, "Tipo de arquivo PGM inválido ou não é P2.\n");
        fclose(f);
        return NULL;
    }

    // Lê dimensões e valor máximo de cinza
    if (fscanf(f, "%d %d %d", largura, altura, maxval) != 3) {
        fprintf(stderr, "Erro ao ler largura, altura ou maxval.\n");
        fclose(f);
        return NULL;
    }

    // Alocação dinâmica da matriz da imagem
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
            fprintf(f, "%d ", img[i][j] * 255); // Escala 0/1 de volta para 0/255 (preto/branco)
        fprintf(f, "\n");
    }

    fclose(f);
}

// Transforma tons de cinza em 0 ou 1 baseada no limiar (Thresholding)
void binarizar(int **img, int largura, int altura, int limiar) {
    for (int i = 0; i < altura; i++)
        for (int j = 0; j < largura; j++)
            img[i][j] = (img[i][j] > limiar) ? 1 : 0;
}

/* Compressão RLE (Run-Length Encoding)
   Lógica: Grava o valor do primeiro pixel e depois conta quantos pixels 
   iguais existem na sequência. Quando o valor muda, grava a contagem e reseta.
   Ex: 0 0 0 1 1 -> Grava "0" (valor inicial) e depois "3 2" (quantidades)
*/
void comprimir(int **img, int largura, int altura, FILE *out) {
    int atual = img[0][0];
    long cont = 0; 
    
    fprintf(out, "%d ", atual); // Cabeçalho: Qual valor começamos?

    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++) {
            if (img[i][j] == atual) {
                cont++;
            } else {
                fprintf(out, "%ld ", cont); // Escreve a quantidade da sequência anterior
                cont = 1; 
                atual = img[i][j]; // Alterna o valor esperado
            }
        }
    }
    fprintf(out, "%ld\n", cont); // Grava o último bloco
}

int **descomprimir(FILE *in, int largura, int altura) {
    int primeiro;
    if (fscanf(in, "%d", &primeiro) != 1) return NULL; // Lê valor inicial (0 ou 1)

    int total = largura * altura;
    int *valores = malloc(total * sizeof(int)); // Vetor linear temporário
    if (!valores) { perror("Erro de alocação em descomprimir"); return NULL; }

    int pos = 0, atual = primeiro;
    
    // Reconstrói o vetor linear baseado nas contagens RLE
    while (pos < total) {
        long qtd;
        if (fscanf(in, "%ld", &qtd) != 1) { 
            if (pos < total) fprintf(stderr, "Aviso: Arquivo de dados truncado (pos %d de %d).\n", pos, total);
            break;
        }

        for (long i = 0; i < qtd && pos < total; i++) {
            valores[pos++] = atual;
        }
        
        atual = 1 - atual; // Alterna automaticamente: se era 0 vira 1, se era 1 vira 0
    }

    // Converte vetor linear para matriz 2D
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

// --- Integração com B-Tree ---

void adicionar_multiplos() {
    char arquivo[256], nome[256], linha_limiares[256];
    
    printf("Caminho PGM: "); if (scanf("%s", arquivo) != 1) return;
    printf("Nome base: "); if (scanf("%s", nome) != 1) return;
    getchar(); // Limpa buffer do \n
    printf("Limiares (ex: 10 50 128): "); fgets(linha_limiares, sizeof(linha_limiares), stdin);

    int largura, altura, maxval;
    int **img_original = lerPGM(arquivo, &largura, &altura, &maxval);
    if (!img_original) { printf("Erro ao ler PGM.\n"); return; }

    // Tokeniza string de limiares para processar múltiplos de uma vez
    char *token = strtok(linha_limiares, " ");
    while (token != NULL) {
        int limiar = atoi(token);
        if (limiar >= 0 && limiar <= 255) {
            // 1. Cria cópia binarizada em memória
            int **img_bin = malloc(altura * sizeof(int*));
            for(int i=0; i<altura; i++) {
                img_bin[i] = malloc(largura * sizeof(int));
                for(int j=0; j<largura; j++) img_bin[i][j] = (img_original[i][j] > limiar) ? 1 : 0;
            }

            // 2. Grava dados COMPRIMIDOS no final do arquivo .dat
            FILE *dados = fopen("banco.dat", "ab");
            if (!dados) dados = fopen("banco.dat", "wb");
            long pos = ftell(dados); // Pega o offset para salvar na árvore
            fprintf(dados, "%d %d %d ", largura, altura, limiar); // Metadados da imagem
            comprimir(img_bin, largura, altura, dados);
            fclose(dados);
            liberar_matriz(img_bin, altura);

            // 3. Insere metadados na Árvore B
            Registro reg; strcpy(reg.nome, nome);
            reg.limiar = limiar; reg.largura = largura; reg.altura = altura;
            reg.posicao_dados = pos; // Link crucial: Índice -> Dados Brutos
            reg.removido = 0; 
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
    // Busca no índice
    if (buscar_chave(nome, limiar, &reg)) {
        // Se achou, vai no arquivo de dados na posição indicada
        FILE *dados = fopen("banco.dat", "rb");
        fseek(dados, reg.posicao_dados, SEEK_SET);
        int w, h, l;
        fscanf(dados, "%d %d %d", &w, &h, &l); // Pula metadados (já temos no reg, mas precisa avançar o cursor)
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
    // Remove apenas logicamente (seta flag). O dado continua no .dat até compactação.
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
