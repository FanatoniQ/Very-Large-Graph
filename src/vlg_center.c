//
// Created by geoffrey on 08/05/2020.
//

#include "vlg_center.h"
#include "magnien_utils.h"
#include "magnien_queue.h"

/******** GRAPH CENTER functions - begin *********/

/** MOD: ADDED
 ** Return the depth of each node
 ** And compute magnien tree at the same time
 ** magnien_tree: node links with bfs computation
 **/
int *depth_bfs_tree(graph *g, int v, int *max, int **magnien_tree)
{
    int u, i;
    int *depth_tree; // depth tree
    int *tree; // magnien tree
    int curr_depth = 0;
    queue *q;
    q = empty_queue(g->n + 1);
    if( (depth_tree = (int*)malloc((g->n + 1) * sizeof(int))) == NULL )
        report_error("bfs_tree: malloc() error");
    if( (tree=(int *)malloc(g->n*sizeof(int))) == NULL )
        report_error("bfs_tree: malloc() error");
    for (i=0;i<g->n;++i){
        depth_tree[i] = -1;
        tree[i] = -1;
    }
    queue_add(q,v);
    queue_add(q, -1); // -1 special value acts as level seperator
    depth_tree[v] = curr_depth++;
    tree[v] = v;
    while (!is_empty_queue(q)) {
        v = queue_get(q);
        if (v == -1){
            if (is_empty_queue(q))
                break;
            curr_depth++;
            queue_add(q, -1);
            continue;
        }
        for (i=0;i<g->degrees[v];++i) {
            u = g->links[v][i];
            if (depth_tree[u]==-1){
                queue_add(q,u);
                depth_tree[u] = curr_depth;
                tree[u] = v;
            }
        }
    }
    *max = curr_depth - 1;
    free_queue(q);

    if (magnien_tree != NULL)
        *magnien_tree = tree;
    else
        free(tree);
    return(depth_tree);
}

/** MOD: Added in order to get the list of vertices located in the middle level(s) of the bfs tree
 ** TODO: depth_bfs_tree could return the by level vertices list for quicker computation
 **/
int* compute_central_vertices(graph *g, int start, int *resulting_size, int* next_node, int *diameter, int *diam_upper)
{
    int i = 0;
    int max = -1;
    int *magnien_tree = NULL;
    int *depth_tree = depth_bfs_tree(g, start, &max, &magnien_tree);
    if (max == -1 || max == 0)
        report_error("compute_central_vertices: depth computation error");
    // On calcul le milieu, si nombre impaire il va falloir prendre
    // deux milieux: middle et middle + 1
    int middle = max/2;
    int is_odd = max%2;
    int counter = 0;
    int *middle_nodes;
    *diameter = max;

    for (i = 0; i < g->n; ++i){
        if (depth_tree[i] == middle || (is_odd && depth_tree[i] == middle + 1))
            counter++;
    }
    if (counter == 0) {
        free(depth_tree);
        fprintf(stderr, "compute_central_vertices: no middle vertices could be found !"); // ? report_error
        *resulting_size = 0;
        return NULL;
    }
    *next_node = random_node_depthtree(depth_tree, g->n, max); // next node to use for multisweep
    *diam_upper = tree_max_dist(magnien_tree, g->n);
    free(magnien_tree);

    if ((middle_nodes = (int*) malloc((counter + 1) * sizeof(int))) == NULL)
        report_error("compute_central_vertices: middle_nodes: malloc() error");
    int j = 0;
    for (i = 0 ; i < g->n; ++i){
        if (depth_tree[i] == middle || (is_odd && depth_tree[i] == middle + 1))
            middle_nodes[j++] = i;
    }
    *resulting_size = j;
    free(depth_tree);
    return middle_nodes;
}

/** MOD: Added to compute intersection between two lists
 **/
