/*
 * fics.cpp
 *
 *  Created on: 08.08.2011
 *      Author: golmman
 */

#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0600		// define WINVER to use winsock2

#include "fics.h"

#include "search.h"
#include "position.h"
#include "debug.h"
#include "pgn.h"
#include "ucioption.h"
#include "main.h"

#include <pthread.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <ctype.h>

#undef WIN32_LEAN_AND_MEAN

using std::cout;
using std::cin;
using std::endl;

#if defined (__GNUC__)
	void WSAAPI freeaddrinfo (struct addrinfo*);
	int WSAAPI getaddrinfo (const char*,const char*,const struct addrinfo*, struct addrinfo**);
#endif

// global
queue<Style12*> cmd_queue;

bool game_started = false;
bool game_ended = false;
bool thinking = false;
bool quit = false;
bool playing = false;
bool performance_test = false;

Move curr_ponder_move = MOVE_NONE;
Move last_ponder_move = MOVE_NONE;
bool pondering = false;
bool ponderhit = false;

int gameend_cmd_count = 0;
bool auto_accept = false;
bool send_batman_wisdom = false;
bool startgame_beep = false;

bool sendlinesFics_kill = false;
bool unrated = false;
int skill = 20;


SOCKET fics_socket;

char waitbuf[4096]; 	// collects data until \n appears
char gameendstr[200]; 	// stores string to recognize end of the game
char gameendstr_dummy[] = "ÄÄÄ";	// a string which is never sent by fics 
char excuse[BATMAN_LONGEST_TEXT + 50];

char start_fen[200];		// stores the start position as fen
char start_name_white[30];
char start_name_black[30];
int start_init_time;
int start_init_inc;

Move movelist[1024];
int movelist_counter = 0;
File double_pawn_push;



void connectFics() {
	waitbuf[0] = 0;
	strcpy(gameendstr, gameendstr_dummy);
	
	/*
	 * startup
	 */
	WSADATA wsaData;
	
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
	    printf("WSAStartup failed: %d\n", iResult);
	    exit(EXIT_FAILURE);
	}
	
	/*
	 * create a client socket
	 */
	struct addrinfo *result = NULL,
	                *ptr = NULL,
	                hints;

	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	
	// Resolve the server address and port
	
	// starting timeseal
	PROCESS_INFORMATION processInformation;
	STARTUPINFO startupInfo;
    memset(&processInformation, 0, sizeof(processInformation));
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
	
	if (!CreateProcessA(
		NULL, 
		"timeseal.bat", 
		NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,
		NULL, NULL, &startupInfo, &processInformation)) {
		cout << "ERROR: timeseal.exe not found" << endl;
	}
	Sleep(2000);
	
	//iResult = getaddrinfo("www.freechess.org", "23", &hints, &result);
	iResult = getaddrinfo("localhost", "5503", &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	
	fics_socket = INVALID_SOCKET;
	
	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr=result;

	// Create a SOCKET for connecting to server
	fics_socket = socket(ptr->ai_family, ptr->ai_socktype, 
	    ptr->ai_protocol);
	
	if (fics_socket == INVALID_SOCKET) {
		// %ld
	    printf("Error at socket(): %i\n", WSAGetLastError());
	    freeaddrinfo(result);
	    WSACleanup();
	    exit(EXIT_FAILURE);
	}
	
	/*
	 * connect to socket
	 */
	
	// Connect to server.
	iResult = connect(fics_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
	    closesocket(fics_socket);
	    fics_socket = INVALID_SOCKET;
	}

	// Should really try the next address returned by getaddrinfo
	// if the connect call failed
	// But for this simple example we just free the resources
	// returned by getaddrinfo and print an error message

	freeaddrinfo(result);

	if (fics_socket == INVALID_SOCKET) {
	    printf("Unable to connect to server!\n");
	    WSACleanup();
	    exit(EXIT_FAILURE);
	}
	
	/*
	 * sending and receiving
	 */
	
#if defined(_MSC_VER)
	CreateThread(NULL, 0, recvFics, 0, 0, NULL);
#else
	pthread_t recv_thread;
	pthread_create(&recv_thread, 0, recvFics, 0);
#endif

	
	
	//pthread_t user_thread;
	//pthread_create(&user_thread, NULL, userInput, NULL);
	
	
	
	/*
	 * disconnecting
	 */

}

