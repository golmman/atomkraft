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

#if !defined(TYPES_H_INCLUDED)
#define TYPES_H_INCLUDED

#include <climits>
#include <cstdlib>

#if defined(_MSC_VER)

// Disable some silly and noisy warning from MSVC compiler
#pragma warning(disable: 4800) // Forcing value to bool 'true' or 'false'
#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type

// MSVC does not support <inttypes.h>
typedef   signed __int8    int8_t;
typedef unsigned __int8   uint8_t;
typedef   signed __int16  int16_t;
typedef unsigned __int16 uint16_t;
typedef   signed __int32  int32_t;
typedef unsigned __int32 uint32_t;
typedef   signed __int64  int64_t;
typedef unsigned __int64 uint64_t;

#else

#include <inttypes.h>

#endif

#define Min(x, y) (((x) < (y)) ? (x) : (y))
#define Max(x, y) (((x) < (y)) ? (y) : (x))

////
//// Configuration
////

//// For Linux and OSX configuration is done automatically using Makefile.
//// To get started type "make help".
////
//// For windows part of the configuration is detected automatically, but
//// some switches need to be set manually:
////
//// -DNDEBUG       | Disable debugging mode. Use always.
////
//// -DNO_PREFETCH  | Disable use of prefetch asm-instruction. A must if you want the
////                | executable to run on some very old machines.
////
//// -DUSE_POPCNT   | Add runtime support for use of popcnt asm-instruction.
////                | Works only in 64-bit mode. For compiling requires hardware
////                | with popcnt support. Around 4% speed-up.
////
//// -DOLD_LOCKS    | By default under Windows are used the fast Slim Reader/Writer (SRW)
////                | Locks and Condition Variables: these are not supported by Windows XP
////                | and older, to compile for those platforms you should enable OLD_LOCKS.

// Automatic detection for 64-bit under Windows
#if defined(_WIN64)
#define IS_64BIT
#endif

// Automatic detection for use of bsfq asm-instruction under Windows
#if defined(_WIN64)
#define USE_BSFQ
#endif

// Intel header for _mm_popcnt_u64() intrinsic
#if defined(USE_POPCNT) && defined(_MSC_VER) && defined(__INTEL_COMPILER)
#include <nmmintrin.h>
#endif

// Cache line alignment specification
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define CACHE_LINE_ALIGNMENT __declspec(align(64))
#else
#define CACHE_LINE_ALIGNMENT  __attribute__ ((aligned(64)))
#endif

// Define a __cpuid() function for gcc compilers, for Intel and MSVC
// is already available as an intrinsic.
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
inline void __cpuid(int CPUInfo[4], int InfoType)
{
  int* eax = CPUInfo + 0;
  int* ebx = CPUInfo + 1;
  int* ecx = CPUInfo + 2;
  int* edx = CPUInfo + 3;

  *eax = InfoType;
  *ecx = 0;
  __asm__("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
                  : "0" (*eax), "2" (*ecx));
}
#else
inline void __cpuid(int CPUInfo[4], int)
{
   CPUInfo[0] = CPUInfo[1] = CPUInfo[2] = CPUInfo[3] = 0;
}
#endif

// Define FORCE_INLINE macro to force inlining overriding compiler choice
#if defined(_MSC_VER)
#define FORCE_INLINE  __forceinline
#elif defined(__GNUC__)
#define FORCE_INLINE  inline __attribute__((always_inline))
#else
#define FORCE_INLINE  inline
#endif

/// cpu_has_popcnt() detects support for popcnt instruction at runtime
inline bool cpu_has_popcnt() {

  int CPUInfo[4] = {-1};
  __cpuid(CPUInfo, 0x00000001);
  return (CPUInfo[2] >> 23) & 1;
}

/// CpuHasPOPCNT is a global constant initialized at startup that
/// is set to true if CPU on which application runs supports popcnt
/// hardware instruction. Unless USE_POPCNT is not defined.
#if defined(USE_POPCNT)
const bool CpuHasPOPCNT = cpu_has_popcnt();
#else
const bool CpuHasPOPCNT = false;
#endif


/// CpuIs64Bit is a global constant initialized at compile time that
/// is set to true if CPU on which application runs is a 64 bits.
#if defined(IS_64BIT)
const bool CpuIs64Bit = true;
#else
const bool CpuIs64Bit = false;
#endif

