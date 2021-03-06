/* Clemence Magnien and Matthieu Latapy */
/* September 2007 */
/* http://www-rp.lip6.fr/~magnien/Diameter */
/* clemence.magnien@lip6.fr */

#include "magnien_prelim.h"
#include "magnien_utils.h"
#include "magnien_graph_utils.h"
#include "magnien_graph_component.h"
#include "magnien_distances.h"
#include "vlg_save.h"
#include "vlg_center.h"

#include "vlg_tests.h"

/* Output functions */

void usage(char *c){
  fprintf(stderr,"Usage: %s -diam nb_max difference\n",c);
  fprintf(stderr,"Usage: %s -center nb_iteration check_centers\n",c); // MOD: center bruteforce calculation option
  fprintf(stderr,"Usage: %s -centerconv nb_iteration\n",c); // MOD: center convergence calculation option
  fprintf(stderr,"Usage: %s -prec nb_max precision\n",c);
  fprintf(stderr,"Usage: %s -tlb|dslb|tub|rtub|hdtub nb [deg_begin]\n",c);
  fprintf(stderr, "\n");
  fprintf(stderr," -diam nb_max difference: compute bounds for the diameter until the difference between the best bounds is at most 'difference', or until nb_max iterations have been done.\n");
  fprintf(stderr," -prec nb_max precision: compute bounds for the diameter until it is evaluated with a relative error of at most 'precision', or until nb_max iterations have been done.\n");
  fprintf(stderr," -savegiant fpath: saves the giant component to the specified folder.\n"); // MOD: Added this option to save the giant component
  fprintf(stderr," -savegiantbfs fpath: saves the giant component to the specified folder using BFS reordering.\n"); // MOD: Added this option to save the giant component (BFS method)
  fprintf(stderr," -center nb_iteration check_centers: compute the best graph centers/radius/diameter candidates with a bruteforce multisweep method.\n"); // MOD: Added this option to compute the center (BFS probability method)
  fprintf(stderr," check_centers>0 means performing a bfs at the end on all found centers and approximating better bounds and radius, if ==0 then it will be performed only on a random one\n");
  fprintf(stderr," -centerconv nb_iteration: compute the best graph centers/radius/diameter candidates with a convergence of leafs method.\n"); // MOD: Added this option to compute the center (BFS intersection method)
  fprintf(stderr, "\n");
  fprintf(stderr, " -tlb nb: computes trivial lower bounds, from nb randomly chosen nodes.\n");
  fprintf(stderr," -dslb nb: computes double-sweep lower bounds, from nb randomly chosen nodes.\n");
  fprintf(stderr, " -tub nb: computes trivial upper bounds, from nb randomly chosen nodes.\n");
  fprintf(stderr, " -rtub nb: computes random tree upper bounds, from nb randomly chosen nodes.\n");
  fprintf(stderr," -hdtub nb deg_begin: computes highest degree tree upper bounds, from nb nodes starting from first node with degree lesser than or equal to deg_begin. 0 means start from the highest degree node.\n");
  exit(-1);
}

