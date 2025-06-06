#include "grafo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STARTING_VERT_NUM 50

// TODO
// maybe the fact that add_vert always search for the vertex being added makes it too slow
// If I used a heap sorted by the name I could make a divide and conquer search
// maybe a heap to which I can pass a sorting function?
//
// set_vertex_cut deve executar para cada componente, nao cada vertice

struct neighbor;
struct vertice;

typedef unsigned int uint;
typedef struct neighbor {
    struct vertice* vert;
    int weight; // peso da aresta da cabeca da lista ate o nodo
    struct neighbor* next;
} neighbor;

typedef struct vertice {
    char* name;

    struct neighbor* n_list;
    uint n_num;

    struct vertice* pai;
    uint dist;
    uint componente;
    uint lowpoint;
    uint nivel;
    int estado;
} vertice;

struct grafo {
    char* name;
    vertice* v;
    uint v_num;
    uint max_v_num;
    uint n_componentes;
};

typedef struct queue {
    void* elem;
    struct queue* prev; // aponta para o elemento anterior na fila
    struct queue* next; // aponta para o elemento seguinte na fila
} queue_t;

grafo* create_graph(char* name);
void destroy_neighbor_list(neighbor* head);
neighbor* add_neighbor(vertice* v, vertice* neigh_v, int weight);
vertice* add_vert(vertice** V, uint* size, uint* max, char* name);
vertice* add_graph_vert(grafo* g, char* name);
void add_graph_edge(grafo* g, char* name_a, char* name_b, int weight);
vertice* search_vert(vertice* V, uint size, char* name);
neighbor* create_neighbor(vertice* v, int weight);
int parse_line(char* line, char** x, char** y, int* p);
int get_valid_string(char* line, int size, FILE* f);
void set_states(vertice* V, uint size, int state);
void mininumDistances(grafo* g, vertice* v);
int queue_append(queue_t** queue, void* elem);
void destroy_queue(queue_t** queue);
void* queue_pop(queue_t** queue);
uint set_components(grafo* g);
void set_vertex_cut(grafo* g, vertice** vertex_cut, uint* size, uint* max);
int comp_vert_name(const void* a, const void* b);
char* create_name_list(vertice* V, uint size);
void low_point(grafo* g, vertice* r, vertice** vertex_cut, uint* size, uint* max);

// appends a new element to the queue
int queue_append(queue_t** queue, void* elem)
{
    if (!queue) {
        fprintf(stderr, "Erro: fila nao existe\n");
        return -1;
    }

    if (!elem) {
        fprintf(stderr, "Erro: elemento nao existe\n");
        return -2;
    }

    queue_t* node;
    if (!(node = malloc(sizeof(queue_t))))
        return -3;

    if (!(*queue)) {
        // caso a lista esteja vazia, adiciona o elemento
        node->elem = elem;
        node->next = node->prev = node;
        (*queue) = node;
        return 0;
    }

    queue_t* next_node = (*queue)->next;
    queue_t* prev_node = (*queue)->prev;

    // insere elemento no final da fila
    node->elem = elem;
    node->prev = prev_node;
    node->next = (*queue);

    (*queue)->prev = node;

    if (next_node == (*queue))
        // caso haja um unico elemento, altera tambem o next
        (*queue)->next = node;
    else
        // caso hajam mais elementos, altera next do ex-ultimo da fila
        prev_node->next = elem;

    return 0;
}

// pops firs element of the queue and returns a pointer to its content
void* queue_pop(queue_t** queue)
{
    void* elem;

    if (*queue == NULL)
        return NULL;

    elem = (*queue)->elem;
    if ((*queue)->next == *queue) {
        printf("last element\n");
        free(*queue);
        *queue = NULL;
    } else {
        printf("not last element\n");
        printf("next %s\n", ((vertice*)(*queue)->prev->elem)->name);
        (*queue)->next->prev = (*queue)->prev;
        (*queue)->prev->next = (*queue)->next;
        queue_t* aux = *queue;
        *queue = (*queue)->next;
        free(aux);
    }

    return elem;
}

