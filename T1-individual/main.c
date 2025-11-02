#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define o tamanho máximo para strings como nomes de arquivos ou identificadores.
#define MAX 256

// Estrutura para armazenar metadados de uma imagem no arquivo de índice.
// Contém nome, limiar de binarização, dimensões, posição no arquivo de dados e status de remoção.
typedef struct {
    char nome[MAX];
    int limiar;
    int largura, altura;
    long posicao;
    int removido;
} Indice;

// Libera a memória alocada dinamicamente para uma matriz 2D da imagem.
void liberar_matriz(int **matriz, int altura) {
    if (matriz == NULL) return;
    for (int i = 0; i < altura; i++) {
        if (matriz[i] != NULL) {
            free(matriz[i]);
        }
    }
    free(matriz);
}

// Lê um arquivo de imagem no formato PGM de tipo P2 e retorna uma matriz 2D de inteiros.
// Também preenche os ponteiros para largura, altura e valor máximo de pixel (maxval).
int **lerPGM(char *arquivo, int *largura, int *altura, int *maxval) {
    FILE *f = fopen(arquivo, "r");
    if (!f) {
        perror("Erro ao abrir arquivo PGM");
        return NULL;
    }

    char tipo[3];
    if (fscanf(f, "%2s", tipo) != 1 || strcmp(tipo, "P2") != 0) {
        fprintf(stderr, "Tipo de arquivo PGM inválido ou não é P2.\n");
        printf("Tipo de arquivo PGM inválido ou não é P2.\n");
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

// Salva uma matriz 2D de inteiros como um arquivo de imagem PGM (P2).
// Assume que os valores na matriz são 0 ou 1 e os converte para 0 ou 255.
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

// Aplica a binarização (limiarização) na imagem, convertendo os pixels para 1 (se > limiar) ou 0 (se <= limiar).
// Modifica a matriz de imagem diretamente.
void binarizar(int **img, int largura, int altura, int limiar) {
    for (int i = 0; i < altura; i++)
        for (int j = 0; j < largura; j++)
            img[i][j] = (img[i][j] > limiar) ? 1 : 0;
}

// Comprime a imagem binária usando o método visto em sala de aula: Run-Length Encoding (RLE).
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

// Descomprime os dados de imagem a partir do arquivo de entrada (in) usando RLE.
// Retorna a matriz 2D da imagem binária reconstruída (valores 0 ou 1).
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

// Função para adicionar uma nova imagem ao banco.
// Lê um arquivo do PGM, binariza, comprime e salva os dados em 'banco.dat' e o índice em 'banco.idx'.
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

// Lista todas as imagens registradas no arquivo de índice 'banco.idx', além do seu status de remoção.
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

// Realiza a remoção lógica de um registro de imagem, marcando-o como "removido" no índice 'banco.idx'.
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

// Extrai uma imagem pelo seu ID. Lê o índice, busca a posição, descomprime os dados de 'banco.dat'
// e salva a imagem reconstruída em um novo arquivo PGM.
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

// Realiza a compactação do banco de dados (remoção física).
// Cria novos arquivos de dados, copiando apenas os registros não removidos literalmente,
// e atualizando suas posições. Em seguida, substitui os arquivos originais pelos novos.
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

// Reconstrói uma imagem média a partir de todas as versões salvas com o mesmo nome e que não foram removidas.
// É aquela parte 'bônus' do trabalho
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

// Função para ordenar o arquivo banco.idx usando K-Way Merge Sort externo (no arquivo 'k-merge.c').
// Ela cria runs temporários (cada run é um arquivo, podendo conter diversos registros), ordena-os e depois os intercala em um arquivo final ordenado.
//Essa é a função de 'pré-processamento' que prepara o arquivo de índice para ser ordenado via K-Way Merge.
void ordenar() {
    const char *ARQUIVO_ENTRADA = "banco.idx";
    const char *ARQUIVO_SAIDA_TEMP = "banco.idx.tmp";
    char nomes_runs[MAX_RUNS][MAX]; 
    
    FILE *fp_entrada = fopen(ARQUIVO_ENTRADA, "r");
    if (fp_entrada == NULL) {
        printf("Nenhum índice encontrado para ordenar (banco.idx).\n");
        return;
    }

    
    Indice *buffer_ram = (Indice *)malloc(RECORDS_PER_RUN * sizeof(Indice));
    if (buffer_ram == NULL) {
        perror("Erro de alocação de memória para o buffer");
        fclose(fp_entrada);
        return;
    }

    int run_count = 0;
    size_t lidos = 0;
    Indice reg_lido;

    while (fscanf(fp_entrada, "%s %d %d %d %ld %d",
                  reg_lido.nome, &reg_lido.limiar, &reg_lido.largura, &reg_lido.altura, &reg_lido.posicao, &reg_lido.removido) == 6) {

        if (lidos < RECORDS_PER_RUN) {
            buffer_ram[lidos++] = reg_lido;
        } else {
            if (run_count >= MAX_RUNS) {
                fprintf(stderr, "Erro: Limite de runs temporários excedido (%d).\n", MAX_RUNS);
                break;
            }
            
            // Ordenar e escrever o run binário
            qsort(buffer_ram, lidos, sizeof(Indice), comparar_indice);

            sprintf(nomes_runs[run_count], "run_%d.tmp", run_count);
            FILE *fp_saida = fopen(nomes_runs[run_count], "wb");
            if (fp_saida == NULL) break;
            fwrite(buffer_ram, sizeof(Indice), lidos, fp_saida);
            fclose(fp_saida);

            run_count++;
            lidos = 0;
            buffer_ram[lidos++] = reg_lido; // Adiciona o registro transbordado
        }
    }
    
    // Processa o último run incompleto
    if (lidos > 0 && run_count < MAX_RUNS) {
        qsort(buffer_ram, lidos, sizeof(Indice), comparar_indice);
        sprintf(nomes_runs[run_count], "run_%d.tmp", run_count);
        FILE *fp_saida = fopen(nomes_runs[run_count], "wb");
        if (fp_saida != NULL) {
            fwrite(buffer_ram, sizeof(Indice), lidos, fp_saida);
            fclose(fp_saida);
            run_count++;
        }
    }

    free(buffer_ram);
    fclose(fp_entrada);
    
    
    if (run_count <= 0) return;

    const char *runs_ptr[run_count];
    for (int i = 0; i < run_count; i++) {
        runs_ptr[i] = nomes_runs[i];
    }
    
    int resultado = k_way_merge(runs_ptr, run_count, ARQUIVO_SAIDA_TEMP, K_MERGE);
    
    // 3. Finalização e Limpeza
    for (int i = 0; i < run_count; i++) {
        remove(nomes_runs[i]); // Remove os runs temporários
    }

    if (resultado == 0) {
        printf("Substituindo índice antigo pelo novo ordenado.\n");
        remove(ARQUIVO_ENTRADA);
        rename(ARQUIVO_SAIDA_TEMP, ARQUIVO_ENTRADA);
        printf("Ordenação do 'banco.idx' concluída com sucesso!\n");
    } else {
        printf("Erro durante a ordenação K-Way Merge. O arquivo de índice não foi alterado.\n");
        remove(ARQUIVO_SAIDA_TEMP);
    }
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
        printf("7. ORDENAR índice (Merge Sort Externo)\n");
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
            case 7: ordenar(); break;
            case 0: printf("Encerrando o gerenciador de imagens...\n"); break;
            default: printf("Opção inválida! Por favor, tente novamente.\n");
        }
    } while (opcao != 0);
    
    return 0;
}
