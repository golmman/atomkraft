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
#include <iostream>
#include <iomanip>
#include <sstream>

#include "bitcount.h"
#include "evaluate.h"
#include "material.h"
#include "pawns.h"
#include "thread.h"
#include "ucioption.h"

#include "atomicdata.h"
#include "debug.h"

NEW int matDifFactor = 200;

namespace {

  // Struct EvalInfo contains various information computed and collected
  // by the evaluation functions.
  struct EvalInfo {

    // Pointers to material and pawn hash table entries
    MaterialInfo* mi;
    PawnInfo* pi;

    // attackedBy[color][piece type] is a bitboard representing all squares
    // attacked by a given color and piece type, attackedBy[color][0] contains
    // all squares attacked by the given color.
    Bitboard attackedBy[2][8];

    // kingZone[color] is the zone around the enemy king which is considered
    // by the king safety evaluation. This consists of the squares directly
    // adjacent to the king, and the three (or two, for a king on an edge file)
    // squares two ranks in front of the king. For instance, if black's king
    // is on g8, kingZone[WHITE] is a bitboard containing the squares f8, h8,
    // f7, g7, h7, f6, g6 and h6.
    Bitboard kingZone[2];

    // kingAttackersCount[color] is the number of pieces of the given color
    // which attack a square in the kingZone of the enemy king.
    int kingAttackersCount[2];

    // kingAttackersWeight[color] is the sum of the "weight" of the pieces of the
    // given color which attack a square in the kingZone of the enemy king. The
    // weights of the individual piece types are given by the variables
    // QueenAttackWeight, RookAttackWeight, BishopAttackWeight and
    // KnightAttackWeight in evaluate.cpp
    int kingAttackersWeight[2];

    // kingAdjacentZoneAttacksCount[color] is the number of attacks to squares
    // directly adjacent to the king of the given color. Pieces which attack
    // more than one square are counted multiple times. For instance, if black's
    // king is on g8 and there's a white knight on g5, this knight adds
    // 2 to kingAdjacentZoneAttacksCount[BLACK].
    int kingAdjacentZoneAttacksCount[2];
    
    
    STARTNEW
    // the same as halfOpenFiles in PawnInfo but for minor pieces
    // rooks are often blocked by bishops or knights so only give the
    // full open file bonus if not blocked by minor pieces
    int halfOpenFiles_mp[2];
    
    int file_is_half_open_mp(Color c, File f) const;
    ENDNEW
  };
  
  STARTNEW
  inline int EvalInfo::file_is_half_open_mp(Color c, File f) const {
    return halfOpenFiles_mp[c] & (1 << int(f));
  }
  ENDNEW

  // Evaluation grain size, must be a power of 2
  const int GrainSize = 8;

  // Evaluation weights, initialized from UCI options
  enum { Mobility, PassedPawns, Space, KingDangerUs, KingDangerThem };
  Score Weights[6];

  typedef Value V;
  #define S(mg, eg) make_score(mg, eg)

  // Internal evaluation weights. These are applied on top of the evaluation
  // weights read from UCI parameters. The purpose is to be able to change
  // the evaluation weights while keeping the default values of the UCI
  // parameters at 100, which looks prettier.
  //
  // Values modified by Joona Kiiski
  const Score WeightsInternal[] = {
      S(248, 271), S(252, 259), S(46, 0), S(247, 0), S(259, 0)
  };

  // MobilityBonus[PieceType][attacked] contains mobility bonuses for middle and
  // end game, indexed by piece type and number of attacked squares not occupied
  // by friendly pieces.
  const Score MobilityBonus[][32] = {
     {}, {},
     { S(-38,-33), S(-25,-23), S(-12,-13), S( 0, -3), S(12,  7), S(25, 17), // Knights
       S( 31, 22), S( 38, 27), S( 38, 27) },
     { S(-25,-30), S(-11,-16), S(  3, -2), S(17, 12), S(31, 26), S(45, 40), // Bishops
       S( 57, 52), S( 65, 60), S( 71, 65), S(74, 69), S(76, 71), S(78, 73),
       S( 79, 74), S( 80, 75), S( 81, 76), S(81, 76) },
     { S(-20,-36), S(-14,-19), S( -8, -3), S(-2, 13), S( 4, 29), S(10, 46), // Rooks
       S( 14, 62), S( 19, 79), S( 23, 95), S(26,106), S(27,111), S(28,114),
       S( 29,116), S( 30,117), S( 31,118), S(32,118) },
       
       
//     { S(-10,-18), S( -8,-13), S( -6, -7), S(-3, -2), S(-1,  3), S( 1,  8), // Queens
//       S(  3, 13), S(  5, 19), S(  8, 23), S(10, 27), S(12, 32), S(15, 34),
//       S( 16, 35), S( 17, 35), S( 18, 35), S(20, 35), S(20, 35), S(20, 35),
//       S( 20, 35), S( 20, 35), S( 20, 35), S(20, 35), S(20, 35), S(20, 35),
//       S( 20, 35), S( 20, 35), S( 20, 35), S(20, 35), S(20, 35), S(20, 35),
//       S( 20, 35), S( 20, 35) }
     
     { S(-40,-40), S(-30,-30), S(-20,-20), S(-10,-10), S(-1,  3), S( 1,  8), // Queens
       S(  3, 13), S(  5, 19), S(  8, 23), S(10, 27), S(12, 32), S(15, 34),
       S( 16, 35), S( 17, 35), S( 18, 35), S(20, 35), S(20, 35), S(20, 35),
       S( 20, 35), S( 20, 35), S( 20, 35), S(20, 35), S(20, 35), S(20, 35),
       S( 20, 35), S( 20, 35), S( 20, 35), S(20, 35), S(20, 35), S(20, 35),
       S( 20, 35), S( 20, 35) }
  };

  // OutpostBonus[PieceType][Square] contains outpost bonuses of knights and
  // bishops, indexed by piece type and square (from white's point of view).
  const Value OutpostBonus[][64] = {
  {
  //  A     B     C     D     E     F     G     H
    V(0), V(0), V(0), V(0), V(0), V(0), V(0), V(0), // Knights
    V(0), V(0), V(0), V(0), V(0), V(0), V(0), V(0),
    V(0), V(0), V(4), V(8), V(8), V(4), V(0), V(0),
    V(0), V(4),V(17),V(26),V(26),V(17), V(4), V(0),
    V(0), V(8),V(26),V(35),V(35),V(26), V(8), V(0),
    V(0), V(4),V(17),V(17),V(17),V(17), V(4), V(0) },
  {
    V(0), V(0), V(0), V(0), V(0), V(0), V(0), V(0), // Bishops
    V(0), V(0), V(0), V(0), V(0), V(0), V(0), V(0),
    V(0), V(0), V(5), V(5), V(5), V(5), V(0), V(0),
    V(0), V(5),V(10),V(10),V(10),V(10), V(5), V(0),
    V(0),V(10),V(21),V(21),V(21),V(21),V(10), V(0),
    V(0), V(5), V(8), V(8), V(8), V(8), V(5), V(0) }
  };

  // ThreatBonus[attacking][attacked] contains threat bonuses according to
  // which piece type attacks which one.
  const Score ThreatBonus[][8] = {
    {}, {},
    { S(0, 0), S( 7, 39), S( 0,  0), S(24, 49), S(41,100), S(41,100) }, // KNIGHT
    { S(0, 0), S( 7, 39), S(24, 49), S( 0,  0), S(41,100), S(41,100) }, // BISHOP
    { S(0, 0), S(-1, 29), S(15, 49), S(15, 49), S( 0,  0), S(24, 49) }, // ROOK
    { S(0, 0), S(15, 39), S(15, 39), S(15, 39), S(15, 39), S( 0,  0) }  // QUEEN
  };

  // ThreatenedByPawnPenalty[PieceType] contains a penalty according to which
  // piece type is attacked by an enemy pawn.
//  const Score ThreatenedByPawnPenalty[] = {
//    S(0, 0), S(0, 0), S(56, 70), S(56, 70), S(76, 99), S(86, 118)
//  };
  STARTNEW
  // material is far less concerning in atomic chess, so everything / 3
  const Score ThreatenedByPawnPenalty[] = {
		  S(0, 0), S(0, 0), S(18, 23), S(18, 23), S(25, 33), S(28, 39)
  };
  ENDNEW

  #undef S

  // Rooks and queens on the 7th rank (modified by Joona Kiiski)
  const Score RookOn7thBonus  = make_score(47, 98);
  const Score QueenOn7thBonus = make_score(27, 54);

  // Rooks on open files (modified by Joona Kiiski)
  //OLD const Score RookOpenFileBonus = make_score(43, 43);
  //OLD const Score RookHalfOpenFileBonus = make_score(19, 19);
  NEW const Score RookOpenFileBonus = make_score(100, 100);
  NEW const Score RookHalfOpenFileBonus = make_score(50, 50);
  NEW const Score RookAntiHalfOpenFileBonus = make_score(60, 0);

  // Penalty for rooks trapped inside a friendly king which has lost the
  // right to castle.
  const Value TrappedRookPenalty = Value(180);

  // Penalty for a bishop on a1/h1 (a8/h8 for black) which is trapped by
  // a friendly pawn on b2/g2 (b7/g7 for black). This can obviously only
  // happen in Chess960 games.
  const Score TrappedBishopA1H1Penalty = make_score(100, 100);

