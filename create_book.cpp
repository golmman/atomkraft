/*
 * simple_book.cpp
 *
 *  Created on: 10.08.2011
 *      Author: golmman
 */




#include "create_book.h"
#include "search.h"
#include "movegen.h"
#include "book.h"
#include "debug.h"
#include <list>
#include <queue>
#include <cctype>

using namespace std;


std::vector<MoveStack> book_moves_aux;
std::vector<MoveStack> book_moves;


Move searchMoves[MAX_MOVES];


void findChildren(MoveTreeNode* node, Position& pos, int* index) {
	searchMoves[0] = MOVE_NONE;

	SearchLimits limits;
	limits.maxTime = MAX_SEARCH_TIME;
    limits.increment = 0;

    Move bestmove, pondermove;

	think(pos, limits, searchMoves, bestmove, pondermove);
	insertion_sort<MoveStack, base::iterator>(book_moves.begin(), book_moves.end());

	for (base::iterator it = book_moves.begin(); it != book_moves.end(); ++it) {

		node->child[node->child_count] = new MoveTreeNode();
		node->child[node->child_count]->parent = node;
		node->child[node->child_count]->child_count = 0;
		node->child[node->child_count]->ms.move = it->move;
		node->child[node->child_count]->ms.score = it->score;
		node->child[node->child_count]->index = *index;

		cout << "  " << move_to_string(node->child[node->child_count]->ms.move) << " "
			 << node->child[node->child_count]->ms.score << endl;

		++(*index);
		++node->child_count;
		if (node->child_count >= MAX_BOOK_BRANCHING) {
			break;
		}
	}
}


void killChildren(MoveTreeNode* node) {
	for (int k = 0; k < node->child_count; ++k) {
		killChildren(node->child[k]);
		delete node->child[k];
	}
}




File getFile(char c) {
	return (c >= 'a' && c <= 'h') ? File(c - 'a') : FILE_NONE;
}

Rank getRank(char c) {
	return (c >= '1' && c <= '7') ? Rank(c - '1') : RANK_NONE;
}

PieceType getPiece(char c) {
	switch (c) {
	case 'N': return KNIGHT;
	case 'B': return BISHOP;
	case 'R': return ROOK;
	case 'Q': return QUEEN;
	case 'K': return KING;
	default:
		return PIECE_TYPE_NONE;
	}
}


/*
 * converts san string to a Move
 * Note that it doesn't work for all san strings, but for those used in the eao
 */
Move san_to_move(Position& pos, char* s) {
	char str[10];
	strcpy(str, s);

	// generate legal moves
	MoveStack mlist[MAX_MOVES];
	MoveStack* last = generate<MV_LEGAL>(pos, mlist);


	// get rid of 'x' chars
	char* x = strchr(str, 'x');
	if (x) while (*x) *x++ = *(x+1);


	// get piece type
	int p = 1;
	PieceType pt = getPiece(str[0]);
	if (pt == PIECE_TYPE_NONE) {
		pt = PAWN;
		p = 0;
	}

	// there may be additional info regarding the file/rank
	File fromFile = getFile(str[p+0]);
	Rank fromRank = getRank(str[p+0]);
	Square to;

	// TODO: san can be like Bf3e4

	if (fromRank != RANK_NONE || getFile(str[p+1]) != FILE_NONE) {
		// san begins with a number e.g. B8d6 or with two letters e.g. Bfd6
		to = make_square(File(str[p+1] - 'a'), Rank(str[p+2] - '1'));
	} else {
		// san is like Bd6
		to = make_square(File(str[p+0] - 'a'), Rank(str[p+1] - '1'));
		fromFile = FILE_NONE;
	}


	// traverse legal moves and pick the right one
	Move m = MOVE_NONE;
	int possible_move_count = 0; // used to detect errors

	for (MoveStack* cur = mlist; cur != last; cur++) {
		Square sqfrom = move_from(cur->move);
		Square sqto = move_to(cur->move);

		if (strcmp(str, "O-O") == 0) {
			if (move_is_short_castle(cur->move)) {
				m = cur->move;
				possible_move_count = 1;
				break;
			}
		} else if (strcmp(str, "O-O-O") == 0) {
			if (move_is_long_castle(cur->move)) {
				m = cur->move;
				possible_move_count = 1;
				break;
			}
		} else {

			if (sqto == to && pt == pos.type_of_piece_on(sqfrom)) {
				if (fromFile != FILE_NONE) {
					if (square_file(sqfrom) == fromFile) {
						m = cur->move;
						++possible_move_count;
						//break;
					}
				} else if (fromRank != RANK_NONE) {
					if (square_rank(sqfrom) == fromRank) {
						m = cur->move;
						++possible_move_count;
						//break;
					}
				} else {
					m = cur->move;
					++possible_move_count;
					//break;
				}
			}
		}
	}


	if (possible_move_count != 1) {
		m = MOVE_NONE;
		cout << "incorrect SAN! (it may be ambiguous )" << endl;
	}


	return m;
}



