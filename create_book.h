/*
 * simple_book.h
 *
 *  Created on: 10.08.2011
 *      Author: golmman
 */

#ifndef CREATE_BOOK_H_
#define CREATE_BOOK_H_

#include "position.h"
#include "move.h"
#include <vector>
#include "book.h"
#include "position.h"

extern std::vector<MoveStack> book_moves_aux;
extern std::vector<MoveStack> book_moves;

const int MAX_BOOK_BRANCHING = 20;
const int MAX_BOOK_NODES = 100;
const int MAX_SEARCH_TIME = 3000;

struct MoveTreeNode {
	MoveTreeNode* parent;
	MoveTreeNode* child[MAX_BOOK_BRANCHING];
	int child_count;
	MoveStack ms;
	
	int index;
	int depth;
	bool visited;
};

typedef std::vector<MoveStack> base;


void findChildren(MoveTreeNode* node, Position& pos, int* index);
void killChildren(MoveTreeNode* node);

void createBookEAO();
Move san_to_move(Position&, char*);


#endif /* CREATE_BOOK_H_ */
