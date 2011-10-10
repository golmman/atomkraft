/*
 * debug.cpp
 *
 *  Created on: 22.07.2011
 *      Author: golmman
 */


#include "debug.h"

bool useImprovements;


void print_pos(const Position& pos) {
	Piece piece;
	
	cout << "    0   1   2   3   4   5   6   7" << endl;
	cout << "  +---+---+---+---+---+---+---+---+" << endl;
	
	for (int rank = 7; rank >= 0; --rank) {
		cout << rank + 1 << " |";
		for (int file = 0; file < 8; ++file) {
			piece = pos.piece_on(BOARDVTOI[rank][file]);
			if (piece < 0 || piece > 16) {
				cout << "ERROR" << endl;
				return;
			}
			cout << " " << PIECE_NAME[piece] << " |";
		}
		cout << " " << rank << endl;
		cout << "  +---+---+---+---+---+---+---+---+" << endl;
	}
	
	cout << "    a   b   c   d   e   f   g   h" << endl;
}

void print_bb(Bitboard bb) {
	Piece piece;
	Piece pieces[64];
	
	for (int k = 0; k < 64; ++k) {
		pieces[k] = PIECE_NONE;
	}
	
	while (bb) {
		pieces[pop_1st_bit(&bb)] = PIECE_NONE_DARK_SQ;
	}
	
	cout << "    0   1   2   3   4   5   6   7" << endl;
	cout << "  +---+---+---+---+---+---+---+---+" << endl;
	
	for (int rank = 7; rank >= 0; --rank) {
		cout << rank + 1 << " |";
		for (int file = 0; file < 8; ++file) {
			piece = pieces[BOARDVTOI[rank][file]];
			if (piece < 0 || piece > 16) {
				cout << "ERROR" << endl;
				return;
			}
			cout << " " << PIECE_NAME[piece] << " |";
		}
		cout << " " << rank << endl;
		cout << "  +---+---+---+---+---+---+---+---+" << endl;
	}
	
	cout << "    a   b   c   d   e   f   g   h" << endl;
}


void print_castleRights(const Position& pos) {
	cout << (pos.can_castle_queenside(BLACK) ? 1 : 0);
	cout << (pos.can_castle_queenside(WHITE) ? 1 : 0);
	cout << (pos.can_castle_kingside(BLACK) ? 1 : 0);
	cout << (pos.can_castle_kingside(WHITE) ? 1 : 0);
	cout << endl;
}


void print_debuginfo(const Position& pos) {
	char names[7][50] = {
			"PIECES", "PAWNS", "KNIGHTS", "BISHOPS",
			"ROOKS", "QUEENS", "KINGS"	
	};
	
	cout << endl;
	cout << "--- START DEBUG INFORMATION ---" << endl;
	cout << endl;
	
	cout << "CASTLING RIGHTS" << endl;
	print_castleRights(pos);
	cout << endl;
	
	
	for (int pt = 0; pt < 7; ++pt) {
		cout << "PIECELIST WHITE " << names[pt] << endl;
		for (int i = 0; i < pos.piece_count(WHITE, (PieceType)pt); ++i) {
			cout << pos.piece_list(WHITE, (PieceType)pt, i) << " ";
		}
		cout << endl << endl;
	}
	
	for (int pt = 0; pt < 7; ++pt) {
		cout << "PIECELIST BLACK " << names[pt] << endl;
		for (int i = 0; i < pos.piece_count(BLACK, (PieceType)pt); ++i) {
			cout << pos.piece_list(BLACK, (PieceType)pt, i) << " ";
		}
		cout << endl << endl;
	}
	
	cout << "BOARD" << endl;
	print_pos(pos);
	cout << endl;
	
	for (int pt = 0; pt < 7; ++pt) {
		cout << "WHITE " << names[pt] << endl;
		print_bb(pos.pieces((PieceType)pt, WHITE));
		cout << endl;
	}
	
	for (int pt = 0; pt < 7; ++pt) {
		cout << "BLACK " << names[pt] << endl;
		print_bb(pos.pieces((PieceType)pt, BLACK));
		cout << endl;
	}
	
	cout << endl;
	cout << "--- END DEBUG INFORMATION ---" << endl;
	cout << endl;
}













