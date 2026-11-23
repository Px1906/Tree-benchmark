#ifndef AVL_H
#define AVL_H

typedef struct NoAVL {
    int chave;
    int altura;                 // altura do nó em níveis: folha = 1, NULL = 0
    struct NoAVL *esq, *dir;
} NoAVL;

typedef struct {
    NoAVL *raiz;
    long rotacoes;              // rotações simples realizadas (dupla conta 2)
} ArvoreAVL;

void avl_iniciar(ArvoreAVL *t);
int  avl_inserir(ArvoreAVL *t, int chave);    // 1 se inseriu, 0 se já existia
int  avl_buscar(const ArvoreAVL *t, int chave);
int  avl_remover(ArvoreAVL *t, int chave);    // 1 se removeu, 0 se não existia
int  avl_altura(const ArvoreAVL *t);          // em arestas: vazia = -1, só a raiz = 0
int  avl_validar(const ArvoreAVL *t);         // 1 se válida, 0 se não (motivo em stderr)
void avl_liberar(ArvoreAVL *t);

#endif
