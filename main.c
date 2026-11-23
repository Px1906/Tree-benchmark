#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "avl.h"
#include "rb.h"

#define QTD(v) (int)(sizeof(v) / sizeof((v)[0]))

// ====================== CONFIGURAÇÃO ======================

const char *arvores[] = {"avl", "rb"};

const char *cenarios[] = {"crescente", "decrescente", "aleatorio"};

const int tamanhos_vetor[] = {1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000};

#define REPETICOES 50

// 1 = mede também a remoção de todas as chaves em ordem aleatória (bônus)
#define MEDIR_REMOCAO 1

// 1 = roda a validação após cada construção/remoção, FORA da medição de tempo
//     (garante a corretude durante o experimento, mas deixa a execução mais lenta)
#define VALIDAR_NO_BENCHMARK 0

// arvores:  0 = avl, 1 = rb
const int arvores_a_rodar[] = {0, 1};

// cenarios: 0 = crescente, 1 = decrescente, 2 = aleatorio
const int cenarios_a_rodar[] = {0, 1, 2};

// tamanhos: 0 = 1000,   1 = 2000,   2 = 5000,   3 = 10000,  4 = 20000,
//           5 = 50000,  6 = 100000, 7 = 200000, 8 = 500000, 9 = 1000000
const int tamanhos_a_rodar[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

// tamanho usado no modo de demonstração da validação (./main validar)
#define N_VALIDACAO 10000
// =======================================================================

typedef struct {
    double tempo_construcao, tempo_busca, tempo_remocao;
    int altura;
    long rotacoes, rotacoes_remocao;
} Medicao;

double segundos(struct timespec inicio, struct timespec fim) {
    return (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
}

void falhar(const char *arvore, const char *etapa) {
    fprintf(stderr, "ERRO: %s inválida %s\n", arvore, etapa);
    exit(1);
}

// ---------- geração das entradas ----------
void preencher_crescente(int *v, int n) {
    for (int k = 0; k < n; k++) {
        v[k] = k + 1;
    }
}

void preencher_decrescente(int *v, int n) {
    for (int k = 0; k < n; k++) {
        v[k] = n - k;
    }
}

// Permutação aleatória de 1..n (Fisher-Yates): chaves distintas por construção.
void preencher_aleatorio(int *v, int n) {
    preencher_crescente(v, n);
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);        // RAND_MAX da glibc = 2^31 - 1 > n
        int tmp = v[i];
        v[i] = v[j];
        v[j] = tmp;
    }
}

void preencher(int c, int *v, int n) {
    switch (c) {
    case 0:
        preencher_crescente(v, n);
        break;
    case 1:
        preencher_decrescente(v, n);
        break;
    case 2:
        preencher_aleatorio(v, n);
        break;
    default:
        break;
    }
}

// n chaves sorteadas em 1..n: todas existem na árvore em qualquer cenário.
void sortear_buscas(int *b, int n) {
    for (int k = 0; k < n; k++) {
        b[k] = 1 + rand() % n;
    }
}

// ---------- execução de uma repetição em cada árvore ----------
// As duas funções seguem exatamente os mesmos passos; só mudam as chamadas.
// A validação (quando ativada) roda entre as medições, nunca dentro delas.

void executar_avl(const int *chaves, const int *buscas, const int *remocao, int n, Medicao *m) {
    struct timespec inicio, fim;
    ArvoreAVL t;
    avl_iniciar(&t);

    clock_gettime(CLOCK_MONOTONIC, &inicio);
    for (int i = 0; i < n; i++)
        avl_inserir(&t, chaves[i]);
    clock_gettime(CLOCK_MONOTONIC, &fim);
    m->tempo_construcao = segundos(inicio, fim);
    m->rotacoes = t.rotacoes;
    m->altura = avl_altura(&t);
    if (VALIDAR_NO_BENCHMARK && !avl_validar(&t))
        falhar("AVL", "após a construção");

    int encontradas = 0;
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    for (int i = 0; i < n; i++)
        encontradas += avl_buscar(&t, buscas[i]);
    clock_gettime(CLOCK_MONOTONIC, &fim);
    m->tempo_busca = segundos(inicio, fim);
    // usar o resultado impede o compilador de descartar as buscas
    if (encontradas != n)
        falhar("AVL", "na busca (chave existente não encontrada)");

    if (MEDIR_REMOCAO) {
        t.rotacoes = 0;
        clock_gettime(CLOCK_MONOTONIC, &inicio);
        for (int i = 0; i < n; i++)
            avl_remover(&t, remocao[i]);
        clock_gettime(CLOCK_MONOTONIC, &fim);
        m->tempo_remocao = segundos(inicio, fim);
        m->rotacoes_remocao = t.rotacoes;
        if (t.raiz != NULL || (VALIDAR_NO_BENCHMARK && !avl_validar(&t)))
            falhar("AVL", "após a remoção");
    }
    avl_liberar(&t);
}

