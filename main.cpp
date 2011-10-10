




#include "main.h"

#include "bitboard.h"
#include "movegen.h"
#include "debug.h"
#include "simple_search.h"
#include "atomicdata.h"
#include "search.h"
#include "bitboard.h"
#include "thread.h"
#include "ucioption.h"
#include "book.h"
#include "fics.h"
#include "create_book.h"
#include "pgn.h"
#include "evaluate.h"

#include <pthread.h>
#include <queue>
#include <iostream>
#include <sstream>
#include <string>
#include <time.h>
#include <conio.h>
using namespace std;




int perft(Position& pos, int depth);


extern void init_kpk_bitbase();
extern void init_kpk_bitbase();

#ifdef UCI_VERSION
extern bool execute_uci_command(const string& cmd);
#endif




void* testest(void* var) {
	Sleep(10000);
	//stdin_fake << "ponderhit" << endl;
	cout << "sending stop" << endl;
	stdin_fake << "stop" << endl;
	return 0;
}

void* swen_version_thread(void* var) {
	
	for(;;) {
		char c = getch();
	
		if (c == 's' || c == 'S') {
			cout << "rechnen beenden..." << endl;
			stdin_fake << "stop" << endl;
			break;
		}
	}
	
	return 0;
}




int main(int argc, char* argv[]) {
	


	
//	Style12 s12;
//	style12FromString(s12, "<12> rnbqkb-r pppppppp -----n-- -------- ----P--- -------- PPPPKPPP RNBQ-BNR B 4 0 0 1 1 52 7 Newton Einstein 1 2 12 39 39 119 122 246 K/e1-e2 (0:06) Ke2 0", File(-1));
//	
//	cout << s12.fen << endl;
//	
//	return 0;
	
	
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	cout.rdbuf()->pubsetbuf(NULL, 0);
	cin.rdbuf()->pubsetbuf(NULL, 0);
	
	// Startup initializations
	init_bitboards();
	Position::init_zobrist();
	Position::init_piece_square_tables();
	init_kpk_bitbase();
	init_search();
	Threads.init();
	generate_explosionSquares();
	generate_squaresTouch();
	
	srand(time(0));
	
//	char c = getch();
//	if (c == '1') {
//		OpenClipboard(NULL);
//		cout << (char*)GetClipboardData(CF_TEXT) << endl;
//	}
//	system("pause");
//	return 0;
	
	
//	char* oppname = "golmman";
//	char excuse1[BATMAN_LONGEST_TEXT + 50];
//	char excuse2[BATMAN_LONGEST_TEXT];
//	sprintf(excuse2, const_cast<const char*>(batman_wisdom[15]), oppname);
//	sprintf(excuse1, "tell %s %s\n", "99", excuse2);
//	
//	cout << excuse1;
//	cout << "test" << endl;
//	
//	return 0;

//	createBookEAO();
//	system("pause");
//	return 0;
	
#ifdef SWEN_VERSION
	Options["Threads"] = UCIOption(2, 1, MAX_THREADS);
	Options["Hash"] = UCIOption(512, 4, 8192);
	Options["Book File"] = UCIOption("eao.bin");
#endif
	
	
#ifndef NDEBUG
	cout << "Debug version of atomkraft, define NDEBUG to get the release version." << endl;
#endif
	

	
	// Print copyright notice
	cout << engine_name() << endl;
	cout << "by " << engine_authors() << endl;
	cout << "built " << __DATE__ << " " << __TIME__ << endl << endl;
	
	if (CpuHasPOPCNT)
		cout << "Good! CPU has hardware POPCNT." << endl;
	
	
//	stdin_fake << "test1" << endl;
//	stdin_fake << "test2" << endl;
//
//	
//	string cmd;
//	
//
//	
//	while (getline(stdin_fake, cmd)) {
//		cout << "NEW CMD: "<< cmd << endl;
//	}
//	stdin_fake.clear();
//
//	stdin_fake << "bal1" << endl;
//	stdin_fake << "as2" << endl;
//	
//	while (getline(stdin_fake, cmd)) {
//		cout << "NEW CMD: "<< cmd << endl;
//	}
//	stdin_fake.clear();
//	
//	return 0;
	
	
	
#ifdef BOOK_VERSION
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
	
	return 0;
	
	
#endif
	
#ifdef FICS_VERSION
	
	

	
	
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
					
					pthread_t gameendthread;
					pthread_create(&gameendthread, 0, sendlinesFics, buffer0);
					
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
	return 0;

#endif
	
	
	
	
	
	
#ifdef UCI_VERSION

	
	// Stockfishs main

	if (argc < 2)
	{
		// Print copyright notice
		//cout << engine_name() << " by " << engine_authors() << endl;

		//if (CpuHasPOPCNT)
		//	cout << "Good! CPU has hardware POPCNT." << endl;

		// Wait for a command from the user, and passes this command to
		// execute_uci_command() and also intercepts EOF from stdin to
		// ensure that we exit gracefully if the GUI dies unexpectedly.
		string cmd;
		while (getline(cin, cmd) && execute_uci_command(cmd)) {}
	}

	Threads.exit();
	return 0;
	
#endif
	
	
	
	
	
	
	
#ifdef TEST_VERSION
	
	const char BOOK_FILE[] = "d:/eao-full.bin";
	
	
	Book book;
	book.open(BOOK_FILE);
	
	
	Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0, 0);
	
	//Position pos("rnbqkbnr/ppp1p1pp/5p2/3p2N1/8/4P3/PPPP1PPP/RNBQKB1R b KQkq - 0 3", 0, 0);
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
	limits.maxTime = 60000;//3600000;
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
	Color improvementsUser = COLOR_NONE;
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
		return 0;
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
    
    
    return 0;
    
    
    
    
