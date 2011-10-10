/*
 * test_search.h
 *
 *  Created on: 26.07.2011
 *      Author: golmman
 */

#ifndef SIMPLE_SEARCH_H_
#define SIMPLE_SEARCH_H_

#include "movegen.h"
#include "position.h"
#include <iostream>
#include <stdio.h>

using namespace std;

int test_eval(const Position& pos);
int test_search(Position &pos, int alpha, int beta, int depth);
Move test_bestmove(Position &pos, int maxdepth);

#endif /* SIMPLE_SEARCH_H_ */
