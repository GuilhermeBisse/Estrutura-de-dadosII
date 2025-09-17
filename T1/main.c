//Trabalho feito pelos alunos Guilherme Bisse, Gustavo Henrique, Felipe Matsuo, Pietro Franca e Pedro Ito (Atividade da Aula 02 e 03)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char nome[256];
    long offset;
    long tamanho;
} IndexEntry;

typedef struct {
    int h, w, cinza;
    short int* pixels;
} imgb;

imgb read_pgm(FILE* fimg) {
  imgb I = {0, 0, 0, NULL};
  char tipo[3];
  int h, w, cinza;

  if (fscanf(fimg, "%2s", tipo) != 1) return I;
  if (strcmp(tipo, "P5") != 0 && strcmp(tipo, "P2") != 0) return I;
  fgetc(fimg);

  char ch = fgetc(fimg);
  while (ch == '#') {
    while (fgetc(fimg) != '\n');
    ch = fgetc(fimg);
  }
  ungetc(ch, fimg);

  if (fscanf(fimg, "%d%d", &w, &h) != 2) return I;
  if (fscanf(fimg, "%d", &cinza) != 1) return I;
  fgetc(fimg);

  I.w = w;
  I.h = h;
  I.cinza = cinza;
  I.pixels = (short int*)malloc(w * h * sizeof(short int));
  if(I.pixels == NULL) return I;

  if (strcmp(tipo, "P5") == 0) {
    for (int i = 0; i < w * h; i++) {
      unsigned char byte;
      if (fread(&byte, sizeof(unsigned char), 1, fimg) != 1) {
        free(I.pixels);
        I.pixels = NULL;
        return I;
      }
      I.pixels[i] = (short int)byte;
    }
  } else { 
      for (int i = 0; i < w * h; i++) {
        if (fscanf(fimg, "%hd", &I.pixels[i]) != 1) {
          free(I.pixels);
          I.pixels = NULL;
          return I;
        }
      }
  }
  return I;
}

void write_bin(char* filename, imgb *I) {
  FILE* fbin = fopen(filename, "wb");
  if(fbin == NULL) {
    perror("Erro ao abrir arquivo");
    return;
  }

  fwrite(&I->w, sizeof(int), 1, fbin);
  fwrite(&I->h, sizeof(int), 1, fbin);
  fwrite(&I->cinza, sizeof(int), 1, fbin);
  fwrite(I->pixels, sizeof(short int), I->w * I->h, fbin);

  fclose(fbin);
}

imgb read_bin(FILE* fimg) {
  imgb I = {0, 0, 0, NULL};

  fread(&I.w, sizeof(int), 1, fimg);
  fread(&I.h, sizeof(int), 1, fimg);
  fread(&I.cinza, sizeof(int), 1, fimg);
  
  I.pixels = (short int*)malloc(I.w * I.h * sizeof(short int));
  if (I.pixels == NULL) return I;

  if (fread(I.pixels, sizeof(short int), I.w * I.h, fimg) != (size_t)(I.w * I.h)) {
    free(I.pixels);
    I.pixels = NULL;
  }
  return I;
}

void limiar(imgb *I, int lim) {
  for(int i=0; i < I->w*I->h; i++) {
    if (I->pixels[i] < lim) {
      I->pixels[i] = 0;
    } else {
      I->pixels[i] = I->cinza;
    }
  }
}

void write_pgm (char *filename, imgb *I) {
  FILE* fpgm = fopen(filename, "wb");
  if(fpgm == NULL) {
    perror("Erro ao abrir arquivo para escrita PGM");
    return;
  }

  fprintf(fpgm, "P2\n");
  fprintf(fpgm, "%d %d\n", I->w, I->h);
  fprintf(fpgm, "%d\n", I->cinza);

  for (int i=0; i < I->w * I->h; i++) {
    fprintf(fpgm, "%d ", I->pixels[i]);
  }

  fclose(fpgm);
}

void negativo(imgb *I) {
  for (int i = 0; i < I->w * I->h; i++) {
    I->pixels[i] = I->cinza - I->pixels[i];
  }
}

