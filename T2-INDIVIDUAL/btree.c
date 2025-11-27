#include "btree.h"

typedef struct {
    long raiz_offset;
} CabecalhoArquivo;

Pagina *raiz_virtual = NULL;
int raiz_modificada = 0;

// --- Gerenciamento de Arquivo ---

// Versão genérica para permitir leitura de backups (.old)
void ler_pagina_generica(char *arquivo, long offset, Pagina *p) {
    FILE *f = fopen(arquivo, "rb");
    if (!f) return;
    fseek(f, offset, SEEK_SET);
    fread(p, sizeof(Pagina), 1, f);
    fclose(f);
}

void ler_pagina(long offset, Pagina *p) {
    ler_pagina_generica(ARQUIVO_ARVORE, offset, p);
}

void escrever_pagina(Pagina *p) {
    FILE *f = fopen(ARQUIVO_ARVORE, "rb+");
    if (!f) f = fopen(ARQUIVO_ARVORE, "wb+");
    fseek(f, p->meu_offset, SEEK_SET);
    fwrite(p, sizeof(Pagina), 1, f);
    fclose(f);
}

long alocar_pagina() {
    FILE *f = fopen(ARQUIVO_ARVORE, "rb+");
    if (!f) f = fopen(ARQUIVO_ARVORE, "wb+");
    fseek(f, 0, SEEK_END);
    long offset = ftell(f);
    Pagina p_vazia = {0}; 
    fwrite(&p_vazia, sizeof(Pagina), 1, f);
    fclose(f);
    return offset;
}

long ler_header_raiz(char *arquivo) {
    FILE *f = fopen(arquivo, "rb");
    if (!f) return -1;
    CabecalhoArquivo cab;
    if (fread(&cab, sizeof(CabecalhoArquivo), 1, f) != 1) { fclose(f); return -1; }
    fclose(f);
    return cab.raiz_offset;
}

void atualizar_header_raiz(long raiz_off) {
    FILE *f = fopen(ARQUIVO_ARVORE, "rb+");
    if (!f) f = fopen(ARQUIVO_ARVORE, "wb+");
    CabecalhoArquivo cab = { raiz_off };
    fseek(f, 0, SEEK_SET);
    fwrite(&cab, sizeof(CabecalhoArquivo), 1, f);
    fclose(f);
}

// --- Comparação e Busca ---

int comparar(char *n1, int l1, char *n2, int l2) {
    int r = strcmp(n1, n2);
    if (r != 0) return r;
    return l1 - l2;
}

int buscar_recursivo(long offset_pag, char *nome, int limiar, Registro *retorno) {
    Pagina p;
    if (raiz_virtual && offset_pag == raiz_virtual->meu_offset) p = *raiz_virtual;
    else ler_pagina(offset_pag, &p);

    int i = 0;
    while (i < p.num_chaves && comparar(nome, limiar, p.chaves[i].nome, p.chaves[i].limiar) > 0) i++;

    if (i < p.num_chaves && comparar(nome, limiar, p.chaves[i].nome, p.chaves[i].limiar) == 0) {
        if (p.chaves[i].removido == 1) return 0; // Ignora removidos logicamente
        *retorno = p.chaves[i];
        return 1;
    }

    if (p.folha) return 0;
    return buscar_recursivo(p.filhos[i], nome, limiar, retorno);
}

int buscar_chave(char *nome, int limiar, Registro *reg_retorno) {
    if (!raiz_virtual) return 0;
    return buscar_recursivo(raiz_virtual->meu_offset, nome, limiar, reg_retorno);
}

// Retorna: 0 (não existe), 1 (ativo), 2 (removido)
int buscar_status(long offset, char *nome, int limiar, Pagina *pag_out, int *idx_out) {
    Pagina p;
    if (raiz_virtual && offset == raiz_virtual->meu_offset) p = *raiz_virtual;
    else ler_pagina(offset, &p);

    int i = 0;
    while (i < p.num_chaves && comparar(nome, limiar, p.chaves[i].nome, p.chaves[i].limiar) > 0) i++;

    if (i < p.num_chaves && comparar(nome, limiar, p.chaves[i].nome, p.chaves[i].limiar) == 0) {
        if (pag_out) *pag_out = p;
        if (idx_out) *idx_out = i;
        return (p.chaves[i].removido) ? 2 : 1;
    }
    if (p.folha) return 0;
    return buscar_status(p.filhos[i], nome, limiar, pag_out, idx_out);
}

// --- Inserção ---

