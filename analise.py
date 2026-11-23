from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from scipy import stats

# ====================== CONFIGURAÇÃO (edite aqui) ======================
AQUI = Path(__file__).resolve().parent
RESULTADOS = AQUI / "resultados"
SAIDA = AQUI / "saida"
SAIDA.mkdir(exist_ok=True)

ARVORES = ["avl", "rb"]
CENARIOS = ["crescente", "decrescente", "aleatorio"]
CORES = {"avl": "tab:blue", "rb": "tab:red"}
NOMES = {"avl": "AVL", "rb": "Rubro-Negra"}
REPS_ESPERADAS = 50

# Métricas de tempo: média, desvio, IC 95%, mínimo e máximo
METRICAS_TEMPO = ["tempo_construcao_s", "tempo_busca_s", "tempo_remocao_s"]
# Métricas estruturais: só a média é exigida
METRICAS_MEDIA = ["altura", "rotacoes", "rotacoes_remocao"]

# O que gerar (True = gera, False = pula)
GERAR_TABELAS = True
GERAR_GRAFICOS = True                 # um gráfico por cenário e métrica
GERAR_REGRESSAO_TEMPO = True          # tempo de construção x n·log(n)
GERAR_REGRESSAO_ALTURA = True         # altura x a·log2(n) + b

# Limites teóricos de altura (coeficiente de log2 n)
LIMITE_TEORICO = {"avl": 1.44, "rb": 2.0}
# =======================================================================


# ---------- 1. Carregar ----------
def carregar():
    """Lê os CSVs que existirem e junta tudo (uma linha por repetição)."""
    partes = []
    for cen in CENARIOS:
        arquivos = sorted(RESULTADOS.glob(f"*/{cen}/*.csv"))
        if arquivos:
            partes.extend(pd.read_csv(caminho) for caminho in arquivos)
        else:
            print(f"[aviso] nenhum arquivo do cenário '{cen}' em resultados/, cenário ignorado.")
    if not partes:
        raise SystemExit("Nenhum CSV encontrado em resultados/. Rode ./main antes.")

    df = pd.concat(partes, ignore_index=True)
    df["arvore"] = df["arvore"].str.strip().str.lower()
    df["cenario"] = df["cenario"].str.strip().str.lower()

    obrigatorias = ["n", "repeticao", "tempo_construcao_s", "tempo_busca_s", "altura", "rotacoes"]
    for col in obrigatorias + ["tempo_remocao_s", "rotacoes_remocao"]:
        df[col] = pd.to_numeric(df[col], errors="coerce")
    invalidas = df[df[obrigatorias].isna().any(axis=1)]
    if len(invalidas) > 0:
        print(f"[ATENÇÃO] {len(invalidas)} linha(s) inválida(s) removida(s):")
        print(invalidas.to_string())
        df = df.drop(invalidas.index)
    df["n"] = df["n"].astype(int)

    contagem = df.groupby(["arvore", "cenario", "n"]).size()
    problemas = contagem[contagem != REPS_ESPERADAS]
    if len(problemas) > 0:
        print(f"[ATENÇÃO] grupos sem {REPS_ESPERADAS} repetições:")
        print(problemas.to_string())
    return df


# ---------- 2. Estatísticas ----------
def estatisticas(df):
    """Uma linha por (arvore, cenario, n). Para cada tempo: média, desvio, IC 95%,
    mínimo e máximo. Para altura e rotações: média."""
    g = df.groupby(["arvore", "cenario", "n"])
    r = g.size().rename("reps").reset_index()
    t = stats.t.ppf(0.975, r["reps"] - 1)            # t de Student, 95% bilateral
    for m in METRICAS_TEMPO:
        if df[m].isna().all():
            continue                                  # remoção desativada no main.c
        a = g[m].agg(["mean", "std", "min", "max"]).reset_index(drop=True)
        r[f"{m}_media"] = a["mean"]
        r[f"{m}_desvio"] = a["std"]
        r[f"{m}_ic95"] = t * a["std"] / np.sqrt(r["reps"])
        r[f"{m}_minimo"] = a["min"]
        r[f"{m}_maximo"] = a["max"]
    for m in METRICAS_MEDIA:
        if not df[m].isna().all():
            r[f"{m}_media"] = g[m].mean().reset_index(drop=True)
    return r


def dados(r, arv, cen):
    """Linhas de UMA árvore em UM cenário, ordenadas por n."""
    return r[(r["arvore"] == arv) & (r["cenario"] == cen)].sort_values("n")


# ---------- 3. Tabelas ----------
def salvar_tabelas(r):
    for arv in ARVORES:
        for cen in CENARIOS:
            d = dados(r, arv, cen)
            if not d.empty:
                d.drop(columns=["arvore", "cenario"]).to_csv(
                    SAIDA / f"tabela_{arv}_{cen}.csv", index=False)