/* MAIN */
int main(int argc, char **argv){
  graph *g;
  int i;
  int *sorted_nodes, *dist;
  int tlb, diam, rtub, dslb, tub,  hdtub, nb_max, prec_option, 
  savegiant, savegiantbfs, center, centerconv; // MOD
  char *savegiant_path = NULL; // MOD
  int deg_begin=0;
  float precision;
  int *c, *c_s, nb_c, c_giant, size_giant;
  int num_iteration, check_centers;


  srandom(time(NULL));

  /* parse the command line */
  tlb=0; diam=0; prec_option=0; dslb=0; tub=0; rtub=0;
  hdtub=0, savegiant=0, savegiantbfs=0, center=0, centerconv=0; // MOD
  for (i=1; i<argc; i++){
    if (strcmp(argv[i],"-tlb")==0) {
      tlb = 1;
      if( i == argc-1 )
 	usage(argv[0]); 
      nb_max = atoi(argv[++i]);
    }
    else if (strcmp(argv[i],"-savegiant")==0) { // MOD: Added option
      savegiant = 1;
      if( i == argc-1 )
 	usage(argv[0]); 
      savegiant_path = argv[++i];
    }
    else if (strcmp(argv[i],"-savegiantbfs")==0) { // MOD: Added option
      savegiantbfs = 1;
      if( i == argc-1 )
 	usage(argv[0]); 
      savegiant_path = argv[++i];
    }
    else if (strcmp(argv[i],"-center")==0) { // MOD: Added option
      center = 1;
      if ((i==argc-2) || (i==argc-1))
        usage(argv[0]);
      num_iteration = atoi(argv[++i]);
      check_centers = atoi(argv[++i]);
    }
    else if (strcmp(argv[i],"-centerconv")==0) { // MOD: Added option
      centerconv = 1;
      if (i == argc - 1)
        usage(argv[0]);
      num_iteration = atoi(argv[++i]);
    }
    else if (strcmp(argv[i],"-diam")==0){
      diam = 1;
      if ((i==argc-2) || (i==argc-1))
	usage(argv[0]);
      nb_max = atoi(argv[++i]);
      precision = atof(argv[++i]);
    }
    else if (strcmp(argv[i],"-prec")==0){
      prec_option = 1;
      if ((i==argc-2) || (i==argc-1))
	usage(argv[0]);
      nb_max = atoi(argv[++i]);
      precision = atof(argv[++i]);
    }
    else if (strcmp(argv[i],"-hdtub")==0){
      hdtub = 1;
      if ((i==argc-2) || (i==argc-1))
	usage(argv[0]);
      nb_max = atoi(argv[++i]);
      deg_begin = atoi(argv[++i]);
    }
    else if( strcmp(argv[i], "-dslb") == 0 ){
      dslb=1;
      if( (i==argc-1) )
	usage(argv[0]);
      nb_max=atoi(argv[++i]);
    }
    else if( strcmp(argv[i], "-tub") == 0 ){
      tub=1;
      if( (i==argc-1) )
	usage(argv[0]);
      nb_max=atoi(argv[++i]);
    }
    else if( strcmp(argv[i], "-rtub") == 0 ){
      rtub=1;
      if( (i==argc-1) )
	usage(argv[0]);
      nb_max=atoi(argv[++i]);
    }
    else
      usage(argv[0]);
  }
  if (tlb+diam+prec_option+rtub+dslb+tub+hdtub+savegiant+savegiantbfs+center+centerconv != 1){ // MOD
    usage(argv[0]);
  }
  
  fprintf(stderr,"Preprocessing the graph...\n");
  fprintf(stderr," reading...\n");
  g = graph_from_file(stdin);
  //fprintf(stderr," random renumbering...\n"); // MOD
  //random_renumbering(g); // MOD
  fprintf(stderr," %d nodes, %d links.\n",g->n,g->m);
  fflush(stderr);

  /* compute connected components */
  fprintf(stderr," computing connected components...\n");
  fflush(stderr);
  if( (c=(int *)malloc(g->n*sizeof(int))) == NULL )
    report_error("main: malloc() error");
  if( (c_s=(int *)malloc(g->n*sizeof(int))) == NULL )
    report_error("main: malloc() error");
  for (i=0;i<g->n;i++)
    c[i] = c_s[i] = -1;
  nb_c = connected_components(g,c,c_s);
  c_giant = giant(c_s,nb_c);
  size_giant = c_s[c_giant];
  fprintf(stderr," %d components; giant component: %d nodes\n",nb_c,size_giant);
  fflush(stderr);

  if( (dist=(int *)malloc(g->n*sizeof(int))) == NULL )
    report_error("main, dist: malloc() error");
  
  /* trivial lower bound */
  if(tlb) {
    int v,  step=0;
    int new_lower;
    printf("#1:iteration_number #2:i-th_node #3:degree_of_ith_node #4:i-th_lower_bound\n");
    while ( step < nb_max ) {
      /* choose v randomly in the giant component */
      v = random()%g->n;
      while (c[v] != c_giant)
	v = random()%g->n;
      /* lower bound */
      for (i=0;i<g->n;i++)
	dist[i] = -1;
      distances(g,v,dist);
      new_lower = max_in_array(dist,g->n);
      printf("%d %d %d %d\n",step++,v, g->degrees[v], new_lower);
      fflush(stdout);
    }
  } else if (savegiant || savegiantbfs) {
    clock_t begin, end;
    double elapsed;
    printf("%s\n", "Saving graph...");
    begin = clock();
    if (savegiant)
      save_giant(g, c, c_giant, size_giant, savegiant_path);
    else
      save_giant_bfs(g, c, c_giant, size_giant, savegiant_path);
    end = clock();
    elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Saved in %f seconds\n", elapsed);
  }
  /* Center random calculation with radius and diameter */
  else if (center) // MOD: Added
  {
    // Current results heavilly depend on random starting points
    clock_t begin, end;
    double elapsed;
    printf("%s\n", "Computing graph center approximation (alongside rayon and diameter)...");
    begin = clock();
    int v = random()%g->n;
    while (c[v] != c_giant)
      v = random()%g->n;
    calculate_center(g, v, num_iteration, c, c_giant, check_centers!=0);
    fflush(stdout);
    end = clock();
    elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("approximated in %f seconds\n", elapsed);
  }
  else if (centerconv) // MOD: Added Center computation convergence
  {
    clock_t begin, end;
    double elapsed;
    printf("%s\n", "Computing graph center approximation (alongside rayon and diameter)...");
    begin = clock();
    compute_center_convergence(g, num_iteration, c, c_giant);
    fflush(stdout);
    end = clock();
    elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("approximated in %f seconds\n", elapsed);
  }
  /* double-sweep lower bound and highest degree tree upper bound for the diameter */
  else if (diam) {
    int v, upper_step=0, step=0;
    int lower_bound = 0, new_lower;
    int upper_node, new_upper, upper_bound = g->n, *t;
    sorted_nodes = sort_nodes_by_degrees(g);
    printf("#1:i=iteration_number #2:i-th_lower_node #3:i-th_upper_node #4:degree_of_i-th_lower_node #5:degree_of_i-th_upper_node #6:i-th_lower_bound #7:i-th_upper_bound #8:i-th_best_lower_bound #9:i-th_best_upper_bound\n");
    while ((step<nb_max) && (upper_bound-lower_bound > precision)) {
      /* choose v randomly in the giant component */
      v = random()%g->n;
      while (c[v] != c_giant)
	v = random()%g->n;
      /* lower_bound */
      for (i=0;i<g->n;i++)
	dist[i] = -1;
      distances(g,v,dist);
      new_lower = max_distance(g,rand_index_max_array(dist,g->n));
      if (new_lower > lower_bound)
	lower_bound = new_lower;
      /* highest degree tree upper bound */
      upper_node = sorted_nodes[upper_step++];
      while ((upper_step<g->n) && (c[upper_node] != c_giant))
	upper_node = sorted_nodes[upper_step++];
      if (upper_step==g->n)
	report_error("main: too many iterations");
      t = bfs_tree(g,upper_node);
      new_upper = tree_max_dist(t,g->n);
      if (new_upper < upper_bound)
	upper_bound = new_upper;
      printf("%d %d %d %d %d %d %d %d %d\n",step++,v,upper_node, g->degrees[v], g->degrees[upper_node], new_lower, new_upper, lower_bound, upper_bound);
      fflush(stdout);
      free(t);
    }
    free(sorted_nodes);
  }

  else if (prec_option) {
    int v, upper_step=0, step=0;
    int lower_bound = 0, new_lower;
    int upper_node, new_upper, upper_bound = g->n, *t;
    sorted_nodes = sort_nodes_by_degrees(g);

    printf("#1:i=iteration_number #2:i-th_lower_node #3:i-th_upper_node #4:degree_of_i-th_lower_node #5:degree_of_i-th_upper_node #6:i-th_lower_bound #7:i-th_upper_bound #8:i-th_best_lower_bound #9:i-th_best_upper_bound\n");
    while ( (step<nb_max) && ( (float)(upper_bound-lower_bound)/lower_bound > precision) ) {
      /* choose v randomly in the giant component */
      v = random()%g->n;
      while (c[v] != c_giant)
	v = random()%g->n;
      /* lower_bound */
      for (i=0;i<g->n;i++)
	dist[i] = -1;
      distances(g,v,dist);
      new_lower = max_distance(g,rand_index_max_array(dist,g->n));
      if (new_lower > lower_bound)
	lower_bound = new_lower;
      /* highest degree tree upper bound */
      upper_node = sorted_nodes[upper_step++];
      while ((upper_step<g->n) && (c[upper_node] != c_giant))
	upper_node = sorted_nodes[upper_step++];
      if (upper_step==g->n)
	report_error("main: too many iterations");
      t = bfs_tree(g,upper_node);
      new_upper = tree_max_dist(t,g->n);
      if (new_upper < upper_bound)
	upper_bound = new_upper;
      printf("%d %d %d %d %d %d %d %d %d\n",step++,v,upper_node, g->degrees[v], g->degrees[upper_node], new_lower, new_upper, lower_bound, upper_bound);
      fflush(stdout);
      free(t);
    }
    free(sorted_nodes);
  }

  /* highest degree tree upper bound */
  else if (hdtub) {
    int upper_step=0, step=0;
    int upper_node, new_upper, *t;
    sorted_nodes = sort_nodes_by_degrees(g);
    printf("#1:i=iteration_number #2:i-th_node #3:degree_of_i-th_node #4:i-th_upper_bound\n");
    /* Switch to first node with degree lesser than or equal to deg_begin */
    if( deg_begin != 0 ){
      upper_node = sorted_nodes[upper_step];
      while( g->degrees[upper_node] > deg_begin ){
	upper_step++;
	upper_node=sorted_nodes[upper_step];
      }
    }
    while ( step < nb_max ) {
      /* upper bound */
      upper_node = sorted_nodes[upper_step++];
      while ((upper_step<g->n) && (c[upper_node] != c_giant))
	upper_node = sorted_nodes[upper_step++];
      if (upper_step==g->n+1)
	report_error("main: too many iterations");
      t = bfs_tree(g,upper_node);
      new_upper = tree_max_dist(t,g->n);
      printf("%d %d %d %d\n",step++,upper_node, g->degrees[upper_node], new_upper);
      fflush(stdout);
      free(t);
    }
    free(sorted_nodes);
  }

  /* double-sweep lower bound */
  if(dslb) {
    int v,  step=0;
    int new_lower;
    printf("#1:iteration_number #2:i-th_node #3:degree_of_ith_node #4:i-th_lower_bound\n");
    while ( step < nb_max ) {
      /* choose v randomly in the giant component */
      v = random()%g->n;
      while (c[v] != c_giant)
	v = random()%g->n;
      /* lower bound */
      for (i=0;i<g->n;i++)
	dist[i] = -1;
      distances(g,v,dist);
      new_lower = max_distance(g,rand_index_max_array(dist,g->n));
      printf("%d %d %d %d\n",step++,v, g->degrees[v], new_lower);
      fflush(stdout);
    }
  }

  /* trivial upper bound */
  else if(tub){
    int step=0;
    int v, new_upper;
    printf("#1:i=iteration_number #2:i-th_node #3:degree_of_i-th_node #4:i-th_upper_bound\n");
    while ( step < nb_max ){
      /* choose v randomly in the giant component */
      v = random()%g->n;
      while (c[v] != c_giant)
	v = random()%g->n;
      /* upper bound */
      for (i=0;i<g->n;i++)
	dist[i] = -1;
      distances(g,v,dist);
      new_upper = 2*max_in_array(dist,g->n);
      printf("%d %d %d %d\n",step++, v, g->degrees[v], new_upper);
      fflush(stdout);
    }
  }

  /* random tree upper bound */
  else if(rtub){
    int step=0;
    int upper_node, new_upper, *t;
    sorted_nodes = sort_nodes_by_degrees(g);
    printf("#1:i=iteration_number #2:i-th_node #3:degree_of_i-th_node #4:i-th_upper_bound\n");
    while ( step < nb_max ){
      /* upper bound, from a randomly chosen node */
      upper_node = random()%g->n;
      while (c[upper_node] != c_giant)
	upper_node = random()%g->n;
      /* upper bound */
      t = bfs_tree(g,upper_node);
      new_upper = tree_max_dist(t,g->n);
      printf("%d %d %d %d\n",step++,upper_node, g->degrees[upper_node], new_upper);
      fflush(stdout);
      free(t);
    }
    free(sorted_nodes);
  }
  
  /* cannot be used because of renumbering... */
  free_graph(g); // MOD no renumbering so can be used
  free(dist);
  free(c);
  free(c_s);
  return(0);
}