  // The SpaceMask[Color] contains the area of the board which is considered
  // by the space evaluation. In the middle game, each side is given a bonus
  // based on how many squares inside this area are safe and available for
  // friendly minor pieces.
  const Bitboard SpaceMask[] = {
    (1ULL << SQ_C2) | (1ULL << SQ_D2) | (1ULL << SQ_E2) | (1ULL << SQ_F2) |
    (1ULL << SQ_C3) | (1ULL << SQ_D3) | (1ULL << SQ_E3) | (1ULL << SQ_F3) |
    (1ULL << SQ_C4) | (1ULL << SQ_D4) | (1ULL << SQ_E4) | (1ULL << SQ_F4),
    (1ULL << SQ_C7) | (1ULL << SQ_D7) | (1ULL << SQ_E7) | (1ULL << SQ_F7) |
    (1ULL << SQ_C6) | (1ULL << SQ_D6) | (1ULL << SQ_E6) | (1ULL << SQ_F6) |
    (1ULL << SQ_C5) | (1ULL << SQ_D5) | (1ULL << SQ_E5) | (1ULL << SQ_F5)
  };

  // King danger constants and variables. The king danger scores are taken
  // from the KingDangerTable[]. Various little "meta-bonuses" measuring
  // the strength of the enemy attack are added up into an integer, which
  // is used as an index to KingDangerTable[].
  //
  // KingAttackWeights[PieceType] contains king attack weights by piece type
  const int KingAttackWeights[] = { 0, 0, 2, 2, 3, 5 };

  // Bonuses for enemy's safe checks
  //STARTOLD
//  const int QueenContactCheckBonus = 6;
//  const int RookContactCheckBonus  = 4;
//  const int QueenCheckBonus        = 3;
//  const int RookCheckBonus         = 2;
//  const int BishopCheckBonus       = 1;
//  const int KnightCheckBonus       = 1;
  //ENDOLD
  STARTNEW
  const int QueenContactCheckBonus = 12;
  const int RookContactCheckBonus  = 8;
  const int QueenCheckBonus        = 3;
  const int RookCheckBonus         = 2;
  const int BishopCheckBonus       = 1;
  const int KnightCheckBonus       = 1;
  ENDNEW
  
  NEW const int AdjacencyPenalty = -5;
  
  // InitKingDanger[Square] contains penalties based on the position of the
  // defending king, indexed by king's square (from white's point of view).
  const int InitKingDanger[] = {
     2,  0,  2,  5,  5,  2,  0,  2,
     2,  2,  4,  8,  8,  4,  2,  2,
     7, 10, 12, 12, 12, 12, 10,  7,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15
  };

  // KingDangerTable[Color][attackUnits] contains the actual king danger
  // weighted scores, indexed by color and by a calculated integer number.
  Score KingDangerTable[2][128];

  // TracedTerms[Color][PieceType || TracedType] contains a breakdown of the
  // evaluation terms, used when tracing.
  Score TracedScores[2][16];
  std::stringstream TraceStream;

  enum TracedType {
      PST = 8, IMBALANCE = 9, MOBILITY = 10, THREAT = 11,
      PASSED = 12, UNSTOPPABLE = 13, SPACE = 14, TOTAL = 15
  };

  // Function prototypes
  template<bool HasPopCnt, bool Trace>
  Value do_evaluate(const Position& pos, Value& margin, bool* expl_threat);

  template<Color Us, bool HasPopCnt>
  void init_eval_info(const Position& pos, EvalInfo& ei);

  template<Color Us, bool HasPopCnt, bool Trace>
  Score evaluate_pieces_of_color(const Position& pos, EvalInfo& ei, Score& mobility);

  template<Color Us, bool HasPopCnt, bool Trace>
  Score evaluate_king(const Position& pos, EvalInfo& ei, Value margins[]);

  template<Color Us>
  Score evaluate_threats(const Position& pos, EvalInfo& ei);

  template<Color Us, bool HasPopCnt>
  int evaluate_space(const Position& pos, EvalInfo& ei);

  template<Color Us>
  Score evaluate_passed_pawns(const Position& pos, EvalInfo& ei);

  template<bool HasPopCnt>
  Score evaluate_unstoppable_pawns(const Position& pos, EvalInfo& ei);

  inline Score apply_weight(Score v, Score weight);
  Value scale_by_game_phase(const Score& v, Phase ph, ScaleFactor sf);
  Score weight_option(const std::string& mgOpt, const std::string& egOpt, Score internalWeight);
  void init_safety();
  double to_cp(Value v);
  void trace_add(int idx, Score term_w, Score term_b = SCORE_ZERO);
}


/// evaluate() is the main evaluation function. It always computes two
/// values, an endgame score and a middle game score, and interpolates
/// between them based on the remaining material.
NEW // the argument bool* expl_threat is set to true if the side to move has to react
NEW // on an explosion threat by the enemey else it is set to false
Value evaluate(const Position& pos, Value& margin, bool* expl_threat) {
  NEW assert(pos.piece_count(pos.side_to_move(), KING));
  
  //NEW // if there is no king anymore, we are check mated
  //NEW if (pos.piece_count(pos.side_to_move(), KING) == 0) return VALUE_MATED_IN_PLY_MAX;
  
  return CpuHasPOPCNT ? do_evaluate<true, false>(pos, margin, expl_threat)
                      : do_evaluate<false, false>(pos, margin, expl_threat);
}

