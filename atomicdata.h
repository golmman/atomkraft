/*
 * atomicData.h
 *
 *  Created on: 27.07.2011
 *      Author: golmman
 */

#ifndef ATOMICDATA_H_
#define ATOMICDATA_H_

#include "types.h"
#include <iostream>
#include <stdio.h>
#include <assert.h>

using namespace std;

#define SQUARE_LEGAL(r, f) ((r) >= 0 && (r) <= 7 && (f) >= 0 && (f) <= 7)
#define BIT(x) ((Bitboard)1 << (x))
#define HI64(x) ((uint32_t)((x) >> 32))
#define LO64(x) ((uint32_t)(x))

/*
 * macro to indicate whether code is added or deleted
 */
#define NEW 
#define STARTNEW
#define ENDNEW
// "comment" macro, /##/ does not work since comments are deleted before preprocessoring
#define OLD if (0)
#define STARTOLD if (0) {
#define ENDOLD }

// masks all squares that are effected during an explosion
const Bitboard explBB[64] = {
	0x0000000000000303ULL, 0x0000000000000707ULL, 0x0000000000000E0EULL, 0x0000000000001C1CULL, 
	0x0000000000003838ULL, 0x0000000000007070ULL, 0x000000000000E0E0ULL, 0x000000000000C0C0ULL, 
	0x0000000000030303ULL, 0x0000000000070707ULL, 0x00000000000E0E0EULL, 0x00000000001C1C1CULL, 
	0x0000000000383838ULL, 0x0000000000707070ULL, 0x0000000000E0E0E0ULL, 0x0000000000C0C0C0ULL, 
	0x0000000003030300ULL, 0x0000000007070700ULL, 0x000000000E0E0E00ULL, 0x000000001C1C1C00ULL, 
	0x0000000038383800ULL, 0x0000000070707000ULL, 0x00000000E0E0E000ULL, 0x00000000C0C0C000ULL, 
	0x0000000303030000ULL, 0x0000000707070000ULL, 0x0000000E0E0E0000ULL, 0x0000001C1C1C0000ULL, 
	0x0000003838380000ULL, 0x0000007070700000ULL, 0x000000E0E0E00000ULL, 0x000000C0C0C00000ULL, 
	0x0000030303000000ULL, 0x0000070707000000ULL, 0x00000E0E0E000000ULL, 0x00001C1C1C000000ULL, 
	0x0000383838000000ULL, 0x0000707070000000ULL, 0x0000E0E0E0000000ULL, 0x0000C0C0C0000000ULL, 
	0x0003030300000000ULL, 0x0007070700000000ULL, 0x000E0E0E00000000ULL, 0x001C1C1C00000000ULL, 
	0x0038383800000000ULL, 0x0070707000000000ULL, 0x00E0E0E000000000ULL, 0x00C0C0C000000000ULL, 
	0x0303030000000000ULL, 0x0707070000000000ULL, 0x0E0E0E0000000000ULL, 0x1C1C1C0000000000ULL, 
	0x3838380000000000ULL, 0x7070700000000000ULL, 0xE0E0E00000000000ULL, 0xC0C0C00000000000ULL, 
	0x0303000000000000ULL, 0x0707000000000000ULL, 0x0E0E000000000000ULL, 0x1C1C000000000000ULL, 
	0x3838000000000000ULL, 0x7070000000000000ULL, 0xE0E0000000000000ULL, 0xC0C0000000000000ULL
};

// if an explosion occurs near a corner we can remove castling rights
// 1111 = 15 = all castles
// 1110 = 14 = remove white king side castling
// 1101 = 13 = remove black king side castling
// 1011 = 11 = remove white queen side castling
// 0111 =  7 = remove black queen side castling
const int expl_castleRightsMask[64] = {
	11, 11, 15, 15, 15, 15, 14, 14,
	11, 11, 15, 15, 15, 15, 14, 14,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	 7,  7, 15, 15, 15, 15, 13, 13,
	 7,  7, 15, 15, 15, 15, 13, 13
};




// stores all pieces standing in a 3x3-square when an explosion occurs
// does not store the attacking piece!
struct ExplosionData {
	Square square[9];
	Piece piece[9];
	int32_t size;
};


// 1 if the two squares directly face each other, 0 else.
// TODO: squares_touch is just a special case of square_dist
extern int8_t squares_touch[64][64];
extern int8_t square_dist_arr[64][64];

extern ExplosionData explSq[64];

void generate_squaresTouch();
void generate_explosionSquares();
void generate_explosionBitboards();

inline int square_dist(Square s1, Square s2) {
	assert(square_is_ok(s1));
	assert(square_is_ok(s2));
	return square_dist_arr[s1][s2];
}


#endif /* ATOMICDATA_H_ */