#include <string>

typedef uint64_t Key;
typedef uint64_t Bitboard;

const int PLY_MAX = 100;
const int PLY_MAX_PLUS_2 = PLY_MAX + 2;

enum ValueType {
  VALUE_TYPE_NONE  = 0,
  VALUE_TYPE_UPPER = 1,
  VALUE_TYPE_LOWER = 2,
  VALUE_TYPE_EXACT = VALUE_TYPE_UPPER | VALUE_TYPE_LOWER
};

enum Value {
  VALUE_ZERO      = 0,
  VALUE_DRAW      = 0,
  VALUE_KNOWN_WIN = 15000,
  VALUE_MATE      = 30000,
  VALUE_INFINITE  = 30001,
  VALUE_NONE      = 30002,

  VALUE_MATE_IN_PLY_MAX  =  VALUE_MATE - PLY_MAX,
  VALUE_MATED_IN_PLY_MAX = -VALUE_MATE + PLY_MAX,

  VALUE_ENSURE_INTEGER_SIZE_P = INT_MAX,
  VALUE_ENSURE_INTEGER_SIZE_N = INT_MIN
};

enum PieceType {
  PIECE_TYPE_NONE = 0,
  PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6
};

enum Piece {
  PIECE_NONE_DARK_SQ = 0, WP = 1, WN = 2, WB = 3, WR = 4, WQ = 5, WK = 6,
  BP = 9, BN = 10, BB = 11, BR = 12, BQ = 13, BK = 14, PIECE_NONE = 16
};

enum Color {
  WHITE, BLACK, COLOR_NONE
};

enum Depth {

  ONE_PLY = 2,

  DEPTH_ZERO         =  0 * ONE_PLY,
// OLD DEPTH_QS_CHECKS    = -1 * ONE_PLY,
// OLD DEPTH_QS_NO_CHECKS = -2 * ONE_PLY,
  
  DEPTH_QS_CHECKS    = -1 * ONE_PLY,
  DEPTH_QS_NO_CHECKS = -2 * ONE_PLY,

  DEPTH_NONE = -127 * ONE_PLY
};

enum Square {
  SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
  SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
  SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
  SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
  SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
  SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
  SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
  SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
  SQ_NONE,

  DELTA_N =  8,
  DELTA_E =  1,
  DELTA_S = -8,
  DELTA_W = -1,

  DELTA_NN = DELTA_N + DELTA_N,
  DELTA_NE = DELTA_N + DELTA_E,
  DELTA_SE = DELTA_S + DELTA_E,
  DELTA_SS = DELTA_S + DELTA_S,
  DELTA_SW = DELTA_S + DELTA_W,
  DELTA_NW = DELTA_N + DELTA_W
};

enum File {
  FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE = -1
};

enum Rank {
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE = -1
};

enum SquareColor {
  DARK, LIGHT
};

enum ScaleFactor {
  SCALE_FACTOR_ZERO   = 0,
  SCALE_FACTOR_NORMAL = 64,
  SCALE_FACTOR_MAX    = 128,
  SCALE_FACTOR_NONE   = 255
};


/// Score enum keeps a midgame and an endgame value in a single
/// integer (enum), first LSB 16 bits are used to store endgame
/// value, while upper bits are used for midgame value. Compiler
/// is free to choose the enum type as long as can keep its data,
/// so ensure Score to be an integer type.
enum Score {
    SCORE_ZERO = 0,
    SCORE_ENSURE_INTEGER_SIZE_P = INT_MAX,
    SCORE_ENSURE_INTEGER_SIZE_N = INT_MIN
};

#define ENABLE_OPERATORS_ON(T) \
inline T operator+ (const T d1, const T d2) { return T(int(d1) + int(d2)); } \
inline T operator- (const T d1, const T d2) { return T(int(d1) - int(d2)); } \
inline T operator* (int i, const T d) {  return T(i * int(d)); } \
inline T operator* (const T d, int i) {  return T(int(d) * i); } \
inline T operator/ (const T d, int i) { return T(int(d) / i); } \
inline T operator- (const T d) { return T(-int(d)); } \
inline T operator++ (T& d, int) {d = T(int(d) + 1); return d; } \
inline T operator-- (T& d, int) { d = T(int(d) - 1); return d; } \
inline T operator++ (T& d) {d = T(int(d) + 1); return d; } \
inline T operator-- (T& d) { d = T(int(d) - 1); return d; } \
inline void operator+= (T& d1, const T d2) { d1 = d1 + d2; } \
inline void operator-= (T& d1, const T d2) { d1 = d1 - d2; } \
inline void operator*= (T& d, int i) { d = T(int(d) * i); } \
inline void operator/= (T& d, int i) { d = T(int(d) / i); }