void* userInput(void* arg) {
	char sendbuf[100];
	sendbuf[0] = 0;
	
	char test[100];
	
	for (;;) {
		cout << "test" << endl;
		
		cin >> test;
		
		if (test[strlen(test)-1] != ';') strcat(test, " ");
		
		strcat(sendbuf, test);
		

		
		if (sendbuf[strlen(sendbuf)-1] == ';') {
			sendbuf[strlen(sendbuf)-1] = '\n';
			
			sendFics(sendbuf);
			
			if (strcmp(sendbuf, "quit;") == 0) {
				quit = true;
				return 0;
			}
			
			sendbuf[0] = 0;
		}
		

		
	}
	
	return 0;
}


void disconnectFics() {
	// shutdown the send half of the connection since no more data will be sent
	if (shutdown(fics_socket, SD_SEND) == SOCKET_ERROR) {
	    printf("shutdown failed: %d\n", WSAGetLastError());
	    closesocket(fics_socket);
	    WSACleanup();
	    return;
	}
	
	// cleanup
	closesocket(fics_socket);
	WSACleanup();
}

#if defined(_MSC_VER)
	DWORD WINAPI recvFics(LPVOID arg)
#else
	void* recvFics(void* arg)
#endif
	{
	int iResult;
	
	char recvbuf[2048];
	int recvbuflen = 2048;
	
	
	// Receive data until the server closes the connection
	do {
		// recv blocks if there is nothing received
	    iResult = recv(fics_socket, recvbuf, recvbuflen - 1, 0);
	    recvbuf[iResult] = 0;

	    
	    if (iResult > 0) {
	    	// fics ends lines with \n\r so output line wise
	    	
	    	char* startbuf = recvbuf;
	    	char* endbuf = strchr(startbuf, 13);
	    	
	    	for(;;) {
	    		
	    		if (endbuf != 0) {
	    			*endbuf = 0;
	    			
	    			// replace char 7's, they make the computer beep
	    			char* beep = strchr(startbuf, 7);
	    			if (beep) *beep = ' ';
	    			
	    			// it seems the unsynchronized stdout operations tend to crash the programm
#ifdef NDEBUG
	    			if (!playing) 
#endif
	    				printf(startbuf);
	    			
	    			
	    			parseFics(startbuf);
	    			
	    			startbuf = endbuf + 1;
	    			endbuf = strchr(startbuf, 13);
	    		} else {
	    			
	    			// replace char 7's, they make the computer beep
	    			char* beep = strchr(startbuf, 7);
	    			if (beep) *beep = ' ';
#ifdef NDEBUG
	    			if (!playing) 
#endif
	    				printf(startbuf);
	    			
	    			
	    			parseFics(startbuf);
	    			
	    			break;
	    		}
	    	};
	    	
	    } else if (iResult == 0) {
	        printf("Connection closed\n");
	    } else {
	        printf("recv failed: %d\n", WSAGetLastError());
	    }
	    
	} while (iResult > 0);
	
	return 0;
}

