/*
 * atomicdata.cpp
 *
 *  Created on: 27.07.2011
 *      Author: golmman
 */


#include "atomicdata.h"
#include "debug.h"

ExplosionData explSq[64];
int8_t squares_touch[64][64];
int8_t square_dist_arr[64][64];

void generate_squaresTouch() {
	for (int i = 0; i < 64; ++i) {
		for (int j = 0; j < 64; ++j) {
			if (explBB[i] & ((Bitboard)1 << j)) {
				squares_touch[i][j] = 1;
			} else {
				squares_touch[i][j] = 0;
			}
			square_dist_arr[i][j] = square_distance(Square(i), Square(j));
		}
	}
}


void generate_explosionSquares() {
	int rank, file, currR, currF;
	
	int r[9] = { 0,  0,  1,  1,  1,  0, -1, -1, -1};
	int f[9] = { 0,  1,  1,  0, -1, -1, -1,  0,  1}; 
	
	for (int sq = 0; sq < 64; ++sq) {
		rank = sq / 8;
		file = sq % 8;
		
		explSq[sq].size = 0;
		
		for (int k = 0; k < 9; ++k) {
			currR = rank + r[k];
			currF = file + f[k];
			if (SQUARE_LEGAL(currR, currF)) {
				explSq[sq].square[explSq[sq].size] = (Square)(currR * 8 + currF);
				++explSq[sq].size;
			}
		}
	}
}


void generate_explosionBitboards() {
	generate_explosionSquares();
	
	Bitboard b;
	
	printf("const Bitboard explosionBB[64] = {\n\t");
	for (int sq = 0; sq < 64; ++sq) {
		b = 0;
		for (int k = 0; k < explSq[sq].size; ++k) {
			b |= BIT(explSq[sq].square[k]);	
		}
		printf("0x%.8X%.8XULL", HI64(b), LO64(b));
		if (sq < 63) printf(", ");
		if (sq % 4 == 3) printf("\n");
		if (sq < 63 && sq % 4 == 3) printf("\t");
	}
	printf("};");
	
}















