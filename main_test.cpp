/*
 * main_test.cpp
 *
 *  Created on: 11.10.2011
 *      Author: golmman
 */



#include "main.h"

#include "position.h"
#include "evaluate.h"
#include "movegen.h"
#include "debug.h"
#include "search.h"
#include "thread.h"
#include "ucioption.h"
#include "book.h"
#include "pgn.h"

#include <iostream>
#include <time.h>
using namespace std;


void main_test() {
	
	const char BOOK_FILE[] = "d:/eao-full.bin";
	
	
	Book book;
	book.open(BOOK_FILE);
	
	
	Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0, 0);
	
	//Position pos("5k2/8/5p2/5Pp1/6Pp/1B5P/8/K7 w - - 0 1", 0, 0);
	//Position pos("6k1/ppp1bN2/8/1N5p/P3p3/2P1P2K/1P6/8 w - - 0 26", 0, 0);
	
	// crash in a game against sordid
	//Position pos("8/6kp/q7/1Np1pp2/3p4/B4PP1/P1PP4/1R3RK1 w - - 0 27", 0, 0);
	
	// sordid saw the mate in 6 seconds...
	//Position pos("r4rk1/4p1b1/2N3p1/p6p/1p1Pn1b1/BP2PP2/P1P3PP/R3K2R b KQ - 0 17", 0, 0);
	
//	CheckInfo ci(pos);
//	print_bb(ci.dcCandidates);
//	cout << pos.move_gives_check(make_move(SQ_A3, SQ_B4)) << endl;
//	return 0;
	
	
	
//	MaterialInfo* mi = Threads[pos.thread()].materialTable.get_material_info(pos);
//	
//	if (mi->specialized_eval_exists()) {
//		mi->evaluate(pos);
//	}
//	
//	return 0;
	


	Options["Hash"] = UCIOption(128, 4, 8192);
	Options["Book File"] = UCIOption(BOOK_FILE);
	Options["OwnBook"] = UCIOption(true);
	Options["Ponder"] = UCIOption(false);
    
	
	char inputc[100];
	
	Move searchMoves[MAX_MOVES];
	searchMoves[0] = MOVE_NONE;
	SearchLimits limits;
    //limits.time = 60000;
	limits.maxTime = 5000;//3600000;
	//limits.maxDepth = 8;
    limits.increment = 0;
    Move bestmove, pondermove;
    
	Move move = MOVE_NONE;
	StateInfo st[1000];
	int counter = 0;
	
	/*
	 * only search first move
	 */
    //pthread_t tt;
    //pthread_create(&tt, 0, testest, 0);
    
    