void executar_rb(const int *chaves, const int *buscas, const int *remocao, int n, Medicao *m) {
    struct timespec inicio, fim;
    ArvoreRB t;
    rb_iniciar(&t);

    clock_gettime(CLOCK_MONOTONIC, &inicio);
    for (int i = 0; i < n; i++)
        rb_inserir(&t, chaves[i]);
    clock_gettime(CLOCK_MONOTONIC, &fim);
    m->tempo_construcao = segundos(inicio, fim);
    m->rotacoes = t.rotacoes;
    m->altura = rb_altura(&t);
    if (VALIDAR_NO_BENCHMARK && !rb_validar(&t))
        falhar("RB", "após a construção");

    int encontradas = 0;
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    for (int i = 0; i < n; i++)
        encontradas += rb_buscar(&t, buscas[i]);
    clock_gettime(CLOCK_MONOTONIC, &fim);
    m->tempo_busca = segundos(inicio, fim);
    if (encontradas != n)
        falhar("RB", "na busca (chave existente não encontrada)");

    if (MEDIR_REMOCAO) {
        t.rotacoes = 0;
        clock_gettime(CLOCK_MONOTONIC, &inicio);
        for (int i = 0; i < n; i++)
            rb_remover(&t, remocao[i]);
        clock_gettime(CLOCK_MONOTONIC, &fim);
        m->tempo_remocao = segundos(inicio, fim);
        m->rotacoes_remocao = t.rotacoes;
        if (t.raiz != t.nil || (VALIDAR_NO_BENCHMARK && !rb_validar(&t)))
            falhar("RB", "após a remoção");
    }
    rb_liberar(&t);
}

void executar(int a, const int *chaves, const int *buscas, const int *remocao, int n, Medicao *m) {
    memset(m, 0, sizeof(*m));
    if (a == 0)
        executar_avl(chaves, buscas, remocao, n, m);
    else
        executar_rb(chaves, buscas, remocao, n, m);
}

// ---------- benchmark ----------
int benchmark(void) {
    mkdir("resultados", 0755);

    for (int ic = 0; ic < QTD(cenarios_a_rodar); ic++) {
        int c = cenarios_a_rodar[ic];
        for (int it = 0; it < QTD(tamanhos_a_rodar); it++) {
            int n = tamanhos_vetor[tamanhos_a_rodar[it]];
            int *chaves = malloc(n * sizeof(int));
            int *buscas = malloc(n * sizeof(int));
            int *remocao = malloc(n * sizeof(int));
            FILE *arquivos[QTD(arvores)] = {NULL};
            if (!chaves || !buscas || !remocao) {
                perror("malloc");
                return 1;
            }

            // um arquivo por árvore: resultados/<arvore>/<cenario>/<n>.csv
            for (int ia = 0; ia < QTD(arvores_a_rodar); ia++) {
                int a = arvores_a_rodar[ia];
                char pasta[96], nome[128];
                snprintf(pasta, sizeof(pasta), "resultados/%s", arvores[a]);
                mkdir(pasta, 0755);
                snprintf(pasta, sizeof(pasta), "resultados/%s/%s", arvores[a], cenarios[c]);
                mkdir(pasta, 0755);
                snprintf(nome, sizeof(nome), "%s/%07d.csv", pasta, n);
                arquivos[a] = fopen(nome, "w");
                if (!arquivos[a]) {
                    perror(nome);
                    return 1;
                }
                fprintf(arquivos[a], "arvore,cenario,n,repeticao,tempo_construcao_s,tempo_busca_s,"
                                     "altura,rotacoes,tempo_remocao_s,rotacoes_remocao\n");
            }

            for (int j = 0; j < REPETICOES; j++) {
                // Gera as entradas UMA vez por repetição e entrega os mesmos vetores
                // (chaves, buscas e ordem de remoção) às duas árvores.
                // A semente muda a cada repetição: vetor aleatório diferente em cada uma.
                srand(j + 1);
                preencher(c, chaves, n);
                sortear_buscas(buscas, n);
                preencher_aleatorio(remocao, n);

                for (int ia = 0; ia < QTD(arvores_a_rodar); ia++) {
                    int a = arvores_a_rodar[ia];
                    Medicao m;
                    executar(a, chaves, buscas, remocao, n, &m);
                    fprintf(arquivos[a], "%s,%s,%d,%d,%.9f,%.9f,%d,%ld,",
                            arvores[a], cenarios[c], n, j + 1,
                            m.tempo_construcao, m.tempo_busca, m.altura, m.rotacoes);
                    if (MEDIR_REMOCAO)
                        fprintf(arquivos[a], "%.9f,%ld\n", m.tempo_remocao, m.rotacoes_remocao);
                    else
                        fprintf(arquivos[a], ",\n");
                    fflush(arquivos[a]);
                }
            }

            for (int ia = 0; ia < QTD(arvores_a_rodar); ia++)
                fclose(arquivos[arvores_a_rodar[ia]]);
            free(chaves);
            free(buscas);
            free(remocao);
            printf("[ok] %-11s n = %7d\n", cenarios[c], n);
            fflush(stdout);
        }
    }
    return 0;
}