#endif
    
    
#ifdef SWEN_BOOK_VERSION
    
	
	
	Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0, 0);
	
	
	char inputc[100];
	char inp[200];
	int hashsize;
	int multipv;
	char c;
	
	Move searchMoves[MAX_MOVES];
	searchMoves[0] = MOVE_NONE;
	SearchLimits limits;
    Move bestmove, pondermove;
    
    
    Options["Ponder"] = UCIOption(false);
    Options["OwnBook"] = UCIOption(false);
    Options["Threads"] = UCIOption(1, 1, MAX_THREADS);
    Options["Hash"] = UCIOption(128, 4, 8192);
    
    cout << "**************************" << endl;
    cout << "*                        *" << endl;
    cout << "*  SWEN SPECIAL VERSION  *" << endl;
    cout << "*                        *" << endl;
    cout << "**************************" << endl;
    
	
	for (;;) {
		cout << "Aktuelle Stellung: " << endl;
		print_pos(pos);
		
		cout << "Was soll getan werden?" << endl;
		cout << " 1 --> Stellung aendern" << endl;
		cout << " 2 --> Hashgroesse aendern" << endl;
		cout << " 3 --> Variantenanzahl (MultiPV) aendern " << endl;
		cout << " 4 --> Analyse starten" << endl;
		cout << " 5 --> Beenden" << endl;
		cout << endl << "Eingabe >> ";
		cin.getline(inputc, 100);
		
		switch (inputc[0]) {
		case '1':
			cout << "Stellung im FEN-Format eingeben: " << endl;
			//cin.getline(inp, 200);
			
			c = getch();
			if (c == '1') {
				OpenClipboard(NULL);
				strcpy(inp, (char*)GetClipboardData(CF_TEXT));
				CloseClipboard();
				cout << inp << endl;
				pos.from_fen(inp, 0);
			} else {
				cout << "Keine Stellung eingelesen!" << endl;
			}
			
			
			break;
		case '2':
			cout << "Aktuelle Hashgroesse: " << Options["Hash"].value<int>() << endl;
			cout << "Aendern zu: ";
			cin.getline(inp, 200);
			hashsize = atoi(inp);
			if (hashsize) {
				Options["Hash"] = UCIOption(hashsize, 4, 8192);
				cout << "Hashgroesse geaendert zu: " << Options["Hash"].value<int>() << endl;
			}
			else cout << "Ungueltiger Wert!" << endl;
			break;
		case '3':
			cout << "Aktuelle Variantenanzahl: " << Options["MultiPV"].value<int>() << endl;
			cout << "Aendern zu: ";
			cin.getline(inp, 200);
			multipv = atoi(inp);
			if (multipv) {
				Options["MultiPV"] = UCIOption(multipv, 1, 500);
				cout << "Variantenanzahl geaendert zu: " << Options["MultiPV"].value<int>() << endl;
			}
			else cout << "Ungueltiger Wert!" << endl;
			break;
		case '4':
			cout << "Starte Analyse..." << endl;
			cout << "Beenden mit 's'" << endl;
			
			limits.infinite = true;
			Move searchMoves[MAX_MOVES];
			searchMoves[0] = MOVE_NONE;
			
			pthread_t swen_thread;
			pthread_create(&swen_thread, 0, swen_version_thread, 0);
			
			think(pos, limits, searchMoves, bestmove, pondermove);
			
			break;
		case '5':
			cout << "Adios!" << endl;
			system("pause");
			return 0;
			break;
		default:
			cout << "ungueltige Eingabe" << endl;
			break;
		}
	}
    
    
    return 0;
    