void printTree(MoveTreeNode* node) {

	if (node->child_count == 0) {
		cout << endl;
		return;
	}

	// print first move
	cout << move_to_string(node->child[0]->ms.move) << " ";
	printTree(node->child[0]);

	// print following moves
	for (int k = 1; k < node->child_count; ++k) {
		// indentation
		for (int s = 0; s < node->depth; ++s) {
			cout << "     ";
		}

		// print moves
		cout << move_to_string(node->child[k]->ms.move) << " ";
		printTree(node->child[k]);
	}
}

int numberOfNodes(MoveTreeNode* node) {

	int sum = 0;

	for (int k = 0; k < node->child_count; ++k) {
		sum += numberOfNodes(node->child[k]);
	}

	// node->ms.score was set true or false, false means not playable
	if (node->ms.score) {
		node->ms.score = 1;//sum + 1;	// a move with score 0 will never be played
	}
	return sum + 1;
}



template<typename T>
void write_number(FILE* file, T& n) {

	for (size_t i = 0; i < sizeof(T); i++) {
		uint8_t c = uint8_t((n >> ((sizeof(T) - 1) * 8)));
		n <<= 8;
		fputc(c, file);
	}

}

void write_entry(FILE* file, uint64_t key, Move move, uint16_t count) {
	// convert Move to polyglot move
	uint16_t m = make_move(move_from(move), move_to(move)) | (move_promotion_piece(move) << 12);
	uint32_t learn = 0;

	write_number(file, key);
	write_number(file, m);
	write_number(file, count);
	write_number(file, learn);


	if (ferror(file)) {
		cerr << "Failed to write book." << endl;
		exit(EXIT_FAILURE);
	}

//	bookFile.flush();
//
//	if (!bookFile.good()) {
//		cerr << "Failed to flush book." << endl;
//		exit(EXIT_FAILURE);
//	}
}


bool compare_key(BookEntry* be1, BookEntry* be2) {
	return be1->key < be2->key;
}


/*
 * creates an opening book from the formatted EAO
 * http://www.nicklong.net/chess/atomic/EAO.txt
 */

const int STATEINFO_COUNT = 500;
const int LINE_COUNT = 2000;
const int MAX_LINE_LENGTH = 500;
const int BOOK_MAX_PLY = 100;

// global since the stack is too small
StateInfo st[STATEINFO_COUNT]; // textpad says eao.txt has 1210 words
struct BookPV {
	Move move;
	bool playable;
} pv[LINE_COUNT][BOOK_MAX_PLY];
char str[LINE_COUNT][MAX_LINE_LENGTH];	// no line is longer than 85 characters in the eao.txt

