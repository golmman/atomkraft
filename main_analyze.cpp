/*
 * main_analyze.cpp
 *
 *  Created on: 11.10.2011
 *      Author: golmman
 */

#include "main.h"

#include "search.h"
#include "ucioption.h"
#include "position.h"
#include "thread.h"
#include "debug.h"

#if !defined(_MSC_VER)
	#include <pthread.h>
#else
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#undef WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <conio.h>

using namespace std;



#if defined(_MSC_VER)

  DWORD WINAPI swen_version_thread(LPVOID threadID) {
	
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

#else

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

#endif



void main_analyze() {
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
    
    //cout << "**************************" << endl;
    //cout << "*                        *" << endl;
    //cout << "*  SWEN SPECIAL VERSION  *" << endl;
    //cout << "*                        *" << endl;
    //cout << "**************************" << endl;
    
	
	for (;;) {
		cout << "current position: " << endl;
		print_pos(pos);
		
		cout << "What to do?" << endl;
		cout << " 1 --> set position" << endl;
		cout << " 2 --> set hash table size" << endl;
		cout << " 3 --> set number of variants (multi pv)" << endl;
		cout << " 4 --> analyze" << endl;
		cout << " 5 --> quit" << endl;
		cout << endl << " >> ";
		cin.getline(inputc, 100);
		
		switch (inputc[0]) {
		case '1':
			cout << "enter position in FEN-format: " << endl;
			//cin.getline(inp, 200);
			
			c = getch();
			if (c == '1') {
				OpenClipboard(NULL);
				strcpy(inp, (char*)GetClipboardData(CF_TEXT));
				CloseClipboard();
				cout << inp << endl;
				pos.from_fen(inp, 0);
			} else {
				cout << "no position read!" << endl;
			}
			
			
			break;
		case '2':
			cout << "current hash table size: " << Options["Hash"].value<int>() << endl;
			cout << "set to: ";
			cin.getline(inp, 200);
			hashsize = atoi(inp);
			if (hashsize) {
				Options["Hash"] = UCIOption(hashsize, 4, 8192);
				cout << "hash table size set to: " << Options["Hash"].value<int>() << endl;
			}
			else cout << "invalid value!" << endl;
			break;
		case '3':
			cout << "current number of variations: " << Options["MultiPV"].value<int>() << endl;
			cout << "set to: ";
			cin.getline(inp, 200);
			multipv = atoi(inp);
			if (multipv) {
				Options["MultiPV"] = UCIOption(multipv, 1, 500);
				cout << "number of variations set to: " << Options["MultiPV"].value<int>() << endl;
			}
			else cout << "invalid value!" << endl;
			break;
		case '4':
			cout << "analyse..." << endl;
			cout << "stop analyzation by hitting 's'" << endl;
			
			limits.infinite = true;
			Move searchMoves[MAX_MOVES];
			searchMoves[0] = MOVE_NONE;
			
			

#if defined(_MSC_VER)
			CreateThread(NULL, 0, swen_version_thread, 0, 0, NULL);
#else
			pthread_t swen_thread;
			pthread_create(&swen_thread, 0, swen_version_thread, 0);
#endif
			
			searchMoves[0] = MOVE_NONE;
			think(pos, limits, searchMoves, bestmove, pondermove);
			
			break;
		case '5':
			cout << "Bye!" << endl;
			system("pause");
			return;
			break;
		default:
			cout << "invalid input" << endl;
			break;
		}
	}
    
}
