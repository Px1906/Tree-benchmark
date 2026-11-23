#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "rb.h"

// Implementação baseada no CLRS (Cormen et al., cap. 13), com nó sentinela 'nil'.

// ---------- rotações (cada chamada conta 1) ----------
static void rotacao_esquerda(ArvoreRB *t, NoRB *x) {
    NoRB *y = x->dir;
    x->dir = y->esq;
    if (y->esq != t->nil)
        y->esq->pai = x;
    y->pai = x->pai;
    if (x->pai == t->nil)
        t->raiz = y;
    else if (x == x->pai->esq)
        x->pai->esq = y;
    else
        x->pai->dir = y;
    y->esq = x;
    x->pai = y;
    t->rotacoes++;
}

static void rotacao_direita(ArvoreRB *t, NoRB *y) {
    NoRB *x = y->esq;
    y->esq = x->dir;
    if (x->dir != t->nil)
        x->dir->pai = y;
    x->pai = y->pai;
    if (y->pai == t->nil)
        t->raiz = x;
    else if (y == y->pai->dir)
        y->pai->dir = x;
    else
        y->pai->esq = x;
    x->dir = y;
    y->pai = x;
    t->rotacoes++;
}

// ---------- inserção ----------
// O novo nó z entra vermelho. Só a propriedade 4 (vermelho não tem filho
// vermelho) pode ser violada, quando o pai de z também é vermelho.
// Quem decide o caso é o TIO de z (irmão do pai):
//   caso 1: tio vermelho              -> recolore pai, tio e avô; sobe z para o avô
//   caso 2: tio preto, z em "triângulo" -> rotaciona no pai (vira caso 3)
//   caso 3: tio preto, z em "linha"     -> recolore e rotaciona no avô
static void inserir_ajuste(ArvoreRB *t, NoRB *z) {
    while (z->pai->cor == VERMELHO) {
        NoRB *avo = z->pai->pai;
        if (z->pai == avo->esq) {
            NoRB *tio = avo->dir;
            if (tio->cor == VERMELHO) {                 // caso 1
                z->pai->cor = PRETO;
                tio->cor = PRETO;
                avo->cor = VERMELHO;
                z = avo;
            } else {
                if (z == z->pai->dir) {                 // caso 2 (triângulo)
                    z = z->pai;
                    rotacao_esquerda(t, z);
                }
                z->pai->cor = PRETO;                    // caso 3 (linha)
                z->pai->pai->cor = VERMELHO;
                rotacao_direita(t, z->pai->pai);
            }
        } else {                                        // espelho: pai é filho direito
            NoRB *tio = avo->esq;
            if (tio->cor == VERMELHO) {                 // caso 1
                z->pai->cor = PRETO;
                tio->cor = PRETO;
                avo->cor = VERMELHO;
                z = avo;
            } else {
                if (z == z->pai->esq) {                 // caso 2 (triângulo)
                    z = z->pai;
                    rotacao_direita(t, z);
                }
                z->pai->cor = PRETO;                    // caso 3 (linha)
                z->pai->pai->cor = VERMELHO;
                rotacao_esquerda(t, z->pai->pai);
            }
        }
    }
    t->raiz->cor = PRETO;                               // propriedade 2
}

int rb_inserir(ArvoreRB *t, int chave) {
    NoRB *pai = t->nil;
    NoRB *x = t->raiz;
    while (x != t->nil) {                               // descida de ABB comum
        pai = x;
        if (chave < x->chave)      x = x->esq;
        else if (chave > x->chave) x = x->dir;
        else                       return 0;            // chave repetida
    }

    NoRB *z = malloc(sizeof(NoRB));
    if (!z) {
        perror("malloc");
        exit(1);
    }
    z->chave = chave;
    z->cor = VERMELHO;
    z->esq = z->dir = t->nil;
    z->pai = pai;
    if (pai == t->nil)         t->raiz = z;
    else if (chave < pai->chave) pai->esq = z;
    else                       pai->dir = z;

    inserir_ajuste(t, z);
    return 1;
}