int *intersection_lists(int *list1, int *list2, int size1, int size2, int *resulting_size)
{
    int final_size = size1 > size2 ? size1 : size2;
    int *new_list;
    if ((new_list = (int*) malloc((final_size + 1) * sizeof(int))) == NULL)
        report_error("intersections_lists: error malloc()");
    int k = 0;
    for (int i = 0; i < size1; ++i){
        for (int j = 0; j < size2; ++j){
            if (list1[i] == list2[j]){
                new_list[k++] = list1[i];
            }
        }
    }
    *resulting_size = k;
    return new_list;
}

void update_histogram(int* histo, int* middle_nodes, int size)
{
    if (histo == NULL || middle_nodes == NULL)
        return;
    for (int i = 0; i < size; ++i)
        histo[middle_nodes[i]]++;
}

int *ratio_histo(int *histo, int size, int *result_size, float ratio_retention)
{
    int max = find_maximum(histo, size);
    float ratio;
    int counter = 0;
    for (int i = 0; i < size; ++i){
        ratio = (float)histo[i] / (float)max;
        if (ratio > ratio_retention) // ratio of 80% is nice
            counter++;
    }

    int *nodes_result;
    if ((nodes_result = malloc((counter+1) * sizeof(int))) == NULL){
        report_error("ratio histo: error malloc nodes_result");
        return NULL;
    }

    int hh = 0;
    for (int i = 0; i < size; ++i){
        ratio = (float)histo[i] / (float)max;
        if (ratio > ratio_retention){
            nodes_result[hh++] = i;
        }
    }
    *result_size = counter;
    return nodes_result;
}
/**
 * MOD: return random maximum eccentricity node
 **/
int random_node_depthtree(int *tree, int size, int max)
{
    int counter = 0;
    for (int i = 0; i < size; ++i){
        if (tree[i] == max)
            counter++;
    }

    int index_ref = random()%counter;
    for (int i = 0; i < size; ++i){
        if (tree[i] == max){
            if (index_ref <= 0)
                return i;
            else
                index_ref--;
        }
    }
    report_error("random_node_depthtree: couldn't find a random index");
    return -1;
}

int get_multisweep_node(graph *g, int start, int *max_ecc)
{
    int max = 0;
    int *tree = depth_bfs_tree(g, start, &max, NULL);
    if (max == -1 || max == 0){
        report_error("get_multisweep_node: depth computation error");
        return -1;
    }
    int job_node = random_node_depthtree(tree, g->n, max);
    free(tree); // No need of the first initial depth tree anymore
    *max_ecc = max;
    return job_node;
}

/** MOD: Added
 ** Steps:
 ** - depth_bfs_tree() gets the farthest points
 ** - Do bfs from these nodes
 ** - Store the bfs somehow (tree or list) (depth list)
 ** - We get a center approximation and a rayon approximation
 ** - (First version TODO) make intersection between lists found
 ** TODO: entire graph loop costs (same with center rayon's comment):
 ** depth_bfs_tree could return the by level vertices list for quicker computation
 **/