void split_child(Pagina *x, int i) {
    long offset_y = x->filhos[i];
    Pagina y; ler_pagina(offset_y, &y);
    Pagina z; z.meu_offset = alocar_pagina(); z.folha = y.folha;
    
    z.num_chaves = 1;
    z.chaves[0] = y.chaves[MAX_CHAVES - 1]; 

    if (!y.folha) {
        for (int j = 0; j < 2; j++) z.filhos[j] = y.filhos[j + 2];
    }
    y.num_chaves = 1;

    for (int j = x->num_chaves; j >= i + 1; j--) x->filhos[j+1] = x->filhos[j];
    x->filhos[i+1] = z.meu_offset;

    for (int j = x->num_chaves - 1; j >= i; j--) x->chaves[j+1] = x->chaves[j];
    x->chaves[i] = y.chaves[1]; 
    x->num_chaves++;
    
    escrever_pagina(&y); escrever_pagina(&z);
    if (x == raiz_virtual) raiz_modificada = 1; else escrever_pagina(x);
}

void insert_non_full(Pagina *x, Registro k) {
    int i = x->num_chaves - 1;
    if (x->folha) {
        while (i >= 0 && comparar(k.nome, k.limiar, x->chaves[i].nome, x->chaves[i].limiar) < 0) {
            x->chaves[i+1] = x->chaves[i]; i--;
        }
        x->chaves[i+1] = k;
        x->chaves[i+1].removido = 0; 
        x->num_chaves++;
        if (x == raiz_virtual) raiz_modificada = 1; else escrever_pagina(x);
    } else {
        while (i >= 0 && comparar(k.nome, k.limiar, x->chaves[i].nome, x->chaves[i].limiar) < 0) i--;
        i++;
        Pagina filho; ler_pagina(x->filhos[i], &filho);
        if (filho.num_chaves == MAX_CHAVES) {
            split_child(x, i);
            if (comparar(k.nome, k.limiar, x->chaves[i].nome, x->chaves[i].limiar) > 0) {
                i++; ler_pagina(x->filhos[i], &filho);
            }
        }
        insert_non_full(&filho, k);
    }
}

void inserir_chave(Registro k) {
    Pagina p_ex; int idx;
    int status = buscar_status(raiz_virtual->meu_offset, k.nome, k.limiar, &p_ex, &idx);
    
    if (status == 1) { printf("Erro: Chave ja existe.\n"); return; }
    if (status == 2) {
        // Ressuscita chave removida
        p_ex.chaves[idx] = k;
        p_ex.chaves[idx].removido = 0;
        if (p_ex.meu_offset == raiz_virtual->meu_offset) { *raiz_virtual = p_ex; raiz_modificada = 1; }
        else escrever_pagina(&p_ex);
        printf("Chave reativada.\n");
        return;
    }

    if (raiz_virtual->num_chaves == MAX_CHAVES) {
        Pagina s; s.meu_offset = alocar_pagina(); s.folha = 0; s.num_chaves = 0;
        Pagina antiga = *raiz_virtual; antiga.meu_offset = alocar_pagina();
        escrever_pagina(&antiga);
        s.filhos[0] = antiga.meu_offset;
        *raiz_virtual = s; 
        split_child(raiz_virtual, 0);
        insert_non_full(raiz_virtual, k);
        raiz_modificada = 1;
    } else insert_non_full(raiz_virtual, k);
}

// --- Remoção Lógica ---

int remover_logico_rec(long offset, char *nome, int limiar) {
    Pagina p;
    if (raiz_virtual && offset == raiz_virtual->meu_offset) p = *raiz_virtual;
    else ler_pagina(offset, &p);

    int i = 0;
    while (i < p.num_chaves && comparar(nome, limiar, p.chaves[i].nome, p.chaves[i].limiar) > 0) i++;

    if (i < p.num_chaves && comparar(nome, limiar, p.chaves[i].nome, p.chaves[i].limiar) == 0) {
        if (p.chaves[i].removido) return 0;
        p.chaves[i].removido = 1; // Marca flag
        if (raiz_virtual && offset == raiz_virtual->meu_offset) { *raiz_virtual = p; raiz_modificada = 1; }
        else escrever_pagina(&p);
        return 1;
    }
    if (p.folha) return 0;
    return remover_logico_rec(p.filhos[i], nome, limiar);
}

int remover_chave(char *nome, int limiar) {
    if (!raiz_virtual) return 0;
    return remover_logico_rec(raiz_virtual->meu_offset, nome, limiar);
}