ENABLE_OPERATORS_ON(Value)
ENABLE_OPERATORS_ON(PieceType)
ENABLE_OPERATORS_ON(Piece)
ENABLE_OPERATORS_ON(Color)
ENABLE_OPERATORS_ON(Depth)
ENABLE_OPERATORS_ON(Square)
ENABLE_OPERATORS_ON(File)
ENABLE_OPERATORS_ON(Rank)

#undef ENABLE_OPERATORS_ON

// Extra operators for adding integers to a Value
inline Value operator+ (Value v, int i) { return Value(int(v) + i); }
inline Value operator- (Value v, int i) { return Value(int(v) - i); }

// Extracting the _signed_ lower and upper 16 bits it not so trivial
// because according to the standard a simple cast to short is
// implementation defined and so is a right shift of a signed integer.
inline Value mg_value(Score s) { return Value(((int(s) + 32768) & ~0xffff) / 0x10000); }

// Unfortunatly on Intel 64 bit we have a small speed regression, so use a faster code in
// this case, although not 100% standard compliant it seems to work for Intel and MSVC.
#if defined(IS_64BIT) && (!defined(__GNUC__) || defined(__INTEL_COMPILER))
inline Value eg_value(Score s) { return Value(int16_t(s & 0xffff)); }
#else
inline Value eg_value(Score s) { return Value((int)(unsigned(s) & 0x7fffu) - (int)(unsigned(s) & 0x8000u)); }
#endif

inline Score make_score(int mg, int eg) { return Score((mg << 16) + eg); }

// Division must be handled separately for each term
inline Score operator/(Score s, int i) { return make_score(mg_value(s) / i, eg_value(s) / i); }

// Only declared but not defined. We don't want to multiply two scores due to
// a very high risk of overflow. So user should explicitly convert to integer.
inline Score operator*(Score s1, Score s2);

// Remaining Score operators are standard
inline Score operator+ (const Score d1, const Score d2) { return Score(int(d1) + int(d2)); }
inline Score operator- (const Score d1, const Score d2) { return Score(int(d1) - int(d2)); }
inline Score operator* (int i, const Score d) {  return Score(i * int(d)); }
inline Score operator* (const Score d, int i) {  return Score(int(d) * i); }
inline Score operator- (const Score d) { return Score(-int(d)); }
inline void operator+= (Score& d1, const Score d2) { d1 = d1 + d2; }
inline void operator-= (Score& d1, const Score d2) { d1 = d1 - d2; }
inline void operator*= (Score& d, int i) { d = Score(int(d) * i); }
inline void operator/= (Score& d, int i) { d = Score(int(d) / i); }

// OLD PIECE VALUES
//const Value PawnValueMidgame   = Value(0x0C6);	// 198
//const Value PawnValueEndgame   = Value(0x102);	// 258
//const Value KnightValueMidgame = Value(0x331);	// 817
//const Value KnightValueEndgame = Value(0x34E);	// 846
//const Value BishopValueMidgame = Value(0x344);	// 836
//const Value BishopValueEndgame = Value(0x359);	// 857
//const Value RookValueMidgame   = Value(0x4F6);	// 1270
//const Value RookValueEndgame   = Value(0x4FE);	// 1278
//const Value QueenValueMidgame  = Value(0x9D9);	// 2521
//const Value QueenValueEndgame  = Value(0x9FE);	// 2558

// NEW PIECE VALUES
const float VALUE_SCALE = 0.75f;
//const Value PawnValueMidgame   = Value(int(200  * VALUE_SCALE));
//const Value PawnValueEndgame   = Value(int(300  * VALUE_SCALE));
//const Value KnightValueMidgame = Value(int(300  * VALUE_SCALE));
//const Value KnightValueEndgame = Value(int(350  * VALUE_SCALE));
//const Value BishopValueMidgame = Value(int(300  * VALUE_SCALE));
//const Value BishopValueEndgame = Value(int(320  * VALUE_SCALE));
//const Value RookValueMidgame   = Value(int(600  * VALUE_SCALE));
//const Value RookValueEndgame   = Value(int(800  * VALUE_SCALE));
//const Value QueenValueMidgame  = Value(int(1300 * VALUE_SCALE));
//const Value QueenValueEndgame  = Value(int(1600 * VALUE_SCALE));

