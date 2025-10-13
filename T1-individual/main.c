#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 256

typedef struct {
    char nome[MAX];
    int limiar;
    int largura, altura;
    long posicao;
    int removido;
} Indice;

int **lerPGM(char *arquivo, int *largura, int *altura, int *maxval) {
    FILE *f = fopen(arquivo, "r");
    if (!f) return NULL;
    char tipo[3];
    fscanf(f, "%2s", tipo);
    if (strcmp(tipo, "P2") != 0) { fclose(f); return NULL; }
    fscanf(f, "%d %d %d", largura, altura, maxval);
    int **img = malloc((*altura) * sizeof(int *));
    for (int i = 0; i < *altura; i++) {
        img[i] = malloc((*largura) * sizeof(int));
        for (int j = 0; j < *largura; j++)
            fscanf(f, "%d", &img[i][j]);
    }
    fclose(f);
    return img;
}

void salvarPGM(char *arquivo, int **img, int largura, int altura) {
    FILE *f = fopen(arquivo, "w");
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
    int atual = img[0][0], cont = 0;
    fprintf(out, "%d ", atual);
    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++) {
            if (img[i][j] == atual) cont++;
            else { fprintf(out, "%d ", cont); cont = 1; atual = img[i][j]; }
        }
    }
    fprintf(out, "%d\n", cont);
}

int **descomprimir(FILE *in, int largura, int altura) {
    int primeiro;
    fscanf(in, "%d", &primeiro);
    int total = largura * altura;
    int *valores = malloc(total * sizeof(int));
    int pos = 0, atual = primeiro;
    while (pos < total) {
        int qtd;
        if (fscanf(in, "%d", &qtd) != 1) break;
        for (int i = 0; i < qtd && pos < total; i++)
            valores[pos++] = atual;
        atual = 1 - atual;
    }
    int **img = malloc(altura * sizeof(int *));
    pos = 0;
    for (int i = 0; i < altura; i++) {
        img[i] = malloc(largura * sizeof(int));
        for (int j = 0; j < largura; j++)
            img[i][j] = valores[pos++];
    }
    free(valores);
    return img;
}

void adicionar() {
    char arquivo[MAX], nome[MAX];
    int limiar;
    printf("Digite o caminho da imagem PGM (P2): ");
    scanf("%s", arquivo);
    printf("Digite um nome para identificar a imagem: ");
    scanf("%s", nome);
    printf("Digite o valor do limiar (0-255): ");
    scanf("%d", &limiar);
    int largura, altura, maxval;
    int **img = lerPGM(arquivo, &largura, &altura, &maxval);
    if (!img) return;
    binarizar(img, largura, altura, limiar);
    FILE *dados = fopen("banco.dat", "ab");
    long pos = ftell(dados);
    fprintf(dados, "%d %d %d ", largura, altura, limiar);
    comprimir(img, largura, altura, dados);
    fclose(dados);
    FILE *idx = fopen("banco.idx", "a");
    fprintf(idx, "%s %d %d %d %ld %d\n", nome, limiar, largura, altura, pos, 0);
    fclose(idx);
    printf("Imagem '%s' adicionada com sucesso!\n", nome);
}

void listar() {
    FILE *f = fopen("banco.idx", "r");
    if (!f) { printf("Nenhum índice encontrado.\n"); return; }
    printf("\nID | Nome | Limiar | Removido\n");
    int id = 0;
    Indice e;
    while (fscanf(f, "%s %d %d %d %ld %d",
                  e.nome, &e.limiar, &e.largura, &e.altura, &e.posicao, &e.removido) == 6) {
        printf("%d | %s | %d | %s\n", id++, e.nome, e.limiar, e.removido ? "sim" : "não");
    }
    fclose(f);
}

void removerID() {
    int id;
    printf("Digite o ID da imagem para remover: ");
    scanf("%d", &id);
    FILE *f = fopen("banco.idx", "r");
    FILE *temp = fopen("temp.idx", "w");
    if (!f) return;
    int atual = 0;
    Indice e;
    while (fscanf(f, "%s %d %d %d %ld %d",
                  e.nome, &e.limiar, &e.largura, &e.altura, &e.posicao, &e.removido) == 6) {
        if (atual == id) e.removido = 1;
        fprintf(temp, "%s %d %d %d %ld %d\n",
                e.nome, e.limiar, e.largura, e.altura, e.posicao, e.removido);
        atual++;
    }
    fclose(f);
    fclose(temp);
    remove("banco.idx");
    rename("temp.idx", "banco.idx");
    printf("Imagem %d marcada como removida.\n", id);
}

