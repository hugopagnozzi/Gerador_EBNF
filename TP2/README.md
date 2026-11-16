aluno: <2022.1905.023-5>
aluno: <2022.1905.068-5>
uso_ia: <sim>

# Compiladores I — Trabalho Prático 2: Gerador de Analisadores Sintáticos Descendentes Recursivos

Implementação de um gerador de analisadores sintáticos descendentes recursivos. O programa lê uma gramática em EBNF e gera, em ANSI C, um analisador sintático descendente recursivo para a linguagem descrita. Veja o enunciado para os requisitos completos.

As duas primeiras linhas deste arquivo seguem o formato do TP1: uma linha `aluno:` por integrante e a linha `uso_ia:` logo abaixo. Registre o uso de IA em `USO_IA.md`.

## Compilar e executar

```
make                      # compila o gerador e, se microc/microc.flex existir, o analisador de Micro C
make test                 # executa os testes de testes/
make microc               # gera e compila build/microc/parser_microc
make clean
```

```
./gerador gramatica.ebnf -o saida/parser [--scanner microc] [--sets] [--tree] [--debug]
gcc -ansi -pedantic -Wall -Wextra -o parser saida/parser.c
./parser [--tree] programa.txt        # ou: ./parser --tokens tokens.txt
```

A opção `--tree` habilita a impressão da árvore de derivação no analisador gerado.

Todo o código deve compilar sem avisos com `gcc -ansi -pedantic -Wall -Wextra`. A única exceção é o `lex.yy.c` gerado pelo flex.

## Estado atual do projeto

O gerador está implementado e gera analisadores sintáticos descendentes recursivos em ANSI C a partir de gramáticas EBNF. As etapas incluem análise léxica e sintática da EBNF, verificações semânticas, cálculo de conjuntos anuláveis/FIRST/FOLLOW, verificação LL(1) e geração de código C.

O projeto também inclui a integração com o scanner de Micro C adaptado do TP1.

Os módulos principais são:

| Módulo | Responsabilidade |
| --- | --- |
| `ebnf_lexer.c` e `ebnf_parser.c` | Leitura e análise da gramática EBNF |
| `verificacao.c` | Verificações semânticas |
| `conjuntos.c` | Cálculo de anuláveis, FIRST e FOLLOW |
| `ll1.c` | Verificação das condições LL(1) |
| `codegen.c` | Geração do analisador sintático e do código C |
| `microc_mapa.c` | Mapeamento dos tokens de Micro C |
| `microc/scanner.c` | Adaptação do scanner do TP1 |

Os testes incluem cinco gramáticas:

- `atribuicoes`: sequências de atribuições simples;
- `expr`: expressões aritméticas;
- `microc`: linguagem Micro C;
- `conflito_ll1`: exemplo de conflito LL(1);
- `recursao_esquerda`: exemplo de recursão à esquerda.

As três primeiras são gramáticas válidas. As duas últimas são utilizadas para verificar a detecção de erros.

## Integração com o scanner do TP1

O projeto inclui o scanner de Micro C adaptado do TP1, localizado em `microc/`. A integração utiliza o Flex para gerar o código do scanner e conecta sua interface ao analisador sintático gerado.

O comando `make microc` realiza automaticamente as seguintes etapas:

1. Gera o analisador sintático a partir de `testes/gramaticas/microc.ebnf`.
2. Gera o código C do scanner a partir de `microc/microc.flex`.
3. Compila e vincula o analisador ao scanner e aos demais arquivos de suporte do TP1.

A gramática de Micro C é baseada na do Apêndice B do livro-texto e foi fatorada para LL(1). O `else` pendente gera um aviso de conflito em `[ "else" comando ]`. O conflito é resolvido de forma gulosa: o `else` fica com o `if` mais próximo.

## Testes

O script `testes/executar.sh` compara as saídas do gerador (`testes/esperado/*.gerador`) e dos analisadores gerados (`testes/programas/<gramatica>/*.esperado`) com os resultados esperados. Cada caso também verifica o código de saída do programa.

A suíte contém cinco gramáticas: três válidas (`atribuicoes`, `expr` e `microc`) e duas inválidas (`conflito_ll1` e `recursao_esquerda`). Para cada gramática válida, há pelo menos três programas aceitos e três rejeitados.

Os programas de Micro C só rodam depois de `make microc`. O caso `rejeitado_lexico.mc` espera a mensagem "String nao terminada" do scanner adaptado do TP1.
