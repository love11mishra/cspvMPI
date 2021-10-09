// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "run_crest/concolic_search.h"

int main(int argc, char* argv[]) {
  //assert(argc >= 5);
  if(argc < 5){
    printf("Usage: <crest_dir/bin>/run_crest <num_procs> .\/<target_prog> <num_iterations> <search_strategy>\n");
    exit(0);
  }
  int num_procs = atoi(argv[1]);
  string prog = argv[2];
  int num_iters = atoi(argv[3]);
  string search_type = argv[4];

  // Initialize the random number generator.
#if 1
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  srand(ts.tv_nsec);
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  srand((tv.tv_sec << 20) + tv.tv_usec);
#endif

  crest::Search* strategy;
  if (search_type == "-random") {
    strategy = new crest::RandomSearch(num_procs, prog, num_iters);
  } else if (search_type == "-random_input") {
    strategy = new crest::RandomInputSearch(num_procs, prog, num_iters);
  } else if (search_type == "-dfs") {
    if (argc == 5) {
      strategy = new crest::BoundedDepthFirstSearch(num_procs, prog, num_iters, 1000000);
    } else {
      strategy = new crest::BoundedDepthFirstSearch(num_procs, prog, num_iters, atoi(argv[5]));
    }
  } else if (search_type == "-cfg") {
    strategy = new crest::CfgHeuristicSearch(num_procs, prog, num_iters);
  } else if (search_type == "-cfg_baseline") {
    strategy = new crest::CfgBaselineSearch(num_procs, prog, num_iters);
  } else if (search_type == "-hybrid") {
    strategy = new crest::HybridSearch(num_procs, prog, num_iters, 100);
  } else if (search_type == "-uniform_random") {
    if (argc == 5) {
      strategy = new crest::UniformRandomSearch(num_procs, prog, num_iters, 100000000);
    } else {
      strategy = new crest::UniformRandomSearch(num_procs, prog, num_iters, atoi(argv[5]));
    }
  } else {
    fprintf(stderr, "Unknown search strategy: %s\n", search_type.c_str());
  }

  strategy->Run();

  delete strategy;
  return 0;
}