void createBookEAO() {

	FILE* rf;
	rf = fopen("d:/eao.txt", "r");
	if (rf == NULL) {
		cout << "ERROR: open eao.txt" << endl;
	}

	FILE* wf;
	wf = fopen("d:/eao.bin", "wb");
	if (rf == NULL) {
		cout << "ERROR: open eao.bin" << endl;
	}

	Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0, 0);




	int st_index = 0;

	//Move pv[LINE_COUNT][BOOK_MAX_PLY];	// deepest variation has 22 plies
	int pv_index = 0;


	list<char*> strlist;
	char token[MAX_LINE_LENGTH];
	char* s;	// traverses str
	char* t;	// traverses token

	int lines = 0;
	while (!feof(rf)) {
		// read a line
		fgets(str[lines], MAX_LINE_LENGTH, rf);

		// clear the line of unwanted characters and comments
		char* c = strchr(str[lines], '#');
		if (c) *c = 0;
		c = strchr(str[lines], 10);
		if (c) *c = 0;

		for (c = str[lines] + strlen(str[lines]) - 1; c >= str[lines]; --c) {
			if (isspace(*c) || *c == 0) {
				*c = 0;
			} else {
				break;
			}
		}


		if (*str[lines]) {
			//cout << str[lines] << endl;

			strlist.push_back(str[lines]);
			++lines;
		}
	}


	// 1. clear the file of redundancies
	//    that way we guarantee that the position at the end of every pv is unique
	for (list<char*>::iterator it1 = strlist.begin(); it1 != strlist.end(); ++it1) {
		for (list<char*>::iterator it2 = strlist.begin(); it2 != strlist.end(); ++it2) {
			if (*it1 != *it2) {
				if (strlen(*it2) < strlen (*it1)) {
					if (strncmp(*it2, *it1, strlen(*it2) - 1) == 0) {
						// *it1 starts with of *it2 so delete *it2
						it2 = strlist.erase(it2);
					}
				}
			}
		}
	}

	// 2. fill the pv array
	for (list<char*>::iterator it = strlist.begin(); it != strlist.end(); ++it) {
		cout << *it << endl;

		pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0);

		// parse the string
		s = *it;
		t = token;

		Color color = WHITE;
		int move_count = 0;
		while (*s != 10 && *s != 0) {

			*t++ = *s++;

			if (*s == ' ' || *s == 10 || *s == 0) {
				*t = 0;

				// we have a new token
				bool playable = (*token != '?');

				Move m = san_to_move(pos, playable ? token : token+1);

				//cout << "  " << token << " ";
				if (m == MOVE_NONE) {
					cout << "ERROR: " << token << endl;
					return;
				} else {
					//cout << move_to_string(m) << endl;
				}

				pos.do_move(m, st[st_index]);
				pv[pv_index][move_count].move = m;
				pv[pv_index][move_count].playable = playable;


				++st_index;
				if (st_index >= STATEINFO_COUNT) st_index = 0;
				++move_count;
				color = opposite_color(color);
				t = token;
				if (*s == ' ') ++s;
			}
		}

		// undo all moves of the pv
		for (int k = move_count - 1; k >= 0; --k) {
			pos.undo_move(pv[pv_index][k].move );
		}
		pv[pv_index][move_count].move  = MOVE_NONE;
		++pv_index;
	}

	// 3. create a tree from the pvs
	//    note that it is important that the pvs are already sorted
	MoveTreeNode rootNode;
	MoveTreeNode* node = &rootNode;
	rootNode.ms.move = MOVE_NONE;
	rootNode.child_count = 0;
	rootNode.depth = 0;
	rootNode.parent = 0;
	rootNode.visited = false;

	BookPV* pv_curr;
	BookPV* pv_next;
	int depth = 0;
	int split_depth = 0;

	for (int i = 0; i < pv_index; ++i) {
		pv_curr = pv[i];

		if (i + 1 < pv_index) {
			pv_next = pv[i+1];
		} else {
			pv_next = pv_curr;
		}

		depth = split_depth;

		// correct indentation
		for (int k = 0; k < split_depth; ++k) {
			//cout << "     ";
		}

		while (pv_curr[depth].move != MOVE_NONE) {
			//cout << move_to_string(pv_curr[depth]) << " ";

			// create new node
			node->child[node->child_count] = new MoveTreeNode();
			node->child[node->child_count]->child_count = 0;
			node->child[node->child_count]->depth = depth + 1;
			node->child[node->child_count]->parent = node;
			node->child[node->child_count]->ms.move  = pv_curr[depth].move;
			node->child[node->child_count]->ms.score = pv_curr[depth].playable;
			node->child[node->child_count]->visited = false;
			++node->child_count;

			node = node->child[node->child_count-1];


			++depth;
		}
		//cout << endl;

		// find the split depth
		depth = 0;
		split_depth = 0;
		// the ending position of any pv is unique so the exit condition will never be reached
		while (pv_curr[depth].move != MOVE_NONE) {
			if (pv_curr[depth].move != pv_next[depth].move) {
				split_depth = depth;
				break;
			}
			++depth;
		}

		// move inside the tree such that node->depth == split_depth
		while (node->depth > split_depth) {
			node = node->parent;
		}

		assert(node->depth == split_depth);
	}



	// 4. score the nodes by the number of successors
	//    if ms.score == false holds, the move is considered not playable
	rootNode.ms.score = 1;

	printTree(&rootNode);
	cout << "positions: " << numberOfNodes(&rootNode) << endl;