namespace {

template<bool HasPopCnt, bool Trace>
Value do_evaluate(const Position& pos, Value& margin, bool* expl_threat) {

  NEW *expl_threat = false;
  
  
  
  EvalInfo ei;
  Value margins[2];
  Score score, mobilityWhite, mobilityBlack;

  assert(pos.is_ok());
  assert(pos.thread() >= 0 && pos.thread() < MAX_THREADS);
  assert(!pos.in_check());

  // Initialize score by reading the incrementally updated scores included
  // in the position object (material + piece square tables).
  score = pos.value();

  // margins[] store the uncertainty estimation of position's evaluation
  // that typically is used by the search for pruning decisions.
  margins[WHITE] = margins[BLACK] = VALUE_ZERO;

  // Probe the material hash table
  ei.mi = Threads[pos.thread()].materialTable.get_material_info(pos);
  score += ei.mi->material_value();
  
  STARTNEW
  // in the endgame just the difference in material count is important
  //Color us = pos.side_to_move();
  //Color them = opposite_color(us);
  int mat_dif;
  
  if (HasPopCnt) {
	  mat_dif = 
			  count_1s<CNT_POPCNT>(pos.pieces_of_color(WHITE))
			- count_1s<CNT_POPCNT>(pos.pieces_of_color(BLACK));
  } else {
	  mat_dif = 
			  pos.piece_count(WHITE, PAWN)     + pos.piece_count(WHITE, KNIGHT)
			+ pos.piece_count(WHITE, BISHOP)   + pos.piece_count(WHITE, ROOK)
			+ pos.piece_count(WHITE, QUEEN)
			- pos.piece_count(BLACK, PAWN)     - pos.piece_count(BLACK, KNIGHT)
			- pos.piece_count(BLACK, BISHOP)   - pos.piece_count(BLACK, ROOK)
			- pos.piece_count(BLACK, QUEEN);
  }
  
  //if (useImprovements) {
	//  score += make_score(mat_dif * PawnValueMidgame / 3, mat_dif * 2 * PawnValueMidgame);
  //} else {
	  score += make_score(mat_dif * matDifFactor / 4, mat_dif * matDifFactor);
  //}

  ENDNEW

  NEW // material evaluation recognizes some endgame patterns for chess
  
  // If we have a specialized evaluation function for the current material
  // configuration, call it and return.
  if (ei.mi->specialized_eval_exists())
  {
      margin = VALUE_ZERO;
      OLD return ei.mi->evaluate(pos);
      
      STARTNEW
      Value special_eval = ei.mi->evaluate(pos);
      if (special_eval != VALUE_NONE) {
    	  // some new special evaluations may return VALUE_NONE if 
    	  // the outcome of the game is not clear
    	  return special_eval;
      }
      ENDNEW
  }
  

  // Probe the pawn hash table
  ei.pi = Threads[pos.thread()].pawnTable.get_pawn_info(pos);
  score += ei.pi->pawns_value();

  // Initialize attack and king safety bitboards
  init_eval_info<WHITE, HasPopCnt>(pos, ei);
  init_eval_info<BLACK, HasPopCnt>(pos, ei);

  // Evaluate pieces and mobility
  score +=  evaluate_pieces_of_color<WHITE, HasPopCnt, Trace>(pos, ei, mobilityWhite)
          - evaluate_pieces_of_color<BLACK, HasPopCnt, Trace>(pos, ei, mobilityBlack);
  
  STARTNEW
  // evaluate_pieces_of_color fills the attackedBy arrays
  
  // create a shortcut if we can explode opponents king
  Color us_aux = pos.side_to_move();
  Color them_aux = opposite_color(us_aux);
  Bitboard expl_threat_squares
  	  = ei.attackedBy[us_aux][0] & ei.attackedBy[them_aux][KING] & pos.pieces_of_color(them_aux);

  if (expl_threat_squares) {
	  return VALUE_KNOWN_WIN;
  }
  
  // compute the explosion thread
  expl_threat_squares
  	  = ei.attackedBy[them_aux][0] & ei.attackedBy[us_aux][KING] & pos.pieces_of_color(us_aux);
  *expl_threat = (expl_threat_squares != 0ULL);
  ENDNEW

  score += apply_weight(mobilityWhite - mobilityBlack, Weights[Mobility]);

  // Evaluate kings after all other pieces because we need complete attack
  // information when computing the king safety evaluation.
  score +=  evaluate_king<WHITE, HasPopCnt, Trace>(pos, ei, margins)
          - evaluate_king<BLACK, HasPopCnt, Trace>(pos, ei, margins);

  // Evaluate tactical threats, we need full attack information including king
  score +=  evaluate_threats<WHITE>(pos, ei)
          - evaluate_threats<BLACK>(pos, ei);

  // Evaluate passed pawns, we need full attack information including king
  score +=  evaluate_passed_pawns<WHITE>(pos, ei)
          - evaluate_passed_pawns<BLACK>(pos, ei);

  // If one side has only a king, check whether exists any unstoppable passed pawn
  if (!pos.non_pawn_material(WHITE) || !pos.non_pawn_material(BLACK))
      score += evaluate_unstoppable_pawns<HasPopCnt>(pos, ei);

  // Evaluate space for both sides, only in middle-game.
  if (ei.mi->space_weight())
  {
      int s = evaluate_space<WHITE, HasPopCnt>(pos, ei) - evaluate_space<BLACK, HasPopCnt>(pos, ei);
      score += apply_weight(make_score(s * ei.mi->space_weight(), 0), Weights[Space]);
  }

  // Scale winning side if position is more drawish that what it appears
  NEW // this cannot be trusted
  //NEW ScaleFactor sf = SCALE_FACTOR_NORMAL;
  //OLD ScaleFactor sf = eg_value(score) > VALUE_DRAW ? ei.mi->scale_factor(pos, WHITE)
                                                //: ei.mi->scale_factor(pos, BLACK);
  
  
  STARTNEW
  ScaleFactor sf = SCALE_FACTOR_NORMAL;
  
  sf = eg_value(score) > VALUE_DRAW ? ei.mi->scale_factor(pos, WHITE)
		  : ei.mi->scale_factor(pos, BLACK);

  // if there is no material difference the game is drawish
  // calculated here because we already have mat_dif
  if (   ei.mi->game_phase() < PHASE_MIDGAME 
		  && sf == SCALE_FACTOR_NORMAL
		  && mat_dif == 0) {

	  if (eg_value(score) > VALUE_DRAW) {
		  if (pos.non_pawn_material(WHITE) <= RookValueMidgame) {
			  sf = ScaleFactor(32);
		  } else if (pos.non_pawn_material(WHITE) <= KnightValueMidgame) {
			  sf = ScaleFactor(16);
		  }
	  } else {
		  if (pos.non_pawn_material(BLACK) <= RookValueMidgame) {
			  sf = ScaleFactor(32);
		  } else if (pos.non_pawn_material(BLACK) <= KnightValueMidgame) {
			  sf = ScaleFactor(16);
		  }
	  } 
  }
  
  // adjacent kings make the game hard to win
  static const float KING_DIST_SF[] = {0.0f, 0.5f, 0.7f, 0.8f, 0.9f, 1.0f, 1.0f, 1.0f};
  
  int k_dist = square_dist(pos.king_square(WHITE), pos.king_square(BLACK));
  if (ei.mi->game_phase() == PHASE_ENDGAME) {
	  sf = ScaleFactor(int(float(sf) * KING_DIST_SF[k_dist]));
  }
  
  ENDNEW
  
  // If we don't already have an unusual scale factor, check for opposite
  // colored bishop endgames, and use a lower scale for those.
  NEW // this cannot be trusted
  STARTOLD
  if (   ei.mi->game_phase() < PHASE_MIDGAME
      && pos.opposite_colored_bishops()
      && sf == SCALE_FACTOR_NORMAL)
  {
      // Only the two bishops ?
      if (   pos.non_pawn_material(WHITE) == BishopValueMidgame
          && pos.non_pawn_material(BLACK) == BishopValueMidgame)
      {
          // Check for KBP vs KB with only a single pawn that is almost
          // certainly a draw or at least two pawns.
          bool one_pawn = (pos.piece_count(WHITE, PAWN) + pos.piece_count(BLACK, PAWN) == 1);
          sf = one_pawn ? ScaleFactor(8) : ScaleFactor(32);
      }
      else
          // Endgame with opposite-colored bishops, but also other pieces. Still
          // a bit drawish, but not as drawish as with only the two bishops.
           sf = ScaleFactor(50);
  }
  ENDOLD

  // Interpolate between the middle game and the endgame score
  margin = margins[pos.side_to_move()];
  Value v = scale_by_game_phase(score, ei.mi->game_phase(), sf);
  

  
  STARTNEW
  // if we are bad in the endgame maybe we can force a draw by touching the opposite king
  
  ENDNEW

  // In case of tracing add all single evaluation contributions for both white and black
  if (Trace)
  {
      trace_add(PST, pos.value());
      trace_add(IMBALANCE, ei.mi->material_value());
      trace_add(PAWN, ei.pi->pawns_value());
      trace_add(MOBILITY, apply_weight(mobilityWhite, Weights[Mobility]), apply_weight(mobilityBlack, Weights[Mobility]));
      trace_add(THREAT, evaluate_threats<WHITE>(pos, ei), evaluate_threats<BLACK>(pos, ei));
      trace_add(PASSED, evaluate_passed_pawns<WHITE>(pos, ei), evaluate_passed_pawns<BLACK>(pos, ei));
      trace_add(UNSTOPPABLE, evaluate_unstoppable_pawns<false>(pos, ei));
      Score w = make_score(ei.mi->space_weight() * evaluate_space<WHITE, false>(pos, ei), 0);
      Score b = make_score(ei.mi->space_weight() * evaluate_space<BLACK, false>(pos, ei), 0);
      trace_add(SPACE, apply_weight(w, Weights[Space]), apply_weight(b, Weights[Space]));
      trace_add(TOTAL, score);
      TraceStream << "\nUncertainty margin: White: " << to_cp(margins[WHITE])
                  << ", Black: " << to_cp(margins[BLACK])
                  << "\nScaling: " << std::noshowpos
                  << std::setw(6) << 100.0 * ei.mi->game_phase() / 128.0 << "% MG, "
                  << std::setw(6) << 100.0 * (1.0 - ei.mi->game_phase() / 128.0) << "% * "
                  << std::setw(6) << (100.0 * sf) / SCALE_FACTOR_NORMAL << "% EG.\n"
                  << "Total evaluation: " << to_cp(v);
  }

  return pos.side_to_move() == WHITE ? v : -v;
}

} // namespace


/// read_weights() reads evaluation weights from the corresponding UCI parameters

void read_evaluation_uci_options(Color us) {

  // King safety is asymmetrical. Our king danger level is weighted by
  // "Cowardice" UCI parameter, instead the opponent one by "Aggressiveness".
  const int kingDangerUs   = (us == WHITE ? KingDangerUs   : KingDangerThem);
  const int kingDangerThem = (us == WHITE ? KingDangerThem : KingDangerUs);

  Weights[Mobility]       = weight_option("Mobility (Middle Game)", "Mobility (Endgame)", WeightsInternal[Mobility]);
  Weights[PassedPawns]    = weight_option("Passed Pawns (Middle Game)", "Passed Pawns (Endgame)", WeightsInternal[PassedPawns]);
  Weights[Space]          = weight_option("Space", "Space", WeightsInternal[Space]);
  Weights[kingDangerUs]   = weight_option("Cowardice", "Cowardice", WeightsInternal[KingDangerUs]);
  Weights[kingDangerThem] = weight_option("Aggressiveness", "Aggressiveness", WeightsInternal[KingDangerThem]);

  // If running in analysis mode, make sure we use symmetrical king safety. We do this
  // by replacing both Weights[kingDangerUs] and Weights[kingDangerThem] by their average.
  if (Options["UCI_AnalyseMode"].value<bool>())
      Weights[kingDangerUs] = Weights[kingDangerThem] = (Weights[kingDangerUs] + Weights[kingDangerThem]) / 2;

  init_safety();
}


namespace {

  // init_eval_info() initializes king bitboards for given color adding
  // pawn attacks. To be done at the beginning of the evaluation.

  template<Color Us, bool HasPopCnt>
  void init_eval_info(const Position& pos, EvalInfo& ei) {

    const BitCountType Max15 = HasPopCnt ? CNT_POPCNT : CpuIs64Bit ? CNT64_MAX15 : CNT32_MAX15;
    const Color Them = (Us == WHITE ? BLACK : WHITE);

    Bitboard b = ei.attackedBy[Them][KING] = pos.attacks_from<KING>(pos.king_square(Them));
    ei.attackedBy[Us][PAWN] = ei.pi->pawn_attacks(Us);
    
    STARTNEW
    // init halfOpenFiles for minor pieces
	// this is done analog to pi->halfOpenFiles
    ei.halfOpenFiles_mp[WHITE] = ei.halfOpenFiles_mp[BLACK] = 0xFF;
    
    Square s;
    
    const Square* ptr = pos.piece_list_begin(Us, KNIGHT);
    while ((s = *ptr++) != SQ_NONE) {
		ei.halfOpenFiles_mp[Us] &= ~(1 << square_file(s));
    }
    
    ptr = pos.piece_list_begin(Us, BISHOP);
    while ((s = *ptr++) != SQ_NONE) {
		ei.halfOpenFiles_mp[Us] &= ~(1 << square_file(s));
    }
    ENDNEW

    // Init king safety tables only if we are going to use them
    if (   pos.piece_count(Us, QUEEN)
        && pos.non_pawn_material(Us) >= QueenValueMidgame + RookValueMidgame)
    {
        ei.kingZone[Us] = (b | (Us == WHITE ? b >> 8 : b << 8));
        b &= ei.attackedBy[Us][PAWN];
        ei.kingAttackersCount[Us] = b ? count_1s<Max15>(b) / 2 : 0;
        ei.kingAdjacentZoneAttacksCount[Us] = ei.kingAttackersWeight[Us] = 0;
    } else
        ei.kingZone[Us] = ei.kingAttackersCount[Us] = 0;
  }


  // evaluate_outposts() evaluates bishop and knight outposts squares

