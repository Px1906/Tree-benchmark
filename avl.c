#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "avl.h"

// ---------- utilidades ----------
static int h(const NoAVL *no) {
    return no ? no->altura : 0;
}

static int maior(int a, int b) {
    return a > b ? a : b;
}

static void atualizar_altura(NoAVL *no) {
    no->altura = 1 + maior(h(no->esq), h(no->dir));
}

// fator de balanceamento = altura(esq) - altura(dir)
static int fator(const NoAVL *no) {
    return h(no->esq) - h(no->dir);
}

static NoAVL *novo_no(int chave) {
    NoAVL *no = malloc(sizeof(NoAVL));
    if (!no) {
        perror("malloc");
        exit(1);
    }
    no->chave = chave;
    no->altura = 1;
    no->esq = no->dir = NULL;
    return no;
}

// ---------- rotações (cada chamada conta 1) ----------
/*        y                x
 *       / \              / \
 *      x   C    ==>     A   y
 *     / \                  / \
 *    A   B                B   C
 * (rotação à direita; a rotação à esquerda é o espelho)
 */
static NoAVL *rotacao_direita(NoAVL *y, long *rotacoes) {
    NoAVL *x = y->esq;
    y->esq = x->dir;
    x->dir = y;
    atualizar_altura(y);        // y agora é filho de x: atualiza y primeiro
    atualizar_altura(x);
    (*rotacoes)++;
    return x;
}

static NoAVL *rotacao_esquerda(NoAVL *x, long *rotacoes) {
    NoAVL *y = x->dir;
    x->dir = y->esq;
    y->esq = x;
    atualizar_altura(x);
    atualizar_altura(y);
    (*rotacoes)++;
    return y;
}

// Restaura o balanceamento de 'no' (usado na inserção e na remoção).
// Devolve a nova raiz da subárvore.
static NoAVL *balancear(NoAVL *no, long *rotacoes) {
    atualizar_altura(no);
    int fb = fator(no);

    if (fb > 1) {                                   // pesado à esquerda
        if (fator(no->esq) < 0)                     // caso LR: filho pesado à direita
            no->esq = rotacao_esquerda(no->esq, rotacoes);
        return rotacao_direita(no, rotacoes);       // caso LL (ou 2ª rotação do LR)
    }
    if (fb < -1) {                                  // pesado à direita
        if (fator(no->dir) > 0)                     // caso RL: filho pesado à esquerda
            no->dir = rotacao_direita(no->dir, rotacoes);
        return rotacao_esquerda(no, rotacoes);      // caso RR (ou 2ª rotação do RL)
    }
    return no;                                      // |fb| <= 1: nada a fazer
}

// ---------- inserção ----------
static NoAVL *inserir(NoAVL *no, int chave, long *rotacoes, int *inseriu) {
    if (!no) {
        *inseriu = 1;
        return novo_no(chave);
    }
    if (chave < no->chave)
        no->esq = inserir(no->esq, chave, rotacoes, inseriu);
    else if (chave > no->chave)
        no->dir = inserir(no->dir, chave, rotacoes, inseriu);
    else
        return no;                                  // chave repetida: não insere
    return balancear(no, rotacoes);
}

int avl_inserir(ArvoreAVL *t, int chave) {
    int inseriu = 0;
    t->raiz = inserir(t->raiz, chave, &t->rotacoes, &inseriu);
    return inseriu;
}

// ---------- busca ----------
int avl_buscar(const ArvoreAVL *t, int chave) {
    const NoAVL *no = t->raiz;
    while (no) {
        if (chave < no->chave)      no = no->esq;
        else if (chave > no->chave) no = no->dir;
        else                        return 1;
    }
    return 0;
}

// ---------- remoção (bônus) ----------
static NoAVL *remover(NoAVL *no, int chave, long *rotacoes, int *removeu) {
    if (!no)
        return NULL;                                // chave não existe
    if (chave < no->chave) {
        no->esq = remover(no->esq, chave, rotacoes, removeu);
    } else if (chave > no->chave) {
        no->dir = remover(no->dir, chave, rotacoes, removeu);
    } else {
        *removeu = 1;
        if (!no->esq || !no->dir) {                 // 0 ou 1 filho: sobe o filho
            NoAVL *filho = no->esq ? no->esq : no->dir;
            free(no);
            return filho;
        }
        // 2 filhos: copia o sucessor (menor da direita) e remove-o da direita
        NoAVL *suc = no->dir;
        while (suc->esq)
            suc = suc->esq;
        no->chave = suc->chave;
        no->dir = remover(no->dir, suc->chave, rotacoes, removeu);
    }
    return balancear(no, rotacoes);
}

int avl_remover(ArvoreAVL *t, int chave) {
    int removeu = 0;
    t->raiz = remover(t->raiz, chave, &t->rotacoes, &removeu);
    return removeu;
}

// ---------- altura ----------
// Calculada percorrendo a árvore (não usa o campo 'altura'),
// do mesmo jeito que na Rubro-Negra.
static int altura(const NoAVL *no) {
    if (!no)
        return -1;
    return 1 + maior(altura(no->esq), altura(no->dir));
}

int avl_altura(const ArvoreAVL *t) {
    return altura(t->raiz);
}

// ---------- validação ----------
// Devolve a altura (em níveis) da subárvore, ou -1 se alguma propriedade falhar.
// Em cada nó verifica, nesta ordem:
//   1. propriedade de ABB: min < chave < max (limites herdados dos ancestrais);
//   2. campo 'altura' coerente com as alturas reais dos filhos;
//   3. fator de balanceamento entre -1 e 1.
// A ABB vem primeiro porque, sem ela, a árvore nem é de busca e o balanceamento
// perde o sentido.
static int validar(const NoAVL *no, long min, long max) {
    if (!no)
        return 0;
    if (no->chave <= min || no->chave >= max) {
        fprintf(stderr, "[AVL] chave %d viola a propriedade de ABB\n", no->chave);
        return -1;
    }
    int he = validar(no->esq, min, no->chave);
    if (he < 0)
        return -1;
    int hd = validar(no->dir, no->chave, max);
    if (hd < 0)
        return -1;
    if (no->altura != 1 + maior(he, hd)) {
        fprintf(stderr, "[AVL] nó %d com altura armazenada incorreta\n", no->chave);
        return -1;
    }
    if (he - hd > 1 || he - hd < -1) {
        fprintf(stderr, "[AVL] nó %d com fator de balanceamento %d\n", no->chave, he - hd);
        return -1;
    }
    return 1 + maior(he, hd);
}

int avl_validar(const ArvoreAVL *t) {
    return validar(t->raiz, LONG_MIN, LONG_MAX) >= 0;
}

// ---------- criação e liberação ----------
void avl_iniciar(ArvoreAVL *t) {
    t->raiz = NULL;
    t->rotacoes = 0;
}

static void liberar(NoAVL *no) {
    if (!no)
        return;
    liberar(no->esq);
    liberar(no->dir);
    free(no);
}

void avl_liberar(ArvoreAVL *t) {
    liberar(t->raiz);
    t->raiz = NULL;
}