// destroys queue and sets its value to NULL
void destroy_queue(queue_t** queue)
{
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

// set the state of the vertices in list 'V' of size 'size' as 'state'
void set_states(vertice* V, uint size, int state)
{
    if (V == NULL)
        return;

    for (uint i = 0; i < size; i++)
        V[i].estado = state;
}

// set the number of the component of all vertices in the graph g
// and returns number of components of the graph (uses BFS)
uint set_components(grafo* g)
{
    queue_t* queue = NULL;
    vertice* v;
    neighbor* w;
    vertice* r;
    uint c = 0;

    // initialize veritces state
    set_states(g->v, g->v_num, 0);

    // initialize component number of all vertices
    for (uint i = 0; i < g->v_num; i++)
        g->v[i].componente = 0;

    // for each vevrtice, set the component of the connected subgraph it belongs to
    for (uint i = 0; i < g->v_num; i++) {
        if (g->v[i].componente == 0) {
            r = &g->v[i];
            c++;

            r->pai = NULL;
            r->estado = 1;
            r->componente = c;
            printf("queue append %s\n", r->name);
            queue_append(&queue, r);

            // pop vertice from queue until it empties
            while ((v = queue_pop(&queue))) {
                printf("queue pop %s\n", v->name);
                // iterate through neighbors
                w = v->n_list;
                while (w != NULL) {
                    if (w->vert->estado == 0) {
                        printf("queue append %s\n", w->vert->name);
                        queue_append(
                            &queue, w->vert); // if neighbor wasnt processed, append to queue
                        w->vert->pai = v;
                        w->vert->estado = 1;
                        w->vert->componente = c;
                    }
                    w = w->next;
                }
                printf("queue poping\n");
            }
        }
    }

    destroy_queue(&queue);
    return c;
}

// set distances between vertice 'r' and all other vertices in the same compoment as 'r'
void mininumDistances(grafo* g, vertice* r)
{
    queue_t* queue = NULL;
    vertice* v;
    neighbor* w;

    set_states(g->v, g->v_num, 0);

    r->pai = NULL;
    r->dist = 0;
    r->estado = 1;
    queue_append(&queue, (void*)r);

    // pop vertice from queue
    while ((v = queue_pop(&queue))) {
        // iterate through neighbors
        w = v->n_list;
        while (w != NULL) {
            if (w->vert->estado == 0) {
                // processe w
                w->vert->pai = v;
                w->vert->dist = v->dist + 1;
                queue_append(&queue, w->vert);
                w->vert->estado = 1;
            }
            w = w->next;
        }
    }

    destroy_queue(&queue);
}

// search vertice of name 'name' in the list 'V' of size 'size'
// returns pointer to vertice, if it's found, else retuns NULL
vertice* search_vert(vertice* V, uint size, char* name)
{
    printf("searching %s in list of size %u\n", name, size);
    for (uint i = 0; i < size; i++) {
        printf("cmp(V[%d].name = %s, %s)\n", i, V[i].name, name);
        if (strcmp(V[i].name, name) == 0)
            return &V[i];
    }
    printf("end of search\n");

    return NULL;
}

// create a node of type neighbor pointing to vertice 'v', with wheight 'weight'
neighbor* create_neighbor(vertice* v, int weight)
{
    neighbor* neigh;

    if (!(neigh = malloc(sizeof(neighbor))))
        return NULL;

    // node->name = name;
    neigh->vert = v;
    neigh->weight = weight;
    neigh->next = NULL;

    return neigh;
}

// Adds a vertice with name 'name' to the list 'V' of current size 'size' and maximum
// size 'max', adding one to the current size variable. if theres no space left in then list,
// reallocates it and alters 'max' accordingly
vertice* add_vert(vertice** V, uint* size, uint* max, char* name)
{
    vertice* v = search_vert(*V, *size, name);

    if (v == NULL) {
        if (*size >= *max) {
            if (*max == 0) {
                *V = NULL;
                *max = STARTING_VERT_NUM;
            }
            // increase size of V(G)
            uint new_max = *max + 10;
            if (!(*V = realloc(*V, sizeof(vertice) * new_max)))
                return NULL;
            *max = new_max;
        }
        (*size)++;
        (*V)[*size - 1].name = name;
        printf("V[%d].name = %s\n", *size - 1, (*V)[*size - 1].name);
        return &((*V)[*size - 1]);
    } else // if vertice is already in the list, dont do anything
        return v;
}

vertice* add_graph_vert(grafo* g, char* name)
{
    return add_vert(&(g->v), &g->v_num, &g->max_v_num, name);
}

// Adds to the vertice 'v' neighbor list a neighbor pointing to vertice 'neigh_v' with given weight.
neighbor* add_neighbor(vertice* v, vertice* neigh_v, int weight)
{
    neighbor* new_neigh = create_neighbor(neigh_v, weight);

    if (v->n_list == NULL)
        v->n_list = new_neigh;
    else {
        struct neighbor* neigh = v->n_list;

        // search end of linked list
        while (neigh->next != NULL) {
            neigh = neigh->next;
            // if vertice is already in the list of neighbors, return
            if (strcmp(neigh->vert->name, neigh_v->name) == 0)
                return neigh;
        }

        neigh->next = new_neigh;
    }
    (v->n_num)++;

    return new_neigh;
}

// destroys a neighbor list
void destroy_neighbor_list(neighbor* head)
{
    neighbor* neigh;

    while (head != NULL) {
        neigh = head->next;
        free(head);
        head = neigh;
    }
}

// creates a graph with given name
grafo* create_graph(char* name)
{
    grafo* g;

    if (!(g = malloc(sizeof(grafo))))
        return NULL;

    g->name = name;

    if (!(g->v = malloc(sizeof(vertice) * STARTING_VERT_NUM))) {
        free(g);
        return NULL;
    }

    g->v_num = 0;
    g->max_v_num = STARTING_VERT_NUM;
    g->n_componentes = 0;

    for (uint i = 0; i < STARTING_VERT_NUM; i++) {
        g->v->name = NULL;
        g->v->n_list = NULL;
        g->v->n_num = 0;
        // g->v->componente = 0; (setado na funcao set_components)
    }

    return g;
}

// Parses string 'line' and sets 'x', 'y' and 'p' with the names of vertices and weight
// presented in the string
int parse_line(char* line, char** x, char** y, int* p)
{
    char aux1[20], aux2[20];
    *p = -1; // initialize with default value

    // Parse the line
    int res = sscanf(line, "%9s %*s %9s %d", aux1, aux2, p);

    if ((res == EOF) || res == 0)
        return 0;

    if (!(*x = malloc(sizeof(char) * strlen(aux1) + 1))) {
        return 0;
    }

    if (!(*y = malloc(sizeof(char) * strlen(aux2) + 1))) {
        free(*x);
        return 0;
    }

    strcpy(*x, aux1);
    strcpy(*y, aux2);

    return res;
}

// Reads file 'f' until a valid line is read (or EOF is found)
// recieves a buffer 'line' with size 'size' and sets the buffer with the valid lilne.
// If no valid line could be found, returns 0
int get_valid_string(char* line, int size, FILE* f)
{
    while (1) {
        // Read an entire line from input
        if (!fgets(line, size, f))
            return 0; // EOF or error
        else if ((line[0] == '/')
            || (line[0] == '\n')) // If the line stars with / or is empty, skip it
            continue;
        return 1;
    }
}

// Adds the vertices of name 'name_a' and 'name_b' to the graph 'g'
// sets name_b as neighbor of name_a with weight 'weight', and vice-versa
void add_graph_edge(grafo* g, char* name_a, char* name_b, int weight)
{
    vertice *vert_a, *vert_b;
    vert_a = add_graph_vert(g, name_a);
    vert_b = add_graph_vert(g, name_b);
    add_neighbor(vert_a, vert_b, weight);
    add_neighbor(vert_b, vert_a, weight);
}

//------------------------------------------------------------------------------
// lê um grafo de f e o devolve
grafo* le_grafo(FILE* f)
{
    char *g_name, *name_x, *name_y, line[200];
    int p, res;

    if (!get_valid_string(line, 200, f))
        return NULL;
    line[strlen(line) - 1] = '\0';

    // alloca e copia nome
    if (!(g_name = malloc(sizeof(char) * strlen(line) + 1)))
        return NULL;
    strcpy(g_name, line);

    grafo* g = create_graph(g_name);

    printf("graph name: %s\n", g->name);

    while (1) {
        if (!get_valid_string(line, 200, f))
            break;

        res = parse_line(line, &name_x, &name_y, &p);

        if (res == 0) {
            destroi_grafo(g);
            return NULL;
        } else if (res > 1) {
            printf("\nname_x:%s name_y:%s p:%d\n", name_x, name_y, p);
            printf("\nadd v\n");
            add_graph_edge(g, name_x, name_y, p);
        } else if (res == 1) {
            printf("\nadd v\n");
            add_graph_vert(g, name_x);
        }
    }
    return g;
}

//------------------------------------------------------------------------------
// desaloca toda a estrutura de dados alocada em g
// devolve 1 em caso de sucesso e 0 em caso de erro
unsigned int destroi_grafo(grafo* g)
{
    if (g == NULL)
        return 0;

    if (g->name)
        free(g->name);

    if (g->v) {
        for (uint i = 0; i < g->v_num; i++) {
            if (g->v[i].name)
                free(g->v[i].name);
            destroy_neighbor_list(g->v[i].n_list);
        }
        free(g->v);
    }

    free(g);

    return 1;
}

//------------------------------------------------------------------------------
// devolve o nome de g
char* nome(grafo* g) { return g->name; }

//------------------------------------------------------------------------------
// devolve o número de vértices em g
unsigned int n_vertices(grafo* g) { return g->v_num; }

//------------------------------------------------------------------------------
// devolve o número de arestas em g
unsigned int n_arestas(grafo* g)
{
    uint arestas = 0;

    for (uint i = 0; i < g->v_num; i++)
        arestas += g->v[i].n_num;

    return arestas / 2;
}

//------------------------------------------------------------------------------
// devolve o número de componentes em g
unsigned int n_componentes(grafo* g)
{
    printf("setting components\n");
    // ja que vertices so sao adicionados ao criar o grafo,
    // nao eh necessario setar componentes de um grafo mais que uma vez
    if (g->n_componentes != 0)
        return g->n_componentes;

    g->n_componentes = set_components(g);
    printf("setted\n");

    return g->n_componentes;
}

// calculate lowpoints of vertices in the tree of which 'r' is the root.
// adds the vertices that are cut vertex to the 'vertex_cut' list of size 'size' and maximum size
// 'max', altering the values of size and max as needed.
void low_point(grafo* g, vertice* r, vertice** vertex_cut, uint* size, uint* max)
{
    printf("low_point v: %s\n", r->name);
    r->estado = 1;

    // iterate through neighbors
    neighbor* w = r->n_list;
    while (w != NULL) {
        printf("processando %s vizinho de %s\n", w->vert->name, r->name);
        if ((w->vert->estado == 1) && (w->vert->nivel < r->lowpoint) && (w->vert != r->pai))
            r->lowpoint = w->vert->nivel;
        else if (w->vert->estado == 0) {
            w->vert->pai = r;

            w->vert->nivel = r->nivel + 1;
            low_point(g, w->vert, vertex_cut, size, max);

            if (r->nivel > w->vert->lowpoint) {
                // add vertex to vertex cut
                printf("vertice de corte encontrado: %s\n", r->name);
                add_vert(vertex_cut, size, max, r->name);
                printf("%s\n", (*vertex_cut)[*size - 1].name);
            }

            if (w->vert->lowpoint < r->lowpoint)
                r->lowpoint = w->vert->lowpoint;
        }
        w = w->next;
    }
    printf("lowpoint do v %s: %u\n", r->name, r->lowpoint);

    r->estado = 2;
}

void set_vertex_cut(grafo* g, vertice** vertex_cut, uint* size, uint* max)
{
    printf("set_vertex_cut\n");
    set_states(g->v, g->v_num, 0);

    for (uint i = 0; i < g->v_num; i++) {
        if (g->v[i].estado == 0) {
            g->v[i].lowpoint = g->v[i].nivel = 0;
            low_point(g, &g->v[i], vertex_cut, size, max);
        }
    }
}

int comp_vert_name(const void* a, const void* b)
{
    return strcmp(((const vertice*)a)->name, ((const vertice*)b)->name);
}

// creates a string with names of vertices of given vertice list 'V' of size 'size' sorted
// alphabetically
char* create_name_list(vertice* V, uint size)
{
    printf("create_name_list\n");
    printf("size: %u\n", size);
    char* string;
    if (size == 0) {
        if (!(string = malloc(sizeof(char) * 1)))
            return NULL;
        string[0] = '\0';
        return string;
    }

    qsort(V, size, sizeof(vertice), comp_vert_name);

    size_t total_len = 0;

    // calcluate total size of output string
    for (uint i = 0; i < size; i++) {
        total_len += strlen(V[i].name);
        total_len++;
    }
    printf("total_len: %zu\n", total_len);

    // allocate string
    if (!(string = malloc(sizeof(char) * total_len)))
        return NULL;

    string[0] = '\0';
    // concatanate name of all vertices to the output string
    uint i;
    for (i = 0; i < size - 1; i++) {
        printf("concat name %s\n", V[i].name);
        strcat(string, V[i].name);
        strcat(string, " ");
    }
    strcat(string, V[i].name);

    return string;
}

//------------------------------------------------------------------------------
// devolve uma "string" com os nomes dos vértices de corte de g em
// ordem alfabética, separados por brancos
char* vertices_corte(grafo* g)
{
    printf("vertices_corte\n");
    vertice* vertex_cut = NULL;
    uint size = 0;
    uint max = 0;

    set_vertex_cut(g, &vertex_cut, &size, &max);

    char* name_list = create_name_list(vertex_cut, size);

    return name_list;
}

/*
//------------------------------------------------------------------------------
// devolve 1 se g é bipartido e 0 caso contrário
unsigned int bipartido(grafo *g);

//------------------------------------------------------------------------------
// devolve uma "string" com os diâmetros dos componentes de g separados por
brancos
// em ordem não decrescente
char *diametros(grafo *g);

//------------------------------------------------------------------------------
// devolve uma "string" com as arestas de corte de g em ordem alfabética,
separadas por brancos
// cada aresta é o par de nomes de seus vértices em ordem alfabética, separadas
por brancos char *arestas_corte(grafo *g);
*/