// --- Listagem e Reconstrução ---

void percorrer_rec(long offset) {
    if (offset == -1) return;
    Pagina p;
    if (raiz_virtual && offset == raiz_virtual->meu_offset) p = *raiz_virtual;
    else ler_pagina(offset, &p);

    for (int i = 0; i < p.num_chaves; i++) {
        if (!p.folha) percorrer_rec(p.filhos[i]);
        if (!p.chaves[i].removido) printf("Nome: %s | Limiar: %d\n", p.chaves[i].nome, p.chaves[i].limiar);
    }
    if (!p.folha) percorrer_rec(p.filhos[p.num_chaves]);
}

void percorrer_in_order() {
    if (raiz_virtual && raiz_virtual->num_chaves > 0) percorrer_rec(raiz_virtual->meu_offset);
}

// Copia dados da árvore antiga (.old) para a nova, ignorando removidos
void copiar_dados_rec(long offset_old, char *arq_tree_old, FILE *arq_dat_old, FILE *arq_dat_new) {
    Pagina p;
    ler_pagina_generica(arq_tree_old, offset_old, &p);

    for (int i = 0; i < p.num_chaves; i++) {
        if (!p.folha && p.filhos[i] != -1) copiar_dados_rec(p.filhos[i], arq_tree_old, arq_dat_old, arq_dat_new);
        
        if (!p.chaves[i].removido) {
            fseek(arq_dat_old, p.chaves[i].posicao_dados, SEEK_SET);
            int w, h, l;
            if (fscanf(arq_dat_old, "%d %d %d", &w, &h, &l) == 3) {
                long nova_pos = ftell(arq_dat_new);
                fprintf(arq_dat_new, "%d %d %d ", w, h, l);
                fgetc(arq_dat_old); // Pula espaço
                char c;
                while((c = fgetc(arq_dat_old)) != EOF && c != '\n') fputc(c, arq_dat_new);
                fputc('\n', arq_dat_new);
                
                Registro reg_novo = p.chaves[i];
                reg_novo.posicao_dados = nova_pos;
                reg_novo.removido = 0;
                inserir_chave(reg_novo);
            }
        }
    }
    if (!p.folha && p.filhos[p.num_chaves] != -1) copiar_dados_rec(p.filhos[p.num_chaves], arq_tree_old, arq_dat_old, arq_dat_new);
}

void reconstruir_banco_completo() {
    finalizar_arvore();
    
    // Backup dos arquivos atuais
    remove("banco.btree.old"); remove("banco.dat.old");
    rename(ARQUIVO_ARVORE, "banco.btree.old");
    rename(ARQUIVO_DADOS, "banco.dat.old");
    
    inicializar_arvore(); // Cria nova árvore zerada
    
    FILE *old_dat = fopen("banco.dat.old", "rb");
    FILE *new_dat = fopen(ARQUIVO_DADOS, "wb"); 
    
    if (!old_dat || !new_dat) {
        printf("Erro na reconstrução. Restaurando...\n");
        if(old_dat) fclose(old_dat); if(new_dat) fclose(new_dat);
        rename("banco.btree.old", ARQUIVO_ARVORE);
        rename("banco.dat.old", ARQUIVO_DADOS);
        inicializar_arvore(); return;
    }

    long raiz_old = ler_header_raiz("banco.btree.old");
    if (raiz_old != -1) copiar_dados_rec(raiz_old, "banco.btree.old", old_dat, new_dat);
    
    fclose(old_dat); fclose(new_dat);
    // remove("banco.btree.old"); remove("banco.dat.old"); // Descomente para apagar backups
    printf("Reconstrução concluída.\n");
}

void inicializar_arvore() {
    long raiz_off = ler_header_raiz(ARQUIVO_ARVORE);
    raiz_virtual = (Pagina *)malloc(sizeof(Pagina));
    if (raiz_off == -1) {
        raiz_virtual->num_chaves = 0; raiz_virtual->folha = 1;
        raiz_virtual->meu_offset = sizeof(CabecalhoArquivo);
        for(int i=0; i<ORDEM; i++) raiz_virtual->filhos[i] = -1;
        atualizar_header_raiz(raiz_virtual->meu_offset);
    } else ler_pagina(raiz_off, raiz_virtual);
    raiz_modificada = 0;
}

void finalizar_arvore() {
    if (raiz_virtual) { escrever_pagina(raiz_virtual); free(raiz_virtual); raiz_virtual = NULL; }
}