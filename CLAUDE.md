# Tree-benchmark

Comparação experimental das árvores AVL e Rubro-Negra em C, com análise em Python e relatório em LaTeX (abnTeX2) em `docs/`, no mesmo padrão do projeto Sorting-benchmark.

## Registro obrigatório de uso de IA

**Toda prompt direcionada ao desenvolvimento do projeto precisa ser registrada em `docs/registro_ia.toml`.** Isso vale para código, análise, relatório, revisão de texto ou qualquer outra parte. O relatório gera a declaração de uso de IA a partir desse arquivo.

A cada prompt recebida nesta pasta:

1. Se for a primeira prompt da sessão, crie um novo `[[sessao]]` com ferramenta, modelo, `modelo_id` exato, empresa, interface (e versão), configurações relevantes (busca na web, memória, modo de permissões, plugins, parâmetros) e instruções de sistema ou de projeto ativas (este CLAUDE.md, memórias). O prompt de sistema padrão da ferramenta não é reproduzido.
2. Acrescente um `[[sessao.interacao]]` com `data`, `prompt` (texto **literal e completo**, incluindo trechos colados, entre `'''`), `saida` (resposta completa, ou a parte usada, ou um resumo fiel do que foi produzido, com arquivos e commits), `finalidade` e `verificacao`.
3. Não invente a verificação humana: deixe `verificacao = '''PENDENTE: ...'''` para o autor preencher.
4. Depois rode `python3 docs/gerar_declaracao_ia.py` para atualizar `docs/recursos/declaracao_ia.tex` e `docs/recursos/registro_ia.tex`.

## Relatório

- Compilar: `cd docs && latexmk -pdf relatorio.tex`
- `\pendente{...}` marca em vermelho o que falta completar antes da entrega.
- Seguir as normas ABNT já configuradas no preâmbulo (NBR 14724, 10520:2023, 6023:2018).
