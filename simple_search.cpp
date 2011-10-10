/*
 * test_search.cpp
 *
 *  Created on: 26.07.2011
 *      Author: golmman
 */

#include "simple_search.h"


Move test_bestmove(Position& pos, int maxdepth) {
	Move bestmove = MOVE_NONE;
	

	
	
	
	MoveStack mlist[MAX_MOVES];
	StateInfo st;
	Move m;

	// Generate all legal moves
	MoveStack* last = generate<MV_LEGAL>(pos, mlist);
	
	
	for (int it_depth = 1; it_depth <= maxdepth; ++it_depth) {
	
		bool bSearchPv = true;
		int alpha = -100000;
		int beta  =  100000;
		int score = alpha;
		
		for (MoveStack* cur = mlist; cur != last; cur++) {
			m = cur->move;
			pos.do_move(m, st);
			if (bSearchPv) {
				score = -test_search(pos, -beta, -alpha, it_depth - 1);
			} else {
				score = -test_search(pos, -alpha-1, -alpha, it_depth - 1);
				if (score > alpha) // in fail-soft ... && score < beta ) is common
					score = -test_search(pos, -beta, -alpha, it_depth - 1); // re-search
			}
			pos.undo_move(m);
			
			if(score > alpha) {
				alpha = score; // alpha acts like max in MiniMax
				bestmove = m;
			}
			bSearchPv = false;
		}
		
		
		cout << it_depth << " " << move_to_uci(bestmove, 0) << " " << score << endl;
		
	}
	
	
	return bestmove;
}


int test_search(Position &pos, int alpha, int beta, int depth) {
	
	if(depth == 0) return test_eval(pos);
	bool bSearchPv = true;
	int score;
	
	MoveStack mlist[MAX_MOVES];
	StateInfo st;
	Move m;

	// Generate all legal moves
	MoveStack* last = generate<MV_LEGAL>(pos, mlist);
	
	if (mlist == last) {
		return -100000;
	}
	
	for (MoveStack* cur = mlist; cur != last; cur++) {
		m = cur->move;
		pos.do_move(m, st);
		if (bSearchPv) {
			score = -test_search(pos, -beta, -alpha, depth - 1);
		} else {
			score = -test_search(pos, -alpha-1, -alpha, depth - 1);
			if (score > alpha) // in fail-soft ... && score < beta ) is common
				score = -test_search(pos, -beta, -alpha, depth - 1); // re-search
		}
		pos.undo_move(m);
		if(score >= beta)
			return beta;   // fail-hard beta-cutoff
		if(score > alpha) {
			alpha = score; // alpha acts like max in MiniMax
		}
		bSearchPv = false;
	}
	
	
	
	return alpha; // fail-hard
}

int test_eval(const Position& pos) {
	Color us = pos.side_to_move();
	Color them = opposite_color(us);
	return 
			pos.piece_count(us, PAWN) 		* 100 +
			pos.piece_count(us, KNIGHT) 	* 150 +
			pos.piece_count(us, BISHOP) 	* 150 +
			pos.piece_count(us, ROOK) 		* 300 +
			pos.piece_count(us, QUEEN) 		* 600 -
			pos.piece_count(them, PAWN) 	* 100 -
			pos.piece_count(them, KNIGHT) 	* 150 -
			pos.piece_count(them, BISHOP) 	* 150 -
			pos.piece_count(them, ROOK) 	* 300 -
			pos.piece_count(them, QUEEN) 	* 600;
}