  template<PieceType Piece, Color Us>
  Score evaluate_outposts(const Position& pos, EvalInfo& ei, Square s) {

    const Color Them = (Us == WHITE ? BLACK : WHITE);

    assert (Piece == BISHOP || Piece == KNIGHT);
    
    STARTNEW
    
    Value bonus = Value(0);
    
//    if (useImprovements) {
//    
//    if (explBB[s] & pos.pieces(KING, Them)) {
//    	bonus = Value(30);
//    }
//    
//    }
    
//    if (explBB[s] & (pos.pieces_of_color(Them) & ~pos.pieces(PAWN, Them))) {
//    	// outpost can't be removed without exploding their pieces
//    	bonus = OutpostBonus[Piece == BISHOP][relative_square(Us, s)];
//    	
//    	if (explBB[s] & pos.pieces(KING, Them)) {
//    		// piece is adjacent to opponents king
//    		bonus += bonus;
//    	} else if (explBB[s] & (pos.pieces_of_color(Us) & ~pos.pieces(PAWN, Us))) {
//    		// lower the bonus if our pieces are around outpost
//    		bonus /= 2;
//    	}
//    }
    ENDNEW
    
    
    
    STARTOLD
    // Initial bonus based on square
    Value bonus = OutpostBonus[Piece == BISHOP][relative_square(Us, s)];
    
    NEW // in atomic chess there is no such kind of 'support'
    
    // Increase bonus if supported by pawn, especially if the opponent has
    // no minor piece which can exchange the outpost piece.
    if (bonus && bit_is_set(ei.attackedBy[Us][PAWN], s))
    {
        if (    pos.pieces(KNIGHT, Them) == EmptyBoardBB
            && (SquaresByColorBB[square_color(s)] & pos.pieces(BISHOP, Them)) == EmptyBoardBB)
            bonus += bonus + bonus / 2;
        else
            bonus += bonus / 2;
    }
    ENDOLD
    
    return make_score(bonus, bonus);
  }


  // evaluate_pieces<>() assigns bonuses and penalties to the pieces of a given color

  template<PieceType Piece, Color Us, bool HasPopCnt, bool Trace>
  Score evaluate_pieces(const Position& pos, EvalInfo& ei, Score& mobility, Bitboard mobilityArea) {
	  
	//NEW Bitboard captures_bb;
	//NEW Square s_aux;
	//NEW Value best_capture;
    Bitboard b;
    Square s, ksq;
    int mob;
    File f;
    Score score = SCORE_ZERO;

    const BitCountType Full  = HasPopCnt ? CNT_POPCNT : CpuIs64Bit ? CNT64 : CNT32;
    const BitCountType Max15 = HasPopCnt ? CNT_POPCNT : CpuIs64Bit ? CNT64_MAX15 : CNT32_MAX15;
    const Color Them = (Us == WHITE ? BLACK : WHITE);
    const Square* ptr = pos.piece_list_begin(Us, Piece);

    ei.attackedBy[Us][Piece] = EmptyBoardBB;

    while ((s = *ptr++) != SQ_NONE)
    {
        // Find attacked squares, including x-ray attacks for bishops and rooks
        if (Piece == KNIGHT || Piece == QUEEN)
            b = pos.attacks_from<Piece>(s);
        else if (Piece == BISHOP)
            b = bishop_attacks_bb(s, pos.occupied_squares() & ~pos.pieces(QUEEN, Us));
        else if (Piece == ROOK)
            b = rook_attacks_bb(s, pos.occupied_squares() & ~pos.pieces(ROOK, QUEEN, Us));
        else
            assert(false);
        
        
        //NEW captures_bb = b & pos.pieces_of_color(Them);
        

        // Update attack info
        ei.attackedBy[Us][Piece] |= b;

        // King attacks
        if (b & ei.kingZone[Us])
        {
            ei.kingAttackersCount[Us]++;
            ei.kingAttackersWeight[Us] += KingAttackWeights[Piece];
            Bitboard bb = (b & ei.attackedBy[Them][KING]);
            if (bb)
                ei.kingAdjacentZoneAttacksCount[Us] += count_1s<Max15>(bb);
        }

        // Mobility
        STARTOLD
        mob = (Piece != QUEEN ? count_1s<Max15>(b & mobilityArea)
                              : count_1s<Full >(b & mobilityArea));
        
        mobility += MobilityBonus[Piece][mob];
        ENDOLD
        
        STARTNEW
        mob = (Piece != QUEEN ? count_1s<Max15>(b & mobilityArea)
                              : count_1s<Full >(b & mobilityArea & ~pos.pieces_of_color(Them)));
        // seems to be good
        mobility += 2 * MobilityBonus[Piece][mob];
        
        if (Piece == QUEEN) {
        	mobility += MobilityBonus[Piece][mob];
        }
        ENDNEW
        
        
//        NEW if (useImprovements) {
//        NEW		mobility += 2 * MobilityBonus[Piece][mob];
//        NEW} else {
//        	mobility += MobilityBonus[Piece][mob];
//        NEW}
        
        STARTNEW
        /*
         * this is strong but it slows down the search a lot
         */
//        Value value = VALUE_ZERO;
//        best_capture = VALUE_ZERO;
//        
//        //if (useImprovements) {
//        
//        // calculate the current pieces best capture move value
//        while (captures_bb) {
//            s_aux = pop_1st_bit(&captures_bb);
//            
//            value = pos.midgame_value_of_piece_on(s_aux) - pos.midgame_value_of_piece_on(s);
//            
//            for (int k = 1; k < explSq[s].size; ++k) {
//            	if (pos.type_of_piece_on(explSq[s].square[k]) != PAWN) {
//
//            		if (pos.color_of_piece_on(explSq[s].square[k]) == Us) {
//            			value -= pos.midgame_value_of_piece_on(explSq[s].square[k]);
//            		} else if (pos.color_of_piece_on(explSq[s].square[k]) == Them) {
//            			value += pos.midgame_value_of_piece_on(explSq[s].square[k]);
//            		}
//
//            	}
//            }
//            
//            if (best_capture < value) {
//            	best_capture = value;
//            }
//        }
//        score += make_score(best_capture / 10, best_capture / 10);
//        
//        //}
//		
//        //if (!useImprovements) {
//        	
//        // more generally: measure the value of the cluster which is affected if explosion occurs
//        // this is done similiar to pos::see
//
//        value = VALUE_ZERO;
//        for (int k = 1; k < explSq[s].size; ++k) {
//        	if (pos.type_of_piece_on(explSq[s].square[k]) != PAWN) {
//
//        		if (pos.color_of_piece_on(explSq[s].square[k]) == Us) {
//        			value -= pos.midgame_value_of_piece_on(explSq[s].square[k]);
//        		} else if (pos.color_of_piece_on(explSq[s].square[k]) == Them) {
//        			value += pos.midgame_value_of_piece_on(explSq[s].square[k]);
//        		}
//
//        	}
//        }
//
//        value /= 25; // 50 ~ 37.3%, 25 ~ 38.6%, 15 ~ >30%
//        score += make_score(value, value);
        
//        } else {
//		
//			// penalty for every own piece adjacent to the piece
//			Bitboard adjacent_bb = pos.attacks_from(WK, s) & pos.pieces_of_color(Us); 
//			if (adjacent_bb) {
//				int penalty = count_1s<Max15>(adjacent_bb) * AdjacencyPenalty;
//				score += make_score(penalty, penalty);
//			}
//			
//        }
		
        ENDNEW

        // Decrease score if we are attacked by an enemy pawn. Remaining part
        // of threat evaluation must be done later when we have full attack info.
        if (bit_is_set(ei.attackedBy[Them][PAWN], s))
            score -= ThreatenedByPawnPenalty[Piece];

        // Bishop and knight outposts squares
        OLD if ((Piece == BISHOP || Piece == KNIGHT) && pos.square_is_weak(s, Us))
            score += evaluate_outposts<Piece, Us>(pos, ei, s);
        
        STARTNEW
        if ((Piece == BISHOP || Piece == KNIGHT)) {
        	score += evaluate_outposts<Piece, Us>(pos, ei, s);
        }            
        ENDNEW
        
        STARTOLD
        // Queen or rook on 7th rank
        if (  (Piece == ROOK || Piece == QUEEN)
            && relative_rank(Us, s) == RANK_7
            && relative_rank(Us, pos.king_square(Them)) == RANK_8)
        {
            score += (Piece == ROOK ? RookOn7thBonus : QueenOn7thBonus);
        }
        ENDOLD

        // Special extra evaluation for bishops
        if (Piece == BISHOP && pos.is_chess960())
        {
            // An important Chess960 pattern: A cornered bishop blocked by
            // a friendly pawn diagonally in front of it is a very serious
            // problem, especially when that pawn is also blocked.
            if (s == relative_square(Us, SQ_A1) || s == relative_square(Us, SQ_H1))
            {
                Square d = pawn_push(Us) + (square_file(s) == FILE_A ? DELTA_E : DELTA_W);
                if (pos.piece_on(s + d) == make_piece(Us, PAWN))
                {
                    if (!pos.square_is_empty(s + d + pawn_push(Us)))
                        score -= 2*TrappedBishopA1H1Penalty;
                    else if (pos.piece_on(s + 2*d) == make_piece(Us, PAWN))
                        score -= TrappedBishopA1H1Penalty;
                    else
                        score -= TrappedBishopA1H1Penalty / 2;
                }
            }
        }
        
        
        STARTNEW
        
        static const int QUEEN_TROPISM_BONUS[] = {0, 250, 100, 33, 12, 3, 1, 0, 0};
        
        if (Piece == QUEEN) {
        	// queen tropism
        	ksq = pos.king_square(Them);
        	if (square_dist(ksq, pos.king_square(Us)) > 1) {
				int queen_tropism = QUEEN_TROPISM_BONUS[square_dist(s, ksq)];
				score += make_score(queen_tropism, queen_tropism);
        	}
        	
        	
//        	// anti half open file
//        	if (relative_rank(Us, s) <= RANK_2) {
//				f = square_file(s);
//				if (!ei.pi->file_is_half_open(Us, f) && ei.pi->file_is_half_open(Them, f)) {
//					
//					score += make_score(30, 0);
//					if (ei.file_is_half_open_mp(Them, f)) {
//						score += make_score(30, 0);
//					}
//	 
//				}
//        	}
        	
        	
        }
        ENDNEW
        
        
        // Special extra evaluation for rooks
        if (Piece == ROOK)
        {
            // Open and half-open files
            f = square_file(s);
            
            
            // seems to be bad
//            STARTNEW
//            int fiho_us = ei.pi->file_is_half_open(Us, f);
//            int fiho_them = ei.pi->file_is_half_open(Them, f);
//            int fiho_mp = ei.file_is_half_open_mp(Them, f);
//            bool rook_rank_3or4or5or6 =    (square_rank(s) >= RANK_3)
//            							&& (square_rank(s) <= RANK_6);
//            
//            if (rook_rank_3or4or5or6 && useImprovements) {
//            	// if the rooks rank is >= 3 there is a chance that it is in front of obstacles
//            	// rooks on 7th or 8th rank don't need that bonus
//            	
//            	Bitboard file_in_front = in_front_bb(Us, s) & file_bb(s);
//            	
//				if (!fiho_us) {
//					if (!(file_in_front & pos.pieces(PAWN, Us))) {
//						// rook is in front of our pawns
//						fiho_us = true;
//					}
//				}
//				
//				if (fiho_us && !fiho_them) {
//					if (!(file_in_front & pos.pieces(PAWN, Them))) {
//						// rook is in front of their pawns
//						fiho_them = true;
//					}
//				}
//				
//				if (fiho_us && fiho_them && !fiho_mp) {
//					if (!(file_in_front & pos.pieces(BISHOP, Them) & pos.pieces(KNIGHT, Them))) {
//						// rook is in front of their minor pieces
//						fiho_mp = true;
//					}
//				}
//            
//        	}
//            ENDNEW
            
            Score openfile_score = SCORE_ZERO;
            
            
            if (ei.pi->file_is_half_open(Us, f)) {
            	
            	if (ei.pi->file_is_half_open(Them, f)) {
                    NEW openfile_score += RookOpenFileBonus;
            		OLD score += RookOpenFileBonus;
                    
                    STARTNEW
                    // give a huge bonus if the file is 
                    // also free of opponents minor pieces
                    if (ei.file_is_half_open_mp(Them, f)) {
                    	NEW openfile_score += RookOpenFileBonus;
                    	//score += RookOpenFileBonus;
                    }
                    ENDNEW 
                } else {
                	NEW openfile_score += RookHalfOpenFileBonus;
                    OLD score += RookHalfOpenFileBonus;
                }
            } 
            STARTNEW
            // give a bonus for "anti-half open" files as well
            // that is we have a file with a pawn and a rook on it and 
            // the opponent has no pawn on that file
            // this is good since the pawn can't be removed unless we get an open file
            // so this pawn can almost be treated like passed
            else if (ei.pi->file_is_half_open(Them, f)) {
            	
            	
            	if (!(ei.pi->passed_pawns(Us) && file_bb(f))) {

            		openfile_score += RookAntiHalfOpenFileBonus;
            		//score += RookAntiHalfOpenFileBonus;
            		if (ei.file_is_half_open_mp(Them, f)) {
            			openfile_score += RookAntiHalfOpenFileBonus;
            			//score += RookAntiHalfOpenFileBonus;
            		}
            	}

            	
            }
            
            
            // bonus if our rook is on this or neighboring files of their king
            ksq = pos.king_square(Them);
            if (abs(f - square_file(ksq)) <= 1) {
            	openfile_score = 5 * openfile_score / 4;
            }
            
            
            
            
//            if (useImprovements) {
//            	// lower the score if both rooks occupy the same file
//            	if (pos.piece_count(Us, ROOK) >= 2) {
//            		if (   square_file(pos.piece_list(Us, ROOK, 0))
//            			== square_file(pos.piece_list(Us, ROOK, 1))) {
//            			openfile_score = 4 * openfile_score / 7;
//            		}
//            	}
//            }
            
            score += openfile_score;
            
            ENDNEW
            

            // Penalize rooks which are trapped inside a king. Penalize more if
            // king has lost right to castle.
            if (mob > 6 || ei.pi->file_is_half_open(Us, f))
                continue;

            ksq = pos.king_square(Us);

            if (    square_file(ksq) >= FILE_E
                &&  square_file(s) > square_file(ksq)
                && (relative_rank(Us, ksq) == RANK_1 || square_rank(ksq) == square_rank(s)))
            {
                // Is there a half-open file between the king and the edge of the board?
                if (!ei.pi->has_open_file_to_right(Us, square_file(ksq)))
                    score -= make_score(pos.can_castle(Us) ? (TrappedRookPenalty - mob * 16) / 2
                                                           : (TrappedRookPenalty - mob * 16), 0);
            }
            else if (    square_file(ksq) <= FILE_D
                     &&  square_file(s) < square_file(ksq)
                     && (relative_rank(Us, ksq) == RANK_1 || square_rank(ksq) == square_rank(s)))
            {
                // Is there a half-open file between the king and the edge of the board?
                if (!ei.pi->has_open_file_to_left(Us, square_file(ksq)))
                    score -= make_score(pos.can_castle(Us) ? (TrappedRookPenalty - mob * 16) / 2
                                                           : (TrappedRookPenalty - mob * 16), 0);
            }
        }
    }

    if (Trace)
        TracedScores[Us][Piece] = score;

    return score;
  }