void calculate_center(graph *g, int start, int num_iterations)
{
    // MultiSweep technique
    int copy_node = 0;
    int middle_nodes_size = 0;
    int *middle_nodes = NULL;
    int temp_middle_size = 0;
    int *temp_middle_nodes = NULL;
    int max_dist = 0;
    int *histo_center_nodes;
    if ((histo_center_nodes = calloc(g->n + 1, sizeof(int)))== NULL){
        report_error("calculate_center: malloc error histo");
        return;
    }
    int upper_diam = -1, lower_diam, rayon;
    int temp_upper_diam = 0;
    float cur_rayon_approx;

    fprintf(stderr, "Starting bfs with node %d\n", start);
    int *multisweep_check;
    if ((multisweep_check = calloc(g->n + 1, sizeof(int)))== NULL){
        report_error("calculate_center: multisweep_check: error malloc()");
    }
    int counter_tries = 0;
    int counter_limit = 100; // FIXME use a parameter instead

    int *results = NULL;
    int job_node = get_multisweep_node(g, start, &max_dist);
    cur_rayon_approx = max_dist; // this is a not diametral vertice we do not /= 2
    lower_diam = max_dist;

    if (job_node == -1){ // error case
        report_error("calculate_center: get_multisweep_node: error finding node");
        return;
    }

    printf("#1:i=iteration_number #2:best_lower_diam_bound #3:best_upper_diam_bound #4:best_rayon #5:current_bfs_rayon_approx\n");
    for (int i = 0; i < num_iterations; ++i) {
        // !!! FIXME: some nodes aren't diametral
        fprintf(stderr, "Processing bfs with node %d\n", job_node);
        multisweep_check[job_node] = 1; // set to already done
        temp_middle_nodes = compute_central_vertices(g, job_node, &temp_middle_size, &job_node, &max_dist, &temp_upper_diam);
        if (temp_middle_nodes == NULL)
            return; // non allocated array/ error happened

/*
        if (max > upper_diam || upper_diam == -1)
            upper_diam = 2*max; // ecc(u) <= D(G) <= 2ecc(u)
        else if (max > lower_diam || lower_diam == 0)
            lower_diam = max;
*/

        lower_diam = max(lower_diam, max_dist);
        // a la toute fin on fait un BFS à partir
        // we do not compute a BFS from this iteration since it is costly
        // an approximation is current BFS rayon:
        // !!! FIXME division avec reste -> cur_rayon_approx est float pour fix
        cur_rayon_approx = min(cur_rayon_approx, max_dist/2);
        // on stocke dans cur_rayon_approx le rayon du BFS courant
        
        // TODO: paramètre pour une meilleure approximation du rayon
        // (attention *2 plus de BFS)
        // maybe add a parameter, parcours des nouveaux centres pour
        // maj de la variable contenant le meilleur centre (nouvelle variable),
        // pour ensuite effectuer un BFS a partir de celui ci

        // upper_diam = min(upper_diam, 2ecc(x))
        // !!! TODO: better upper bound approximation inspiring ourselves from -diam
        // upper_diam = 4*cur_rayon_approx; // previous upper diam computation
        if (upper_diam == -1)
            upper_diam = temp_upper_diam;
        if (temp_upper_diam < upper_diam)
            upper_diam = temp_upper_diam;
        // taking the min, since slight chance cur_rayon_approx will be better
        rayon = min(rayon,(upper_diam + lower_diam) / 4); // FIXME? divison by 4
        // use at the end cur_rayon_approx to provide maybe a better approximation of the rayon ?
        rayon = min(rayon, cur_rayon_approx); // (diametral node eccentricity)/2 can be a better rayon

        update_histogram(histo_center_nodes, temp_middle_nodes, temp_middle_size);
        free(temp_middle_nodes);
        fprintf(stdout, "%dth iteration %d %d %d %0.2f\n", i, lower_diam, upper_diam, rayon, cur_rayon_approx);

        // check if node has already been used, if so, pick a new node
        copy_node = job_node;
        while (multisweep_check[job_node]){
            job_node = random()%g->n;
            counter_tries++;
            if (counter_tries >= counter_limit){
                memset(multisweep_check, 0, g->n * sizeof(*multisweep_check)); // reset on too many iterations
                counter_tries = 0;
            }
        }
        if (copy_node != job_node){ // check if loop changed the current node, if so, bfs needed
            job_node = get_multisweep_node(g, job_node, &max_dist);
            if (job_node == -1){ // error case
                report_error("calculate_center: get_multisweep_node: error finding node");
                return;
            }
        }

    }
    fprintf(stdout, "Center nodes found:\n");
    middle_nodes = ratio_histo(histo_center_nodes, g->n, &middle_nodes_size, 0.8); // FIXME: remove hardcoded and use parameter
    for (int i = 0; i < middle_nodes_size; ++i)
        fprintf(stdout, "%d ", middle_nodes[i]);
    fprintf(stdout, "\n");
    fprintf(stdout, "%d BFS done\n", num_iterations + 1);
    fprintf(stdout, "Final values: %d %d %d\n", lower_diam, upper_diam, rayon);
    free(middle_nodes);
    free(multisweep_check);
}

/******** GRAPH CENTER functions - end *********/