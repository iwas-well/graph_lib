#include "grafo.h"
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STARTING_VERT_NUM 1
#define STARTING_CUT_LIST_NUM 1

typedef unsigned int uint;

struct neighbor;
struct vertice;

typedef struct neighbor {
    struct vertice* vert;
    uint weight; // peso da aresta ateh o vizinho
    struct neighbor* next;
} neighbor;

typedef struct vertice {
    char* name;
    struct neighbor* n_list;
    uint n_num;
    struct vertice* pai;
    uint componente;
    uint dist;
    uint lowpoint;
    uint nivel;
    int estado;
} vertice;

typedef struct str_list {
    char** str;
    uint size;
    uint max;
} str_list;

struct grafo {
    char* name;
    vertice* v;
    uint v_num;
    uint max_v_num;
    uint n_componentes;
    str_list vertex_cut;
    str_list edge_cut;
    str_list diametros;
};

typedef struct queue {
    void* elem;
    struct queue* prev;
    struct queue* next;
} queue_t;

typedef struct {
    vertice** items;
    uint size;
    uint capacity;
} Heap;

grafo* create_graph(char* name);
void destroy_neighbor_list(neighbor* head);
neighbor* add_neighbor(vertice* v, vertice* neigh_v, uint weight);
vertice* add_vert(vertice** V, uint* size, uint* max, char* name);
vertice* add_graph_vert(grafo* g, char* name);
void add_graph_edge(grafo* g, char* name_a, char* name_b, uint weight);
vertice* search_vert(vertice* V, uint size, char* name);
neighbor* create_neighbor(vertice* v, uint weight);
int parse_line(char* line, char** x, char** y, int* p);
int get_valid_string(char* line, int size, FILE* f);
void set_states(vertice* V, uint size, int state);
void mininumDistances(grafo* g, vertice* v);
uint set_components(grafo* g);
void low_point(grafo* g, vertice* r, str_list* vertex_cut, str_list* edge_cut);
void set_cut(grafo* g);
char* create_edge_name(const char* name1, const char* name2);
int comp_name(const void* a, const void* b);
int comp_number(const void* a, const void* b);
char* create_str_from_str_list(char** str, uint size, int (*func)(const void* a, const void* b));
char** add_name(str_list* list, char* name);
void dijkstra_max_dist(Heap* h);
void set_diameters(grafo* g);
void destroy_str_list(str_list list);

// queue functions
void queue_append(queue_t** queue, void* elem);
void destroy_queue(queue_t** queue);
void* queue_pop(queue_t** queue);

// min-heap functions
Heap* heap_create(void);
void heap_free(Heap* h);
void heap_append_vertex(Heap* h, vertice* v);
vertice* heap_pop_vertex(Heap* h);
void heapify_up(Heap* h, uint idx);
void heapify_down(Heap* h, uint idx);
void heapify_vertex_up(Heap* h, vertice* v);
void swap(vertice** a, vertice** b);

