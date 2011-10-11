/*
 * main_book.cpp
 *
 *  Created on: 11.10.2011
 *      Author: golmman
 */

#include "main.h"

#include "position.h"
#include "search.h"
#include "ucioption.h"
#include "book.h"
#include "create_book.h"

#include <queue>
#include <iostream>
using namespace std;


void main_book_from_file() {
	createBookEAO();
	system("pause");
}


void main_book_from_thinking() {
	
	//	Position testpos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0, 0);
	//	
	//	Book book;
	//	book.open("d:/atomkraft_book.bin");
	//
	//	cout << book.name() << endl;
	//	
	//	book.write_entry(book_key(testpos), make_move(SQ_G1, SQ_F3), 1);
	//	book.write_entry(book_key(testpos), make_move(SQ_E2, SQ_E4), 2);
	//	
	//	//Sleep(10000);
	//	
	//	cout << move_to_string(book.get_move(testpos, false)) << endl;
	//	
	////	BookEntry entry = book.read_entry(0); cout << endl;
	////	cout << book_key(testpos) << endl;
	////	cout << entry.key << endl;
	//	
	//	
	//	book.close();
	//	
	//	return 0;


	/*
	 * NOTE TO MYSELF:
	 * search.cpp line 619:         
	 * bestMove = Rml[0].pv[0];
	 * *ponderMove = Rml[0].pv[1];
	 * 
	 * 
	 * 
	 */


	Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0, 0);

	Move pv[20];
	int pv_len = 0;


	StateInfo st[MAX_BOOK_NODES];
	int st_counter;


	MoveTreeNode RootNode;
	RootNode.parent = 0;
	RootNode.child_count = 0;
	RootNode.index = 0;


	int nodes_searched = 1;
	cout << "start position" << endl;
	findChildren(&RootNode, pos, &nodes_searched);



	queue<MoveTreeNode*> qu;
	bool visited[MAX_BOOK_NODES];
	for (int i = 0; i < MAX_BOOK_NODES; ++i) visited[i] = false;

	qu.push(&RootNode);
	visited[0] = true;


	// breadth first search
	while(!qu.empty()) {
		MoveTreeNode* node = qu.front();
		qu.pop();

		if(nodes_searched >= MAX_BOOK_NODES) {
			break;
		}

		// get the pv of our current node in reversed order
		pv_len = 0;
		MoveTreeNode* node_aux = node;
		while (node_aux->parent) {
			pv[pv_len] = node_aux->ms.move;
			++pv_len;
			node_aux = node_aux->parent;
		}

		// do all moves of the pv
		for (int k = pv_len - 1; k >= 0; --k) {
			pos.do_move(pv[k], st[st_counter]);
			++st_counter;
		}

		for (int k = 0; k < node->child_count; ++k) {

			if(visited[node->child[k]->index] == false) {

				// output
				cout << endl;
				cout << "searching ";
				for (int j = pv_len - 1; j >= 0; --j) {
					cout << move_to_string(pv[j]) << " "; 
				}
				cout << move_to_string(node->child[k]->ms.move) << " ..." << endl;

				//
				qu.push(node->child[k]);
				visited[node->child[k]->index] = true;

				// only search good moves
				if (node->child[k]->ms.score < 400 && node->child[k]->ms.score > -400) {
					pos.do_move(node->child[k]->ms.move, st[st_counter]); ++st_counter;
					findChildren(node->child[k], pos, &nodes_searched);
					pos.undo_move(node->child[k]->ms.move);
				} else {
					cout << "  score too good/bad: " << node->child[k]->ms.score << endl;
				}

			}
		}


		// undo all moves of the pv
		for (int k = 0; k < pv_len; ++k) {
			pos.undo_move(pv[k]);
		}

	}

	killChildren(&RootNode);
		
	
}