# ---------- 4. Gráficos ----------
def grafico(r, cen, metrica):
    """Barras agrupadas (uma cor por árvore). Nos tempos, barras de erro = IC 95%."""
    coluna = f"{metrica}_media"
    if coluna not in r.columns:
        return
    d = r[r["cenario"] == cen]
    ns = sorted(d["n"].unique())
    x = np.arange(len(ns))
    larg = 0.38
    eh_tempo = metrica in METRICAS_TEMPO

    fig, ax = plt.subplots(figsize=(11, 6))
    for i, arv in enumerate(ARVORES):
        s = d[d["arvore"] == arv].set_index("n").reindex(ns)
        erro = s[f"{metrica}_ic95"] if eh_tempo else None
        ax.bar(x + (i - 0.5) * larg, s[coluna], larg, yerr=erro,
               capsize=3, label=NOMES[arv], color=CORES[arv])
    ax.set_xticks(x)
    ax.set_xticklabels(ns, rotation=45)
    ax.set_xlabel("Tamanho da entrada (n)")
    if eh_tempo or metrica.startswith("rotacoes"):
        ax.set_yscale("log")                          # valores em várias ordens de grandeza
    rotulo = {
        "tempo_construcao_s": "Tempo médio de construção (s, escala log)",
        "tempo_busca_s": "Tempo médio de n buscas (s, escala log)",
        "tempo_remocao_s": "Tempo médio de remoção de n chaves (s, escala log)",
        "altura": "Altura média",
        "rotacoes": "Rotações médias na construção (escala log)",
        "rotacoes_remocao": "Rotações médias na remoção (escala log)",
    }[metrica]
    ax.set_ylabel(rotulo)
    ax.set_title(f"{rotulo.split(' (')[0]}, cenário {cen}")
    ax.legend(title="Árvore")
    fig.tight_layout()
    fig.savefig(SAIDA / f"grafico_{metrica}_{cen}.png", dpi=200)
    plt.close(fig)


# ---------- 5. Regressões ----------
def ajuste_linear(x, y):
    """Mínimos quadrados y = a·x + b. Devolve (a, b, R²)."""
    a, b = np.polyfit(x, y, 1)
    pred = a * x + b
    r2 = 1 - np.sum((y - pred) ** 2) / np.sum((y - y.mean()) ** 2)
    return a, b, r2


def regressao_tempo(r):
    """Tempo de construção em função de n. Compara o ajuste T = a·n·log2(n) + b
    com T = a·n + b e T = a·n² + b pelo R²."""
    modelos = {
        "n": lambda n: n,
        "n·log2(n)": lambda n: n * np.log2(n),
        "n²": lambda n: n ** 2,
    }
    linhas = []
    for arv in ARVORES:
        for cen in CENARIOS:
            d = dados(r, arv, cen)
            if len(d) < 3:
                continue
            n = d["n"].to_numpy(float)
            y = d["tempo_construcao_s_media"].to_numpy(float)
            for nome, f in modelos.items():
                a, b, r2 = ajuste_linear(f(n), y)
                linhas.append({"arvore": arv, "cenario": cen, "modelo": f"a·{nome} + b",
                               "a": a, "b": b, "R2": round(r2, 6)})
    pd.DataFrame(linhas).to_csv(SAIDA / "regressao_tempo.csv", index=False)


def regressao_altura(r):
    """Altura média em função de n: h = a·log2(n) + b. Compara 'a' com o limite teórico."""
    linhas = []
    for arv in ARVORES:
        for cen in CENARIOS:
            d = dados(r, arv, cen)
            if len(d) < 3:
                continue
            x = np.log2(d["n"].to_numpy(float))
            y = d["altura_media"].to_numpy(float)
            a, b, r2 = ajuste_linear(x, y)
            linhas.append({"arvore": arv, "cenario": cen, "a": round(a, 4), "b": round(b, 4),
                           "R2": round(r2, 6), "limite_teorico_a": LIMITE_TEORICO[arv],
                           "dentro_do_limite": a <= LIMITE_TEORICO[arv]})
    pd.DataFrame(linhas).to_csv(SAIDA / "regressao_altura.csv", index=False)


def limites_altura(r):
    """Altura média de cada n comparada aos limites teóricos exatos:
    AVL: h < 1,4405·log2(n + 2) - 0,3277;  RB: h <= 2·log2(n + 1)."""
    d = r[["arvore", "cenario", "n", "altura_media"]].copy()
    n = d["n"].to_numpy(float)
    d["log2_n"] = np.log2(n)
    d["limite"] = np.where(d["arvore"] == "avl",
                           1.4405 * np.log2(n + 2) - 0.3277,
                           2 * np.log2(n + 1))
    d["respeita"] = d["altura_media"] <= d["limite"]
    d.sort_values(["arvore", "cenario", "n"]).to_csv(SAIDA / "limites_altura.csv", index=False)


# ---------- Main ----------
def main():
    df = carregar()
    r = estatisticas(df)
    cenarios_com_dados = [c for c in CENARIOS if c in set(r["cenario"])]

    r.to_csv(SAIDA / "estatisticas_completas.csv", index=False)

    if GERAR_TABELAS:
        salvar_tabelas(r)
    if GERAR_GRAFICOS:
        for cen in cenarios_com_dados:
            for m in METRICAS_TEMPO + METRICAS_MEDIA:
                grafico(r, cen, m)
    if GERAR_REGRESSAO_TEMPO:
        regressao_tempo(r)
    if GERAR_REGRESSAO_ALTURA:
        regressao_altura(r)
        limites_altura(r)

    print(f"Pronto. Arquivos gerados em: {SAIDA}")


if __name__ == "__main__":
    main()
