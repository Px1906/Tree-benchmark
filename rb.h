#ifndef RB_H
#define RB_H

typedef enum { VERMELHO, PRETO } Cor;

typedef struct NoRB {
    int chave;
    Cor cor;
    struct NoRB *esq, *dir, *pai;
} NoRB;

typedef struct {
    NoRB *raiz;
    NoRB *nil;                  // sentinela: representa todas as folhas (sempre preta)
    long rotacoes;              // rotações simples realizadas (dupla conta 2)
} ArvoreRB;

void rb_iniciar(ArvoreRB *t);
int  rb_inserir(ArvoreRB *t, int chave);      // 1 se inseriu, 0 se já existia
int  rb_buscar(const ArvoreRB *t, int chave);
int  rb_remover(ArvoreRB *t, int chave);      // 1 se removeu, 0 se não existia
int  rb_altura(const ArvoreRB *t);            // em arestas: vazia = -1, só a raiz = 0
int  rb_validar(const ArvoreRB *t);           // 1 se válida, 0 se não (motivo em stderr)
void rb_liberar(ArvoreRB *t);

#endif
