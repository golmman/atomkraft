/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2010 Marco Costalba, Joona Kiiski, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>

#include "bitboard.h"
#include "bitcount.h"
#include "pawns.h"
#include "position.h"
#include "atomicdata.h"
#include "debug.h"

namespace {

  #define S(mg, eg) make_score(mg, eg)

  // Doubled pawn penalty by opposed flag and file
  const Score DoubledPawnPenalty[2][8] = {
  { S(13, 43), S(20, 48), S(23, 48), S(23, 48),
    S(23, 48), S(23, 48), S(20, 48), S(13, 43) },
  { S(13, 43), S(20, 48), S(23, 48), S(23, 48),
    S(23, 48), S(23, 48), S(20, 48), S(13, 43) }};

  // Isolated pawn penalty by opposed flag and file
  const Score IsolatedPawnPenalty[2][8] = {
  { S(37, 45), S(54, 52), S(60, 52), S(60, 52),
    S(60, 52), S(60, 52), S(54, 52), S(37, 45) },
  { S(25, 30), S(36, 35), S(40, 35), S(40, 35),
    S(40, 35), S(40, 35), S(36, 35), S(25, 30) }};

  // Backward pawn penalty by opposed flag and file
  const Score BackwardPawnPenalty[2][8] = {
  { S(30, 42), S(43, 46), S(49, 46), S(49, 46),
    S(49, 46), S(49, 46), S(43, 46), S(30, 42) },
  { S(20, 28), S(29, 31), S(33, 31), S(33, 31),
    S(33, 31), S(33, 31), S(29, 31), S(20, 28) }};

  // Pawn chain membership bonus by file
  const Score ChainBonus[8] = {
    S(11,-1), S(13,-1), S(13,-1), S(14,-1),
    S(14,-1), S(13,-1), S(13,-1), S(11,-1)
  };

  // Candidate passed pawn bonus by rank
  const Score CandidateBonus[8] = {
    S( 0, 0), S( 6, 13), S(6,13), S(14,29),
    S(34,68), S(83,166), S(0, 0), S( 0, 0)
  };

  const Score PawnStructureWeight = S(233, 201);
  
  STARTNEW
  const Score PawnTropism[8] = {
	//S(0, 0), S(40, 10), S(20, 5), S(16, 4), S(12, 3), S(8, 2), S(4, 1), S(0, 0)	// 31.7%
	//S(0, 0), S(80, 20), S(40, 10), S(20, 5), S(10, 2), S(5, 1), S(2, 0), S(0, 0)	// 29.0%
	S(0, 0), S(40, 10), S(20, 5), S(5, 1), S(1, 0), S(0, 0), S(0, 0), S(0, 0)		// 35.0%, 35.6%
	//S(0, 0), S(30, 10), S(10, 3), S(3, 1), S(0, 0), S(0, 0), S(0, 0), S(0, 0)		// 33.0%
  };
  ENDNEW

  #undef S

  inline Score apply_weight(Score v, Score w) {
    return make_score((int(mg_value(v)) * mg_value(w)) / 0x100,
                      (int(eg_value(v)) * eg_value(w)) / 0x100);
  }
}


/// PawnInfoTable::get_pawn_info() takes a position object as input, computes
/// a PawnInfo object, and returns a pointer to it. The result is also stored
/// in an hash table, so we don't have to recompute everything when the same
/// pawn structure occurs again.

PawnInfo* PawnInfoTable::get_pawn_info(const Position& pos) const {

  assert(pos.is_ok());

  Key key = pos.get_pawn_key();
  PawnInfo* pi = probe(key);

  // If pi->key matches the position's pawn hash key, it means that we
  // have analysed this pawn structure before, and we can simply return
  // the information we found the last time instead of recomputing it.
  if (pi->key == key)
      return pi;

  // Initialize PawnInfo entry
  pi->key = key;
  pi->passedPawns[WHITE] = pi->passedPawns[BLACK] = 0;
  pi->kingSquares[WHITE] = pi->kingSquares[BLACK] = SQ_NONE;
  pi->halfOpenFiles[WHITE] = pi->halfOpenFiles[BLACK] = 0xFF;

  // Calculate pawn attacks
  Bitboard wPawns = pos.pieces(PAWN, WHITE);
  Bitboard bPawns = pos.pieces(PAWN, BLACK);
  pi->pawnAttacks[WHITE] = ((wPawns << 9) & ~FileABB) | ((wPawns << 7) & ~FileHBB);
  pi->pawnAttacks[BLACK] = ((bPawns >> 7) & ~FileABB) | ((bPawns >> 9) & ~FileHBB);

  // Evaluate pawns for both colors and weight the result
  pi->value =  evaluate_pawns<WHITE>(pos, wPawns, bPawns, pi)
             - evaluate_pawns<BLACK>(pos, bPawns, wPawns, pi);

  pi->value = apply_weight(pi->value, PawnStructureWeight);

  return pi;
}