#endif
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    /*
     * TEST SEARCH
     */
    
    
//	MoveStack mlist[MAX_MOVES];
//	
//	char inputc[100];
//	
//	Move move;
//	int failstep;
//	StateInfo st[100];
//	int counter = 0;
//	
//	
//	print_pos(pos);
//	for (int k = 0; k < st[counter].expl.size; ++ k) {
//		cout << PIECE_NAME[st[counter].expl.piece[k]] 
//		                   << " at " << st[counter].expl.square[k] << endl;
//	}
//	cout << (pos.is_ok(&failstep) ? "position is ok" : "position is broken") << endl;
//	cout << "number of black kings: " << pos.piece_count(BLACK, KING) << endl;
//	cout << "castling: "; print_castleRights(pos);
//	
//	MoveStack* last = generate<MV_LEGAL>(pos, mlist);
//	for (MoveStack* cur = mlist; cur != last; cur++) {
//		cout << move_to_uci(cur->move, 0) << " ";
//	}
//	cout << endl;
//	
//	for (;;) {
//		++counter;
//		
//		
//		if (pos.side_to_move() == WHITE || pos.side_to_move() == BLACK) {
//			cout << "input: "; cin >> inputc;
//			
//			if (inputc[0] == 'q') {
//				break; 
//			} else if (inputc[0] == 'u'){
//				pos.undo_move(move);
//			} else if (inputc[0] == 'D'){
//				print_debuginfo(pos);
//			} else {
//				
//				string str(inputc);
//				move = move_from_uci(pos, str);
//				
//				if (move != MOVE_NONE) {
//					pos.do_move(move, st[counter]);
//				} else {
//					continue;
//				}
//				
//			}
//		} else {
//			pos.do_move(test_bestmove(pos, 5), st[counter]);
//		}
//		
//		print_pos(pos);
//		for (int k = 0; k < st[counter].expl.size; ++ k) {
//			cout << PIECE_NAME[st[counter].expl.piece[k]] 
//			                   << " at " << st[counter].expl.square[k] << endl;
//		}
//		cout << (pos.is_ok(&failstep) ? "position is ok" : "position is broken") << endl;
//		cout << "number of black kings: " << pos.piece_count(BLACK, KING) << endl;
//		cout << "castling: "; print_castleRights(pos);
//		
//		MoveStack* last = generate<MV_LEGAL>(pos, mlist);
//		for (MoveStack* cur = mlist; cur != last; cur++) {
//			cout << move_to_uci(cur->move, 0) << " ";
//		}
//		cout << endl;
//		
//		
//		if (last == mlist) {
//			cout << "SCHACHMATT ODER SO" << endl;
//			break;
//		}
//	}
//	
	//test_bestmove(pos, 6);
	
	
	
	return 0;
}


int perft(Position& pos, int depth) {
	MoveStack mlist[MAX_MOVES];
	StateInfo st;
	Move m;
	int64_t sum = 0;

	// Generate all legal moves
	MoveStack* last = generate<MV_LEGAL>(pos, mlist);

	// If we are at the last ply we don't need to do and undo
	// the moves, just to count them.
	if (depth == 0)
		return int(last - mlist);

	// Loop through all legal moves
	for (MoveStack* cur = mlist; cur != last; cur++) {
		m = cur->move;
		pos.do_move(m, st);
		sum += perft(pos, depth - 1);
		pos.undo_move(m);
	}
	return sum;
}









