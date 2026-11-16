/*
 * conjuntos.c - Calculo de anulavel, FIRST e FOLLOW (etapa 3).
 */

#include <stdio.h>
#include <string.h>
#include "conjuntos.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* Fornecido                                                          */
/* ------------------------------------------------------------------ */

static void iniciar_expr(Expr *e, void *ctx)
{
    int n = *(int *) ctx;
    bitset_liberar(&e->first);
    bitset_liberar(&e->follow);
    bitset_iniciar(&e->first, n);
    bitset_iniciar(&e->follow, n);
    e->anulavel = 0;
}

void conj_iniciar(Gramatica *g)
{
    int i, n = g->nterms;
    for (i = 0; i < g->nnts; i++) {
        bitset_liberar(&g->nts[i].first);
        bitset_liberar(&g->nts[i].follow);
        bitset_iniciar(&g->nts[i].first, n);
        bitset_iniciar(&g->nts[i].follow, n);
        g->nts[i].anulavel = 0;
        expr_percorrer(g->nts[i].regra, iniciar_expr, &n);
    }
}

void conj_imprimir(FILE *f, const Gramatica *g)
{
    int i;
    size_t largura = strlen("NAO TERMINAL");
    for (i = 0; i < g->nnts; i++)
        if (strlen(g->nts[i].nome) > largura)
            largura = strlen(g->nts[i].nome);
    fprintf(f, "%-*s  ANULAVEL  FIRST / FOLLOW\n", (int) largura, "NAO TERMINAL");
    for (i = 0; i < g->nnts; i++) {
        fprintf(f, "%-*s  %-8s  FIRST  = ", (int) largura, g->nts[i].nome,
                g->nts[i].anulavel ? "sim" : "nao");
        gram_imprimir_conjunto(f, g, &g->nts[i].first);
        fprintf(f, "\n%-*s            FOLLOW = ", (int) largura, "");
        gram_imprimir_conjunto(f, g, &g->nts[i].follow);
        fputc('\n', f);
    }
}

/* ------------------------------------------------------------------ */
/* A implementar                                                      */
/* ------------------------------------------------------------------ */

static int atualizar_anulavel(Expr *e, Gramatica *g, int *mudou)
{
    int i;
    int resultado;
    int filho_anulavel;

    if (e == NULL)
    {
        return 0;
    }

    resultado = 0;

    switch (e->tipo) 
    {
        case E_TERM:
            resultado = 0;
            break;

        case E_VAZIO:
            resultado = 1;
            break;

        case E_NAOTERM:
            resultado = g->nts[e->simbolo].anulavel;
            break;

        case E_SEQ:
            resultado = 1;

            for (i = 0; i < e->nfilhos; i++) 
            {
                filho_anulavel =
                    atualizar_anulavel(e->filhos[i], g, mudou);

                if (!filho_anulavel)
                {
                    resultado = 0;
                }
            }
            break;

        case E_ALT:
            resultado = 0;

            for (i = 0; i < e->nfilhos; i++) 
            {
                filho_anulavel =
                    atualizar_anulavel(e->filhos[i], g, mudou);

                if (filho_anulavel)
                {
                    resultado = 1;
                }
            }
            break;

        case E_OPC:
        case E_REP:
            if (e->nfilhos > 0)
            {
                atualizar_anulavel(e->filhos[0], g, mudou);
            }
            resultado = 1;
            break;
    }

    if (e->anulavel != resultado) 
    {
        e->anulavel = resultado;
        *mudou = 1;
    }

    return resultado;
}

void conj_anulavel(Gramatica *g)
{
    int i;
    int mudou;
    int resultado;

    do 
    {
        mudou = 0;

        for (i = 0; i < g->nnts; i++) 
        {
            if (g->nts[i].regra != NULL) 
            {
                resultado = atualizar_anulavel(
                    g->nts[i].regra, g, &mudou);

                if (g->nts[i].anulavel != resultado) 
                {
                    g->nts[i].anulavel = resultado;
                    mudou = 1;
                }
            }
        }
    } 
    while (mudou);
}