/// PawnInfoTable::evaluate_pawns() evaluates each pawn of the given color

template<Color Us>
Score PawnInfoTable::evaluate_pawns(const Position& pos, Bitboard ourPawns,
                                    Bitboard theirPawns, PawnInfo* pi) {

  const BitCountType Max15 = CpuIs64Bit ? CNT64_MAX15 : CNT32_MAX15;
  const Color Them = (Us == WHITE ? BLACK : WHITE);

  Bitboard b;
  Square s;
  File f;
  Rank r;
  bool passed, isolated, doubled, opposed, chain, backward, candidate;
  Score value = SCORE_ZERO;
  const Square* ptr = pos.piece_list_begin(Us, PAWN);
  
  NEW const Square opp_ksq = pos.king_square(Them);

  // Loop through all pawns of the current color and score each pawn
  while ((s = *ptr++) != SQ_NONE)
  {
      assert(pos.piece_on(s) == make_piece(Us, PAWN));

      f = square_file(s);
      r = square_rank(s);
      
      STARTNEW
      // pawn tropism
      //if (useImprovements) {
	    value += PawnTropism[square_dist(s, opp_ksq)];
      //}
      ENDNEW

      // This file cannot be half open
      pi->halfOpenFiles[Us] &= ~(1 << f);

      // Our rank plus previous one. Used for chain detection
      b = rank_bb(r) | rank_bb(Us == WHITE ? r - Rank(1) : r + Rank(1));

      // Flag the pawn as passed, isolated, doubled or member of a pawn
      // chain (but not the backward one).
      passed   = !(theirPawns & passed_pawn_mask(Us, s));
      doubled  =   ourPawns   & squares_in_front_of(Us, s);
      opposed  =   theirPawns & squares_in_front_of(Us, s);
      isolated = !(ourPawns   & neighboring_files_bb(f));
      chain    =   ourPawns   & neighboring_files_bb(f) & b;

      // Test for backward pawn
      backward = false;

      // If the pawn is passed, isolated, or member of a pawn chain it cannot
      // be backward. If there are friendly pawns behind on neighboring files
      // or if can capture an enemy pawn it cannot be backward either.
      if (   !(passed | isolated | chain)
          && !(ourPawns & attack_span_mask(Them, s))
          && !(pos.attacks_from<PAWN>(s, Us) & theirPawns))
      {
          // We now know that there are no friendly pawns beside or behind this
          // pawn on neighboring files. We now check whether the pawn is
          // backward by looking in the forward direction on the neighboring
          // files, and seeing whether we meet a friendly or an enemy pawn first.
          b = pos.attacks_from<PAWN>(s, Us);

          // Note that we are sure to find something because pawn is not passed
          // nor isolated, so loop is potentially infinite, but it isn't.
          while (!(b & (ourPawns | theirPawns)))
              Us == WHITE ? b <<= 8 : b >>= 8;

          // The friendly pawn needs to be at least two ranks closer than the
          // enemy pawn in order to help the potentially backward pawn advance.
          backward = (b | (Us == WHITE ? b << 8 : b >> 8)) & theirPawns;
      }

      assert(opposed | passed | (attack_span_mask(Us, s) & theirPawns));

      // A not passed pawn is a candidate to become passed if it is free to
      // advance and if the number of friendly pawns beside or behind this
      // pawn on neighboring files is higher or equal than the number of
      // enemy pawns in the forward direction on the neighboring files.
      candidate =   !(opposed | passed | backward | isolated)
                 && (b = attack_span_mask(Them, s + pawn_push(Us)) & ourPawns) != EmptyBoardBB
                 &&  count_1s<Max15>(b) >= count_1s<Max15>(attack_span_mask(Us, s) & theirPawns);

      // Passed pawns will be properly scored in evaluation because we need
      // full attack info to evaluate passed pawns. Only the frontmost passed
      // pawn on each file is considered a true passed pawn.
      if (passed && !doubled)
          set_bit(&(pi->passedPawns[Us]), s);

      // Score this pawn
      OLD if (isolated)
          value -= IsolatedPawnPenalty[opposed][f];

      OLD if (doubled)
          value -= DoubledPawnPenalty[opposed][f];

      if (backward)
          value -= BackwardPawnPenalty[opposed][f];

      OLD if (chain)
          value += ChainBonus[f];

      if (candidate) {
          OLD value += CandidateBonus[relative_rank(Us, s)];
          NEW value += 3 * CandidateBonus[relative_rank(Us, s)] / 2;
      }
  }
  return value;
}