// ---------- demonstração da validação (./main validar) ----------
const char *ok(int valida) {
    return valida ? "OK" : "FALHOU";
}

int validar(void) {
    int n = N_VALIDACAO;
    int *chaves = malloc(n * sizeof(int));
    int *remocao = malloc(n * sizeof(int));
    int erros = 0;
    if (!chaves || !remocao) {
        perror("malloc");
        return 1;
    }

    printf("Validação com n = %d (mesmas chaves para as duas árvores)\n\n", n);
    for (int c = 0; c < QTD(cenarios); c++) {
        srand(42);
        preencher(c, chaves, n);
        preencher_aleatorio(remocao, n);

        ArvoreAVL avl;
        ArvoreRB rb;
        avl_iniciar(&avl);
        rb_iniciar(&rb);
        for (int i = 0; i < n; i++) {
            avl_inserir(&avl, chaves[i]);
            rb_inserir(&rb, chaves[i]);
        }

        // todas as chaves 1..n presentes e chaves fora do intervalo ausentes
        int busca_avl = 1, busca_rb = 1;
        for (int k = 1; k <= n; k++) {
            busca_avl &= avl_buscar(&avl, k);
            busca_rb &= rb_buscar(&rb, k);
        }
        busca_avl &= !avl_buscar(&avl, 0) && !avl_buscar(&avl, n + 1);
        busca_rb &= !rb_buscar(&rb, 0) && !rb_buscar(&rb, n + 1);

        int v_avl = avl_validar(&avl), v_rb = rb_validar(&rb);
        printf("[%s]\n", cenarios[c]);
        printf("  após inserir %d chaves: AVL %s (altura %d, %ld rotações) | RB %s (altura %d, %ld rotações)\n",
               n, ok(v_avl), avl_altura(&avl), avl.rotacoes, ok(v_rb), rb_altura(&rb), rb.rotacoes);
        printf("  busca:                  AVL %s | RB %s\n", ok(busca_avl), ok(busca_rb));
        erros += !v_avl + !v_rb + !busca_avl + !busca_rb;

        // remove metade em ordem aleatória, valida; depois remove o resto
        for (int i = 0; i < n / 2; i++) {
            avl_remover(&avl, remocao[i]);
            rb_remover(&rb, remocao[i]);
        }
        v_avl = avl_validar(&avl);
        v_rb = rb_validar(&rb);
        printf("  após remover metade:    AVL %s (altura %d) | RB %s (altura %d)\n",
               ok(v_avl), avl_altura(&avl), ok(v_rb), rb_altura(&rb));
        erros += !v_avl + !v_rb;

        for (int i = n / 2; i < n; i++) {
            avl_remover(&avl, remocao[i]);
            rb_remover(&rb, remocao[i]);
        }
        v_avl = avl_validar(&avl) && avl.raiz == NULL;
        v_rb = rb_validar(&rb) && rb.raiz == rb.nil;
        printf("  após remover tudo:      AVL %s (vazia) | RB %s (vazia)\n\n", ok(v_avl), ok(v_rb));
        erros += !v_avl + !v_rb;

        avl_liberar(&avl);
        rb_liberar(&rb);
    }

    // Teste negativo: corrompe as árvores de propósito para mostrar que a
    // validação detecta a violação (deve aparecer FALHOU com o motivo).
    printf("[teste negativo: árvores corrompidas de propósito]\n");
    ArvoreAVL avl;
    ArvoreRB rb;
    avl_iniciar(&avl);
    rb_iniciar(&rb);
    for (int k = 1; k <= 100; k++) {
        avl_inserir(&avl, k);
        rb_inserir(&rb, k);
    }
    fflush(stdout);
    avl.raiz->esq->chave = avl.raiz->chave + 1;          // quebra a ABB
    printf("  AVL com filho esquerdo > raiz: %s\n", ok(avl_validar(&avl)));
    fflush(stdout);
    rb.raiz->cor = VERMELHO;                             // quebra a P2
    printf("  RB com raiz vermelha:          %s\n", ok(rb_validar(&rb)));
    avl_liberar(&avl);
    rb_liberar(&rb);

    free(chaves);
    free(remocao);
    printf("\n%s\n", erros == 0 ? "Todas as validações passaram." : "HÁ VALIDAÇÕES COM FALHA.");
    return erros != 0;
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "validar") == 0)
        return validar();
    return benchmark();
}