//    
//    think(pos, limits, searchMoves, bestmove, pondermove);
//    
//    pos.do_move(bestmove, st[0]);
//    if (pondermove != MOVE_NONE) {
//    	pos.do_move(pondermove, st[1]);
//    	
//        pthread_t tt;
//        pthread_create(&tt, 0, testest, 0);
//        
//        limits.ponder = true;
//        
//    	think(pos, limits, searchMoves, bestmove, pondermove);
//    }
//    
//    
//    return 0;
    
	

    
	print_pos(pos);
	cout << "castling: "; print_castleRights(pos);
	int failstep;
	pos.is_ok(&failstep);
	cout << failstep << endl;
	
	MoveStack mlist[MAX_MOVES];
	MoveStack* last = generate<MV_LEGAL>(pos, mlist);
	for (MoveStack* cur = mlist; cur != last; cur++) {
		cout << move_to_uci(cur->move, 0) << " ";
	}
	cout << endl;
	cout << "book moves: "; book.print_all_moves(pos);
	cout << move_to_string(book.get_move(pos, true)) << endl;
	
	useImprovements = true;
	
	Value margin = VALUE_ZERO;
	bool expl_threat;
	read_evaluation_uci_options(pos.side_to_move());
	Value v = evaluate(pos, margin, &expl_threat);
	cout << "evaluation: " << v << endl;
	
	
	Color atomkraftColor = COLOR_NONE;
	
	bool tourney = false;
	Color improvementsUser = BLACK;
	int tourney_white_wins = 0;
	int tourney_black_wins = 0;
	int tourney_draws = 0;
	int tourney_game_count = 0;
	int tourney_max_game_count = 20000;
	Move tourney_moves[1000];
	int tourney_move_count = 0;
	
	
	cout << "farbe von atomkraft: "; cin >> inputc;
	if (inputc[0] == 'w') {			// atomkraft plays white
		atomkraftColor = WHITE;
	} else if (inputc[0] == 'b'){	// atomkraft plays black
		atomkraftColor = BLACK;
	} else if (inputc[0] == 'n'){	// atomkraft does not play
		atomkraftColor = COLOR_NONE;
	} else if (inputc[0] == 't'){	// atomkraft plays tourney (both colors, useImproments)
		tourney = true;
	} else {
		return;
	}
	
	
	srand(time(0));
	
	for (;;) {
		++counter;
		if (counter >= 1000) counter = 0;
		
		if (tourney) {
			float VALUE_SCALE_aux;
			
			if (pos.side_to_move() == improvementsUser) {
				useImprovements = true;
				
//				VALUE_SCALE_aux = 0.75f;
//				PawnValueMidgame   = Value(int(200  * VALUE_SCALE_aux));
//				PawnValueEndgame   = Value(int(300  * VALUE_SCALE_aux));
//				KnightValueMidgame = Value(int(300  * VALUE_SCALE_aux));
//				KnightValueEndgame = Value(int(350  * VALUE_SCALE_aux));
//				BishopValueMidgame = Value(int(300  * VALUE_SCALE_aux));
//				BishopValueEndgame = Value(int(320  * VALUE_SCALE_aux));
//				RookValueMidgame   = Value(int(600  * VALUE_SCALE_aux));
//				RookValueEndgame   = Value(int(800  * VALUE_SCALE_aux));
//				QueenValueMidgame  = Value(int(1400 * VALUE_SCALE_aux));
//				QueenValueEndgame  = Value(int(1700 * VALUE_SCALE_aux));
				
			} else {
				useImprovements = false;
				
//				VALUE_SCALE_aux = 0.75f;
//				PawnValueMidgame   = Value(int(200  * VALUE_SCALE_aux));
//				PawnValueEndgame   = Value(int(300  * VALUE_SCALE_aux));
//				KnightValueMidgame = Value(int(300  * VALUE_SCALE_aux));
//				KnightValueEndgame = Value(int(350  * VALUE_SCALE_aux));
//				BishopValueMidgame = Value(int(300  * VALUE_SCALE_aux));
//				BishopValueEndgame = Value(int(320  * VALUE_SCALE_aux));
//				RookValueMidgame   = Value(int(600  * VALUE_SCALE_aux));
//				RookValueEndgame   = Value(int(800  * VALUE_SCALE_aux));
//				QueenValueMidgame  = Value(int(1200 * VALUE_SCALE_aux));
//				QueenValueEndgame  = Value(int(1400 * VALUE_SCALE_aux));
			}
			
			if (pos.side_to_move() == WHITE) {
				/*
				 * Assuming changes in the evaluation will not
				 * effect the search much we use this configuration for
				 * testing changes to the evaluation function.
				 */
				//limits.maxDepth = 7 + rand() % 2;
				
				/*
				 * Assuming changes in the search will not effect
				 * the nps much we use this configuration for testing changes
				 * to the search function.
				 */
				limits.maxNodes = 20000 + rand() % 5000;
			}
			
			TT.clear();
			
			cout << (pos.side_to_move() == WHITE ? "White" : "Black");
			cout << " started to think and ";
			cout << (useImprovements ? "uses" : "doesn't use") << " improvements.";
			cout << endl;
			
			searchMoves[0] = MOVE_NONE;
			think(pos, limits, searchMoves, bestmove, pondermove);
			pos.do_setup_move(bestmove);
			
			tourney_moves[tourney_move_count] = bestmove;
			++tourney_move_count;
			
		} else if (pos.side_to_move() == opposite_color(atomkraftColor) || atomkraftColor == COLOR_NONE) {
			cout << "input: "; cin >> inputc;
			
			if (inputc[0] == 'q') {
				break; 
			} else if (inputc[0] == 'u'){
				pos.undo_move(move);
			} else if (inputc[0] == 'D'){
				print_debuginfo(pos);
			} else {
				
				string str(inputc);
				move = move_from_uci(pos, str);
				
				cout << "promotion: " << move_is_promotion(move) << endl;
				
				if (move != MOVE_NONE) {
					pos.do_move(move, st[counter]);
				} else {
					continue;
				}
				
				
			}
		} else {
			searchMoves[0] = MOVE_NONE;
			think(pos, limits, searchMoves, bestmove, pondermove);
			pos.do_move(bestmove, st[counter]);
		}
		
		//print_bb(pos.checkers());
		print_pos(pos);
		
		if (!tourney) {
			cout << "book moves: "; book.print_all_moves(pos);
		} else {
			cout << "tourney_move_count: " << tourney_move_count << endl;
		}
		
		if (pos.is_mate() || pos.is_draw()) {
			cout << "  *********************************  " << endl;
			cout << "  *                               *  " << endl;
			cout << "  *     position  draw / mate     *  " << endl;
			cout << "  *                               *  " << endl;
			cout << "  *********************************  " << endl;
			
			if (tourney) {
				ResultPGN result = NO_RESULT;
				
				if (pos.is_draw()) {
					++tourney_draws;
					result = DRAW;
				} else if (pos.is_mate()) {
					if (pos.side_to_move() == WHITE) {
						++tourney_black_wins;
						result = BLACK_WINS;
					} else {
						++tourney_white_wins;
						result = WHITE_WINS;
					}
				}
				
				cout << "tourney results: " << endl;
				cout << "white wins: " << tourney_white_wins << endl;
				cout << "black wins: " << tourney_black_wins << endl;
				cout << "draws:      " << tourney_draws << endl;
				cout << "black score: " 
					 << float(tourney_black_wins) + float(tourney_draws) / 2 << "/" 
					 << tourney_white_wins + tourney_black_wins + tourney_draws
					 << " ~ " <<  100.0f * (float(tourney_black_wins) + float(tourney_draws) / 2)
					 	 	 	 / (tourney_white_wins + tourney_black_wins + tourney_draws)
					 << "%" << endl;
				
#ifdef SWEN_VERSION
				appendPGN("atomkraft_tourney.pgn",
						"atomkraft", "atomkraft", result, 2, 0, 
						"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
						tourney_moves, tourney_move_count);
#else
				appendPGN("d:/atomkraft_tourney.pgn",
						"atomkraft", "atomkraft", result, 2, 0, 
						"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
						tourney_moves, tourney_move_count);
#endif
				
				
				tourney_move_count = 0;
				
				pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0);
				searchMoves[0] = MOVE_NONE;
				counter = 0;
				++tourney_game_count;
				
				if (tourney_game_count >= tourney_max_game_count) {
					Beep(2700, 500);
					system("pause");
					break;
				}
				
				Sleep(2000);
			} else {
				break;
			}
			
			
		}
	} 
	
}