/// PawnInfo::updateShelter() calculates and caches king shelter. It is called
/// only when king square changes, about 20% of total king_shelter() calls.
template<Color Us>
Score PawnInfo::updateShelter(const Position& pos, Square ksq) {

  //OLD const int Shift = (Us == WHITE ? 8 : -8);

  Bitboard pawns;
  //OLD int r;
  int shelter = 0;

  if (relative_rank(Us, ksq) <= RANK_4)
  {
      pawns = pos.pieces(PAWN, Us) & this_and_neighboring_files_bb(ksq);
      
      
//      if (!useImprovements) {
//      
//      NEW // rank of the king, shifted left 3 (rank is indicated by 3 bits)
//      r = ksq & (7 << 3);
//      for (int i = 0; i < 3; i++)
//      {
//    	  NEW // rank of the king added 1 (white) or -1 (black), then shifted left 3
//    	  NEW // note that the relative rank is <= 3 
//    	  NEW // so we search for pawns the 4 ranks in front of the king
//          r += Shift;
//    	  
//    	  NEW // shelter counts the score
//    	  NEW // as i increases the rank in front our king r increases
//    	  NEW // we count the number of pawns in front of our king 
//    	  NEW // and scale the value by the distance
//    	  NEW // in normal chess great distance is bad, in atomic i don't know :)
//    	  NEW // tipau: it is less important to have the pawns close to the king
//          OLD shelter += BitCount8Bit[(pawns >> r) & 0xFF] * (64 >> i);
//    	  shelter += BitCount8Bit[(pawns >> r) & 0xFF] * 64;
//      }
      
      
      Color Them = opposite_color(Us);
      int rook_count = pos.piece_count(Them, ROOK) + pos.piece_count(Them, QUEEN);
      
      // pawn central in front of king?
      if (pos.pieces(PAWN, Us) & squares_in_front_of(Us, ksq)) {
    	  shelter += 60 * rook_count;
      }
      
      if (square_file(ksq) > FILE_A) {
    	  // pawn left (relative) in front of king?
    	  if (  pos.pieces(PAWN, Us) 
    		  & squares_in_front_of(Us, make_square(square_file(ksq) - File(1), square_rank(ksq)))) {
    		  shelter += 48 * rook_count;
    	  }
      } else {
    	  // ksq == FILE_A thus no shelter pawn needed
    	  shelter += 48 * rook_count;
      }
      
      if (square_file(ksq) < FILE_H) {
    	  // pawn right (relative) in front of king?
    	  if (  pos.pieces(PAWN, Us) 
    		  & squares_in_front_of(Us, make_square(square_file(ksq) + File(1), square_rank(ksq)))) {
    		  shelter += 48 * rook_count;
    	  }
      } else {
    	  // ksq == FILE_H thus no shelter pawn needed
    	  shelter += 48 * rook_count;
      }
      
      
      
      
  }
  kingSquares[Us] = ksq;
  kingShelters[Us] = make_score(shelter, 0);
  return kingShelters[Us];
}

// Explicit template instantiation
template Score PawnInfo::updateShelter<WHITE>(const Position& pos, Square ksq);
template Score PawnInfo::updateShelter<BLACK>(const Position& pos, Square ksq);
