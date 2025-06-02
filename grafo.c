#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grafo.h"

#define STARTING_VERT_NUM 50

struct neighbor;
struct vertice;

typedef unsigned int uint;
typedef struct neighbor {
    struct vertice *vert;
    int weight; //peso da aresta da cabeca da lista ate o nodo
    struct neighbor *next;
} neighbor;

typedef struct vertice {
    char *name;

    struct neighbor *n_list;
    uint n_num;

    struct vertice *pai;
    uint dist;
    int estado;
} vertice;

struct grafo{
    char *name;
    vertice *v;
    uint v_num;
    uint max_v_num;
};

typedef struct queue
{
    void *elem;
    struct queue *prev ;  // aponta para o elemento anterior na fila
    struct queue *next ;  // aponta para o elemento seguinte na fila
} queue_t;

grafo *create_graph(char *name);
void destroy_neighbor_list(neighbor *head);
neighbor *add_neighbor(vertice *v, vertice *neigh_v, int weight);
vertice *add_vert(vertice *V, uint *size, uint *max, char *name);
vertice *add_graph_vert(grafo *g, char *name);
void add_graph_edge(grafo *g, char *name_a, char *name_b, int weight);
vertice *search_vert(vertice *V, uint size, char *name);
neighbor *create_neighbor(vertice *v, int weight);
int parse_line(char *line, char **x, char **y, int *p);
int get_valid_string(char *line, int size, FILE* f);

void set_states(vertice *V, uint size, int state);
void mininumDistances(grafo *g, vertice *v);
int queue_append (queue_t **queue, void *elem);
void destroy_queue(queue_t **queue);
void *queue_remove(queue_t **queue);

int queue_append (queue_t **queue, void *elem){
    if (!queue){
        fprintf(stderr,"Erro: fila nao existe\n");
        return -1;
    }

    if (!elem){
        fprintf(stderr,"Erro: elemento nao existe\n");
        return -2;
    }

    queue_t *node;
    if ( !(node = malloc(sizeof(queue_t))) )
        return -3;

    if (!(*queue)){
        //caso a lista esteja vazia, adiciona o elemento
        node->elem = elem;
        node->next = node->prev = node;
        (*queue)=node;
        return 0;
    }

    queue_t *next_node=(*queue)->next;
    queue_t *prev_node=(*queue)->prev;

    //insere elemento no final da fila
    node->elem = elem;
    node->prev = prev_node;
    node->next = (*queue);

    (*queue)->prev=node;

    if (next_node == (*queue))
        //caso haja um unico elemento, altera tambem o next
        (*queue)->next=node;
    else
        //caso hajam mais elementos, altera next do ex-ultimo da fila
        prev_node->next=elem;

    return 0;
}

void *queue_remove(queue_t **queue) {
    void *elem;

    if (*queue == NULL)
        return NULL;

    elem = (*queue)->elem;
    if ((*queue)->next == *queue) {
        free(*queue);
        *queue = NULL;
    }
    else {
        (*queue)->next->prev = (*queue)->prev;
        queue_t *aux = *queue;
        *queue = (*queue)->next;
        free(aux);
    }

    return elem;
}

void destroy_queue(queue_t **queue) {
    if (*queue == NULL)
        return;

    queue_t *node, *aux;
    node = *queue;

    while (node->next != node) {
        node->next->prev = node->prev;
        node->prev->next = node->next;

        aux = node->next;
        free(node);
        node = aux;
    }

    free(node);
    *queue = NULL;
}

void set_states(vertice *V, uint size, int state) {
    if (V == NULL) return;

    for (uint i = 0; i < size; i++)
        V[i].estado = state;
}

// seta distancias minimas entre r e os vertices no componente de r
void mininumDistances(grafo *g, vertice *r) {
    queue_t *queue;
    vertice *v;
    neighbor *w;
    int start, end; //start and end of queue

    set_states(g->v, g->v_num, 0);

    start = end = 0;

    r->pai = NULL;
    r->dist = 0;
    r->estado = 1;
    queue_append(&queue, (void *)r);

    while (start <= end) {
        //desenfileira vertice
        v = queue_remove(&queue);

        //itera por vizinhos
        w = v->n_list;
        while ( w != NULL) {
            if (w->vert->estado == 0) {
                //processe w
                w->vert->pai = v;
                w->vert->dist = v->dist+1;
                queue_append(&queue, w->vert);
                w->vert->estado = 1;
            }
        }
        v->estado=2;
    }

    destroy_queue(&queue);
}

vertice *search_vert(vertice *V, uint size, char *name) {
    printf("search\n");
    printf("size %d\n", size);
    for (uint i = 0; i < size; i++) {
        printf("cmp(V[%d].name = %s, %s)\n", i, V[i].name, name);
        if (strcmp(V[i].name, name) == 0)
            return &V[i];
    }

    return NULL;
}

neighbor *create_neighbor(vertice *v, int weight) {
    neighbor *neigh;

    if ( !(neigh = malloc(sizeof(neighbor))) )
        return NULL;

    //node->name = name;
    neigh->vert = v;
    neigh->weight = weight;
    neigh->next = NULL;

    return neigh;
}

vertice *add_vert(vertice *V, uint *size, uint *max, char *name) {
    vertice *v = search_vert(V, *size, name);

    if (v == NULL) {
        if (*size >= *max) {
            printf("increasing max\n");
            //increase size of V(G)
            uint new_max = *max+10;
            if ( !(V = realloc(V, sizeof(vertice)*new_max)) )
                return NULL;
            *max = new_max;
        }
        (*size)++;
        printf("V[%d].name = %s\n", *size-1, name);
        V[*size-1].name = name;
        return &(V[*size-1]);
    }
    else //if vertice is already in the list, dont do anything
        return v;
}