  // evaluate_threats<>() assigns bonuses according to the type of attacking piece
  // and the type of attacked one.

  template<Color Us>
  Score evaluate_threats(const Position& pos, EvalInfo& ei) {
	
	NEW // there are no 'defending' pawns in the way mentioned here
	NEW return SCORE_ZERO;
	
    const Color Them = (Us == WHITE ? BLACK : WHITE);

    Bitboard b;
    Score score = SCORE_ZERO;

    // Enemy pieces not defended by a pawn and under our attack
    Bitboard weakEnemies =  pos.pieces_of_color(Them)
                          & ~ei.attackedBy[Them][PAWN]
                          & ei.attackedBy[Us][0];
    if (!weakEnemies)
        return SCORE_ZERO;

    // Add bonus according to type of attacked enemy piece and to the
    // type of attacking piece, from knights to queens. Kings are not
    // considered because are already handled in king evaluation.
    for (PieceType pt1 = KNIGHT; pt1 < KING; pt1++)
    {
        b = ei.attackedBy[Us][pt1] & weakEnemies;
        if (b)
            for (PieceType pt2 = PAWN; pt2 < KING; pt2++)
                if (b & pos.pieces(pt2))
                    score += ThreatBonus[pt1][pt2];
    }
    return score;
  }


  // evaluate_pieces_of_color<>() assigns bonuses and penalties to all the
  // pieces of a given color.

  template<Color Us, bool HasPopCnt, bool Trace>
  Score evaluate_pieces_of_color(const Position& pos, EvalInfo& ei, Score& mobility) {

    // OLD const Color Them = (Us == WHITE ? BLACK : WHITE);

    Score score = mobility = SCORE_ZERO;

    // Do not include in mobility squares protected by enemy pawns or occupied by our pieces
    //OLD const Bitboard mobilityArea = ~(ei.attackedBy[Them][PAWN] | pos.pieces_of_color(Us));
    STARTNEW 
    // it is common in atomic chess to move a piece where a pawn might attack
    const Bitboard mobilityArea = ~pos.pieces_of_color(Us);
    ENDNEW
    
    score += evaluate_pieces<KNIGHT, Us, HasPopCnt, Trace>(pos, ei, mobility, mobilityArea);
    score += evaluate_pieces<BISHOP, Us, HasPopCnt, Trace>(pos, ei, mobility, mobilityArea);
    score += evaluate_pieces<ROOK,   Us, HasPopCnt, Trace>(pos, ei, mobility, mobilityArea);
    score += evaluate_pieces<QUEEN,  Us, HasPopCnt, Trace>(pos, ei, mobility, mobilityArea);

    // Sum up all attacked squares
    ei.attackedBy[Us][0] =   ei.attackedBy[Us][PAWN]   | ei.attackedBy[Us][KNIGHT]
                           | ei.attackedBy[Us][BISHOP] | ei.attackedBy[Us][ROOK]
                           | ei.attackedBy[Us][QUEEN]  | ei.attackedBy[Us][KING];
    return score;
  }


  // evaluate_king<>() assigns bonuses and penalties to a king of a given color