//	// don't play bad openings as white
//	for (int k = 0; k < rootNode.child_count; ++k) {
//		Square to = move_to(rootNode.child[k]->ms.move);
//		if (   to == SQ_B4 || to == SQ_G3 || to == SQ_B4 || to == SQ_H3
//			|| to == SQ_E4 || to == SQ_A3 || to == SQ_C3 || to == SQ_D4) {
//			rootNode.child[k]->ms.score = 0;
//		}
//	}


	// 5. breadth-first search to write the moves into the book
	list<BookEntry*> book_entries;

	queue<MoveTreeNode*> qu;

	qu.push(&rootNode);
	rootNode.visited = true;

	pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0);
	st_index = 0;
	Move pv_tree[30];
	int pv_len = 0;
	int moves_made = 0;

	while (!qu.empty()) {
		node = qu.front();
		qu.pop();

//		for (int k = 0; k < node->depth; ++k) {
//			cout << "     ";
//		}
//		cout << move_to_string(node->ms.move) << endl;


		// get the pv of our current node in reversed order
		pv_len = 0;
		MoveTreeNode* node_aux = node;
		while (node_aux->parent) {
			pv_tree[pv_len] = node_aux->ms.move;
			++pv_len;
			node_aux = node_aux->parent;
		}

		// do all moves of the pv
		for (int k = pv_len - 1; k >= 0; --k) {
			pos.do_move(pv_tree[k], st[st_index]);
			++st_index;
			if (st_index >= STATEINFO_COUNT) st_index = 0;
			++moves_made;
		}


		// the actual bfs
		for (int k = 0; k < node->child_count; ++k) {
			if(node->child[k]->visited == false) {
				qu.push(node->child[k]);
				node->child[k]->visited = true;

				// write the move to the book
//				write_entry(
//					wf,
//					book_key(pos),
//					node->child[k]->ms.move,
//					node->child[k]->ms.score
//				);
				BookEntry* be = new BookEntry;
				be->key = book_key(pos);
				be->move = uint16_t(node->child[k]->ms.move);
				be->count = node->child[k]->ms.score;
				be->learn = 0;
				book_entries.push_back(be);
			}
		}



		// undo all moves of the pv
		for (int k = 0; k < pv_len; ++k) {
			//cout << ".";
			pos.undo_move(pv_tree[k]);
		}
	}


	cout << moves_made << " moves made" << endl;

	book_entries.sort(compare_key);
	for (list<BookEntry*>::iterator it = book_entries.begin(); it != book_entries.end(); ++it) {
		//cout << (*it)->key << endl;
		write_entry(wf, (*it)->key, Move((*it)->move), (*it)->count);
		delete *it;
	}
	killChildren(&rootNode);

	fclose(rf);
	fclose(wf);

}
















