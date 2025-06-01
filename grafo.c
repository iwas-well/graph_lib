#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grafo.h"

typedef char* vertice;
typedef struct aresta{
    vertice x;
    vertice y;
    unsigned int peso;
} aresta;

struct grafo{
    char *nome;
};

//------------------------------------------------------------------------------
// lê um grafo de f e o devolve
grafo *le_grafo(FILE *f) {
    char x[10],y[10], line[100], name[100];
    int p;

    if (!fgets(name, sizeof(name), stdin))
        return NULL;

    name[strlen(name)-1] = '\0';
    printf("graph name: %s\n", name);

    while (1) {
        // Read an entire line from input
        if (!fgets(line, sizeof(line), stdin))
            break; // EOF or error

        // If the line stars with / or is empty, skip it
        if ((line[0] == '/') || (line[0] == '\n')) continue;

        // Initialize variables
        x[0] = y[0] = 0;
        p = -1;

        // Parse the line
        int res = sscanf(line, "%9s %*s %9s %d", x, y, &p);
        if (res == 3)
            printf("x:%s y:%s p:%d\n", x, y, p); //read edge with weight
        else if (res == 2)
            printf("x:%s y:%s \n", x, y); //read edge
        else
            printf("x:%s\n", x); //read vertice
    }

    return grafo;
}

//------------------------------------------------------------------------------
// desaloca toda a estrutura de dados alocada em g
// devolve 1 em caso de sucesso e 0 em caso de erro
unsigned int destroi_grafo(grafo *g) {
}

//------------------------------------------------------------------------------
// devolve o nome de g
char *nome(grafo *g);

//------------------------------------------------------------------------------
// devolve 1 se g é bipartido e 0 caso contrário
unsigned int bipartido(grafo *g);

//------------------------------------------------------------------------------
// devolve o número de vértices em g
unsigned int n_vertices(grafo *g);

//------------------------------------------------------------------------------
// devolve o número de arestas em g
unsigned int n_arestas(grafo *g);

//------------------------------------------------------------------------------
// devolve o número de componentes em g
unsigned int n_componentes(grafo *g);

//------------------------------------------------------------------------------
// devolve uma "string" com os diâmetros dos componentes de g separados por brancos
// em ordem não decrescente
char *diametros(grafo *g);

//------------------------------------------------------------------------------
// devolve uma "string" com os nomes dos vértices de corte de g em
// ordem alfabética, separados por brancos
char *vertices_corte(grafo *g);

//------------------------------------------------------------------------------
// devolve uma "string" com as arestas de corte de g em ordem alfabética, separadas por brancos
// cada aresta é o par de nomes de seus vértices em ordem alfabética, separadas por brancos
char *arestas_corte(grafo *g);