static void atualizar_first(Expr *e, Gramatica *g, int *mudou)
{
    int i;
    int continuar;

    if (e == NULL)
    {
        return;
    }

    switch (e->tipo) 
    {
        case E_TERM:
            if (!bitset_tem(&e->first, e->simbolo)) 
            {
                bitset_add(&e->first, e->simbolo);
                *mudou = 1;
            }
            break;

        case E_NAOTERM:
            if (bitset_unir(&e->first,
                            &g->nts[e->simbolo].first)) 
            {
                *mudou = 1;
            }
            break;

        case E_SEQ:
            continuar = 1;

            for (i = 0; i < e->nfilhos; i++) 
            {
                /*
                 * Calcula o FIRST do filho mesmo quando
                 * ele nao contribui para o FIRST da sequencia.
                 */

                atualizar_first(e->filhos[i], g, mudou);

                if (continuar) 
                {
                    if (bitset_unir(&e->first,
                                    &e->filhos[i]->first)) 
                    {
                        *mudou = 1;
                    }

                    if (!e->filhos[i]->anulavel)
                    {
                        continuar = 0;
                    }
                }
            }
            break;

        case E_ALT:
            for (i = 0; i < e->nfilhos; i++) 
            {
                atualizar_first(e->filhos[i], g, mudou);

                if (bitset_unir(&e->first,
                                &e->filhos[i]->first)) 
                {
                    *mudou = 1;
                }
            }
            break;

        case E_OPC:
        case E_REP:
            if (e->nfilhos > 0) 
            {
                atualizar_first(e->filhos[0], g, mudou);

                if (bitset_unir(&e->first,
                                &e->filhos[0]->first)) 
                {
                    *mudou = 1;
                }
            }
            break;

        case E_VAZIO:
            break;
    }
}

void conj_first(Gramatica *g)
{
    int i;
    int mudou;

    do 
    {
        mudou = 0;

        for (i = 0; i < g->nnts; i++) 
        {
            if (g->nts[i].regra != NULL) 
            {
                atualizar_first(g->nts[i].regra,
                                g, &mudou);

                if (bitset_unir(&g->nts[i].first,
                                &g->nts[i].regra->first)) 
                {
                    mudou = 1;
                }
            }
        }
    } 
    while (mudou);
}

static void propagar_follow(Expr *e, Gramatica *g, int *mudou)
{
    int i, j;
    int anulavel_sufixo;

    if (e == NULL)
    {
        return;
    }

    switch (e->tipo) 
    {
        case E_NAOTERM:
            if (bitset_unir(&g->nts[e->simbolo].follow,
                            &e->follow)) 
            {
                *mudou = 1;
            }
            break;

        case E_SEQ:
            for (i = 0; i < e->nfilhos; i++) 
            {
                /*
                 * Calcula o FIRST dos elementos seguintes.
                 */

                anulavel_sufixo = 1;

                for (j = i + 1; j < e->nfilhos; j++) 
                {
                    if (bitset_unir(&e->filhos[i]->follow,
                                    &e->filhos[j]->first)) 
                    {
                        *mudou = 1;
                    }

                    if (!e->filhos[j]->anulavel) 
                    {
                        anulavel_sufixo = 0;
                        break;
                    }
                }

                /*
                 * Se todo o sufixo for anulavel, o FOLLOW
                 * da sequencia tambem pertence ao filho.
                 */

                if (anulavel_sufixo) 
                {
                    if (bitset_unir(&e->filhos[i]->follow,
                                    &e->follow)) 
                    {
                        *mudou = 1;
                    }
                }
                propagar_follow(e->filhos[i], g, mudou);
            }
            break;

        case E_ALT:
        case E_OPC:
            for (i = 0; i < e->nfilhos; i++) 
            {
                if (bitset_unir(&e->filhos[i]->follow,
                                &e->follow)) 
                {
                    *mudou = 1;
                }
                propagar_follow(e->filhos[i], g, mudou);
            }
            break;

        case E_REP:
            if (e->nfilhos > 0) 
            {
                /*
                 * Depois de uma ocorrencia, pode vir outra.
                 * Por isso, o FOLLOW do filho recebe tambem
                 * o FIRST do proprio filho.
                 */

                if (bitset_unir(&e->filhos[0]->follow,
                                &e->follow)) 
                {
                    *mudou = 1;
                }

                if (bitset_unir(&e->filhos[0]->follow,
                                &e->filhos[0]->first)) 
                {
                    *mudou = 1;
                }
                propagar_follow(e->filhos[0], g, mudou);
            }
            break;

        case E_TERM:
        case E_VAZIO:
            break;
    }
}

void conj_follow(Gramatica *g)
{
    int i;
    int mudou;

    /*
     * O simbolo inicial pode ser seguido pelo fim da entrada.
     */

    if (g->inicial >= 0 && g->inicial < g->nnts) 
    {
        bitset_add(&g->nts[g->inicial].follow, TERM_FIM);
    }

    do 
    {
        mudou = 0;

        for (i = 0; i < g->nnts; i++) 
        {
            if (g->nts[i].regra != NULL) 
            {
                /*
                 * O FOLLOW do nao terminal tambem pertence
                 * a expressao que define sua regra.
                 */

                if (bitset_unir(&g->nts[i].regra->follow,
                                &g->nts[i].follow)) 
                {
                    mudou = 1;
                }
                propagar_follow(g->nts[i].regra, g, &mudou);
            }
        }
    } 
    while (mudou);
}