vertice *add_graph_vert(grafo *g, char *name) {
    return add_vert(g->v, &g->v_num, &g->max_v_num, name);
}

// Recieves a vertice a name and a weight.
// Adds to the vertice neighbor list a neighbor with name and weight given.
neighbor *add_neighbor(vertice *v, vertice *neigh_v, int weight) {
    neighbor *new_neigh = create_neighbor(neigh_v, weight);

    if (v->n_list == NULL)
        v->n_list = new_neigh;
    else {
        struct neighbor *neigh = v->n_list;

        //search end of linked list
        while (neigh->next != NULL) {
            neigh = neigh->next;
            //if vertice is already in the list of neighbors, return
            if (strcmp(neigh->vert->name, neigh_v->name) == 0)
                return neigh;
        }

        neigh->next = new_neigh;
        (v->n_num)++;
    }

    return new_neigh;
}

void destroy_neighbor_list(neighbor *head) {
    neighbor *neigh;

    while (head != NULL) {
        neigh=head->next;
        free(head);
        head = neigh;
    }
}

grafo *create_graph(char *name) {
    grafo *g;

    if (! (g = malloc(sizeof(grafo))) )
        return NULL;

    g->name = name;

    if (! (g->v = malloc(sizeof(vertice)*STARTING_VERT_NUM)) ) {
        free(g);
        return NULL;
    }

    g->v_num = 0;
    g->max_v_num = STARTING_VERT_NUM;

    for (uint i = 0; i < STARTING_VERT_NUM; i++) {
        g->v->name = NULL;
        g->v->n_list = NULL;
        g->v->n_num = 0;
    }

    return g;
}

int parse_line(char *line, char **x, char **y, int *p) {
    char aux1[20], aux2[20];
    *p = -1; //initialize with default value

    // Parse the line
    int res = sscanf(line, "%9s %*s %9s %d", aux1, aux2, p);

    if ((res == EOF) || res == 0) return 0;

    if ( !(*x = malloc(sizeof(char)*strlen(aux1)+1)) ) {
        return 0;
    }

    if ( !(*y = malloc(sizeof(char)*strlen(aux2)+1)) ) { 
        free(*x);
        return 0;
    }

    strcpy(*x, aux1);
    strcpy(*y, aux2);

    return res;
}

int get_valid_string(char *line, int size, FILE* f) {
    while (1) {
        // Read an entire line from input
        if (!fgets(line, size, f))
            return 0; // EOF or error
        else if ((line[0] == '/') || (line[0] == '\n')) // If the line stars with / or is empty, skip it
            continue;
        return 1;
    }
}

void add_graph_edge(grafo *g, char *name_a, char *name_b, int weight) {
    vertice *vert_a, *vert_b;
    vert_a = add_graph_vert(g, name_a);
    vert_b = add_graph_vert(g, name_b);
    add_neighbor(vert_a, vert_b, weight);
    add_neighbor(vert_b, vert_a, weight);
}
//------------------------------------------------------------------------------
// lê um grafo de f e o devolve
grafo *le_grafo(FILE *f) {
    char *g_name, *name_x, *name_y, line[200];
    int p, res;

    if (!get_valid_string(line, 200, f)) return NULL;
    line[strlen(line)-1] = '\0';

    //alloca e copia nome
    if ( !(g_name = malloc(sizeof(char)*strlen(line)+1)) )
        return NULL;
    strcpy(g_name, line);

    grafo *g = create_graph(g_name);

    printf("graph name: %s\n", g->name);

    while (1) {
        if (!get_valid_string(line, 200, f)) break;

        res = parse_line(line, &name_x, &name_y, &p);

        if ( res == 0) {
            destroi_grafo(g);
            return NULL;
        }
        else if (res > 1) {
            printf("\nname_x:%s name_y:%s p:%d\n", name_x, name_y, p);
            printf("\nadd v\n");
            add_graph_edge(g, name_x, name_y, p);
        }
        else if (res == 1){
            printf("\nadd v\n");
            add_graph_vert(g, name_x);
        }
    }
    return g;
}

//------------------------------------------------------------------------------
// desaloca toda a estrutura de dados alocada em g
// devolve 1 em caso de sucesso e 0 em caso de erro
unsigned int destroi_grafo(grafo *g) {
    if (g == NULL)
        return 0;

    if (g->name) free(g->name);

    if (g->v) {
        for(uint i = 0; i < g->v_num; i++) {
            if (g->v[i].name) free(g->v[i].name);
            destroy_neighbor_list(g->v[i].n_list);
        }
        free(g->v);
    }

    free(g);

    return 1;
}

//------------------------------------------------------------------------------
// devolve o nome de g
char *nome(grafo *g) {
    return g->name;
}

//------------------------------------------------------------------------------
// devolve o número de vértices em g
unsigned int n_vertices(grafo *g) {
    return g->v_num;
}

//------------------------------------------------------------------------------
// devolve o número de arestas em g
unsigned int n_arestas(grafo *g) {
    uint arestas = 0;
    for (uint  i = 0; i < g->v_num; i++) {
        arestas += g->v[i].n_num;
    }
    return arestas/2;
}

/*
//------------------------------------------------------------------------------
// devolve 1 se g é bipartido e 0 caso contrário
unsigned int bipartido(grafo *g);

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
*/