void parseFics(char* str) {
	
	static char outbuf[1000];
	
//	char* aux = str;
//	while (*aux) {
//		cout << *aux << "(" << (int)*aux << ") ";
//		++aux;
//	}
//	cout << endl;
	
	// parse lines that don't end with a '\n'
	if (stringEndsWith(str, "login: ")) {
#ifdef NDEBUG
		sendFics("atomkraft\n");	// release version, normal login
#else
		sendFics("atmkrft\n");		// debug version, guest login
#endif
	} else if (stringEndsWith(str, "password: ")) {
		char* s = outbuf;
		s[0] = 0;
		strcat(s, "kyypwa\n");
//		strcat(s, "set seek 0 \n");
//		strcat(s, "set shout 0 \n");
//		strcat(s, "set cshout 0 \n");
		strcat(s, "set style 12 \n");
//		strcat(s, "set prompt ficsprompt \n");
		strcat(s, "set interface atomkraft client deluxe\n");
		//strcat(s, "alias gameend0 rematch\n");
		//strcat(s, "alias gameend0 seek 1 0 atomic f\n");
		//strcat(s, "alias gameend1 seek 3 0 atomic f\n");
		//strcat(s, "alias gameend2 seek 5 2 atomic f\n");
//		strcat(s, "set silence 1 \n");
		strcat(s, "iset atomic 1 \n");
		//strcat(s, "-ch 53 \n");
		//strcat(s, "-ch 4 \n");
//		strcat(s, "help mule \n");
		strcat(s, "tell golmman Moin Moin!\n");

#if defined(_MSC_VER)
		CreateThread(NULL, 0, sendlinesFics, (LPVOID)s, 0, NULL);
#else
		pthread_t sendall;
		pthread_create(&sendall, 0, sendlinesFics, (void*)s);
#endif

		
	}
	
	// fill the buffer
	strcat(waitbuf, str);
	
	if (waitbuf[strlen(waitbuf) - 1] == '\n') {
		// we reached a line end so parse
		
		
		
		if (strstr(waitbuf, "Press return to enter the server as")) {

			// enter server as guest
			char* s = outbuf;

			s[0] = 0;
			strcat(s, "\n");
			strcat(s, "set seek 0 \n");
			strcat(s, "set shout 0 \n");
			strcat(s, "set cshout 0 \n");
			strcat(s, "set style 12 \n");
			strcat(s, "set prompt ficsprompt \n");
			strcat(s, "-ch 53 \n");
			strcat(s, "-ch 4 \n");
			strcat(s, "+ch 99 \n");
			strcat(s, "alias gameend0 match sordid 3 0 unrated atomic\n");
//			strcat(s, "help mule \n");
			strcat(s, "set 0 Debug version of atomkraft! \"finger atomkraft\"\n");
			strcat(s, "tell golmman Moin Moin!\n");
			
			
#if defined(_MSC_VER)
			CreateThread(NULL, 0, sendlinesFics, (LPVOID)s, 0, NULL);
#else
			pthread_t sendall;
			pthread_create(&sendall, 0, sendlinesFics, (void*)s);
#endif


//		} else if (strstr(waitbuf, "<12> rnbqkbnr pppppppp -------- -------- -------- -------- PPPPPPPP RNBQKBNR")) {
//			// new game has started
//			game_started = true;
//			movelist_counter = 0;
//			
//			// read board info
//			style12FromString(boardinfo, waitbuf, double_pawn_push);
//			double_pawn_push = boardinfo.double_pawn_push;
			
		
		} else if (stringStartsWith(waitbuf, "Creating: ")) {
			
			// are we playing an unrated game?
			if (strstr(waitbuf, "unrated atomic")) {
				unrated = true;
				
				if (skill == 20) {
					Options["Skill Level"] = UCIOption(20, 0, 20);
					Options["Ponder"] = UCIOption(true);
				}
				
			} else {
				unrated = false;
				Options["Skill Level"] = UCIOption(20, 0, 20);
				Options["Ponder"] = UCIOption(true);
			}
			
		} else if (stringStartsWith(waitbuf, "<12> ")) {
			// opponent has moved or game has started
			
			if (!playing) {
				cout << "fics.cpp game started" << endl;
				
				// first kill the gameend sendlinesFics thread (invoked in main.cpp)
				sendlinesFics_kill = true;
				
				// games starts
				playing = true;
				game_started = true;
				movelist_counter = 0;
				
				// beep
				if (startgame_beep) {
					Beep(2700, 500);
					//startgame_beep = false;
				}
				
				// read board info
				Style12* s12 = new Style12;
				s12->cmd = CMD_NEW;
				style12FromString(*s12, waitbuf, double_pawn_push);
				double_pawn_push = s12->double_pawn_push;
				
				// fill start variables
				strcpy(start_fen, s12->fen);
				strcpy(start_name_white, s12->name_white);
				strcpy(start_name_black, s12->name_black);
				start_init_time = s12->initial_time;
				start_init_inc = s12->initial_inc;
				
				cmd_queue.push(s12);
				
				
				// set skill level in unrated games
				if (unrated && skill < 20) {
					Options["Skill Level"] = UCIOption(skill, 0, 20);
					Options["Ponder"] = UCIOption(false);
					
					// remember: strcmp == 0 when strings are equal
					char* oppname 
					  = strcmp(start_name_white, "atomkraft") ? start_name_white : start_name_black;
					
					outbuf[0] = 0;
					sprintf(outbuf, "tell %s My playing skill in unrated games is currently set to %i. When I am not playing you can change my skill by telling me 'set skill n' where n is an integer from 0 (weakest) to 20 (strongest).\n"
							, oppname, skill);
					sendFics(outbuf);
					
					// whisper that we don't play at full strength
					outbuf[0] = 0;
					sprintf(outbuf, "whisper My playing skill in unrated games is currently set to %i. When I am not playing you can change my skill by telling me 'set skill n' where n is an integer from 0 (weakest) to 20 (strongest).\n"
						, skill);
					sendFics(outbuf);
				}
				
				
				// recognize game end
				gameendstr[0] = 0;
				sprintf(
					gameendstr, 
					"{Game %d (%s vs. %s)", 
					s12->game_number, 
					s12->name_white, 
					s12->name_black);
				
			} else {
				cout << "fics.cpp move made" << endl;
				
				// move has been made
				
				// read board info
				Style12* s12 = new Style12;
				s12->cmd = CMD_MOVE;
				style12FromString(*s12, waitbuf, double_pawn_push);
				double_pawn_push = s12->double_pawn_push;
				cmd_queue.push(s12);
				
				// record the move
				movelist[movelist_counter] = s12->move;
				//cout << "RECORDED MOVE: " << move_to_string(boardinfo.move) << endl;
				++movelist_counter;
				
				// stop pondering
				if (   Options["Ponder"].value<bool>() 
					&& s12->relation == PLAYING_WE_MOVE
					&& pondering) {
					
					cout << "stop pondering" << endl;
					if (s12->move == last_ponder_move) {
						stdin_fake << "ponderhit" << endl;
						ponderhit = true;
					} else {
						stdin_fake << "stop" << endl;
						ponderhit = false;
					}
				}
			}
			
			

			
			
		} else if (stringStartsWith(waitbuf, gameendstr)) {
			cout << "fics.cpp game ended, gameendstr " << gameendstr << endl;
			
			// game ended
			Style12* s12 = new Style12;
			s12->cmd = CMD_END;
			cmd_queue.push(s12);
			
			game_ended = true;
			playing = false;
			sendlinesFics_kill = false;
			
//			cout << "ALL RECORDED MOVES: " << endl;
//			for (int k = 0; k < movelist_counter; ++k) {
//				cout << move_to_string(movelist[k]) << endl;
//			}
			
			
			// reset gameendstr
			strcpy(gameendstr, gameendstr_dummy);
			
			
			if (movelist_counter > 0) {
				// save pgn
				ResultPGN result;
				if (stringEndsWith(waitbuf, "1-0\n")) {
					result = WHITE_WINS;
				} else if (stringEndsWith(waitbuf, "0-1\n")) {
					result = BLACK_WINS;
				} else if (stringEndsWith(waitbuf, "1/2-1/2\n")) {
					result = DRAW;
				} else {
					result = NO_RESULT;
				}
				
				cout << "save pgn..." << endl;
				appendPGN(
					"d:/atomkraft-fics.pgn", 
					start_name_white, 
					start_name_black, 
					result, start_init_time, start_init_inc, 
					start_fen, movelist, movelist_counter);
				cout << "finished" << endl;
				
				// send batman wisdom
				if (send_batman_wisdom) {
					char* oppname;
					
					if (strcmp(start_name_white, "atomkraft") == 0) {
						oppname = start_name_black;
					} else {
						oppname = start_name_white;
					}
					
					if (   (result == WHITE_WINS && oppname == start_name_white)
						|| (result == BLACK_WINS && oppname == start_name_black)) {
						// we lost the game... so send a batman wisdom as an excuse
						cout << "send batman wisdom..." << endl;
						
						int batman_rand = rand() % BATMAN_WISDOM_COUNT;
						char excuse_aux[BATMAN_LONGEST_TEXT];
						
						sprintf(excuse_aux, const_cast<const char*>(batman_wisdom[batman_rand]), oppname);
						sprintf(excuse, "tell %s %s\n", "99", excuse_aux);
						
						sendFics(excuse);
					}
				}
				
				cout << endl;
				movelist_counter = 0;
				
				// tell engine to stop thinking
				ponderhit = false;
				stdin_fake << "stop" << endl;
			}
			
			
		} else if (stringStartsWith(waitbuf, "Challenge: ")) {
			
			// someone challenges atomkraft
			if (auto_accept) {
				sendFics("accept\n");
			}
		
		} else if (strstr(waitbuf, " tells you: get skill")) {
			// TODO: user input
			cout << "GET SKILL" << endl;
			
			char name[30];
			
			char* c1 = waitbuf;
			char* c2 = name;
			while (*c1 != ' ' && *c1 != '(') *c2++ = *c1++;
			*c2 = 0;
			
			sprintf(outbuf, "tell %s Skill is set to %i/20.\n", name, skill);
			sendFics(outbuf);
			
		} else if (strstr(waitbuf, " tells you: set skill ")) {
			
			

			

			char name[30];
			int number = 0;
			bool err = false;

			char* c1 = waitbuf;
			char* c2 = name;
			while (*c1 != ' ' && *c1 != '(') *c2++ = *c1++;
			*c2 = 0;

			cout << name << endl;


			// waitbuf[strlen(waitbuf) - 1] is \n
			char digit1 = waitbuf[strlen(waitbuf) - 3];
			char digit2 = waitbuf[strlen(waitbuf) - 2];

			if ((digit1 == ' ' || isdigit(digit1)) && isdigit(digit2)) {
				if (digit1 == ' ') {
					number = (digit2 - '0');
				} else {
					number = (digit1 - '0') * 10 + (digit2 - '0');
				}

				if (number > 20) err = true;
			} else {
				err = true;
			}

			if (digit1 != ' ' && waitbuf[strlen(waitbuf) - 4] != ' ') err = true;

			outbuf[0] = 0;
			if (err || playing) {
				cout << "SET SKILL DENIED" << endl;
				
				if (playing) {
					sprintf(outbuf, "tell %s Sorry, I can't change the skill while playing a game.\n", name);
					sendFics(outbuf);
				} else {
					sprintf(outbuf, "tell %s Sorry, I didn't understand that.\n", name);
					sendFics(outbuf);
				}
			} else {
				cout << "SET SKILL SUCCESSFUL" << endl;
				
				skill = number;
				sprintf(outbuf, "tell %s My playing skill in unrated games is now set to %i/20.\n", name, number);
				sendFics(outbuf);

				sprintf(outbuf, "tell golmman %s set skill to %i/20.\n", name, skill);
				sendFics(outbuf);
			}
			
			
		} else if (stringStartsWith(waitbuf, "golmman tells you: ")) {
			// golmman sent a command
			
			char* command = waitbuf + strlen("golmman tells you: ");
			cout << "COMMAND: " << command;
			
			if (strlen(command) > 240) {
				cout << "WARNING: command exceeds 240 characters!" << endl;
			}
			
			if (stringStartsWith(command, ">")) {
				// golmman expects some data
				outbuf[0] = 0;
				
				enum OutType {	BOOL, DIGIT/*, STRING*/	};
				struct CmdVar {
					int8_t* var;		char name[30]; 		OutType type;
					CmdVar(int8_t* v, char* n, OutType t) : var(v), type(t) {	strcpy(name, n); }
					
					bool set(int8_t v) {					
						switch (type) {
						case BOOL: 
							if (v == 0 || v == 1) *var = v;
							else return false;
							break;
						case DIGIT: 
							if (v >= 0 && v <= 9) *var = v;
							else return false;
							break;
//						case STRING: 
//							strncpy(ptr, v, strlen(ptr));
//							break;
						}
						return true;
					}
				};
				
				static CmdVar cmd_vars[] = {
					CmdVar((int8_t*)&auto_accept, "auto_accept", BOOL),
					CmdVar((int8_t*)&gameend_cmd_count, "gameend_cmd_count", DIGIT),
					CmdVar((int8_t*)&playing, "playing", BOOL),
					CmdVar((int8_t*)&send_batman_wisdom, "send_batman_wisdom", BOOL),
					CmdVar((int8_t*)&startgame_beep, "startgame_beep", BOOL),
					CmdVar((int8_t*)&performance_test, "performance_test", BOOL),
					CmdVar((int8_t*)&skill, "skill", DIGIT),
					CmdVar(0, "", BOOL)
				};
				
				bool cmd_get = stringStartsWith(command, ">get ");
				bool cmd_set = stringStartsWith(command, ">set ");
				bool cmd_help = stringStartsWith(command, ">help");
				bool valid_input = false;
				char* token0 = command + strlen(">get ");
				char* token1;
				
				CmdVar* cv = cmd_vars;
				do {
					if (cmd_get) {
						if (strncmp(token0, cv->name, strlen(cv->name)) == 0) {
							sprintf(outbuf, "tell golmman %s: %d\n", 
									cv->name, int(*cv->var));
							sendFics(outbuf);
							valid_input = true;
							break;
						}
						
					} else if (cmd_set) {
						if (strncmp(token0, cv->name, strlen(cv->name)) == 0) {
							token1 = token0 + strlen(cv->name) + 1;
							if (cv->set(int8_t(*token1 - '0'))) {
								sprintf(outbuf, "tell golmman %s set to %d\n", 
										cv->name, int(*cv->var));
								sendFics(outbuf);
							} else {
								sendFics("tell golmman invalid parameter input\n");
							}
							valid_input = true;
							break;
						}
						
					} else if (cmd_help) {
						sprintf(outbuf, "tell golmman ");
						CmdVar* cv2 = cmd_vars;
						do {
							char num[5];
							itoa(*cv2->var, num, 10);
							
							strcat(outbuf, cv2->name);
							strcat(outbuf, " = ");
							strcat(outbuf, num);
							strcat(outbuf, ", ");
						} while ((++cv2)->var);
						strcat(outbuf, "\n");
						sendFics(outbuf);
						
						valid_input = true;
						break;
					}
				} while ((++cv)->var);
				
				if (!valid_input) {
					sendFics("tell golmman invalid input\n");
				}
				
			} else {
				// send data to fics server
				
				if (strcmp(command, "quit\n") == 0) {
#ifdef SWEN_VERSION
					cout << "swen version quit" << endl;
					Beep(2700, 1000);
					exit(0);
#endif
					
					// read board info
					Style12* s12 = new Style12;
					s12->cmd = CMD_QUIT;
					cmd_queue.push(s12);
					quit = true;
					
					// tell engine to stop thinking
					stdin_fake << "stop" << endl;
				} else if (strcmp(command, "") == 0) {

				}

				sendFics(command);
				
			}
			
			
 		}
		
		// clear waitbuf
		waitbuf[0] = 0;
	}
	

}