void salvar_imagem(const char *nome_pgm, const char *nome_bin, const char *nome_index) {
  FILE *fpgm = fopen(nome_pgm, "rb");
  if (!fpgm) { perror("Erro ao abrir PGM"); return; }

  imgb img = read_pgm(fpgm);
  fclose(fpgm);

  if (!img.pixels) { fprintf(stderr, "Erro ao ler imagem.\n"); return; }

  // Abre arquivo binário no final
  FILE *fbin = fopen(nome_bin, "ab+");
  if (!fbin) { perror("Erro ao abrir bin"); free(img.pixels); return; }

  fseek(fbin, 0, SEEK_END);
  long offset = ftell(fbin);

  // Grava cabeçalho + pixels
  fwrite(&img.w, sizeof(int), 1, fbin);
  fwrite(&img.h, sizeof(int), 1, fbin);
  fwrite(&img.cinza, sizeof(int), 1, fbin);
  fwrite(img.pixels, sizeof(short int), img.w * img.h, fbin);

  long tamanho = ftell(fbin) - offset;
  fclose(fbin);

  // Atualiza índice
  FILE *fidx = fopen(nome_index, "a");
  if (!fidx) { perror("Erro abrir index"); free(img.pixels); return; }

  fprintf(fidx, "%s %ld %ld\n", nome_pgm, offset, tamanho);
  fclose(fidx);

  free(img.pixels);
  printf("Imagem '%s' salva no banco.\n", nome_pgm);
}

IndexEntry buscar_index(const char *nome_index, const char *nome) {
  FILE *f = fopen(nome_index, "r");
  IndexEntry e = {"", -1, -1};
  if (!f) return e;

  while (fscanf(f, "%255s %ld %ld", e.nome, &e.offset, &e.tamanho) == 3) {
    if (strcmp(e.nome, nome) == 0) {
      fclose(f);
      return e;
    }
  }
  fclose(f);
  e.offset = -1;
  return e;
}

imgb carregar_imagem(const char *nome_bin, IndexEntry e) {
  imgb I = {0,0,0,NULL};
  if (e.offset < 0) return I;

  FILE *fbin = fopen(nome_bin, "rb");
  if (!fbin) return I;

  fseek(fbin, e.offset, SEEK_SET);

  fread(&I.w, sizeof(int), 1, fbin);
  fread(&I.h, sizeof(int), 1, fbin);
  fread(&I.cinza, sizeof(int), 1, fbin);

  I.pixels = malloc(I.w * I.h * sizeof(short int));
  fread(I.pixels, sizeof(short int), I.w * I.h, fbin);

  fclose(fbin);
  return I;
}

int main(void) {
  int opcao;
  char nome[256];

  while (1) {
    printf("\n1 - Inserir imagem\n2 - Exportar imagem\n0 - Sair\n> ");
    scanf("%d", &opcao);

    if (opcao == 0) break;

    if (opcao == 1) {
      printf("Nome do arquivo PGM: ");
      scanf("%s", nome);
      salvar_imagem(nome, "imagens.bin", "index.txt");
    } 
    else if (opcao == 2) {
      printf("Nome da imagem a buscar: ");
      scanf("%s", nome);
      IndexEntry e = buscar_index("index.txt", nome);
      if (e.offset < 0) {
        printf("Imagem nao encontrada.\n");
        continue;
      }

      imgb img = carregar_imagem("imagens.bin", e);
      if (!img.pixels) { printf("Erro ao carregar.\n"); continue; }

      printf("1 - Exportar original\n2 - Exportar limiarizada\n3 - Exportar negativa\n> ");
      scanf("%d", &opcao);

      if (opcao == 2) limiar(&img, 128);
      if (opcao == 3) negativo(&img);

      char saida[300];
      if (opcao == 1) sprintf(saida, "saida_original_%s", nome);
      else if (opcao == 2) sprintf(saida, "saida_limiar_%s", nome);
      else if (opcao == 3) sprintf(saida, "saida_negativo_%s", nome);

      write_pgm(saida, &img);
      printf("Exportada em %s\n", saida);

      free(img.pixels);
    }
  }
  return 0;
}
