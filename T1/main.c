//Trabalho feito pelos alunos Guilherme Bisse, Gustavo Henrique, Felipe Matsuo, Pietro Franca e Pedro Ito (Atividade da Aula 02)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct{
    int h, w;
    int cinza;
    short int* pixels;
} imgb;

imgb read_pgm(FILE* fimg){
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
    }else { 
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

void write_bin(char* filename, imgb *I){
    FILE* fbin = fopen(filename, "wb");
    if(fbin == NULL){
        perror("Erro ao abrir arquivo");
        return;
    }

    fwrite(&I->w, sizeof(int), 1, fbin);
    fwrite(&I->h, sizeof(int), 1, fbin);
    fwrite(&I->cinza, sizeof(int), 1, fbin);
    fwrite(I->pixels, sizeof(short int), I->w * I->h, fbin);

    fclose(fbin);
}

imgb read_bin(FILE* fimg){
    imgb I = {0, 0, 0, NULL};

    fread(&I.w, sizeof(int), 1, fimg);
    fread(&I.h, sizeof(int), 1, fimg);
    fread(&I.cinza, sizeof(int), 1, fimg);
    
    I.pixels = (short int*)malloc(I.w * I.h * sizeof(short int));
    if (I.pixels == NULL) return I;

    if (fread(I.pixels, sizeof(short int), I.w * I.h, fimg) != (size_t)(I.w * I.h)){
        free(I.pixels);
        I.pixels = NULL;
    }
    return I;
}

void limiar(imgb *I, int lim){
    for(int i=0; i < I->w*I->h; i++){
        if (I->pixels[i] < lim){
            I->pixels[i] = 0;
        }else{
            I->pixels[i] = I->cinza;
        }
    }
}

void write_pgm (char *filename, imgb *I){
    FILE* fpgm = fopen(filename, "wb");
    if(fpgm == NULL){
        perror("Erro ao abrir arquivo para escrita PGM");
        return;
    }

    fprintf(fpgm, "P2\n");
    fprintf(fpgm, "%d %d\n", I->w, I->h);
    fprintf(fpgm, "%d\n", I->cinza);

    for (int i=0; i < I->w * I->h; i++){
        fprintf(fpgm, "%d ", I->pixels[i]);
    }

    fclose(fpgm);
}


int main(int argc, char* argv[]){

    FILE *fpgm = fopen("baboon.pgm", "rb");
    if (!fpgm){
        perror("Erro ao abrir o arquivo PGM");
        return 1;
    }

    imgb original_img = read_pgm(fpgm);
    fclose(fpgm);

    if (original_img.pixels == NULL) {
        fprintf(stderr, "Erro ao ler a imagem PGM.\n");
        return 1;
    }

    printf("Imagem PGM lida com sucesso!\n");

    write_bin("imagem_convertida.bin", &original_img);
    printf("Imagem salva em formato binário.\n");

    free(original_img.pixels);
    original_img.pixels = NULL;
    
    FILE *fbin = fopen("imagem_convertida.bin", "rb");
    if (!fbin){
        perror("Erro ao abrir o arquivo BIN");
        return 1;
    }

    imgb bin_img = read_bin(fbin);
    fclose(fbin);

    if (bin_img.pixels == NULL) {
        fprintf(stderr, "Erro ao ler a imagem BIN.\n");
        return 1;
    }

    printf("Imagem lida do arquivo binário.\n");

    limiar(&bin_img, 128);
    printf("Limiar aplicado na imagem.\n");

    write_pgm("imagem_limiar.pgm", &bin_img);
    printf("Imagem final salva em 'imagem_limiar.pgm'.\n");

    free(bin_img.pixels);

    return 0;
}