/*void sendFics(char* str) {
	cout << "Command sent: " << str;
	send(fics_socket, str, strlen(str), 0);
	//pthread_t sendthr;
	//pthread_create(&sendthr, 0, sendFics_thread, (void*)str);
	
}*/

void sendFics(char *str) {
	int len = strlen(str);
    int total = 0;			// how many bytes we've sent
    int bytesleft = len;	// how many we have left to send
    int n;
    
#ifndef NDEBUG
    cout << "SEND: " << str << endl;
#endif

    while(total < len) {
        n = send(fics_socket, str+total, bytesleft, 0);
        if (n == -1) { 
        	cout << "SENDERROR: " << total << "\\" << len << " bytes sent" << endl;
        	break; 
        }
        total += n;
        bytesleft -= n;
    }
} 


#if defined(_MSC_VER)
	DWORD WINAPI sendlinesFics(LPVOID str)
#else
	void* sendlinesFics(void* str)
#endif

	{
	
	char* start = (char*)str;
	char* end = (char*)str;
	char aux;
	
	Sleep(2000);

	end = strchr(end, '\n');

	while (end) {
		
		// kill the thread if it is necessary
		if (sendlinesFics_kill) {
			cout << "sendlinesFics_kill" << endl;
			sendlinesFics_kill = false;
			return 0;
		}
		
		aux = *(end+1);
		*(end+1) = 0;
		
		sendFics(start);
		Sleep(2000);
		
		*(end+1) = aux;
		start = end+1;
		end = strchr(start, '\n');
	}

	return 0;
}



