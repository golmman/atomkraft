/*
 * pgn.h
 *
 *  Created on: 17.08.2011
 *      Author: golmman
 */

#ifndef PGN_H_
#define PGN_H_

#include "types.h"
#include "move.h"

enum ResultPGN {
	WHITE_WINS, BLACK_WINS, DRAW, NO_RESULT
};

void appendPGN(
	char* filename, 
	char* name_white, 
	char* name_black, 
	ResultPGN result,
	int time, int inc,
	char* fen, Move* moves, int move_count);

#endif /* PGN_H_ */
