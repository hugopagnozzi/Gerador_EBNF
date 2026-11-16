/*
 * ll1.c - Verificacao da condicao LL(1) (etapa 4).
 */

#include <stdarg.h>
#include <stdio.h>
#include "ll1.h"
#include "util.h"

/* Reporta um conflito na regra do nao terminal nt, na posicao de e.
 * comum (pode ser NULL) e o conjunto de tokens em conflito. */

static void reportar(const Gramatica *g, int nt, const Expr *e, int eh_aviso,
                     const Bitset *comum, const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s:%d:%d: %s: conflito LL(1) em %s: ",
            g->arquivo, e->linha, e->coluna,
            eh_aviso ? "aviso" : "erro", g->nts[nt].nome);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (comum != NULL) 
    {
        fprintf(stderr, " ");
        gram_imprimir_conjunto(stderr, g, comum);
    }

    fputc('\n', stderr);
}

/*
 * Verifica as condicoes LL(1) de um no da expressao e percorre
 * recursivamente seus filhos.
 *
 * Retorna a quantidade de erros encontrados. Avisos nao contam.
 */

static int verificar_expr(const Gramatica *g, int nt, const Expr *e)
{
    int erros;
    int i;
    int j;
    int anulaveis;
    Bitset comum;

    erros = 0;

    switch (e->tipo) 
    {
    case E_ALT:
        /*
         * Condicao 1: os FIRST das alternativas devem ser
         * disjuntos dois a dois.
         */

        anulaveis = 0;

        for (i = 0; i < e->nfilhos; i++) 
        {
            if (e->filhos[i]->anulavel) 
            {
                anulaveis++;
            }

            for (j = i + 1; j < e->nfilhos; j++) 
            {
                bitset_iniciar(&comum, g->nterms);
                bitset_intersecao(&comum,
                                  &e->filhos[i]->first,
                                  &e->filhos[j]->first);

                if (!bitset_vazio(&comum)) 
                {
                    reportar(g, nt, e, 0, &comum,
                             "alternativas %d e %d comecam com",
                             i + 1, j + 1);
                    erros++;
                }
                bitset_liberar(&comum);
            }
        }

        /*
         * No maximo uma alternativa pode ser anulavel.
         */

        if (anulaveis > 1) 
        {
            reportar(g, nt, e, 0, NULL,
                     "mais de uma alternativa e anulavel");
            erros++;
        }

        /*
         * Condicao 2: se uma alternativa e anulavel,
         * seu FOLLOW deve ser disjunto do FIRST das demais.
         */

        for (i = 0; i < e->nfilhos; i++) 
        {
            if (e->filhos[i]->anulavel) 
            {
                for (j = 0; j < e->nfilhos; j++) 
                {
                    if (i != j) 
                    {
                        bitset_iniciar(&comum, g->nterms);
                        bitset_intersecao(
                            &comum,
                            &e->filhos[i]->follow,
                            &e->filhos[j]->first
                        );

                        if (!bitset_vazio(&comum)) 
                        {
                            reportar(g, nt, e, 0, &comum,
                                     "FOLLOW da alternativa %d conflita "
                                     "com FIRST da alternativa %d",
                                     i + 1, j + 1);
                            erros++;
                        }
                        bitset_liberar(&comum);
                    }
                }
            }
        }
        break;

    case E_OPC:
    case E_REP:
        if (e->nfilhos > 0) 
        {
            /*
             * O conteudo de um opcional ou repeticao
             * nao pode ser anulavel.
             */

            if (e->filhos[0]->anulavel) 
            {
                if (e->tipo == E_OPC) 
                {
                    reportar(g, nt, e, 0, NULL,
                             "conteudo de opcional e anulavel");
                } 
                else 
                {
                    reportar(g, nt, e, 0, NULL,
                             "conteudo de repeticao e anulavel");
                }
                erros++;
            }

            /*
             * A sobreposicao entre FIRST do conteudo e
             * FOLLOW da construcao gera apenas um aviso.
             */

            bitset_iniciar(&comum, g->nterms);
            bitset_intersecao(&comum,
                              &e->filhos[0]->first,
                              &e->follow);

            if (!bitset_vazio(&comum)) 
            {
                if (e->tipo == E_OPC) 
                {
                    reportar(g, nt, e, 1, &comum,
                             "FIRST e FOLLOW de [ ] se intersectam "
                             "(resolvido de forma gulosa) em");
                } 
                else 
                {
                    reportar(g, nt, e, 1, &comum,
                             "FIRST e FOLLOW de { } se intersectam "
                             "(resolvido de forma gulosa) em");
                }
            }
            bitset_liberar(&comum);
        }
        break;

    case E_SEQ:
        break;

    case E_TERM:
    case E_NAOTERM:
    case E_VAZIO:
        break;
    }

    /*
     * Percorre os filhos para verificar construcoes LL(1)
     * que estejam aninhadas dentro desta expressao.
     */

    for (i = 0; i < e->nfilhos; i++) 
    {
        erros += verificar_expr(g, nt, e->filhos[i]);
    }

    return erros;
}

/* Verifica a condicao LL(1) de toda a gramatica. */

int ll1_verificar(const Gramatica *g)
{
    int erros;
    int i;

    erros = 0;

    for (i = 0; i < g->nnts; i++) 
    {
        erros += verificar_expr(g, i, g->nts[i].regra);
    }

    return erros;
}
