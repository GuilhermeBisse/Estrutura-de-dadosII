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

void adicionar() {
    char arquivo[MAX], nome[MAX];
    int limiar;
    
    printf("Digite o caminho da imagem PGM (P2): ");
    if (scanf("%255s", arquivo) != 1) return; 

    printf("Digite um nome para identificar a imagem: ");
    if (scanf("%255s", nome) != 1) return; 

    printf("Digite o valor do limiar (0-255): ");
    if (scanf("%d", &limiar) != 1) return;

    int largura, altura, maxval;
    int **img = lerPGM(arquivo, &largura, &altura, &maxval);
    if (!img) {
        fprintf(stderr, "Falha ao carregar a imagem PGM.\n");
        return;
    }

    binarizar(img, largura, altura, limiar);

    FILE *dados = fopen("banco.dat", "ab");
    if (!dados) {
        perror("Erro ao abrir banco.dat");
        liberar_matriz(img, altura); 
        return;
    }

    long pos = ftell(dados); 
    
    fprintf(dados, "%d %d %d ", largura, altura, limiar);
    comprimir(img, largura, altura, dados);
    fclose(dados);

    FILE *idx = fopen("banco.idx", "a");
    if (!idx) {
        perror("Erro ao abrir banco.idx. A imagem foi comprimida, mas o índice falhou.");
        liberar_matriz(img, altura);
        return;
    }
    fprintf(idx, "%s %d %d %d %ld %d\n", nome, limiar, largura, altura, pos, 0);
    fclose(idx);

    liberar_matriz(img, altura); 
    
    printf("Imagem '%s' adicionada com sucesso!\n", nome);
}

void listar() {
    FILE *f = fopen("banco.idx", "r");
    if (!f) { 
        printf("Nenhum índice encontrado (banco.idx).\n"); 
        return; 
    }
    
    printf("\nID | Nome | Largura x Altura | Limiar | Posição | Removido\n");
    printf("---|-------------------|--------|--------|---------|----------\n");
    
    int id = 0;
    Indice e;
    while (fscanf(f, "%s %d %d %d %ld %d",
                  e.nome, &e.limiar, &e.largura, &e.altura, &e.posicao, &e.removido) == 6) {
        printf("%d | %s | %d x %d | %d | %ld | %s\n", 
               id++, e.nome, e.largura, e.altura, e.limiar, e.posicao, e.removido ? "SIM" : "não");
    }
    
    fclose(f);
}

void removerID() {
    int id;
    printf("Digite o ID da imagem para remover (remoção lógica): ");
    if (scanf("%d", &id) != 1) return;

    FILE *f = fopen("banco.idx", "r");
    if (!f) { printf("Nenhum índice encontrado.\n"); return; }
    
    FILE *temp = fopen("temp.idx", "w");
    if (!temp) { 
        perror("Erro ao criar arquivo temporário");
        fclose(f);
        return;
    }
    
    int atual = 0;
    Indice e;
    int encontrado = 0;

    while (fscanf(f, "%s %d %d %d %ld %d",
                  e.nome, &e.limiar, &e.largura, &e.altura, &e.posicao, &e.removido) == 6) {
        
        if (atual == id) {
            e.removido = 1; 
            encontrado = 1;
        }
        
        fprintf(temp, "%s %d %d %d %ld %d\n",
                e.nome, e.limiar, e.largura, e.altura, e.posicao, e.removido);
        atual++;
    }
    
    fclose(f);
    fclose(temp);
    
    if (!encontrado) {
        printf("Erro: ID %d não encontrado.\n", id);
        remove("temp.idx"); 
    } else {
        remove("banco.idx");
        rename("temp.idx", "banco.idx");
        printf("Imagem ID %d marcada como removida. Execute a Compactação para liberar espaço físico.\n", id);
    }
}

