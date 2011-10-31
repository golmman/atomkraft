




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

#if defined ANALYZE_VERSION
	extern void main_analyze();
#elif defined BOOK_VERSION
	extern void main_book_from_thinking();
	extern void main_book_from_file();
#elif defined FICS_VERSION
	extern void main_fics();
#elif defined TEST_VERSION
	extern void main_test();
#elif defined UCI_VERSION
	extern void main_uci(int argc);
#endif




extern void init_kpk_bitbase();

//#define TEST1
#define TEST2

int main(int argc, char* argv[]) {
	srand(time(0));
	
	
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
	
	
#ifndef NDEBUG
	cout << "Debug version of atomkraft, define NDEBUG to get the release version." << endl;
#endif
	
	// Print copyright notice
	cout << engine_name() << endl;
	cout << "by " << engine_authors() << endl;
	cout << "built " << __DATE__ << " " << __TIME__ << endl << endl;
	
	if (CpuHasPOPCNT)
		cout << "Good! CPU has hardware POPCNT." << endl;
	
	
#ifdef SWEN_VERSION
	Options["Threads"] = UCIOption(2, 1, MAX_THREADS);
	Options["Hash"] = UCIOption(512, 4, 8192);
	Options["Book File"] = UCIOption("eao.bin");
#endif
	
	
	
	
#if defined ANALYZE_VERSION
    
    main_analyze();

#elif defined BOOK_VERSION
	
	main_book_from_file();
	//main_book_from_thinking();
	
#elif defined FICS_VERSION
	
	main_fics();


#elif defined TEST_VERSION
	
	main_test();
    
#elif defined UCI_VERSION

	main_uci(argc);
	
#else
	
	cout << "no version specified" << endl;
	
#endif
	
	
	
	
	return 0;
}









