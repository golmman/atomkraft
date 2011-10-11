/*
 * main_fics.cpp
 *
 *  Created on: 11.10.2011
 *      Author: golmman
 */

#include "main.h"

#include "debug.h"
#include "search.h"
#include "thread.h"
#include "ucioption.h"
#include "book.h"
#include "fics.h"
#include "pgn.h"

#include <pthread.h>
#include <queue>
#include <iostream>
#include <time.h>
using namespace std;


void main_fics() {

	
	
	cout << "connecting..." << endl;
	connectFics();
	
	
	bool go = false;
	
	Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0, 0);
	Color us = COLOR_NONE;
	Color them  = COLOR_NONE;
	
	Move searchMoves[MAX_MOVES];
	searchMoves[0] = MOVE_NONE;
	SearchLimits limits;
    Move bestmove;
	StateInfo st[1000];
	int counter = 0;
	char buffer0[1024];
	char buffer1[1024];
	string str;
	
	clock_t inactive_since = clock();
	clock_t abort_clock = 0;
	
	Options["Ponder"] = UCIOption(true);
	
	for (;;) {
		Sleep(20);
		
		
		// we don't want to be logged out because of inactivity
		if (!playing && ((clock() - inactive_since > 900000) || performance_test)) {

			// performance test
			cout << "performance test..." << endl;
			TT.clear();
			memset(&limits, 0, sizeof(SearchLimits));
			limits.maxDepth = 8;
			pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0);
			
			Options["OwnBook"] = UCIOption(false);
			Options["Ponder"] = UCIOption(false);
			thinking = true;
			Move bm, pm;
			searchMoves[0] = MOVE_NONE;
			think(pos, limits, searchMoves, bm, pm);
			thinking = false;
			Options["Ponder"] = UCIOption(true);
			Options["OwnBook"] = UCIOption(true);
			cout << "performance test finished" << endl;
			
			// reset skill
			skill = 20;
			
			// set busy string
			int batman_rand = rand() % BATMAN_WISDOM_COUNT;
			sprintf(buffer0, const_cast<const char*>(batman_wisdom[batman_rand]), "Robin");
			strcat(buffer0, "\n");
			sprintf(buffer1, "set busy says: %s", buffer0);
			sendFics(buffer1);
			
			
			inactive_since = clock();
			performance_test = false;
		}
		
		// poll the command queue
		if (!cmd_queue.empty()) {
			Style12* cmd = cmd_queue.front();
			
			
			switch (cmd->cmd) {
				case CMD_NEW:
					// starting new game, maybe we continue an adjourned game 
					// so it is not clear if we play white
					cout << "Starting a new game" << endl;
					counter = 0;
					curr_ponder_move = MOVE_NONE;
					last_ponder_move = MOVE_NONE;
					inactive_since = clock();
					abort_clock = clock();
					
					// clear stdin_fake
					while (getline(stdin_fake, str)) {}
					stdin_fake.clear();
					
					// set up position
					//pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0);
					pos.from_fen(cmd->fen, 0);
					TT.clear();
					
					cout << "using fen: " << cmd->fen << endl;
					print_pos(pos);
					
				    go = (cmd->relation == PLAYING_WE_MOVE);
				    us = (go ? pos.side_to_move() : opposite_color(pos.side_to_move()));
				    //   (go ? WHITE : BLACK);
				    them = opposite_color(us);
				    
				    memset(&limits, 0, sizeof(SearchLimits));
				    
				    if (us == WHITE) {
				    	limits.time = cmd->rem_time_white * 1000;
				    } else {
				    	limits.time = cmd->rem_time_black * 1000;
				    }
				    limits.increment = cmd->initial_inc * 1000;
				    
					game_started = false;
					
					break;
					
				case CMD_MOVE:
					
					// set limits
					if (us == WHITE) {
						limits.time = cmd->rem_time_white * 1000;
					} else if (us == BLACK) {
						limits.time = cmd->rem_time_black * 1000;
					} else {
						cout << "ERROR: main no color" << endl;
					}
					limits.increment = cmd->initial_inc * 1000;
					limits.ponder = false;
					
					
					
					
					if (cmd->relation == PLAYING_WE_MOVE) {
						
						
						// undo ponder move if no ponderhit
						if (Options["Ponder"].value<bool>() && !ponderhit && last_ponder_move) {
							cout << "undoing ponder move" << endl;
							pos.undo_move(last_ponder_move);
						}
						
						if (Options["Ponder"].value<bool>() && ponderhit) {
							// ponderhit
							cout << "The opponent has ponderhit-moved: " << move_to_string(cmd->move) << endl;
							
						} else if (pos.side_to_move() == them) {
							// from time to time fics sends some position information twice
							// therefore we check if pos.side_to_move() == them
							cout << "The opponent has moved: " << move_to_string(cmd->move) << endl;
							
							// do the move the opponent made
							pos.do_move(cmd->move, st[counter]);
							++counter;
							
							go = !pos.is_mate();
						} else {
							cout << "The opponent has moved but it is not his turn" << endl;
						}
						
					} else {
						// do the move we already computed
						cout << "We have moved: " << move_to_string(cmd->move) << endl;
						pos.do_move(cmd->move, st[counter]);
						++counter;
						
						// ponder
						last_ponder_move = curr_ponder_move;
						
						if (   Options["Ponder"].value<bool>() 
							&& curr_ponder_move != MOVE_NONE
							&& !pos.is_mate()) {
							
							cout << "Doing pondermove: " << move_to_string(curr_ponder_move) << endl;
							pos.do_move(curr_ponder_move, st[counter]);
							++counter;
							limits.ponder = true;
							pondering = true;
							go = true;
						}
					}
					
					break;
					
				case CMD_END:
					cout << "The game ended" << endl;
					us = COLOR_NONE;
					them = COLOR_NONE;
					curr_ponder_move = MOVE_NONE;
					last_ponder_move = MOVE_NONE;
					
					
					
					// clear stdin_fake
					while (getline(stdin_fake, str)) {}
					stdin_fake.clear();
					
					// clear queue
					// this may be wrong if the rematch request comes too fast
//					while (!cmd_queue.empty()) {
//						delete cmd_queue.front();
//						cmd_queue.pop();
//					}
//					cmd = 0;
					
					// send gameend commands
					buffer0[0] = 0;
					for (int k = 0; k < gameend_cmd_count; ++k) {
						buffer1[0] = 0;
						sprintf(buffer1, "gameend%d\n", k);
						strcat(buffer0, buffer1);
					}
					


#if defined(_MSC_VER)
					CreateThread(NULL, 0, sendlinesFics, (LPVOID)buffer0, 0, NULL);
#else
					pthread_t gameendthread;
					pthread_create(&gameendthread, 0, sendlinesFics, buffer0);
#endif
					
					game_ended = false;
					inactive_since = clock();
					break;
					
				case CMD_QUIT:
					cout << "Bye Bye!" << endl;
					Sleep(2000);
					goto QuitAtomkraft;
					break;
					
				case CMD_NONE:
					cout << "ERROR: CMD_NONE" << endl;
					break;
				default:
					cout << "ERROR: CMD_NONE2" << endl;
					break;
			}
			
			
			// pop the queue only if the game has not ended
			if (cmd) {
				delete cmd;
				cmd_queue.pop();
			}
		}
		
		
		
		if (go) {
			cout << "start " << (pondering ? "pondering..." : "thinking...") << endl;
			
			//last_ponder_move = curr_ponder_move;
			
			// if the command queue is not empty and we want to start to ponder continue
			// the for loop here since the remaining queue command is the opponents move
			if (pondering && !cmd_queue.empty()) {
				cout << "fast move! no pondering" << endl;
				ponderhit = false;
				pondering = false;
				go = false;
				continue;
			}
			
			// start to think
			thinking = true;
			searchMoves[0] = MOVE_NONE;
			think(pos, limits, searchMoves, bestmove, curr_ponder_move);
			thinking = false;
			
			limits.ponder = false;
			
			cout << "thinking finished" << endl;
			
			// don't send the move if there was no ponderhit
			if (pondering && !ponderhit) {
				pondering = false;
				go = false;
				continue;
			}
			
			
			// we do the move only when fics says it is ok
			// pos.do_move(bestmove, st[counter]);
			//++counter;
			
			// convert the move
			Square from = move_from(bestmove);
			Square to = move_to(bestmove);
			
			PieceType promotion = move_promotion_piece(bestmove);
			
			string movestr;
			
			switch (promotion) {
			case KNIGHT: sendFics("promote knight\n"); break;
			case BISHOP: sendFics("promote bishop\n"); break;
			case ROOK: sendFics("promote rook\n"); break;
			case QUEEN: sendFics("promote queen\n"); break;
			default: break;
			}
			
			if (move_is_castle(bestmove)) {
				  if (move_is_short_castle(bestmove))
				      movestr = from == SQ_E1 ? "e1-g1" : "e8-g8";

				  if (move_is_long_castle(bestmove))
					  movestr = from == SQ_E1 ? "e1-c1" : "e8-c8";
			} else {
				movestr += (square_to_string(from) + "-" +  square_to_string(to));
			}
			
			movestr += "\n";
			
			sendFics((char*)movestr.c_str());
			
			pondering = false;
			go = false;
		}
		
		
		
	}

	
QuitAtomkraft:
	
	disconnectFics();
	Threads.exit();
	
	
	system("pause");
	
}