  template<Color Us, bool HasPopCnt, bool Trace>
  Score evaluate_king(const Position& pos, EvalInfo& ei, Value margins[]) {

    const BitCountType Max15 = HasPopCnt ? CNT_POPCNT : CpuIs64Bit ? CNT64_MAX15 : CNT32_MAX15;
    const Color Them = (Us == WHITE ? BLACK : WHITE);

    Bitboard undefended, b, b1, b2, safe;
    int attackUnits;
    const Square ksq = pos.king_square(Us);
    
    
    // King shelter
    Score score = ei.pi->king_shelter<Us>(pos, ksq);
    
    STARTNEW
    //if (useImprovements) {
		// penalty for every own piece adjacent to the king
		Bitboard adjacent_bb = pos.attacks_from(WK, ksq) & pos.pieces_of_color(Us);
		if (adjacent_bb) {
			int penalty = count_1s<Max15>(adjacent_bb) * 2 * AdjacencyPenalty;
			score += make_score(penalty, penalty);
		}
    //}
    ENDNEW
    
    // King safety. This is quite complicated, and is almost certainly far
    // from optimally tuned.
    OLD     if (   ei.kingAttackersCount[Them] >= 2
            && ei.kingAdjacentZoneAttacksCount[Them]) {}
    
    NEW if (   ei.kingAttackersCount[Them] >= 1
        && ei.kingAdjacentZoneAttacksCount[Them])
    {
        // Find the attacked squares around the king which has no defenders
        // apart from the king itself
        undefended = ei.attackedBy[Them][0] & ei.attackedBy[Us][KING];
        
        
        NEW // in atomic there is no defense of squares near the king
        OLD undefended &= ~(  ei.attackedBy[Us][PAWN]   | ei.attackedBy[Us][KNIGHT]
                        | ei.attackedBy[Us][BISHOP] | ei.attackedBy[Us][ROOK]
                        | ei.attackedBy[Us][QUEEN]);

        // Initialize the 'attackUnits' variable, which is used later on as an
        // index to the KingDangerTable[] array. The initial value is based on
        // the number and types of the enemy's attacking pieces, the number of
        // attacked and undefended squares around our king, the square of the
        // king, and the quality of the pawn shelter.
        attackUnits =  Min(25, (ei.kingAttackersCount[Them] * ei.kingAttackersWeight[Them]) / 2)
                     + 3 * (ei.kingAdjacentZoneAttacksCount[Them] + count_1s<Max15>(undefended))
                     + InitKingDanger[relative_square(Us, ksq)]
                     - mg_value(ei.pi->king_shelter<Us>(pos, ksq)) / 32;

        // Analyse enemy's safe queen contact checks. First find undefended
        // squares around the king attacked by enemy queen...
        b = undefended & ei.attackedBy[Them][QUEEN] & ~pos.pieces_of_color(Them);
        if (b)
        {
        	NEW // this is not correct since the we are the side to move and can prevent this
        	//NEW // contact check by a queen almost always ends in checkmate
        	//NEW score -= make_score(QueenValueMidgame, QueenValueEndgame);
        	
            STARTOLD
        	// ...then remove squares not supported by another enemy piece
            b &= (  ei.attackedBy[Them][PAWN]   | ei.attackedBy[Them][KNIGHT]
                  | ei.attackedBy[Them][BISHOP] | ei.attackedBy[Them][ROOK]);
        	ENDOLD
        	
        	
            if (b) {
                attackUnits +=  QueenContactCheckBonus
                              * count_1s<Max15>(b)
                              * (Them == pos.side_to_move() ? 2 : 1);
                
                STARTNEW
                //if (useImprovements) {
                	
				// if opponent can contact check we are almost lost,
				// we are not if we can explode opponents king
				if (Them == pos.side_to_move()) {
					Bitboard is_expl_threat = ei.attackedBy[Us][0] 
											& ei.attackedBy[Them][KING] 
											& pos.pieces_of_color(Them);
					if (   !is_expl_threat 
						&&  square_dist(pos.king_square(Us), pos.king_square(Them)) > 1) {
						score -= make_score(QueenValueMidgame, QueenValueMidgame);
					}
				}
                //}
                ENDNEW
            }
            
        }

        // Analyse enemy's safe rook contact checks. First find undefended
        // squares around the king attacked by enemy rooks...
        b = undefended & ei.attackedBy[Them][ROOK] & ~pos.pieces_of_color(Them);

        // Consider only squares where the enemy rook gives check
        b &= RookPseudoAttacks[ksq];

        if (b)
        {
        	//NEW // contact check by a rook almost always ends in at least stalemate
        	//NEW score -= make_score(RookValueMidgame, RookValueEndgame);
        	
        	STARTOLD
        	// ...then remove squares not supported by another enemy piece
        	b &= (  ei.attackedBy[Them][PAWN]   | ei.attackedBy[Them][KNIGHT]
                  | ei.attackedBy[Them][BISHOP] | ei.attackedBy[Them][QUEEN]);
        	ENDOLD
            if (b) {
                attackUnits +=  RookContactCheckBonus
                              * count_1s<Max15>(b)
                              * (Them == pos.side_to_move() ? 2 : 1);
                
                STARTNEW
                //if (useImprovements) {
                
				// if opponent can contact check we are almost lost,
				// we are not if we can explode opponents king
				if (Them == pos.side_to_move()) {
					Bitboard is_expl_threat = ei.attackedBy[Us][0] 
											& ei.attackedBy[Them][KING] 
											& pos.pieces_of_color(Them);
					if (   !is_expl_threat 
						&&  square_dist(pos.king_square(Us), pos.king_square(Them)) > 1) {
						score -= make_score(RookValueMidgame, RookValueMidgame);
					}
				}
                //}
                ENDNEW
            }
            
        }

        // Analyse enemy's safe distance checks for sliders and knights
        safe = ~(pos.pieces_of_color(Them) | ei.attackedBy[Us][0]);

        b1 = pos.attacks_from<ROOK>(ksq) & safe;
        b2 = pos.attacks_from<BISHOP>(ksq) & safe;

        // Enemy queen safe checks
        b = (b1 | b2) & ei.attackedBy[Them][QUEEN];
        if (b)
            attackUnits += QueenCheckBonus * count_1s<Max15>(b);

        // Enemy rooks safe checks
        b = b1 & ei.attackedBy[Them][ROOK];
        if (b)
            attackUnits += RookCheckBonus * count_1s<Max15>(b);

        // Enemy bishops safe checks
        b = b2 & ei.attackedBy[Them][BISHOP];
        if (b)
            attackUnits += BishopCheckBonus * count_1s<Max15>(b);

        // Enemy knights safe checks
        b = pos.attacks_from<KNIGHT>(ksq) & ei.attackedBy[Them][KNIGHT] & safe;
        if (b)
            attackUnits += KnightCheckBonus * count_1s<Max15>(b);

        // To index KingDangerTable[] attackUnits must be in [0, 99] range
        attackUnits = Min(99, Max(0, attackUnits));

        // Finally, extract the king danger score from the KingDangerTable[]
        // array and subtract the score from evaluation. Set also margins[]
        // value that will be used for pruning because this value can sometimes
        // be very big, and so capturing a single attacking piece can therefore
        // result in a score change far bigger than the value of the captured piece.
        score -= KingDangerTable[Us][attackUnits];
        margins[Us] += mg_value(KingDangerTable[Us][attackUnits]);
    } 
    
    STARTNEW
    // even in the endgame we may still need to calculate some king safety
    // unfortunately the code here is redundant
    else {
    	undefended = ei.attackedBy[Us][KING];
    	
        // Analyse enemy's safe queen contact checks. First find undefended
        // squares around the king attacked by enemy queen...
        b = undefended & ei.attackedBy[Them][QUEEN] & ~pos.pieces_of_color(Them);
        if (b) {
        	// if opponent can contact check we are almost lost,
        	// we are not if we can explode opponents king
        	if (Them == pos.side_to_move()) {
        		Bitboard is_expl_threat = ei.attackedBy[Us][0] 
        		                        & ei.attackedBy[Them][KING] 
        		                        & pos.pieces_of_color(Them);
				if (   !is_expl_threat 
					&&  square_dist(pos.king_square(Us), pos.king_square(Them)) > 1) {
        			score -= make_score(QueenValueMidgame, QueenValueMidgame);
        		}
        	}
        }

        // Analyse enemy's safe rook contact checks. First find undefended
        // squares around the king attacked by enemy rooks...
        b = undefended & ei.attackedBy[Them][ROOK] & ~pos.pieces_of_color(Them);

        // Consider only squares where the enemy rook gives check
        b &= RookPseudoAttacks[ksq];

        if (b) {
        	// if opponent can contact check we are almost lost,
        	// we are not if we can explode opponents king
        	if (Them == pos.side_to_move()) {
        		Bitboard is_expl_threat = ei.attackedBy[Us][0] 
        		                        & ei.attackedBy[Them][KING] 
        		                        & pos.pieces_of_color(Them);
				if (   !is_expl_threat 
					&&  square_dist(pos.king_square(Us), pos.king_square(Them)) > 1) {
        			score -= make_score(RookValueMidgame, RookValueMidgame);
        		}
        	}
        }
    }
    ENDNEW

    if (Trace)
        TracedScores[Us][KING] = score;

    return score;
  }


  // evaluate_passed_pawns<>() evaluates the passed pawns of the given color