void extrair() {
    int id;
    char saida[MAX];
    
    printf("Digite o ID da imagem a extrair: ");
    if (scanf("%d", &id) != 1) return;
    
    printf("Digite o nome do arquivo de saída (ex: saida.pgm): ");
    if (scanf("%255s", saida) != 1) return;
    
    FILE *idx = fopen("banco.idx", "r");
    if (!idx) { printf("Índice não encontrado.\n"); return; }
    
    Indice e;
    int atual = 0;
    int extraida = 0;

    while (fscanf(idx, "%s %d %d %d %ld %d",
                  e.nome, &e.limiar, &e.largura, &e.altura, &e.posicao, &e.removido) == 6) {
        
        if (atual == id) {
            if (e.removido) {
                printf("Erro: Imagem ID %d está marcada como removida.\n", id);
            } else {
                FILE *dados = fopen("banco.dat", "rb");
                if (!dados) { perror("Erro ao abrir banco.dat"); break; }
                
                fseek(dados, e.posicao, SEEK_SET);
                
                int largura, altura, limiar;
                if (fscanf(dados, "%d %d %d", &largura, &altura, &limiar) != 3) {
                    fprintf(stderr, "Erro de leitura dos metadados no banco.dat.\n");
                    fclose(dados);
                    break;
                }
                
                int **img = descomprimir(dados, largura, altura);
                fclose(dados);
                
                if (img) {
                    salvarPGM(saida, img, largura, altura);
                    liberar_matriz(img, altura); 
                    printf("Imagem '%s' extraída em %s\n", e.nome, saida);
                    extraida = 1;
                } else {
                    fprintf(stderr, "Falha na descompressão da imagem.\n");
                }
            }
            break;
        }
        atual++;
    }
    
    if (!extraida) {
        if (atual <= id) printf("Erro: ID %d não existe no índice.\n", id);
    }

    fclose(idx);
}

void compactar() {
    FILE *idx = fopen("banco.idx", "r");
    if (!idx) { printf("Nenhum índice encontrado.\n"); return; }

    FILE *banco_dados = fopen("banco.dat", "rb");
    if (!banco_dados) { fclose(idx); printf("Nenhum arquivo de dados encontrado.\n"); return; }

    FILE *novo = fopen("novo.dat", "wb");
    FILE *novoidx = fopen("novo.idx", "w");
    if (!novo || !novoidx) { 
        perror("Erro ao criar arquivos temporários");
        fclose(idx); fclose(banco_dados); 
        if(novo) fclose(novo); if(novoidx) fclose(novoidx); 
        return; 
    }
    
    Indice e;
    int total_movido = 0;

    printf("Iniciando compactação...\n");

    while (fscanf(idx, "%s %d %d %d %ld %d",
                  e.nome, &e.limiar, &e.largura, &e.altura, &e.posicao, &e.removido) == 6) {
        
        if (e.removido) {
            printf("Ignorando registro removido: %s\n", e.nome);
            continue; 
        }
        
        long novaPos = ftell(novo);
        
        fseek(banco_dados, e.posicao, SEEK_SET);
        
        int largura, altura, limiar;
        if (fscanf(banco_dados, "%d %d %d", &largura, &altura, &limiar) != 3) {
            fprintf(stderr, "Erro de leitura em registro: %s. Pulando.\n", e.nome);
            continue;
        }
        
        fprintf(novo, "%d %d %d ", largura, altura, limiar);
        
        int **img = descomprimir(banco_dados, largura, altura);
        
        if (!img) {
             fprintf(stderr, "Falha na descompressão/cópia de registro: %s. Pulando.\n", e.nome);
             continue;
        }
        
        comprimir(img, largura, altura, novo);
        
        liberar_matriz(img, altura); 
        
        fprintf(novoidx, "%s %d %d %d %ld %d\n", e.nome, e.limiar, largura, altura, novaPos, 0);
        total_movido++;
    }
    
    fclose(idx);
    fclose(banco_dados);
    fclose(novo);
    fclose(novoidx);
    
    remove("banco.dat");
    rename("novo.dat", "banco.dat");
    remove("banco.idx");
    rename("novo.idx", "banco.idx");
    
    printf("Compactação concluída! %d registros ativos foram mantidos.\n", total_movido);
}

