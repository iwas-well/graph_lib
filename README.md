# graph_lib
Biblioteca de Manipulação e Análise de Grafos

# Este programa em C permite:
- Criar e armazenar grafos ponderados não direcionados.
- Detectar cortes de vértice e cortes de aresta.
- Calcular componentes conexos e diâmetro dos componentes.
  
# Funcionalidades principais
- le_grafo(FILE* f): Lê um grafo a partir de um arquivo com vértices e arestas.
- destroi_grafo(grafo* g): Libera toda a memória alocada para o grafo.
- n_vertices(grafo* g): Retorna o número de vértices.
- n_arestas(grafo* g): Retorna o número de arestas.
- n_componentes(grafo* g): Calcula e retorna o número de componentes conexas.
- bipartido(grafo *g): Retorna 1 se grafo é bipartido, 0 caso contrário.

# Entrada esperada
As informações do grafo são obtidas da entrada padrão com o seguinte formato:

```
nomeDoGrafo
A
A -- B
B -- C 5
```
Linhas com apenas um nome definem vértices.
Linhas no formato X -- Y P definem arestas com peso P (inteiro positivo) entre vértices X e Y.

# Estruturas principais
- grafo: estrutura principal com lista de vértices, arestas e informações derivadas.
- vertice: armazena nome, lista de vizinhos vizinhos, e atributos de busca/grafo.
- neighbor: nó de lista ligada possuida por cada vértice, cotém o índice do vizinho na lista de vertices e o peso da aresta ate o mesmo.
- str_list: lista dinâmica de strings usada para agrupar resultados (cortes e diâmetros)

# Estruturas auxiliares:
- queue_t: lista duplamente encadeada para guardar listas temporarias de vertices
- heap_t: min-heap usada como fila de prioridades do algoritmo de Dijkstra