char* s12FromString_nextToken(char* c, char* token) {
	// copy string
	int k = 0;
	while (*c != ' ' && *c != 0 && *c != '\n') {
		token[k] = *c;
		++c;
		++k;
	}
	token[k] = 0;
	
	if (*c == ' ') return c + 1;
	if (*c == '\n' || *c == 0) return c + 1;
	
	assert(0);
	return c;
}

void style12FromString(Style12& s12, char* str, File last_double_pawn_push) {
	char token[50];
	
	char* c = str;
	
//	for (int k = 0; k < 31; ++k) {
//		c = s12FromString_nextToken(c, token);
//		cout << token << endl;
//	}
//	cout << "----------" << endl;
//	
//	c = str;
	
	// identifier
	c = s12FromString_nextToken(c, token);
	assert(strcmp(token, "<12>") == 0);
	
	// board
	for (int i = 0; i < 8; ++i) {
		c = s12FromString_nextToken(c, token);
		for (int j = 0; j < 8; ++j) {
			Piece p = PIECE_NONE;
			switch (token[j]) {
			case '-': p = PIECE_NONE; break;
			case 'P': p = WP; break; case 'N': p = WN; break; case 'B': p = WB; break;
			case 'R': p = WR; break; case 'Q': p = WQ; break; case 'K': p = WK; break;
			case 'p': p = BP; break; case 'n': p = BN; break; case 'b': p = BB; break;
			case 'r': p = BR; break; case 'q': p = BQ; break; case 'k': p = BK; break;
			default: cout << "ERROR: style12FromString board" << endl; break;
			}
			s12.board[make_square(File(j), Rank(7-i))] = p;
		}
	}
	
	c = s12FromString_nextToken(c, token);
	s12.side_to_move = (token[0] == 'W' ? WHITE : BLACK);
	
	c = s12FromString_nextToken(c, token);
	s12.double_pawn_push = File(atoi(token));
	
	c = s12FromString_nextToken(c, token);
	s12.castle_short_white = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	s12.castle_long_white = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	s12.castle_short_black = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	s12.castle_long_black = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	s12.ply_clock = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	s12.game_number = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	strcpy(s12.name_white, token);
	
	c = s12FromString_nextToken(c, token);
	strcpy(s12.name_black, token);
	
	c = s12FromString_nextToken(c, token);
	s12.relation = GameRelation(atoi(token));
	
	c = s12FromString_nextToken(c, token);
	s12.initial_time = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	s12.initial_inc = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	s12.material_white = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	s12.material_black = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	s12.rem_time_white = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	s12.rem_time_black = atoi(token);
	
	c = s12FromString_nextToken(c, token);
	s12.move_number = atoi(token);
	
	// move
	c = s12FromString_nextToken(c, token);
	if (strcmp(token, "none") == 0) {
		s12.move = MOVE_NONE;
	} else if (strcmp(token, "o-o") == 0) {
		// Note that castling moves are encoded as "king captures friendly rook" moves
		if (s12.side_to_move == WHITE) {
			s12.move = make_castle_move(SQ_E8, SQ_H8); // black castling short
		} else {
			s12.move = make_castle_move(SQ_E1, SQ_H1); // white castling short
		}
	} else if (strcmp(token, "o-o-o") == 0) {
		if (s12.side_to_move == WHITE) {
			s12.move = make_castle_move(SQ_E8, SQ_A8); // black castling long
		} else {
			s12.move = make_castle_move(SQ_E1, SQ_A1); // white castling long
		}
	} else {
		
		Square from = make_square(File(token[2] - 'a'), Rank(token[3] - '1'));
		Square to   = make_square(File(token[5] - 'a'), Rank(token[6] - '1'));
		
		// promotion
		// capture 'promotions' are not handled as such but as normal capture moves
		if (token[7] == '=' && (square_file(from) == square_file(to))) { 
			switch (token[8]) {
			case 'N':
				s12.move = make_promotion_move(from, to, KNIGHT);
				break;
			case 'B':
				s12.move = make_promotion_move(from, to, BISHOP);
				break;
			case 'R':
				s12.move = make_promotion_move(from, to, ROOK);
				break;
			case 'Q':
				s12.move = make_promotion_move(from, to, QUEEN);
				break;
			default:
				assert(0);
				break;
			}
		} else {
			bool ep = false;
			if (token[0] == 'P' && last_double_pawn_push != -1) {
				File f = last_double_pawn_push;
				Rank r = (s12.side_to_move == WHITE ? RANK_3 : RANK_6);
				
				if (to == make_square(f, r)) { // en passant
					s12.move = make_ep_move(from, to);
					ep = true;
				}
			}
			
			// normal
			if (!ep) s12.move = make_move(from, to);
		}
		
	}
	
	
	
	
	c = s12FromString_nextToken(c, token);
	s12.time_used = 0; //atoi(token);	// TODO: currently unsupported
	
	c = s12FromString_nextToken(c, token);
	// pretty move
	
	c = s12FromString_nextToken(c, token);
	s12.flip_field = atoi(token);

	// create fen
	// "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
	char* fenstr = s12.fen;
	
	// board
	int empty;
	for (int r = 7; r >= 0; --r) {
		empty = 0;
		for (int f = 0; f <= 7; ++f) {
			Square s = make_square(File(f), Rank(r));
			if (s12.board[s] == PIECE_NONE) {
				++empty;
			} else if (empty == 0) {
				*(fenstr++) = PIECE_NAME[s12.board[s]];
			} else {
				*(fenstr++) = '0' + empty;
				*(fenstr++) = PIECE_NAME[s12.board[s]];
				empty = 0;
			}
		}
		if (empty) {
			*(fenstr++) = '0' + empty;
		}
		*(fenstr++) = '/';
	}
	
	*(fenstr-1) = ' ';
	
	// side to move
	*(fenstr++) = (s12.side_to_move == WHITE ? 'w' : 'b');
	*(fenstr++) = ' ';
	
	// castling
	if (s12.castle_short_white) *(fenstr++) = 'K';
	if (s12.castle_long_white)  *(fenstr++) = 'Q';
	if (s12.castle_short_black) *(fenstr++) = 'k';
	if (s12.castle_long_black)  *(fenstr++) = 'q';
	if (*(fenstr-1) == ' ') *(fenstr++) = '-';
	*(fenstr++) = ' ';
	
	// en passant
	if (s12.double_pawn_push == -1) {
		*(fenstr++) = '-';
	} else {
		*(fenstr++) = 'a' + s12.double_pawn_push;
		*(fenstr++) = (s12.side_to_move == WHITE ? '6' : '3');
	}
	*(fenstr++) = ' ';
	
	// ply clock
	if (s12.ply_clock / 10 > 0) {
		*(fenstr++) = '0' + s12.ply_clock / 10;
		*(fenstr++) = '0' + s12.ply_clock - 10 * (s12.ply_clock / 10);
	} else {
		*(fenstr++) = '0' + s12.ply_clock;
	}
	*(fenstr++) = ' ';
	
	// move number
	if (s12.move_number / 100 > 0) {
		*(fenstr++) = '0' + s12.move_number / 100;
		*(fenstr++) = '0' + (s12.move_number - 100 * (s12.move_number / 100)) / 10;
		*(fenstr++) = '0' + s12.move_number 
					      - 100 * (*(fenstr - 2) - '0') - 10 * (*(fenstr - 1) - '0');
	} else if (s12.move_number / 10 > 0) {
		*(fenstr++) = '0' + s12.move_number / 10;
		*(fenstr++) = '0' + s12.move_number - 10 * (s12.move_number / 10);
	} else {
		*(fenstr++) = '0' + s12.move_number;
	}
	*(fenstr++) = 0;

	
}




// test if str1 ends with str2
bool stringEndsWith(const char* str1, const char* str2) {
	if (strlen(str1) < strlen(str2)) return false;
	return strcmp(str1 + strlen(str1) - strlen(str2), str2) == 0;
}

// test if str1 starts with str2
bool stringStartsWith(const char* str1, const char* str2) {
	if (strlen(str1) < strlen(str2)) return false;
	return strncmp(str1, str2, strlen(str2)) == 0;
}