void reconstruir() {
    char nome[MAX], saida[MAX];
    
    printf("Digite o nome da imagem para reconstruir: ");
    if (scanf("%255s", nome) != 1) return;
    
    printf("Digite o nome do arquivo de saída (ex: reconstruida.pgm): ");
    if (scanf("%255s", saida) != 1) return;

    FILE *idx = fopen("banco.idx", "r");
    if (!idx) { printf("Nenhum índice encontrado.\n"); return; }
    
    Indice e;
    int **soma = NULL;
    int largura = 0, altura = 0, count = 0;
    
    while (fscanf(idx, "%s %d %d %d %ld %d",
                  e.nome, &e.limiar, &e.largura, &e.altura, &e.posicao, &e.removido) == 6) {
        
        if (strcmp(e.nome, nome) == 0 && !e.removido) {
            FILE *dados = fopen("banco.dat", "rb");
            if (!dados) { perror("Erro ao abrir banco.dat para reconstrução"); continue; }

            fseek(dados, e.posicao, SEEK_SET);
            
            int w, h, limiar;
            if (fscanf(dados, "%d %d %d", &w, &h, &limiar) != 3) {
                 fprintf(stderr, "Erro de leitura de metadados durante a reconstrução.\n");
                 fclose(dados);
                 continue;
            }
            
            int **img = descomprimir(dados, w, h);
            fclose(dados);

            if (!img) {
                fprintf(stderr, "Falha na descompressão de uma versão da imagem %s. Pulando.\n", e.nome);
                continue;
            }
            
            if (!soma) {
                largura = w;
                altura = h;
                soma = malloc(altura * sizeof(int *));
                if (!soma) { liberar_matriz(img, h); perror("Alocação inicial falhou"); break; }
                
                for (int i = 0; i < altura; i++) {
                    soma[i] = malloc(largura * sizeof(int));
                    if (!soma[i]) { liberar_matriz(soma, i); liberar_matriz(img, h); perror("Alocação inicial falhou"); break; }
                    for (int j = 0; j < largura; j++)
                        soma[i][j] = 0;
                }
            }
            
            if (w != largura || h != altura) {
                fprintf(stderr, "Aviso: Imagem %s com ID %d tem dimensões diferentes (%dx%d). Ignorada.\n", 
                        e.nome, count, w, h);
                liberar_matriz(img, h);
                continue;
            }
            
            for (int i = 0; i < altura; i++)
                for (int j = 0; j < largura; j++)
                    soma[i][j] += img[i][j];
            
            liberar_matriz(img, altura);
            count++;
        }
    }
    
    fclose(idx);

    if (count == 0 || !soma) { 
        printf("Nenhuma versão válida e não removida encontrada para a imagem '%s'.\n", nome); 
        return; 
    }
    
    int **media = malloc(altura * sizeof(int *));
    if (!media) { liberar_matriz(soma, altura); perror("Alocação da média falhou"); return; }
    
    for (int i = 0; i < altura; i++) {
        media[i] = malloc(largura * sizeof(int));
        if (!media[i]) { liberar_matriz(media, i); liberar_matriz(soma, altura); perror("Alocação da média falhou"); return; }
        
        for (int j = 0; j < largura; j++)
            media[i][j] = (soma[i][j] * 2 + count) / (count * 2); 
            
        free(soma[i]); 
    }
    free(soma); 
    
    salvarPGM(saida, media, largura, altura);

    liberar_matriz(media, altura); 
    
    printf("Reconstrução concluída (%d versões usadas) e salva em '%s'.\n", count, saida);
}

int main() {
    int opcao;
    do {
        printf("\n==== GERENCIADOR DE IMAGENS BINÁRIAS (ARQUIVOS) ====\n");
        printf("1. Adicionar imagem (PGM -> Binária + Comprimir)\n");
        printf("2. Listar imagens no banco\n");
        printf("3. Remover imagem (Marcação Lógica)\n");
        printf("4. Extrair imagem (Descomprimir -> PGM)\n");
        printf("5. Compactar banco (Remoção Física de Dados)\n");
        printf("6. Reconstruir imagem média (de versões por nome)\n");
        printf("0. Sair\n");
        printf("Escolha uma opção: ");
        
        if (scanf("%d", &opcao) != 1) {
            while(getchar() != '\n'); 
            opcao = -1; 
        }

        switch (opcao) {
            case 1: adicionar(); break;
            case 2: listar(); break;
            case 3: removerID(); break;
            case 4: extrair(); break;
            case 5: compactar(); break;
            case 6: reconstruir(); break;
            case 0: printf("Encerrando o gerenciador de imagens...\n"); break;
            default: printf("Opção inválida! Por favor, tente novamente.\n");
        }
    } while (opcao != 0);
    
    return 0;
}