// ---------- busca ----------
static NoRB *buscar_no(const ArvoreRB *t, int chave) {
    NoRB *no = t->raiz;
    while (no != t->nil) {
        if (chave < no->chave)      no = no->esq;
        else if (chave > no->chave) no = no->dir;
        else                        return no;
    }
    return t->nil;
}

int rb_buscar(const ArvoreRB *t, int chave) {
    return buscar_no(t, chave) != t->nil;
}

// ---------- remoção (bônus) ----------
static void transplantar(ArvoreRB *t, NoRB *u, NoRB *v) {
    if (u->pai == t->nil)      t->raiz = v;
    else if (u == u->pai->esq) u->pai->esq = v;
    else                       u->pai->dir = v;
    v->pai = u->pai;                                    // vale mesmo se v == nil
}

static NoRB *minimo(const ArvoreRB *t, NoRB *no) {
    while (no->esq != t->nil)
        no = no->esq;
    return no;
}

// x carrega um "preto extra". Quem decide o caso é o IRMÃO w de x:
//   caso 1: w vermelho                          -> rotaciona no pai (vira caso 2, 3 ou 4)
//   caso 2: w preto com dois filhos pretos      -> pinta w de vermelho; sobe x
//   caso 3: w preto, filho próximo vermelho     -> rotaciona em w (vira caso 4)
//   caso 4: w preto, filho distante vermelho    -> rotaciona no pai e termina
static void remover_ajuste(ArvoreRB *t, NoRB *x) {
    while (x != t->raiz && x->cor == PRETO) {
        if (x == x->pai->esq) {
            NoRB *w = x->pai->dir;
            if (w->cor == VERMELHO) {                   // caso 1
                w->cor = PRETO;
                x->pai->cor = VERMELHO;
                rotacao_esquerda(t, x->pai);
                w = x->pai->dir;
            }
            if (w->esq->cor == PRETO && w->dir->cor == PRETO) {   // caso 2
                w->cor = VERMELHO;
                x = x->pai;
            } else {
                if (w->dir->cor == PRETO) {             // caso 3
                    w->esq->cor = PRETO;
                    w->cor = VERMELHO;
                    rotacao_direita(t, w);
                    w = x->pai->dir;
                }
                w->cor = x->pai->cor;                   // caso 4
                x->pai->cor = PRETO;
                w->dir->cor = PRETO;
                rotacao_esquerda(t, x->pai);
                x = t->raiz;
            }
        } else {                                        // espelho: x é filho direito
            NoRB *w = x->pai->esq;
            if (w->cor == VERMELHO) {                   // caso 1
                w->cor = PRETO;
                x->pai->cor = VERMELHO;
                rotacao_direita(t, x->pai);
                w = x->pai->esq;
            }
            if (w->dir->cor == PRETO && w->esq->cor == PRETO) {   // caso 2
                w->cor = VERMELHO;
                x = x->pai;
            } else {
                if (w->esq->cor == PRETO) {             // caso 3
                    w->dir->cor = PRETO;
                    w->cor = VERMELHO;
                    rotacao_esquerda(t, w);
                    w = x->pai->esq;
                }
                w->cor = x->pai->cor;                   // caso 4
                x->pai->cor = PRETO;
                w->esq->cor = PRETO;
                rotacao_direita(t, x->pai);
                x = t->raiz;
            }
        }
    }
    x->cor = PRETO;
}

int rb_remover(ArvoreRB *t, int chave) {
    NoRB *z = buscar_no(t, chave);
    if (z == t->nil)
        return 0;

    NoRB *y = z;                    // nó que sai de fato da árvore
    Cor cor_original = y->cor;
    NoRB *x;                        // nó que ocupa a posição de y

    if (z->esq == t->nil) {
        x = z->dir;
        transplantar(t, z, z->dir);
    } else if (z->dir == t->nil) {
        x = z->esq;
        transplantar(t, z, z->esq);
    } else {                        // 2 filhos: y = sucessor de z
        y = minimo(t, z->dir);
        cor_original = y->cor;
        x = y->dir;
        if (y->pai == z) {
            x->pai = y;
        } else {
            transplantar(t, y, y->dir);
            y->dir = z->dir;
            y->dir->pai = y;
        }
        transplantar(t, z, y);
        y->esq = z->esq;
        y->esq->pai = y;
        y->cor = z->cor;
    }
    free(z);

    if (cor_original == PRETO)      // saiu um preto: altura negra quebrou
        remover_ajuste(t, x);
    return 1;
}

