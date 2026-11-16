/*
 * verificacao.c - Verificacoes semanticas da gramatica (etapa 2).
 *
 * A duplicidade de definicoes ja e detectada durante a leitura
 * (ebnf_parser.c).
 */

#include <stdlib.h>
#include <string.h>
#include "verificacao.h"
#include "microc_mapa.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* Fornecido                                                          */
/* ------------------------------------------------------------------ */

static int verif_indefinidos(Gramatica *g)
{
    int i, erros = 0;
    for (i = 0; i < g->nnts; i++) {
        if (g->nts[i].regra == NULL) {
            diag_erro(g->nts[i].linha, g->nts[i].coluna,
                      "nao terminal %s usado mas nao definido", g->nts[i].nome);
            erros++;
        }
    }
    return erros;
}

static int verif_terminais(Gramatica *g, int modo_microc)
{
    int i, erros = 0, carregado = 0;
    Terminal *t;

    if (modo_microc)
        carregado = microc_carregar_tokens_h(g->tokens_externos) == 0;

    for (i = 1; i < g->nterms; i++) {
        t = &g->terms[i];
        if (modo_microc) {
            if (t->literal && microc_nome_literal(t->nome) == NULL) {
                diag_erro(t->linha, t->coluna, "literal %s nao corresponde a nenhum "
                          "token de Micro C", t->exibicao);
                erros++;
            } else if (!t->literal && carregado && !microc_token_existe(t->nome)) {
                diag_erro(t->linha, t->coluna, "terminal %s nao existe na enumeracao "
                          "TokenType de %s", t->nome, g->tokens_externos);
                erros++;
            }
        } else if (!t->declarado) {
            diag_erro(t->linha, t->coluna, "terminal %s nao declarado em TOKENS", t->nome);
            erros++;
        }
    }
    return erros;
}

/* ------------------------------------------------------------------ */
/* A implementar                                                      */
/* ------------------------------------------------------------------ */

typedef struct 
{
    int *marcados;
    int novos;
} 
ContextoAlcancaveis;

static void marcar_naoterminal(Expr *e, void *ctx)
{
    ContextoAlcancaveis *c;

    c = (ContextoAlcancaveis *) ctx;

    if (e->tipo == E_NAOTERM && !c->marcados[e->simbolo]) 
    {
        c->marcados[e->simbolo] = 1;
        c->novos++;
    }
}

static int verif_inalcancaveis(Gramatica *g)
{
    int i, mudou;
    int *marcados;
    ContextoAlcancaveis ctx;

    marcados = (int *) calloc(g->nnts, sizeof(int));

    if (marcados == NULL)
    {
        return 0;
    }

    marcados[g->inicial] = 1;

    do 
    {
        mudou = 0;

        for (i = 0; i < g->nnts; i++) 
        {
            if (marcados[i] && g->nts[i].regra != NULL) 
            {
                ctx.marcados = marcados;
                ctx.novos = 0;

                expr_percorrer(g->nts[i].regra,
                               marcar_naoterminal, &ctx);

                if (ctx.novos > 0)
                {
                    mudou = 1;
                }
            }
        }
    } 
    while (mudou);

    for (i = 0; i < g->nnts; i++) 
    {
        if (!marcados[i] && g->nts[i].regra != NULL) 
        {
            diag_aviso(g->nts[i].linha_def, 1,
                       "nao terminal %s inalcancavel",
                       g->nts[i].nome);
        }
    }

    free(marcados);
    return 0;
}

static int expr_produtiva(Expr *e, int *produtivos)
{
    int i;

    if (e == NULL)
    {
        return 1;
    }

    switch (e->tipo) 
    {
        case E_TERM:
        case E_VAZIO:
        case E_OPC:
        case E_REP:
            return 1;

        case E_NAOTERM:
            return produtivos[e->simbolo];

        case E_SEQ:
            for (i = 0; i < e->nfilhos; i++) 
            {
                if (!expr_produtiva(e->filhos[i], produtivos))
                {
                    return 0;
                }
            }
            return 1;

        case E_ALT:
            for (i = 0; i < e->nfilhos; i++) 
            {
                if (expr_produtiva(e->filhos[i], produtivos))
                {
                    return 1;
                }
            }
            return 0;
    }

    return 0;
}