//------------------------------------------------------------------------------
// lê um grafo de f e o devolve
grafo* le_grafo(FILE* f)
{
    char *g_name, *name_x, *name_y, line[200];
    int res;
    uint p;

    if (!get_valid_string(line, 200, f))
        return NULL;
    line[strlen(line) - 1] = '\0';

    // alloca e copia nome
    if (!(g_name = malloc(sizeof(char) * strlen(line) + 1)))
        return NULL;
    strcpy(g_name, line);

    grafo* g = create_graph(g_name);

    while (1) {
        if (!get_valid_string(line, 200, f))
            break;

        res = parse_line(line, &name_x, &name_y, (int*)&p);

        if (res == 0) {
            destroi_grafo(g);
            return NULL;
        } else if (res > 1) {
            add_graph_edge(g, name_x, name_y, p);
        } else if (res == 1) {
            add_graph_vert(g, name_x);
        }
    }

    set_cut(g);
    set_diameters(g);

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

    destroy_str_list(g->vertex_cut);
    destroy_str_list(g->edge_cut);
    destroy_str_list(g->diametros);

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
    if (g->n_componentes != 0)
        return g->n_componentes;

    g->n_componentes = set_components(g);

    return g->n_componentes;
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

    // initialize component number of all vertices
    for (uint i = 0; i < g->v_num; i++)
        g->v[i].componente = 0;

    // for each vertice, set the component of the connected subgraph it belongs to
    for (uint i = 0; i < g->v_num; i++) {
        if (g->v[i].componente == 0) {
            // initialize veritces state
            set_states(g->v, g->v_num, 0);

            r = &g->v[i];
            r->pai = NULL;
            r->estado = 1;
            r->componente = ++c;
            queue_append(&queue, r);

            // pop vertex from queue until it empties
            while ((v = queue_pop(&queue))) {
                //  iterate through neighbors
                w = v->n_list;
                while (w != NULL) {
                    if (w->vert->estado == 0) {
                        // if neighbor wasnt processed, append to queue
                        queue_append(&queue, w->vert);
                        w->vert->pai = v;
                        w->vert->estado = 1;
                        w->vert->componente = c;
                    }
                    w = w->next;
                }
            }
        }
    }

    destroy_queue(&queue);
    return c;
}

// search vertice of name 'name' in the list 'V' of size 'size'
// returns pointer to vertice, if it's found, else retuns NULL
vertice* search_vert(vertice* V, uint size, char* name)
{
    for (uint i = 0; i < size; i++) {
        if (strcmp(V[i].name, name) == 0)
            return &V[i];
    }

    return NULL;
}

// create a node of type neighbor pointing to vertice 'v', with wheight 'weight'
neighbor* create_neighbor(vertice* v, uint weight)
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
    uint new_max;

    if (v == NULL) {
        if (*size >= *max) {
            // printf("size %u max %u\n", *size, *max);
            if (*max == 0) {
                *V = NULL;
                *max = STARTING_VERT_NUM;
                new_max = *max;
            } else
                new_max = *max + 1;

            if (!(*V = realloc(*V, sizeof(vertice) * new_max))) {
                fprintf(stderr, "Erro add_vert: nao foi possivel realocar lista de vertices\n");
                exit(EXIT_FAILURE);
            }

            *max = new_max;

            for (uint i = *size; i < *max; i++) {
                printf("v %d list <- NULL\n", i);
                (*V)[i].name = NULL;
                (*V)[i].n_list = NULL;
                (*V)[i].n_num = 0;
            }
        }
        (*size)++;
        (*V)[*size - 1].name = name;
        printf("v %u = %s\n", *size - 1, (*V)[*size - 1].name);
        return &((*V)[*size - 1]);
    } else // if vertice is already in the list, dont do anything
        return v;
}

vertice* add_graph_vert(grafo* g, char* name)
{
    return add_vert(&(g->v), &g->v_num, &g->max_v_num, name);
}