  template<Color Us>
  Score evaluate_passed_pawns(const Position& pos, EvalInfo& ei) {

    const Color Them = (Us == WHITE ? BLACK : WHITE);

    Bitboard b, squaresToQueen, defendedSquares, unsafeSquares, supportingPawns;
    Score score = SCORE_ZERO;

    b = ei.pi->passed_pawns(Us);

    if (!b)
        return SCORE_ZERO;
    
    STARTNEW
//    
//    // a passed pawn is at least worth the value of the least valuable piece
//    // so create a table of the least valuable pieces ordered from cheap to valuable
//    const Value midgameScores[]  = {Value(0), PawnValueMidgame, KnightValueMidgame, 
//    		BishopValueMidgame, RookValueMidgame, QueenValueMidgame};
//    const Value endgameScores[]  = {Value(0), PawnValueEndgame, KnightValueEndgame, 
//        		BishopValueEndgame, RookValueEndgame, QueenValueEndgame};
//    
//    Value leastOppValuesMg[8];
//    Value leastOppValuesEg[8];
//    
//    // as we trace the passed pawns we increment this
//    int passedPawnCounter = 0;
//    
//    if (useImprovements) {
//    // king is the least valuable blocker since it cannot attack nor defend
//    leastOppValuesMg[0] = KnightValueMidgame / 2;
//    leastOppValuesEg[0] = KnightValueEndgame / 2;
//    int k = 1;
//    for (int pt = int(KNIGHT); pt < int(KING); ++pt) {
//		for (int j = 0; j < pos.piece_count(Them, PieceType(pt)); ++j) {
//			leastOppValuesMg[k] = midgameScores[pt];
//			leastOppValuesEg[k] = endgameScores[pt];
//			++k;
//		}
//    }
//    
//    // fill with queen values
//    while (k < 8) {
//    	leastOppValuesMg[k] = QueenValueMidgame;
//    	leastOppValuesEg[k] = QueenValueEndgame;
//    	++k;
//    }
//    }
    ENDNEW

    do {
        Square s = pop_1st_bit(&b);

        assert(pos.pawn_is_passed(Us, s));

        int r = int(relative_rank(Us, s) - RANK_2);
        int rr = r * (r - 1);

        // Base bonus based on rank
        Value mbonus = Value(20 * rr);
        Value ebonus = Value(10 * (rr + r + 1));

        if (rr)
        {
            Square blockSq = s + pawn_push(Us);

			STARTOLD
			// Adjust bonus based on kings proximity
            ebonus += Value(square_distance(pos.king_square(Them), blockSq) * 6 * rr);
            ebonus -= Value(square_distance(pos.king_square(Us), blockSq) * 3 * rr);

            // If blockSq is not the queening square then consider also a second push
            if (square_rank(blockSq) != (Us == WHITE ? RANK_8 : RANK_1))
                ebonus -= Value(square_distance(pos.king_square(Us), blockSq + pawn_push(Us)) * rr);
            ENDOLD
            
            NEW // the king fails to support passed pawns in atomic
            NEW ebonus += Value(square_distance(pos.king_square(Them), blockSq) * 6 * rr);
            
            // If the pawn is free to advance, increase bonus
            if (pos.square_is_empty(blockSq))
            {
                squaresToQueen = squares_in_front_of(Us, s);
                defendedSquares = squaresToQueen & ei.attackedBy[Us][0];

                // If there is an enemy rook or queen attacking the pawn from behind,
                // add all X-ray attacks by the rook or queen. Otherwise consider only
                // the squares in the pawn's path attacked or occupied by the enemy.
                NEW // queens/rook which attack from behind are no threats 
                NEW // since they have to capture the pawn to stop it
                
                STARTOLD
//                if (   (squares_in_front_of(Them, s) & pos.pieces(ROOK, QUEEN, Them))
//                    && (squares_in_front_of(Them, s) & pos.pieces(ROOK, QUEEN, Them) & pos.attacks_from<ROOK>(s)))
//                    unsafeSquares = squaresToQueen;
//                else
                ENDOLD
                    unsafeSquares = squaresToQueen & (ei.attackedBy[Them][0] | pos.pieces_of_color(Them));

                // If there aren't enemy attacks or pieces along the path to queen give
                // huge bonus. Even bigger if we protect the pawn's path.
                if (!unsafeSquares)
                    ebonus += Value(rr * (squaresToQueen == defendedSquares ? 17 : 15));
                // OLD else 
                    // OK, there are enemy attacks or pieces (but not pawns). Are those
                    // squares which are attacked by the enemy also attacked by us ?
                    // If yes, big bonus (but smaller than when there are no enemy attacks),
                    // if no, somewhat smaller bonus.
                	NEW // there is no possibility of defending by recapture
                    OLD ebonus += Value(rr * ((unsafeSquares & defendedSquares) == unsafeSquares ? 13 : 8));

                // At last, add a small bonus when there are no *friendly* pieces
                // in the pawn's path.
                if (!(squaresToQueen & pos.pieces_of_color(Us)))
                    ebonus += Value(rr);
            }
        } // rr != 0

        // Increase the bonus if the passed pawn is supported by a friendly pawn
        // on the same rank and a bit smaller if it's on the previous rank.
        supportingPawns = pos.pieces(PAWN, Us) & neighboring_files_bb(s);
        if (supportingPawns & rank_bb(s))
            ebonus += Value(r * 20);
        else if (supportingPawns & rank_bb(s - pawn_push(Us)))
            ebonus += Value(r * 12);

        // Rook pawns are a special case: They are sometimes worse, and
        // sometimes better than other passed pawns. It is difficult to find
        // good rules for determining whether they are good or bad. For now,
        // we try the following: Increase the value for rook pawns if the
        // other side has no pieces apart from a knight, and decrease the
        // value if the other side has a rook or queen.
        if (square_file(s) == FILE_A || square_file(s) == FILE_H)
        {
            if (pos.non_pawn_material(Them) <= KnightValueMidgame)
                ebonus += ebonus / 4;
            else if (pos.pieces(ROOK, QUEEN, Them))
                ebonus -= ebonus / 4;
        }
        
        
        NEW // maybe this needs to be scaled up
        NEW score += make_score(3 * mbonus / 2, 3 * ebonus / 2);
        //score += make_score(mbonus, ebonus);
        
        STARTNEW
//        if (useImprovements) {
//        // score the passed pawns with the least valuable defenders
//        score += make_score(
//        		leastOppValuesMg[passedPawnCounter] - PawnValueMidgame, 
//        		leastOppValuesEg[passedPawnCounter] - PawnValueEndgame);
//        
//        ++passedPawnCounter;
//        }
        ENDNEW

    } while (b);

    // Add the scores to the middle game and endgame eval
    return apply_weight(score, Weights[PassedPawns]);
  }


  // evaluate_unstoppable_pawns() evaluates the unstoppable passed pawns for both sides, this is quite
  // conservative and returns a winning score only when we are very sure that the pawn is winning.

  template<bool HasPopCnt>
  Score evaluate_unstoppable_pawns(const Position& pos, EvalInfo& ei) {

    const BitCountType Max15 = HasPopCnt ? CNT_POPCNT : CpuIs64Bit ? CNT64_MAX15 : CNT32_MAX15;

    Bitboard b, b2, blockers, supporters, queeningPath, candidates;
    Square s, blockSq, queeningSquare;
    Color c, winnerSide, loserSide;
    bool pathDefended, opposed;
    int pliesToGo, movesToGo, oppMovesToGo, sacptg, blockersCount, minKingDist, kingptg, d;
    int pliesToQueen[] = { 256, 256 };

    // Step 1. Hunt for unstoppable passed pawns. If we find at least one,
    // record how many plies are required for promotion.
    for (c = WHITE; c <= BLACK; c++)
    {
        // Skip if other side has non-pawn pieces
        if (pos.non_pawn_material(opposite_color(c)))
            continue;

        b = ei.pi->passed_pawns(c);

        while (b)
        {
            s = pop_1st_bit(&b);
            queeningSquare = relative_square(c, make_square(square_file(s), RANK_8));
            queeningPath = squares_in_front_of(c, s);

            // Compute plies to queening and check direct advancement
            movesToGo = rank_distance(s, queeningSquare) - int(relative_rank(c, s) == RANK_2);
            oppMovesToGo = square_distance(pos.king_square(opposite_color(c)), queeningSquare) - int(c != pos.side_to_move());
            pathDefended = ((ei.attackedBy[c][0] & queeningPath) == queeningPath);

            if (movesToGo >= oppMovesToGo && !pathDefended)
                continue;

            // Opponent king cannot block because path is defended and position
            // is not in check. So only friendly pieces can be blockers.
            NEW // opponent king can block if he is adjacent to enemy king
            
            assert(!pos.in_check());
            OLD assert((queeningPath & pos.occupied_squares()) == (queeningPath & pos.pieces_of_color(c)));
            STARTNEW
            assert(
            	((queeningPath & pos.occupied_squares()) == (queeningPath & pos.pieces_of_color(c)))
              || squares_touch
              	  [pos.king_square(pos.side_to_move())]
              	   [pos.king_square(opposite_color(pos.side_to_move()))]);
            ENDNEW

            // Add moves needed to free the path from friendly pieces and retest condition
            movesToGo += count_1s<Max15>(queeningPath & pos.pieces_of_color(c));

            if (movesToGo >= oppMovesToGo && !pathDefended)
                continue;

            pliesToGo = 2 * movesToGo - int(c == pos.side_to_move());
            pliesToQueen[c] = Min(pliesToQueen[c], pliesToGo);
        }
    }

    // Step 2. If either side cannot promote at least three plies before the other side then situation
    // becomes too complex and we give up. Otherwise we determine the possibly "winning side"
    if (abs(pliesToQueen[WHITE] - pliesToQueen[BLACK]) < 3)
        return SCORE_ZERO;

    winnerSide = (pliesToQueen[WHITE] < pliesToQueen[BLACK] ? WHITE : BLACK);
    loserSide = opposite_color(winnerSide);

    // Step 3. Can the losing side possibly create a new passed pawn and thus prevent the loss?
    b = candidates = pos.pieces(PAWN, loserSide);

    while (b)
    {
        s = pop_1st_bit(&b);

        // Compute plies from queening
        queeningSquare = relative_square(loserSide, make_square(square_file(s), RANK_8));
        movesToGo = rank_distance(s, queeningSquare) - int(relative_rank(loserSide, s) == RANK_2);
        pliesToGo = 2 * movesToGo - int(loserSide == pos.side_to_move());

        // Check if (without even considering any obstacles) we're too far away or doubled
        if (   pliesToQueen[winnerSide] + 3 <= pliesToGo
            || (squares_in_front_of(loserSide, s) & pos.pieces(PAWN, loserSide)))
            clear_bit(&candidates, s);
    }

    // If any candidate is already a passed pawn it _may_ promote in time. We give up.
    if (candidates & ei.pi->passed_pawns(loserSide))
        return SCORE_ZERO;

    // Step 4. Check new passed pawn creation through king capturing and pawn sacrifices
    b = candidates;

    while (b)
    {
        s = pop_1st_bit(&b);
        sacptg = blockersCount = 0;
        minKingDist = kingptg = 256;

        // Compute plies from queening
        queeningSquare = relative_square(loserSide, make_square(square_file(s), RANK_8));
        movesToGo = rank_distance(s, queeningSquare) - int(relative_rank(loserSide, s) == RANK_2);
        pliesToGo = 2 * movesToGo - int(loserSide == pos.side_to_move());

        // Generate list of blocking pawns and supporters
        supporters = neighboring_files_bb(s) & candidates;
        opposed = squares_in_front_of(loserSide, s) & pos.pieces(PAWN, winnerSide);
        blockers = passed_pawn_mask(loserSide, s) & pos.pieces(PAWN, winnerSide);

        assert(blockers);

        // How many plies does it take to remove all the blocking pawns?
        while (blockers)
        {
            blockSq = pop_1st_bit(&blockers);
            movesToGo = 256;

            // Check pawns that can give support to overcome obstacle, for instance
            // black pawns: a4, b4 white: b2 then pawn in b4 is giving support.
            if (!opposed)
            {
                b2 = supporters & in_front_bb(winnerSide, blockSq + pawn_push(winnerSide));

                while (b2) // This while-loop could be replaced with LSB/MSB (depending on color)
                {
                    d = square_distance(blockSq, pop_1st_bit(&b2)) - 2;
                    movesToGo = Min(movesToGo, d);
                }
            }

            // Check pawns that can be sacrificed against the blocking pawn
            b2 = attack_span_mask(winnerSide, blockSq) & candidates & ~(1ULL << s);

            while (b2) // This while-loop could be replaced with LSB/MSB (depending on color)
            {
                d = square_distance(blockSq, pop_1st_bit(&b2)) - 2;
                movesToGo = Min(movesToGo, d);
            }

            // If obstacle can be destroyed with an immediate pawn exchange / sacrifice,
            // it's not a real obstacle and we have nothing to add to pliesToGo.
            if (movesToGo <= 0)
                continue;

            // Plies needed to sacrifice against all the blocking pawns
            sacptg += movesToGo * 2;
            blockersCount++;

            // Plies needed for the king to capture all the blocking pawns
            d = square_distance(pos.king_square(loserSide), blockSq);
            minKingDist = Min(minKingDist, d);
            kingptg = (minKingDist + blockersCount) * 2;
        }

        // Check if pawn sacrifice plan _may_ save the day
        if (pliesToQueen[winnerSide] + 3 > pliesToGo + sacptg)
            return SCORE_ZERO;

        // Check if king capture plan _may_ save the day (contains some false positives)
        if (pliesToQueen[winnerSide] + 3 > pliesToGo + kingptg)
            return SCORE_ZERO;
    }

    // Winning pawn is unstoppable and will promote as first, return big score
    Score score = make_score(0, (Value) 0x500 - 0x20 * pliesToQueen[winnerSide]);
    return winnerSide == WHITE ? score : -score;
  }


