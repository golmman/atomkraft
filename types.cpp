/*
 * types.cpp
 *
 *  Created on: 06.09.2011
 *      Author: golmman
 */




#include "types.h"

Value PawnValueMidgame   = Value(int(200  * VALUE_SCALE));
Value PawnValueEndgame   = Value(int(308  * VALUE_SCALE));
Value KnightValueMidgame = Value(int(300  * VALUE_SCALE));
Value KnightValueEndgame = Value(int(350  * VALUE_SCALE));
Value BishopValueMidgame = Value(int(300  * VALUE_SCALE));
Value BishopValueEndgame = Value(int(320  * VALUE_SCALE));
Value RookValueMidgame   = Value(int(600  * VALUE_SCALE));
Value RookValueEndgame   = Value(int(800  * VALUE_SCALE));
Value QueenValueMidgame  = Value(int(1300 * VALUE_SCALE));
Value QueenValueEndgame  = Value(int(1600 * VALUE_SCALE));