// Adds to the vertice 'v' neighbor list a neighbor pointing to vertice 'neigh_v' with given weight.
neighbor* add_neighbor(vertice* v, vertice* neigh_v, uint weight)
{
    neighbor* new_neigh = create_neighbor(neigh_v, weight);

    printf("add neigh %s to vert %s\n", neigh_v->name, v->name);
    if (v->n_list == NULL) {
        printf("v %s n_list null\n", v->name);
        v->n_list = new_neigh;
    } else {
        struct neighbor* neigh = v->n_list;
        printf("v %s n_list not null\n", v->name);
        //  search end of linked list
        while (neigh->next != NULL) {
            neigh = neigh->next;
            // if vertice is already in the list of neighbors, return
            if (strcmp(neigh->vert->name, neigh_v->name) == 0)
                return neigh;
        }

        neigh->next = new_neigh;
    }
    (v->n_num)++;

    // printf("ended adding neigh\n");
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

// destroys str_list
void destroy_str_list(str_list list)
{
    if (list.str == NULL)
        return;
    for (uint i = 0; i < list.size; i++)
        free(list.str[i]);
    free(list.str);
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

    g->vertex_cut.str = NULL;
    g->edge_cut.str = NULL;
    g->vertex_cut.size = g->vertex_cut.max = 0;
    g->edge_cut.size = g->edge_cut.max = 0;

    for (uint i = 0; i < STARTING_VERT_NUM; i++) {
        g->v[i].name = NULL;
        // printf("v %d list = NULL\n", i);
        g->v[i].n_list = NULL;
        g->v[i].n_num = 0;
    }

    return g;
}

// Parses string 'line' and sets 'x', 'y' and 'p' with the names of vertices and weight
// presented in the string
int parse_line(char* line, char** x, char** y, int* p)
{
    char aux1[20], aux2[20];
    *p = 1; // initialize with default value

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
void add_graph_edge(grafo* g, char* name_a, char* name_b, uint weight)
{
    vertice *vert_a, *vert_b;
    // vert_a = add_graph_vert(g, name_a);
    add_graph_vert(g, name_a);
    vert_b = add_graph_vert(g, name_b);

    vert_a = search_vert(g->v, g->v_num, name_a);

    add_neighbor(vert_a, vert_b, weight);
    add_neighbor(vert_b, vert_a, weight);
}

char** add_name(str_list* list, char* name)
{
    uint new_max;
    if (list->size >= list->max) {
        if (list->max == 0) {
            list->str = NULL;
            list->max = STARTING_CUT_LIST_NUM;
            new_max = list->max;
        } else
            new_max = list->max + 50;

        if (!(list->str = realloc(list->str, sizeof(char*) * new_max))) {
            fprintf(stderr, "Erro add_name: nao foi possivel realocar lista de strings\n");
            exit(EXIT_FAILURE);
        }

        list->max = new_max;
    }
    (list->size)++;
    (list->str)[list->size - 1] = name;

    return &(list->str[list->size - 1]);
}

char* create_edge_name(const char* name1, const char* name2)
{
    char* str;

    if (name2[0] == '\0') {
        if (!(str = malloc(sizeof(char) * strlen(name1))))
            return NULL;
        strcpy(str, name1);
    } else {
        size_t len = strlen(name1) + strlen(name2) + 2;

        if (!(str = malloc(sizeof(char) * len)))
            return NULL;

        if (strcmp(name1, name2) < 0) {
            strcpy(str, name1);
            strcat(str, " ");
            strcat(str, name2);
        } else {
            strcpy(str, name2);
            strcat(str, " ");
            strcat(str, name1);
        }
    }

    return str;
}

// calculate lowpoints of vertices in the tree of which 'r' is the root.
// adds the vertices that are cut vertex to the 'vertex_cut' list of size 'size' and maximum size
// 'max', altering the values of size and max as needed.
void low_point(grafo* g, vertice* r, str_list* vertex_cut, str_list* edge_cut)
{
    printf("r %s\n", r->name);
    unsigned int n_filhos = 0;
    r->estado = 1;
    char* str;

    // iterate through neighbors
    neighbor* w = r->n_list;
    printf("r nlist %p\n", r->n_list);
    while (w != NULL) {
        printf("AAAAAAA\n");
        printf("w %s\n", w->vert->name);
        if ((w->vert->estado == 1) && (w->vert->nivel < r->lowpoint) && (w->vert != r->pai))
            r->lowpoint = w->vert->nivel;
        else if (w->vert->estado == 0) {
            w->vert->pai = r;
            w->vert->lowpoint = w->vert->nivel = r->nivel + 1;
            low_point(g, w->vert, vertex_cut, edge_cut);

            n_filhos++;
            if ((r->nivel <= w->vert->lowpoint) && (r->pai != NULL)) {
                //  add vertex to vertex cut
                str = create_edge_name(r->name, "");
                add_name(vertex_cut, str);
            }

            if (r->nivel < w->vert->lowpoint) {
                if (r->nivel < w->vert->lowpoint) {
                    str = create_edge_name(r->name, w->vert->name);
                    add_name(edge_cut, str);
                }
            }

            if (w->vert->lowpoint < r->lowpoint)
                r->lowpoint = w->vert->lowpoint;
        }
        w = w->next;
    }

    if ((r->pai == NULL) && (n_filhos > 1)) {
        str = create_edge_name(r->name, "");
        add_name(vertex_cut, str);
    }

    r->estado = 2;
}

// sets graph vertex_cut and edge_cut lists
void set_cut(grafo* g)
{
    // empties vertex_cut list
    if (g->vertex_cut.str != NULL) {
        for (uint i = 0; i < g->vertex_cut.size; i++)
            free(g->vertex_cut.str[i]);
        free(g->vertex_cut.str);
        g->vertex_cut.str = NULL;
        g->vertex_cut.size = g->vertex_cut.max = 0;
    }

    // empties edge_cut list
    if (g->edge_cut.str != NULL) {
        for (uint i = 0; i < g->edge_cut.size; i++)
            free(g->edge_cut.str[i]);
        free(g->edge_cut.str);
        g->edge_cut.str = NULL;
        g->edge_cut.size = g->edge_cut.max = 0;
    }

    set_states(g->v, g->v_num, 0);

    for (uint i = 0; i < g->v_num; i++) {
        if (g->v[i].estado == 0) {
            g->v[i].lowpoint = g->v[i].nivel = 0;
            g->v[i].pai = NULL;
            low_point(g, &g->v[i], &g->vertex_cut, &g->edge_cut);
        }
    }
}

int comp_name(const void* a, const void* b) { return strcmp(*(char* const*)a, *(char* const*)b); }

int comp_number(const void* a, const void* b)
{
    int n1 = atoi(*(char* const*)a);
    int n2 = atoi(*(char* const*)b);
    if (n1 < n2)
        return -1;
    else if (n1 == n2)
        return 0;
    return 1;
}

// creates a string with names of vertices of given vertice list 'V' of size 'size' sorted
// alphabetically
char* create_str_from_str_list(char** str, uint size, int (*func)(const void* a, const void* b))
{
    char* string;
    if (size == 0) {
        if (!(string = malloc(sizeof(char) * 1)))
            return NULL;
        string[0] = '\0';
        return string;
    }

    qsort(str, size, sizeof(char*), func);

    size_t total_len = 0;

    // calcluate total size of output string
    for (uint i = 0; i < size; i++) {
        total_len += strlen(str[i]);
        total_len++;
    }

    // allocate string
    if (!(string = malloc(sizeof(char) * total_len)))
        return NULL;

    string[0] = '\0';
    // concatanate name of all vertices to the output string
    uint i;
    for (i = 0; i < size - 1; i++) {
        strcat(string, str[i]);
        strcat(string, " ");
    }
    strcat(string, str[i]);

    return string;
}

//------------------------------------------------------------------------------
// devolve uma "string" com os nomes dos vértices de corte de g em
// ordem alfabética, separados por brancos
char* vertices_corte(grafo* g)
{
    char* name_list = create_str_from_str_list(g->vertex_cut.str, g->vertex_cut.size, comp_name);

    return name_list;
}

//------------------------------------------------------------------------------
// devolve uma "string" com as arestas de corte de g em ordem alfabética, separadas por brancos
// cada aresta é o par de nomes de seus vértices em ordem alfabética, separadas por brancos
char* arestas_corte(grafo* g)
{
    char* name_list = create_str_from_str_list(g->edge_cut.str, g->edge_cut.size, comp_name);

    return name_list;
}

//------------------------------------------------------------------------------
// devolve 1 se g é bipartido e 0 caso contrário
unsigned int bipartido(grafo* g)
{
    queue_t* queue = NULL;
    vertice* v;
    neighbor* w;
    vertice* r;

    // for each vevrtice, set the component of the connected subgraph it belongs to
    for (uint i = 0; i < g->v_num; i++) {
        // initialize veritces state
        set_states(g->v, g->v_num, 0);

        r = &g->v[i];
        r->pai = NULL;
        r->estado = 1;
        r->nivel = 0;
        queue_append(&queue, r);

        // pop vertice from queue until it empties
        while ((v = queue_pop(&queue))) {
            // iterate through neighbors
            w = v->n_list;
            while (w != NULL) {
                if (w->vert->estado == 1) {
                    if ((w->vert->nivel % 2) == (v->nivel % 2)) {
                        destroy_queue(&queue);
                        return 0;
                    }
                }
                if (w->vert->estado == 0) {
                    // if neighbor wasnt processed, append to queue
                    queue_append(&queue, w->vert);
                    w->vert->pai = v;
                    w->vert->estado = 1;
                    w->vert->nivel = v->nivel + 1;
                }
                w = w->next;
            }
        }
    }

    destroy_queue(&queue);
    return 1;
}

//------------------------------------------------------------------------------
// devolve uma "string" com os diâmetros dos componentes de g separados por brancos em ordem não
// decrescente
char* diametros(grafo* g)
{

    char* name_list = create_str_from_str_list(g->diametros.str, g->diametros.size, comp_number);

    return name_list;
}

void dijkstra_max_dist(Heap* h)
{
    vertice* v;
    neighbor* w;

    while (h->size != 0) {
        v = heap_pop_vertex(h);
        w = v->n_list;
        while (w != NULL) {
            if ((v->dist + w->weight) < w->vert->dist)
                w->vert->dist = v->dist + w->weight;
            heapify_vertex_up(h, w->vert);
            w = w->next;
        }
    }
}

void set_diameters(grafo* g)
{
    char buffer[50];
    char* diam_str = NULL;
    queue_t* queue = NULL;
    Heap* prio_queue = NULL;
    vertice* r;
    uint diametro;
    uint max_dist;
    int len;

    // set_components(g);
    if (g->n_componentes == 0)
        g->n_componentes = set_components(g);

    set_states(g->v, g->v_num, 0);

    for (uint c = 1; c <= g->n_componentes; c++) {
        diametro = 0;

        for (uint i = 0; i < g->v_num; i++) {
            if (g->v[i].componente == c)
                // cria lista com vertices do componente atual
                queue_append(&queue, &(g->v[i]));
        }

        while ((r = queue_pop(&queue))) {
            max_dist = 0;
            prio_queue = heap_create();
            for (uint i = 0; i < g->v_num; i++)
                if (g->v[i].componente == c) {
                    // copia vertices do componente atual para lista de prioridades
                    heap_append_vertex(prio_queue, &(g->v[i]));
                    g->v[i].dist = UINT_MAX;
                }

            r->dist = 0;
            dijkstra_max_dist(prio_queue);

            for (uint i = 0; i < g->v_num; i++)
                if (g->v[i].componente == c)
                    if (max_dist < g->v[i].dist)
                        max_dist = g->v[i].dist;

            if (max_dist > diametro)
                diametro = max_dist;
            heap_free(prio_queue);
        }

        len = snprintf(buffer, 50, "%d", diametro);
        if (len < 0) {
            fprintf(stderr, "Erro ao converter diametro para string\n");
            exit(EXIT_FAILURE);
        }

        if (!(diam_str = malloc(sizeof(char) * (uint)len + 1))) {
            fprintf(stderr, "Erro de alocação. Nao foi possivel alocar string de diametro\n");
            exit(EXIT_FAILURE);
        }

        strcpy(diam_str, buffer);
        add_name(&g->diametros, diam_str);
    }
}

// appends a new element to the queue
void queue_append(queue_t** queue, void* elem)
{
    if (!queue) {
        fprintf(stderr, "Erro queue_append: fila nao existe\n");
        exit(EXIT_FAILURE);
    }

    if (!elem) {
        fprintf(stderr, "Erro queue_append: elemento nao existe\n");
        exit(EXIT_FAILURE);
    }

    queue_t* node;
    if (!(node = malloc(sizeof(queue_t)))) {
        fprintf(stderr, "Erro queue_append: nao foi possivel alocar nodo\n");
        exit(EXIT_FAILURE);
    }

    if (!(*queue)) {
        // caso a lista esteja vazia, adiciona o elemento
        node->elem = elem;
        node->next = node->prev = node;
        (*queue) = node;
        return;
    }

    queue_t* prev_node = (*queue)->prev;

    // insere elemento no final da fila
    node->elem = elem;
    node->prev = prev_node;
    node->next = (*queue);

    (*queue)->prev = node;
    prev_node->next = node;
}

// pops firs element of the queue and returns a pointer to its content
void* queue_pop(queue_t** queue)
{
    void* elem;

    if (*queue == NULL)
        return NULL;

    elem = (*queue)->elem;
    if ((*queue)->next == *queue) {
        free(*queue);
        *queue = NULL;
    } else {
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

// Creates a heap with initial capacity
Heap* heap_create(void)
{
    Heap* h;

    if (!(h = malloc(sizeof(Heap)))) {
        fprintf(stderr, "Erro heap_create: nao foi possivel alocar heap\n");
        exit(EXIT_FAILURE);
    }

    if (!(h->items = malloc(sizeof(vertice*) * STARTING_VERT_NUM))) {
        fprintf(stderr, "Erro heap_create: nao foi possivel alocar itens da heap\n");
        exit(EXIT_FAILURE);
    }

    h->size = 0;
    h->capacity = STARTING_VERT_NUM;

    return h;
}

void heap_free(Heap* h)
{
    free(h->items);
    free(h);
}

void swap(vertice** a, vertice** b)
{
    vertice* temp = *a;
    *a = *b;
    *b = temp;
}

void heapify_up(Heap* h, uint idx)
{
    while (idx > 0) {
        uint parent = (idx - 1) / 2;
        if (h->items[parent]->dist <= h->items[idx]->dist)
            break;
        swap(&h->items[parent], &h->items[idx]);
        idx = parent;
    }
}

void heapify_down(Heap* h, uint idx)
{
    while (1) {
        uint left = 2 * idx + 1;
        uint right = 2 * idx + 2;
        uint smallest = idx;

        if (left < h->size && h->items[left]->dist < h->items[smallest]->dist)
            smallest = left;
        if (right < h->size && h->items[right]->dist < h->items[smallest]->dist)
            smallest = right;

        if (smallest == idx)
            break;

        swap(&h->items[idx], &h->items[smallest]);
        idx = smallest;
    }
}

void heap_append_vertex(Heap* h, vertice* v)
{
    if (h->size >= h->capacity) {
        uint new_capacity = h->capacity * 2;
        if (!(h->items = realloc(h->items, sizeof(vertice*) * new_capacity))) {
            fprintf(stderr, "Erro heap_append_vertex: nao foi possivel realocar itens da heap\n");
            exit(EXIT_FAILURE);
        }

        h->capacity = new_capacity;
    }

    h->items[h->size] = v;
    heapify_up(h, h->size);
    h->size++;
}

vertice* heap_pop_vertex(Heap* h)
{
    if (h->size == 0) {
        fprintf(stderr, "Erro heap_pop_vertex: heap vazia\n");
        exit(EXIT_FAILURE);
    }
    vertice* min = h->items[0];
    h->items[0] = h->items[h->size - 1];
    h->size--;
    heapify_down(h, 0);
    return min;
}

// sobe vertice na heap
void heapify_vertex_up(Heap* h, vertice* v)
{
    for (uint i = 0; i < h->size; i++) {
        if (h->items[i] == v) {
            heapify_up(h, i);
            break;
        }
    }
}
