/*
 * pgn.cpp
 *
 *  Created on: 17.08.2011
 *      Author: golmman
 */


#include "pgn.h"
#include <stdio.h>
#include <time.h>
#include "position.h"

void appendPGN(
	char* filename, 
	char* name_white, 
	char* name_black, 
	ResultPGN result,
	int init_time, int inc,
	char* fen, Move* moves, int move_count) {
	
	FILE* file;
	file = fopen(filename, "a");
	
	char res[20];
	switch (result) {
		case WHITE_WINS: 	strcpy(res, "1-0"); break;
		case BLACK_WINS: 	strcpy(res, "0-1"); break;
		case DRAW: 			strcpy(res, "1/2-1/2"); break;
		case NO_RESULT: 	strcpy(res, "*"); break;
		default: break;
	}
	
	time_t rawtime;
	struct tm* tinfo;
	time(&rawtime);
	tinfo = localtime (&rawtime);
	
	
	// print meta data
	fprintf(file, "[Date \"%i.%i.%i\"]\n", tinfo->tm_year + 1900, tinfo->tm_mon + 1, tinfo->tm_mday);
	fprintf(file, "[Time \"%i:%i:%i\"]\n", tinfo->tm_hour, tinfo->tm_min, tinfo->tm_sec);
	fprintf(file, "[White \"%s\"]\n", name_white);
	fprintf(file, "[Black \"%s\"]\n", name_black);
	fprintf(file, "[Result \"%s\"]\n", res);
	fprintf(file, "[TimeControl \"%i+%i\"]\n", init_time, inc);
	fprintf(file, "[Variant \"atomic\"]\n");
	
	if (strcmp(fen, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")) {
		fprintf(file, "[FEN \"%s\"]\n", fen);
	}
	
	fprintf(file, "\n");
	
	Position pos(fen, 0, 0);
	
	// Babaschess doesn't understand move lists beginning with other numbers than 1
	int move_number = 1; //pos.startpos_ply_counter() / 2 + 1;
	Color curr_color = pos.side_to_move();
	
	// print the moves
	for (int k = 0; k < move_count; ++k) {
		
		//printf("%s\n", move_to_string(moves[k]).c_str());
		
		if (curr_color == WHITE) {
			fprintf(file, "%i. %s ", move_number, move_to_san(pos, moves[k]).c_str());
		} else if (curr_color == BLACK) {	
			if (k == 0) {
				fprintf(file, "%i. ... %s ", move_number, move_to_san(pos, moves[k]).c_str());
			} else {
				fprintf(file, "%s ", move_to_san(pos, moves[k]).c_str());
			}
			++move_number;
		}
		
		pos.do_setup_move(moves[k]);
		curr_color = pos.side_to_move();
	}
	
	// Babaschess doesn't understand this
	
	// finish the moves with '...' and the result
//	if (curr_color == WHITE) {
//		fprintf(file, "%i. ... %s\n\n", move_number, res);
//	} else if (curr_color == BLACK) {	
//		fprintf(file, "... %s\n\n", res);
//	}
	
	// Babaschess demands a final '*'
	fprintf(file, "*\n\n");
	
	fclose(file);
}