//const Value MinorPiecePawnValueDiff = KnightValueMidgame - PawnValueMidgame;

extern Value PawnValueMidgame;
extern Value PawnValueEndgame;
extern Value KnightValueMidgame;
extern Value KnightValueEndgame;
extern Value BishopValueMidgame;
extern Value BishopValueEndgame;
extern Value RookValueMidgame;
extern Value RookValueEndgame;
extern Value QueenValueMidgame;
extern Value QueenValueEndgame;



inline Value value_mate_in(int ply) {
  return VALUE_MATE - ply;
}

inline Value value_mated_in(int ply) {
  return -VALUE_MATE + ply;
}

inline Piece make_piece(Color c, PieceType pt) {
  return Piece((int(c) << 3) | int(pt));
}

inline PieceType type_of_piece(Piece p)  {
  return PieceType(int(p) & 7);
}

inline Color color_of_piece(Piece p) {
  return Color(int(p) >> 3);
}

inline Color opposite_color(Color c) {
  return Color(int(c) ^ 1);
}

inline bool color_is_ok(Color c) {
  return c == WHITE || c == BLACK;
}

inline bool piece_type_is_ok(PieceType pt) {
  return pt >= PAWN && pt <= KING;
}

inline bool piece_is_ok(Piece p) {
  return piece_type_is_ok(type_of_piece(p)) && color_is_ok(color_of_piece(p));
}

inline char piece_type_to_char(PieceType pt) {
  static const char ch[] = " PNBRQK";
  return ch[pt];
}

inline Square make_square(File f, Rank r) {
  return Square((int(r) << 3) | int(f));
}

inline File square_file(Square s) {
  return File(int(s) & 7);
}

inline Rank square_rank(Square s) {
  return Rank(int(s) >> 3);
}

inline Square flip_square(Square s) {
  return Square(int(s) ^ 56);
}

inline Square flop_square(Square s) {
  return Square(int(s) ^ 7);
}

inline Square relative_square(Color c, Square s) {
  return Square(int(s) ^ (int(c) * 56));
}

inline Rank relative_rank(Color c, Rank r) {
  return Rank(int(r) ^ (int(c) * 7));
}

inline Rank relative_rank(Color c, Square s) {
  return relative_rank(c, square_rank(s));
}

inline SquareColor square_color(Square s) {
  return SquareColor(int(square_rank(s) + s) & 1);
}

inline bool opposite_color_squares(Square s1, Square s2) {
  int s = int(s1) ^ int(s2);
  return ((s >> 3) ^ s) & 1;
}

inline int file_distance(Square s1, Square s2) {
  return abs(square_file(s1) - square_file(s2));
}

inline int rank_distance(Square s1, Square s2) {
  return abs(square_rank(s1) - square_rank(s2));
}

inline int square_distance(Square s1, Square s2) {
  return Max(file_distance(s1, s2), rank_distance(s1, s2));
}

inline File file_from_char(char c) {
  return File(c - 'a') + FILE_A;
}

inline char file_to_char(File f) {
  return char(f - FILE_A + int('a'));
}

inline Rank rank_from_char(char c) {
  return Rank(c - '1') + RANK_1;
}

inline char rank_to_char(Rank r) {
  return char(r - RANK_1 + int('1'));
}

inline const std::string square_to_string(Square s) {
  char ch[] = { file_to_char(square_file(s)), rank_to_char(square_rank(s)), 0 };
  return std::string(ch);
}

inline bool file_is_ok(File f) {
  return f >= FILE_A && f <= FILE_H;
}

inline bool rank_is_ok(Rank r) {
  return r >= RANK_1 && r <= RANK_8;
}

inline bool square_is_ok(Square s) {
  return s >= SQ_A1 && s <= SQ_H8;
}

inline Square pawn_push(Color c) {
  return c == WHITE ? DELTA_N : DELTA_S;
}

#endif // !defined(TYPES_H_INCLUDED)
