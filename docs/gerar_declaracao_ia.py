"""Gera a declaração de uso de IA do relatório a partir de docs/registro_ia.toml.

Saídas (incluídas no relatorio.tex por \\input):
  docs/recursos/declaracao_ia.tex  texto da declaração e quadro-resumo das interações
  docs/recursos/registro_ia.tex    apêndice com prompt literal e saída de cada interação

Uso (de qualquer pasta):  python3 docs/gerar_declaracao_ia.py
Só usa a biblioteca padrão (tomllib, Python 3.11+).
"""

import sys
import tomllib
from datetime import date
from pathlib import Path

DOCS = Path(__file__).resolve().parent
REGISTRO = DOCS / "registro_ia.toml"
SAIDA = DOCS / "recursos"

# Caracteres fora do Latin-1 que aparecem com frequência em prompts e têm equivalente em LaTeX
EQUIVALENTES = {
    "→": r"$\to$", "←": r"$\leftarrow$", "×": r"$\times$", "≤": r"$\leq$", "≥": r"$\geq$",
    "≠": r"$\neq$", "≈": r"$\approx$", "−": "-", "–": "--", "—": "---", "…": r"\ldots{}",
    "“": "``", "”": "''", "‘": "`", "’": "'", "•": r"\textbullet{}", "─": "-", "═": "=",
    "²": r"\textsuperscript{2}", "³": r"\textsuperscript{3}", "·": r"$\cdot$",
}
ESPECIAIS = {
    "\\": r"\textbackslash{}", "{": r"\{", "}": r"\}", "$": r"\$", "&": r"\&",
    "#": r"\#", "%": r"\%", "_": r"\_", "^": r"\textasciicircum{}", "~": r"\textasciitilde{}",
    "<": r"\textless{}", ">": r"\textgreater{}", "|": r"\textbar{}", "\t": r"\quad{}",
}

avisos = []


def escapar(texto):
    """Escapa texto livre para LaTeX, sem perder nenhum caractere em silêncio."""
    partes = []
    for c in texto:
        if c in ESPECIAIS:
            partes.append(ESPECIAIS[c])
        elif c in EQUIVALENTES:
            partes.append(EQUIVALENTES[c])
        elif ord(c) < 256:
            partes.append(c)
        else:
            avisos.append(f"caractere sem equivalente em LaTeX: {c!r} (U+{ord(c):04X})")
            partes.append(rf"\textbf{{[U+{ord(c):04X}]}}")
    return "".join(partes)


def campo(texto):
    """Texto de um campo: os marcados como PENDENTE saem com \\pendente{...}."""
    texto = (texto or "").strip()
    if not texto:
        return r"\pendente{não preenchido}"
    if texto.upper().startswith("PENDENTE"):
        resto = texto[len("PENDENTE"):].lstrip(" :")
        return rf"\pendente{{{escapar(resto)}}}"
    return escapar(texto)


def bloco(texto):
    """Texto de várias linhas, preservando parágrafos e quebras de linha."""
    texto = (texto or "").strip()
    if not texto or texto.upper().startswith("PENDENTE"):
        return campo(texto)
    paragrafos = []
    for p in texto.split("\n\n"):
        linhas = [escapar(l) for l in p.split("\n")]
        paragrafos.append(" \\newline\n".join(linhas))
    return "\n\n".join(paragrafos)


def data_br(d):
    d = date.fromisoformat(str(d))
    return d.strftime("%d/%m/%Y")