  // evaluate_space() computes the space evaluation for a given side. The
  // space evaluation is a simple bonus based on the number of safe squares
  // available for minor pieces on the central four files on ranks 2--4. Safe
  // squares one, two or three squares behind a friendly pawn are counted
  // twice. Finally, the space bonus is scaled by a weight taken from the
  // material hash table. The aim is to improve play on game opening.
  template<Color Us, bool HasPopCnt>
  int evaluate_space(const Position& pos, EvalInfo& ei) {

	NEW // i think that doesn't work in atomic or it is quite complicated
	NEW return SCORE_ZERO;
	
	
    const BitCountType Max15 = HasPopCnt ? CNT_POPCNT : CpuIs64Bit ? CNT64_MAX15 : CNT32_MAX15;
    const Color Them = (Us == WHITE ? BLACK : WHITE);

    // Find the safe squares for our pieces inside the area defined by
    // SpaceMask[]. A square is unsafe if it is attacked by an enemy
    // pawn, or if it is undefended and attacked by an enemy piece.
    Bitboard safe =   SpaceMask[Us]
                   & ~pos.pieces(PAWN, Us)
                   & ~ei.attackedBy[Them][PAWN]
                   & (ei.attackedBy[Us][0] | ~ei.attackedBy[Them][0]);

    // Find all squares which are at most three squares behind some friendly pawn
    Bitboard behind = pos.pieces(PAWN, Us);
    behind |= (Us == WHITE ? behind >>  8 : behind <<  8);
    behind |= (Us == WHITE ? behind >> 16 : behind << 16);

    return count_1s<Max15>(safe) + count_1s<Max15>(behind & safe);
  }


  // apply_weight() applies an evaluation weight to a value trying to prevent overflow

  inline Score apply_weight(Score v, Score w) {
    return make_score((int(mg_value(v)) * mg_value(w)) / 0x100,
                      (int(eg_value(v)) * eg_value(w)) / 0x100);
  }


  // scale_by_game_phase() interpolates between a middle game and an endgame score,
  // based on game phase. It also scales the return value by a ScaleFactor array.

  Value scale_by_game_phase(const Score& v, Phase ph, ScaleFactor sf) {

    assert(mg_value(v) > -VALUE_INFINITE && mg_value(v) < VALUE_INFINITE);
    assert(eg_value(v) > -VALUE_INFINITE && eg_value(v) < VALUE_INFINITE);
    assert(ph >= PHASE_ENDGAME && ph <= PHASE_MIDGAME);

    int ev = (eg_value(v) * int(sf)) / SCALE_FACTOR_NORMAL;
    int result = (mg_value(v) * int(ph) + ev * int(128 - ph)) / 128;
    return Value((result + GrainSize / 2) & ~(GrainSize - 1));
  }


  // weight_option() computes the value of an evaluation weight, by combining
  // two UCI-configurable weights (midgame and endgame) with an internal weight.

  Score weight_option(const std::string& mgOpt, const std::string& egOpt, Score internalWeight) {

    // Scale option value from 100 to 256
    int mg = Options[mgOpt].value<int>() * 256 / 100;
    int eg = Options[egOpt].value<int>() * 256 / 100;

    return apply_weight(make_score(mg, eg), internalWeight);
  }


  // init_safety() initizes the king safety evaluation, based on UCI
  // parameters. It is called from read_weights().

  void init_safety() {

    const Value MaxSlope = Value(30);
    const Value Peak = Value(1280);
    Value t[100];

    // First setup the base table
    for (int i = 0; i < 100; i++)
    {
        t[i] = Value(int(0.4 * i * i));

        if (i > 0)
            t[i] = Min(t[i], t[i - 1] + MaxSlope);

        t[i] = Min(t[i], Peak);
    }

    // Then apply the weights and get the final KingDangerTable[] array
    for (Color c = WHITE; c <= BLACK; c++)
        for (int i = 0; i < 100; i++)
            KingDangerTable[c][i] = apply_weight(make_score(t[i], 0), Weights[KingDangerUs + c]);
    
  }


  // A couple of little helpers used by tracing code, to_cp() converts a value to
  // a double in centipawns scale, trace_add() stores white and black scores.

  double to_cp(Value v) { return double(v) / double(PawnValueMidgame); }

  void trace_add(int idx, Score wScore, Score bScore) {

      TracedScores[WHITE][idx] = wScore;
      TracedScores[BLACK][idx] = bScore;
  }

  // trace_row() is an helper function used by tracing code to register the
  // values of a single evaluation term.

  void trace_row(const char *name, int idx) {

    Score wScore = TracedScores[WHITE][idx];
    Score bScore = TracedScores[BLACK][idx];

    switch (idx) {
    case PST: case IMBALANCE: case PAWN: case UNSTOPPABLE: case TOTAL:
        TraceStream << std::setw(20) << name << " |   ---   --- |   ---   --- | "
                    << std::setw(6)  << to_cp(mg_value(wScore)) << " "
                    << std::setw(6)  << to_cp(eg_value(wScore)) << " \n";
        break;
    default:
        TraceStream << std::setw(20) << name << " | " << std::noshowpos
                    << std::setw(5)  << to_cp(mg_value(wScore)) << " "
                    << std::setw(5)  << to_cp(eg_value(wScore)) << " | "
                    << std::setw(5)  << to_cp(mg_value(bScore)) << " "
                    << std::setw(5)  << to_cp(eg_value(bScore)) << " | "
                    << std::showpos
                    << std::setw(6)  << to_cp(mg_value(wScore - bScore)) << " "
                    << std::setw(6)  << to_cp(eg_value(wScore - bScore)) << " \n";
    }
  }
}


/// trace_evaluate() is like evaluate() but instead of a value returns a string
/// suitable to be print on stdout with the detailed descriptions and values of
/// each evaluation term. Used mainly for debugging.

std::string trace_evaluate(const Position& pos) {

    Value margin;
    std::string totals;

    TraceStream.str("");
    TraceStream << std::showpoint << std::showpos << std::fixed << std::setprecision(2);
    memset(TracedScores, 0, 2 * 16 * sizeof(Score));

    NEW bool expl_threat;
    do_evaluate<false, true>(pos, margin, &expl_threat);

    totals = TraceStream.str();
    TraceStream.str("");

    TraceStream << std::setw(21) << "Eval term " << "|    White    |    Black    |     Total     \n"
                <<             "                     |   MG    EG  |   MG    EG  |   MG     EG   \n"
                <<             "---------------------+-------------+-------------+---------------\n";

    trace_row("Material, PST, Tempo", PST);
    trace_row("Material imbalance", IMBALANCE);
    trace_row("Pawns", PAWN);
    trace_row("Knights", KNIGHT);
    trace_row("Bishops", BISHOP);
    trace_row("Rooks", ROOK);
    trace_row("Queens", QUEEN);
    trace_row("Mobility", MOBILITY);
    trace_row("King safety", KING);
    trace_row("Threats", THREAT);
    trace_row("Passed pawns", PASSED);
    trace_row("Unstoppable pawns", UNSTOPPABLE);
    trace_row("Space", SPACE);

    TraceStream <<             "---------------------+-------------+-------------+---------------\n";
    trace_row("Total", TOTAL);
    TraceStream << totals;

    return TraceStream.str();
}
