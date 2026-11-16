/*
 * microc_mapa.c - Correspondencia com os tokens de Micro C (ver microc_mapa.h).
 *
 * A tabela de literais segue o tokens.h do livro-texto (secao 2.3.1):
 *
 *   UNDEF, ID, END_OF_FILE, INTEGERCONST, CHARCONST, STRINGCONST,
 *   PLUS, MINUS, MUL, DIV, MOD, EQ, NEQ, LT, GT, LEQ, GEQ, AND, OR, NOT,
 *   ASSIGN, SEMICOLON, COMMA, LPAREN, RPAREN, LBRACE, RBRACE,
 *   LBRACKET, RBRACKET, MAIN, IF, ELSE, FOR, RETURN, INT, CHAR, PRINT
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "microc_mapa.h"
#include "util.h"

static const char *const literais[][2] = {
    { "+",  "PLUS" },      { "-",  "MINUS" },     { "*",  "MUL" },
    { "/",  "DIV" },       { "%",  "MOD" },
    { "==", "EQ" },        { "!=", "NEQ" },       { "<",  "LT" },
    { ">",  "GT" },        { "<=", "LEQ" },       { ">=", "GEQ" },
    { "&&", "AND" },       { "||", "OR" },        { "!",  "NOT" },
    { "=",  "ASSIGN" },    { ";",  "SEMICOLON" }, { ",",  "COMMA" },
    { "(",  "LPAREN" },    { ")",  "RPAREN" },    { "{",  "LBRACE" },
    { "}",  "RBRACE" },    { "[",  "LBRACKET" },  { "]",  "RBRACKET" },
    { "main", "MAIN" },    { "if", "IF" },        { "else", "ELSE" },
    { "for", "FOR" },      { "return", "RETURN" },{ "int", "INT" },
    { "char", "CHAR" },    { "print", "PRINT" }
};

static char **nomes = NULL;
static int nnomes = 0, capnomes = 0;

const char *microc_nome_literal(const char *literal)
{
    size_t i;
    for (i = 0; i < sizeof(literais) / sizeof(literais[0]); i++)
        if (strcmp(literais[i][0], literal) == 0)
            return literais[i][1];
    return NULL;
}

static void adicionar_nome(const char *s, size_t n)
{
    if (nnomes == capnomes) {
        capnomes = capnomes ? 2 * capnomes : 64;
        nomes = (char **) xrealloc(nomes, (size_t) capnomes * sizeof(char *));
    }
    nomes[nnomes++] = xstrndup(s, n);
}

static void remover_comentarios(char *s)
{
    char *p;

    p = s;
    while (*p != '\0') 
    {
        if (p[0] == '/' && p[1] == '/') 
        {
            while (*p != '\0' && *p != '\n')
            {
                *p++ = ' ';
            }
        }
        else if (p[0] == '/' && p[1] == '*') 
        {
            *p++ = ' ';
            *p++ = ' ';
            while (*p != '\0' && !(p[0] == '*' && p[1] == '/')) 
            {
                if (*p != '\n')
                {
                    *p = ' ';
                }
                p++;
            }
            if (*p != '\0') 
            {
                *p++ = ' ';
                *p++ = ' ';
            }
        }
        else 
        {
            p++;
        }
    }
}

static char *pular_espacos(char *p)
{
    while (*p != '\0' && isspace((unsigned char) *p))
    {
        p++;
    }

    return p;
}

static char *buscar_palavra(char *s, const char *palavra)
{
    char *p;
    size_t n;
    size_t restante;

    n = strlen(palavra);

    for (p = s; *p != '\0'; p++) 
    {
        restante = strlen(p);

        if (restante >= n &&
            (p == s || (!isalnum((unsigned char) p[-1]) && p[-1] != '_')) &&
            strncmp(p, palavra, n) == 0 &&
            (!isalnum((unsigned char) p[n]) && p[n] != '_'))
        {
            return p;
        }
    }

    return NULL;
}

int microc_carregar_tokens_h(const char *caminho)
{
    static int avisado = 0;
    char *fonte;
    char *p;
    char *inicio;
    char *fim;
    int quantidade;

    microc_liberar();

    fonte = ler_arquivo(caminho);
    if (fonte == NULL)
    {
        goto erro;
    }

    remover_comentarios(fonte);

    p = buscar_palavra(fonte, "typedef");
    if (p == NULL)
    {
        goto erro_fonte;
    }

    p = pular_espacos(p + strlen("typedef"));

    if (strlen(p) < 4 ||
        strncmp(p, "enum", 4) != 0 ||
        isalnum((unsigned char) p[4]) || p[4] == '_')
    {
        goto erro_fonte;
    }

    p = pular_espacos(p + 4);

    /* Aceita tanto "enum {" quanto "enum Nome {". */

    if (*p != '{') 
    {
        if (!isalpha((unsigned char) *p) && *p != '_')
        {
            goto erro_fonte;
        }

        while (isalnum((unsigned char) *p) || *p == '_')
        {
            p++;
        }

        p = pular_espacos(p);
    }

    if (*p != '{')
    {
        goto erro_fonte;
    }

    p++;
    quantidade = 0;

    for (;;) 
    {
        p = pular_espacos(p);

        if (*p == '}')
        {
            break;
        }

        if (!isalpha((unsigned char) *p) && *p != '_')
        {
            goto erro_fonte;
        }

        inicio = p;

        while (isalnum((unsigned char) *p) || *p == '_')
        {
            p++;
        }

        fim = p;
        adicionar_nome(inicio, (size_t) (fim - inicio));
        quantidade++;

        p = pular_espacos(p);

        /* Ignora valores explicitos, como TOKEN = 3. */

        if (*p == '=') 
        {
            p++;

            while (*p != '\0' && *p != ',' && *p != '}')
            {
                p++;
            }
        }

        p = pular_espacos(p);

        if (*p == ',')
        {
            p++;
        }
        else if (*p != '}')
        {
            goto erro_fonte;
        }
    }

    if (quantidade == 0)
    {
        goto erro_fonte;
    }

    p = pular_espacos(p + 1);

    if (strlen(p) < 9 ||
        strncmp(p, "TokenType", 9) != 0 ||
        isalnum((unsigned char) p[9]) || p[9] == '_')
    {
        goto erro_fonte;
    }

    p = pular_espacos(p + 9);

    if (*p != ';')
    {
        goto erro_fonte;
    }

    free(fonte);
    return 0;

    erro_fonte:
        free(fonte);

    erro:
        microc_liberar();

    if (!avisado) 
    {
        diag_aviso(0, 0,
                   "nao foi possivel verificar os nomes de tokens em %s",
                   caminho ? caminho : "tokens.h");
        avisado = 1;
    }

    return -1;
}

int microc_token_existe(const char *nome)
{
    int i;
    for (i = 0; i < nnomes; i++)
        if (strcmp(nomes[i], nome) == 0)
            return 1;
    return 0;
}

void microc_liberar(void)
{
    int i;
    for (i = 0; i < nnomes; i++)
        free(nomes[i]);
    free(nomes);
    nomes = NULL;
    nnomes = capnomes = 0;
}