void extrair() {
    int id;
    char saida[MAX];
    printf("Digite o ID da imagem a extrair: ");
    scanf("%d", &id);
    printf("Digite o nome do arquivo de saída (ex: saida.pgm): ");
    scanf("%s", saida);
    FILE *idx = fopen("banco.idx", "r");
    if (!idx) return;
    Indice e;
    int atual = 0;
    while (fscanf(idx, "%s %d %d %d %ld %d",
                  e.nome, &e.limiar, &e.largura, &e.altura, &e.posicao, &e.removido) == 6) {
        if (atual == id && !e.removido) {
            FILE *dados = fopen("banco.dat", "rb");
            fseek(dados, e.posicao, SEEK_SET);
            int largura, altura, limiar;
            fscanf(dados, "%d %d %d", &largura, &altura, &limiar);
            int **img = descomprimir(dados, largura, altura);
            fclose(dados);
            salvarPGM(saida, img, largura, altura);
            printf("Imagem extraída em %s\n", saida);
            break;
        }
        atual++;
    }
    fclose(idx);
}

void compactar() {
    FILE *idx = fopen("banco.idx", "r");
    FILE *novo = fopen("novo.dat", "wb");
    FILE *novoidx = fopen("novo.idx", "w");
    if (!idx || !novo || !novoidx) return;
    Indice e;
    while (fscanf(idx, "%s %d %d %d %ld %d",
                  e.nome, &e.limiar, &e.largura, &e.altura, &e.posicao, &e.removido) == 6) {
        if (e.removido) continue;
        FILE *dados = fopen("banco.dat", "rb");
        fseek(dados, e.posicao, SEEK_SET);
        long novaPos = ftell(novo);
        int largura, altura, limiar;
        fscanf(dados, "%d %d %d", &largura, &altura, &limiar);
        fprintf(novo, "%d %d %d ", largura, altura, limiar);
        int **img = descomprimir(dados, largura, altura);
        comprimir(img, largura, altura, novo);
        fclose(dados);
        fprintf(novoidx, "%s %d %d %d %ld %d\n", e.nome, e.limiar, largura, altura, novaPos, 0);
    }
    fclose(idx);
    fclose(novo);
    fclose(novoidx);
    remove("banco.dat");
    rename("novo.dat", "banco.dat");
    remove("banco.idx");
    rename("novo.idx", "banco.idx");
    printf("Compactação concluída!\n");
}

void reconstruir() {
    char nome[MAX], saida[MAX];
    printf("Digite o nome da imagem para reconstruir: ");
    scanf("%s", nome);
    printf("Digite o nome do arquivo de saída (ex: reconstruida.pgm): ");
    scanf("%s", saida);
    FILE *idx = fopen("banco.idx", "r");
    if (!idx) { printf("Nenhum índice encontrado.\n"); return; }
    Indice e;
    int **soma = NULL;
    int largura = 0, altura = 0, count = 0;
    while (fscanf(idx, "%s %d %d %d %ld %d",
                  e.nome, &e.limiar, &e.largura, &e.altura, &e.posicao, &e.removido) == 6) {
        if (strcmp(e.nome, nome) == 0 && !e.removido) {
            FILE *dados = fopen("banco.dat", "rb");
            fseek(dados, e.posicao, SEEK_SET);
            int w, h, limiar;
            fscanf(dados, "%d %d %d", &w, &h, &limiar);
            int **img = descomprimir(dados, w, h);
            fclose(dados);
            if (!soma) {
                largura = w;
                altura = h;
                soma = malloc(altura * sizeof(int *));
                for (int i = 0; i < altura; i++) {
                    soma[i] = malloc(largura * sizeof(int));
                    for (int j = 0; j < largura; j++)
                        soma[i][j] = 0;
                }
            }
            for (int i = 0; i < altura; i++)
                for (int j = 0; j < largura; j++)
                    soma[i][j] += img[i][j];
            for (int i = 0; i < altura; i++) free(img[i]);
            free(img);
            count++;
        }
    }
    fclose(idx);
    if (count == 0) { printf("Nenhuma versão encontrada para a imagem '%s'.\n", nome); return; }
    int **media = malloc(altura * sizeof(int *));
    for (int i = 0; i < altura; i++) {
        media[i] = malloc(largura * sizeof(int));
        for (int j = 0; j < largura; j++)
            media[i][j] = soma[i][j] / count;
        free(soma[i]);
    }
    free(soma);
    salvarPGM(saida, media, largura, altura);
    for (int i = 0; i < altura; i++) free(media[i]);
    free(media);
    printf("Reconstrução concluída e salva em '%s'.\n", saida);
}

int main() {
    int opcao;
    do {
        printf("\n==== GERENCIADOR DE IMAGENS BINÁRIAS ====\n");
        printf("1. Adicionar imagem\n");
        printf("2. Listar imagens\n");
        printf("3. Remover imagem\n");
        printf("4. Extrair imagem\n");
        printf("5. Compactar banco\n");
        printf("6. Reconstruir imagem média\n");
        printf("0. Sair\n");
        printf("Escolha uma opção: ");
        scanf("%d", &opcao);
        switch (opcao) {
            case 1: adicionar(); break;
            case 2: listar(); break;
            case 3: removerID(); break;
            case 4: extrair(); break;
            case 5: compactar(); break;
            case 6: reconstruir(); break;
            case 0: printf("Encerrando...\n"); break;
            default: printf("Opção inválida!\n");
        }
    } while (opcao != 0);
    return 0;
}