def main():
    with open(REGISTRO, "rb") as f:
        registro = tomllib.load(f)
    sessoes = registro.get("sessao", [])

    # Numeração global das interações (I1, I2, ...) para o quadro e o apêndice
    numero = 0
    for s in sessoes:
        for i in s.get("interacao", []):
            numero += 1
            i["_num"] = numero

    ferramentas = sorted({f"{s['ferramenta']} ({s['modelo']}, {s['modelo_id']}), da {s['empresa']}" for s in sessoes})
    datas = sorted({str(i["data"]) for s in sessoes for i in s.get("interacao", [])})

    # ─── declaracao_ia.tex ────────────────────────────────
    d = []
    d.append("% Gerado por docs/gerar_declaracao_ia.py a partir de docs/registro_ia.toml. Não edite à mão.")
    d.append(
        "Neste trabalho foi utilizada a ferramenta de inteligência artificial generativa "
        + escapar("; ".join(ferramentas))
        + f", em {len(sessoes)} {'sessão' if len(sessoes) == 1 else 'sessões'} e {numero} "
        + ("interação" if numero == 1 else "interações")
        + ", nas datas "
        + escapar(", ".join(data_br(x) for x in datas))
        + ". O \\autoref{quadro:uso-ia} resume a finalidade de cada interação e a verificação feita pelo autor, "
        "e o \\autoref{apendice:registro-ia} reproduz a prompt literal e a saída de cada uma. "
        "Todo o conteúdo produzido com auxílio da ferramenta foi revisado pelo autor, que se responsabiliza integralmente pelo trabalho."
    )
    d.append("")
    for s in sessoes:
        d.append(
            rf"\textbf{{Sessão {escapar(s['id'])}}} -- interface: {campo(s.get('interface'))}. "
            rf"Configurações: {campo(s.get('configuracoes'))}. "
            rf"Instruções de sistema ou de projeto: {campo(s.get('instrucoes_sistema')).rstrip('.')}."
        )
        d.append("")

    d.append(r"\begin{quadro}[htbp]")
    d.append(r"    \caption{Interações com a ferramenta de inteligência artificial}")
    d.append(r"    \label{quadro:uso-ia}")
    d.append(r"    \centering")
    d.append(r"    \footnotesize")
    d.append(r"    \begin{tabular}{|c|c|c|p{4.3cm}|p{5.8cm}|}")
    d.append(r"        \hline")
    d.append(r"        \textbf{Nº} & \textbf{Sessão} & \textbf{Data} & \textbf{Finalidade} & \textbf{Verificação humana} \\ \hline")
    for s in sessoes:
        for i in s.get("interacao", []):
            d.append(
                f"        I{i['_num']} & {escapar(s['id'])} & {data_br(i['data'])} & "
                f"{campo(i.get('finalidade'))} & {campo(i.get('verificacao'))} \\\\ \\hline"
            )
    d.append(r"    \end{tabular}")
    d.append(r"    \fonte{Elaborado pelo autor (2026).}")
    d.append(r"\end{quadro}")

    # ─── registro_ia.tex ──────────────────────────────────
    r = ["% Gerado por docs/gerar_declaracao_ia.py a partir de docs/registro_ia.toml. Não edite à mão."]
    r.append(
        "Este apêndice reproduz, na ordem em que foram enviadas, todas as prompts direcionadas ao "
        "desenvolvimento do projeto, com a saída da ferramenta (completa ou a parte usada), a finalidade "
        "e a verificação feita pelo autor. O registro original, em formato legível por máquina, está no "
        "arquivo \\texttt{docs/registro\\_ia.toml} do repositório."
    )
    for s in sessoes:
        for i in s.get("interacao", []):
            r.append("")
            r.append(rf"\section*{{Interação I{i['_num']} (sessão {escapar(s['id'])}, {data_br(i['data'])})}}")
            r.append(r"\addcontentsline{toc}{section}{Interação I" + str(i["_num"]) + "}")
            r.append(r"{\raggedright")
            r.append(r"\begin{description}")
            r.append(rf"    \item[Ferramenta:] {escapar(s['ferramenta'])}, {escapar(s['modelo'])} (\texttt{{{escapar(s['modelo_id'])}}}), {escapar(s['empresa'])}")
            r.append(rf"    \item[Interface:] {campo(s.get('interface'))}")
            r.append(rf"    \item[Finalidade:] {campo(i.get('finalidade'))}")
            if i.get("arquivos"):
                r.append(r"    \item[Arquivos:] " + ", ".join(rf"\texttt{{{escapar(a)}}}" for a in i["arquivos"]))
            if i.get("commits"):
                r.append(r"    \item[Commits:] " + ", ".join(rf"\texttt{{{escapar(c)}}}" for c in i["commits"]))
            r.append(r"\end{description}")
            r.append("}")
            r.append(r"\textbf{Prompt literal:}")
            r.append(r"\begin{registroia}")
            r.append(bloco(i.get("prompt")))
            r.append(r"\end{registroia}")
            r.append(r"\textbf{Saída:}")
            r.append(r"\begin{registroia}")
            r.append(bloco(i.get("saida")))
            r.append(r"\end{registroia}")
            r.append(r"\textbf{Verificação humana:} " + bloco(i.get("verificacao")))

    SAIDA.mkdir(exist_ok=True)
    (SAIDA / "declaracao_ia.tex").write_text("\n".join(d) + "\n", encoding="utf-8")
    (SAIDA / "registro_ia.tex").write_text("\n".join(r) + "\n", encoding="utf-8")

    pendentes = sum(t.count(r"\pendente{") for t in ("\n".join(d), "\n".join(r)))
    print(f"{len(sessoes)} sessões, {numero} interações -> {SAIDA.relative_to(DOCS.parent)}/declaracao_ia.tex, registro_ia.tex")
    if pendentes:
        print(f"atenção: {pendentes} campo(s) PENDENTE no registro")
    for a in dict.fromkeys(avisos):
        print("aviso:", a, file=sys.stderr)


if __name__ == "__main__":
    main()
