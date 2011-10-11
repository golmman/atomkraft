/*
 * fics.h
 *
 *  Created on: 08.08.2011
 *      Author: golmman
 */

#ifndef FICS_H_
#define FICS_H_

#include "types.h"
#include "move.h"
#include <queue>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

using namespace std;


/*
 * fics style 12 
 * 
 * 
	  The data is all on one line (displayed here as two lines, so it will show on
	your screen).  Here is an example:  [Note: the beginning and ending quotation
	marks are *not* part of the data string; they are needed in this help file
	because some interfaces cannot display the string when in a text file.]
	"<12> rnbqkb-r pppppppp -----n-- -------- ----P--- -------- PPPPKPPP RNBQ-BNR
	 B 4 0 0 1 1 52 7 Newton Einstein 1 2 12 39 39 119 122 246 K/e1-e2 (0:06) Ke2 0"
	This string always begins on a new line, and there are always exactly 31 non-
	empty fields separated by blanks. The fields are:
	* the string "<12>" to identify this line.
	* eight fields representing the board position.  The first one is White's
	  8th rank (also Black's 1st rank), then White's 7th rank (also Black's 2nd),
	Type [next] to see next page.
	fics%
	   etc, regardless of who's move it is.
	* color whose turn it is to move ("B" or "W")
	* -1 if the previous move was NOT a double pawn push, otherwise the chess 
	  board file  (numbered 0--7 for a--h) in which the double push was made
	* can White still castle short? (0=no, 1=yes)
	* can White still castle long?
	* can Black still castle short?
	* can Black still castle long?
	* the number of moves made since the last irreversible move.  (0 if last move
	  was irreversible.  If the value is >= 100, the game can be declared a draw
	  due to the 50 move rule.)
	* The game number
	* White's name
	* Black's name
	* my relation to this game:
		-3 isolated position, such as for "ref 3" or the "sposition" command
		-2 I am observing game being examined
		 2 I am the examiner of this game
		-1 I am playing, it is my opponent's move
		 1 I am playing and it is my move
		 0 I am observing a game being played
	* initial time (in seconds) of the match
	Type [next] to see next page.
	fics%
	 * increment In seconds) of the match
	* White material strength
	* Black material strength
	* White's remaining time
	* Black's remaining time
	* the number of the move about to be made (standard chess numbering -- White's
	  and Black's first moves are both 1, etc.)
	* verbose coordinate notation for the previous move ("none" if there were
	  none) [note this used to be broken for examined games]
	* time taken to make previous move "(min:sec)".
	* pretty notation for the previous move ("none" if there is none)
	* flip field for board orientation: 1 = Black at bottom, 0 = White at bottom.
 */

enum GameRelation {
	ISOLATED = -3,
	OBSERVING_BEING_EXAMINED = -2,
	EXAMINER = 2,
	PLAYING_THEY_MOVE = -1,
	PLAYING_WE_MOVE = 1,
	OBSERVER = 0
};

enum GameCommand {
	CMD_NONE,
	CMD_NEW,
	CMD_MOVE,
	CMD_END,
	CMD_QUIT
};

struct Style12 {
	Piece board[64];
	Color side_to_move;
	File double_pawn_push;
	bool castle_short_white;
	bool castle_long_white;
	bool castle_short_black;
	bool castle_long_black;
	int ply_clock;
	int game_number;
	char name_white[20];
	char name_black[20];
	GameRelation relation;
	int initial_time;
	int initial_inc;
	int material_white;
	int material_black;
	int rem_time_white;
	int rem_time_black;
	int move_number;
	Move move;
	int time_used;
	int flip_field;
	
	// additional information, not specified by style 12
	char fen[200];
	GameCommand cmd;
};