static int verif_improdutivos(Gramatica *g)
{
    int i, mudou, erros;
    int *produtivos;

    produtivos = (int *) calloc(g->nnts, sizeof(int));

    if (produtivos == NULL) 
    {
        diag_erro(0, 0, "memoria insuficiente ao verificar produtividade");
        return 1;
    }

    /*
     * Descobre quais nao terminais conseguem produzir
     * uma sequencia de terminais.
     */

    do 
    {
        mudou = 0;

        for (i = 0; i < g->nnts; i++) 
        {
            if (!produtivos[i] && g->nts[i].regra != NULL) 
            {
                if (expr_produtiva(g->nts[i].regra, produtivos)) 
                {
                    produtivos[i] = 1;
                    mudou = 1;
                }
            }
        }
    } 
    while (mudou);

    /*
     * Reporta os nao terminais definidos que nao
     * conseguiram ser considerados produtivos.
     */

    erros = 0;

    for (i = 0; i < g->nnts; i++) 
    {
        if (g->nts[i].regra != NULL && !produtivos[i]) 
        {
            diag_erro(g->nts[i].linha_def, 1,
                      "nao terminal %s improdutivo",
                      g->nts[i].nome);
            erros++;
        }
    }

    free(produtivos);
    return erros;
}

/*
 * Registra os nao terminais que podem aparecer no inicio
 * de uma expressao, antes que qualquer terminal seja consumido.
 */

static void coletar_prefixos(Expr *e, int origem,
                             int *arestas, int nnts)
{
    int i;

    if (e == NULL)
    {
        return;
    }

    switch (e->tipo) 
    {
        case E_NAOTERM:
            arestas[origem * nnts + e->simbolo] = 1;
            break;

        case E_SEQ:
            for (i = 0; i < e->nfilhos; i++) 
            {
                coletar_prefixos(e->filhos[i],
                                 origem, arestas, nnts);

                /*
                 * So podemos continuar para o proximo filho
                 * se o atual puder produzir vazio.
                 */

                if (!e->filhos[i]->anulavel)
                {
                    break;
                }
            }
            break;

        case E_ALT:
            for (i = 0; i < e->nfilhos; i++) 
            {
                coletar_prefixos(e->filhos[i],
                                 origem, arestas, nnts);
            }
            break;

        case E_OPC:
        case E_REP:
            if (e->nfilhos > 0) 
            {
                coletar_prefixos(e->filhos[0],
                                 origem, arestas, nnts);
            }
            break;

        case E_TERM:
        case E_VAZIO:
            break;
    }
}

int verif_recursao_esquerda(Gramatica *g)
{
    int i, j, k;
    int n;
    int erros;
    int *arestas;
    int *diretas;

    n = g->nnts;

    arestas = (int *) calloc(n * n, sizeof(int));
    diretas = (int *) calloc(n * n, sizeof(int));

    if (arestas == NULL || diretas == NULL) 
    {
        free(arestas);
        free(diretas);

        diag_erro(0, 0,
                  "memoria insuficiente ao verificar recursao a esquerda");
        return 1;
    }

    for (i = 0; i < n; i++) 
    {
        if (g->nts[i].regra != NULL) 
        {
            coletar_prefixos(g->nts[i].regra,
                             i, arestas, n);
        }
    }

    /*
     * Preserva as arestas originais para distinguir
     * recursao direta de indireta.
     */

    for (i = 0; i < n * n; i++)
    {
        diretas[i] = arestas[i];
    }

    /*
     * Fecho transitivo: identifica todos os caminhos
     * entre os nao terminais.
     */

    for (k = 0; k < n; k++) 
    {
        for (i = 0; i < n; i++) 
        {
            for (j = 0; j < n; j++) 
            {
                if (arestas[i * n + k] &&
                    arestas[k * n + j]) 
                {
                    arestas[i * n + j] = 1;
                }
            }
        }
    }

    erros = 0;

    for (i = 0; i < n; i++) 
    {
        if (arestas[i * n + i]) 
        {
            if (diretas[i * n + i]) 
            {
                diag_erro(g->nts[i].linha_def, 0,
                          "nao terminal %s tem recursao a esquerda direta",
                          g->nts[i].nome);
            }
            else 
            {
                diag_erro(g->nts[i].linha_def, 0,
                          "nao terminal %s tem recursao a esquerda indireta",
                          g->nts[i].nome);
            }
            erros++;
        }
    }
    free(arestas);
    free(diretas);

    return erros;
}

int verif_basica(Gramatica *g, int modo_microc)
{
    int erros = 0;
    if (g->inicial < 0) {
        diag_erro(0, 0, "a gramatica nao tem nenhuma regra");
        return 1;
    }
    erros += verif_indefinidos(g);
    erros += verif_terminais(g, modo_microc);
    erros += verif_inalcancaveis(g);
    if (erros == 0)
        erros += verif_improdutivos(g);
    return erros;
}