// ---------- altura ----------
static int altura(const ArvoreRB *t, const NoRB *no) {
    if (no == t->nil)
        return -1;
    int he = altura(t, no->esq);
    int hd = altura(t, no->dir);
    return 1 + (he > hd ? he : hd);
}

int rb_altura(const ArvoreRB *t) {
    return altura(t, t->raiz);
}

// ---------- validação ----------
// Propriedades verificadas:
//   ABB: min < chave < max em todos os nós (verificada primeiro em cada nó,
//        pois sem ela a árvore nem é de busca);
//   P1:  todo nó é vermelho ou preto;
//   P2:  a raiz é preta;
//   P3:  toda folha (nil) é preta;
//   P4:  nó vermelho tem os dois filhos pretos;
//   P5:  todo caminho de um nó até as folhas tem a mesma quantidade de pretos.
// Também confere os ponteiros 'pai'.
// Devolve a altura negra da subárvore, ou -1 se algo falhar.
static int validar(const ArvoreRB *t, const NoRB *no, long min, long max) {
    if (no == t->nil)
        return 1;                                       // folha conta como preta
    if (no->chave <= min || no->chave >= max) {
        fprintf(stderr, "[RB] chave %d viola a propriedade de ABB\n", no->chave);
        return -1;
    }
    if (no->cor != VERMELHO && no->cor != PRETO) {
        fprintf(stderr, "[RB] P1: nó %d sem cor válida\n", no->chave);
        return -1;
    }
    if (no->cor == VERMELHO && (no->esq->cor == VERMELHO || no->dir->cor == VERMELHO)) {
        fprintf(stderr, "[RB] P4: nó vermelho %d com filho vermelho\n", no->chave);
        return -1;
    }
    if ((no->esq != t->nil && no->esq->pai != no) ||
        (no->dir != t->nil && no->dir->pai != no)) {
        fprintf(stderr, "[RB] ponteiro 'pai' incorreto abaixo do nó %d\n", no->chave);
        return -1;
    }
    int ne = validar(t, no->esq, min, no->chave);
    if (ne < 0)
        return -1;
    int nd = validar(t, no->dir, no->chave, max);
    if (nd < 0)
        return -1;
    if (ne != nd) {
        fprintf(stderr, "[RB] P5: nó %d com alturas negras %d e %d\n", no->chave, ne, nd);
        return -1;
    }
    return ne + (no->cor == PRETO);
}

int rb_validar(const ArvoreRB *t) {
    if (t->nil->cor != PRETO) {
        fprintf(stderr, "[RB] P3: sentinela nil não é preta\n");
        return 0;
    }
    if (t->raiz->cor != PRETO) {
        fprintf(stderr, "[RB] P2: raiz não é preta\n");
        return 0;
    }
    if (t->raiz != t->nil && t->raiz->pai != t->nil) {
        fprintf(stderr, "[RB] raiz com ponteiro 'pai' incorreto\n");
        return 0;
    }
    return validar(t, t->raiz, LONG_MIN, LONG_MAX) >= 0;
}

// ---------- criação e liberação ----------
void rb_iniciar(ArvoreRB *t) {
    t->nil = malloc(sizeof(NoRB));
    if (!t->nil) {
        perror("malloc");
        exit(1);
    }
    t->nil->chave = 0;
    t->nil->cor = PRETO;
    t->nil->esq = t->nil->dir = t->nil->pai = t->nil;
    t->raiz = t->nil;
    t->rotacoes = 0;
}

static void liberar(ArvoreRB *t, NoRB *no) {
    if (no == t->nil)
        return;
    liberar(t, no->esq);
    liberar(t, no->dir);
    free(no);
}

void rb_liberar(ArvoreRB *t) {
    liberar(t, t->raiz);
    free(t->nil);
    t->raiz = t->nil = NULL;
}