const int BATMAN_WISDOM_COUNT = 42;
const int BATMAN_LONGEST_TEXT = 300;
const char* const batman_wisdom[] = {
		"Stop fiddling with that atomic pile and come down here!", 
		"In the end, veracity and rectitude always triumph.", 
		"Fore-warned is fore-armed.", 
		"Better three hours too soon than a minute too late.", 
		"Let's go, %s. The longer we tarry, the more dire the peril.", 
		"People who stay in glass hotels shouldn't throw parties.", 
		"Out of the sarcophagus and back into the saddle.", 
		"This is torture, at its most bizarre and terrible.", 
		"Good, even though it's sometimes sidetracked, always, repeat: always triumphs over evil.", 
		"Whatever is fair in love and war is also fair in chess.", 
		"An opportunity well taken is always a weapon of advantage.", 
		"He who hath life hath time. A proverb worth remembering.", 
		"Let's beware and be wary.", 
		"He who knows how to fear, %s, knows how to proceed with safety.", 
		"How little do we know of time, %s. A one-syllable word, a noun. Yesterday's laughter, tomorrow's tears.", 
		"%s, a bit of advice. Always inspect a jukebox carefully. These machines can be deadly.", 
		"I'd rather die than beg for such a small favor as my life.", 
		"A man's gotta do what a man's gotta do.", 
		"This time, %s gave the party. Next time i'll hand out the door prizes.", 
		"It's beddy-bye for you, %s.", 
		"You're a rare lady, %s, you're right on time.", 
		"Your propinquity could make a man forget himself.", 
		"When you get a little older, you'll see how easy it is to become lured by the female of the species.", 
		"Beware of strong stimulants, %s.", 
		"An older head can't be put on younger shoulders.", 
		"%s, England has no king now. England has a queen, and a great lady she is, too.", 
		"You know your neosauruses well, %s. Peanut butter sandwiches it is.", 
		"You're far from mod, %s. And many hippies are older than you are.", 
		"Language is the key to world peace. If we all spoke each other's tongues, perhaps the scourge of war would be ended forever.", 
		"Astronomy is more than mere fun, %s. It helps give us a sense of proportion. Reminds us how little we are, really. People tend to forget that sometimes.", 
		"Human mechanisms are made by human hands, %s. None of them is infallible. It is a lesson that must be faced.", 
		"That's life, %s, full of ups and downs. It ill befits any of us to become to confident.", 
		"Remember %s, always look both ways.", 
		"Saribus Sacer. A species of ancient Egyptian beetle, sacred to the Sun God, Hymeopolos. And from which the term scarab is derived. But, you should know that, %s, if you are up on your studies of Egyptology.", 
		"Experience teaches slowly, %s. And at a cost of many mistakes.", 
		"All virtues are, old chum. Indeed, that's why they're virtues.", 
		"Whoa! You came down that pole like a pro, %s.", 
		"The batcomputer is none too frisky today, %s.", 
		"Olé, %s! Olé!", 
		"I bet even Shakespeare didn't have words for such villainy!", 
		"%s, take the word 'bank' and spell it backwards.", 
		"I never knew there were no punctuation marks in alphabet soup!"
};



extern queue<Style12*> cmd_queue;
extern bool game_started;
extern int move_made;
extern bool game_ended;
extern bool thinking;
extern bool quit;
extern bool playing;
extern bool performance_test;
extern int skill;

extern Move curr_ponder_move;
extern Move last_ponder_move;
extern bool pondering;
extern bool ponderhit;

extern int gameend_cmd_count;
extern bool auto_accept;

extern bool sendlinesFics_kill;


void connectFics();
void disconnectFics();

void sendFics(char* str);

#if defined(_MSC_VER)
	DWORD WINAPI recvFics(LPVOID arg);
	DWORD WINAPI sendlinesFics(LPVOID str);
#else
	void* recvFics(void* arg);
	void* sendlinesFics(void* str);
#endif

void* userInput(void* arg);

void parseFics(char* str);


bool stringStartsWith(const char* str1, const char* str2);
bool stringEndsWith(const char* str1, const char* str2);

void style12FromString(Style12& s12, char* str, File last_double_pawn_push);

#endif /* FICS_H_ */
