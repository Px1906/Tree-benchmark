# Tree Benchmark

Comparação experimental das árvores **AVL** e **Rubro-Negra** em C (inserção, busca, altura, rotações, validação e remoção), com análise estatística e gráficos em Python.

## Cenários

| Cenário       | Chaves inseridas                                                     |
|---------------|----------------------------------------------------------------------|
| `crescente`   | 1, 2, ..., n                                                         |
| `decrescente` | n, ..., 2, 1                                                         |
| `aleatorio`   | permutação aleatória de 1..n (Fisher-Yates), diferente a cada repetição |

Tamanhos testados: 1.000, 2.000, 5.000, 10.000, 20.000, 50.000, 100.000, 200.000, 500.000 e 1.000.000 chaves, com 50 repetições cada.

Em cada repetição, para cada árvore, são medidos:

- **tempo de construção**: inserir as n chaves numa árvore vazia;
- **tempo de busca**: n buscas de chaves existentes, sorteadas em 1..n;
- **altura** da árvore após a construção (em arestas: só a raiz = 0);
- **rotações** feitas na construção (rotação dupla conta como duas);
- **tempo e rotações de remoção** (bônus): remover as n chaves em ordem aleatória até a árvore ficar vazia.

Os tempos são medidos em lote com `clock_gettime(CLOCK_MONOTONIC)`. As chaves, as buscas e a ordem de remoção são geradas uma única vez por repetição (`srand(repeticao)`) e entregues às duas árvores, então AVL e Rubro-Negra recebem exatamente as mesmas entradas.

## Estrutura

```
.
├── main.c            # benchmark: gera as entradas, mede e grava os CSVs; modo de validação
├── avl.c / avl.h     # árvore AVL
├── rb.c / rb.h       # árvore Rubro-Negra (CLRS, com sentinela nil)
├── analise.py        # estatísticas, tabelas, gráficos e regressões
├── Makefile
├── requirements.txt  # dependências do Python
├── resultados/       # CSVs brutos gerados pelo ./main (não versionado)
└── saida/            # tabelas e gráficos gerados pelo analise.py (não versionado)
```

## Como executar

### 1. Benchmark (C)

```bash
make            # compila
./main validar  # demonstra a validação das duas árvores (ou: make validar)
./main          # roda o experimento e grava os CSVs em resultados/ (ou: make run)
```

Rode a partir da raiz do repositório. Com tudo selecionado são 60 arquivos (2 árvores × 3 cenários × 10 tamanhos), em `resultados/<arvore>/<cenario>/<n>.csv`, com `n` em 7 dígitos (por exemplo, `resultados/avl/crescente/0001000.csv`). Cada arquivo tem um cabeçalho e uma linha por repetição:

```
arvore,cenario,n,repeticao,tempo_construcao_s,tempo_busca_s,altura,rotacoes,tempo_remocao_s,rotacoes_remocao
```

As árvores, cenários e tamanhos a rodar, o número de repetições, a medição da remoção (`MEDIR_REMOCAO`) e a validação durante o benchmark (`VALIDAR_NO_BENCHMARK`, sempre fora da medição de tempo) ficam no bloco de configuração no topo do `main.c`.

#### Validação

`./main validar` constrói as duas árvores com n = 10.000 em cada cenário e verifica:

- **AVL**: propriedade de árvore binária de busca, altura armazenada em cada nó e fator de balanceamento entre −1 e 1;
- **Rubro-Negra**: propriedade de árvore binária de busca e as cinco propriedades (cor válida, raiz preta, folhas pretas, vermelho só com filhos pretos, mesma altura negra em todos os caminhos), além dos ponteiros `pai`;
- busca de todas as chaves inseridas e de chaves ausentes;
- validade após remover metade das chaves e após esvaziar a árvore;
- um teste negativo, que corrompe as árvores de propósito para mostrar que a validação detecta o erro.

### 2. Análise (Python)

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python analise.py
```

Os resultados são gravados na pasta `saida/`:

- `estatisticas_completas.csv`: média, desvio padrão, IC 95%, mínimo e máximo de cada tempo; média de altura e rotações, por árvore, cenário e n
- `tabela_<arvore>_<cenario>.csv`: tabela individual de cada combinação
- `grafico_<metrica>_<cenario>.png`: comparação entre as duas árvores (barras de erro com IC 95% nos tempos)
- `regressao_tempo.csv`: tempo de construção ajustado a `a·n·log2(n) + b`, comparado com `a·n + b` e `a·n² + b` pelo R²
- `regressao_altura.csv`: altura média ajustada a `a·log2(n) + b`, com `a` comparado a 1,44 (AVL) e 2 (Rubro-Negra)
- `limites_altura.csv`: altura média de cada n frente aos limites teóricos exatos

As opções da análise ficam no bloco de configuração no topo do `analise.py`.
