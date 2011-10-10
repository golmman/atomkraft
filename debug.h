/*
 * debug.h
 *
 *  Created on: 22.07.2011
 *      Author: golmman
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include "position.h"

#include <cstdio>
#include <iostream>
#include <cstring>

using namespace std;

#ifndef HI64
	#define HI64(x) ((UINT32)((x) >> 32))
#endif
#ifndef HI64
	#define LO64(x) ((UINT32)(x))
#endif
#define PRINT_HEX(x) printf("0x%.8X%.8XULL", HI64(x), LO64(x));


const Square BOARDVTOI[8][8]	// board vector to index: BOARDVTOI[rank][file]
                         = {
                        		  {SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1},
                        		  {SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2},
                        		  {SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3},
                        		  {SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4},
                        		  {SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5},
                        		  {SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6},
                        		  {SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7},
                        		  {SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8}
                         };

const char PIECE_NAME[PIECE_NONE+1] 
    = {'x', 'P', 'N', 'B', 'R', 'Q', 'K', '~', '#', 'p', 'n', 'b', 'r', 'q', 'k', '+', ' '};

void print_pos(const Position& pos);
void print_bb(Bitboard bb);
void print_castleRights(const Position& pos);
void print_debuginfo(const Position& pos);




extern bool useImprovements;


#endif /* DEBUG_H_ */
