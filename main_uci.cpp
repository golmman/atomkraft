/*
 * main_uci.cpp
 *
 *  Created on: 11.10.2011
 *      Author: golmman
 */


#include "main.h"
#include "thread.h"

#include <iostream>
#include <string>
using namespace std;


extern bool execute_uci_command(const string& cmd);


void main_uci(int argc) {

	if (argc < 2)
	{
		// Print copyright notice
		//cout << engine_name() << " by " << engine_authors() << endl;

		//if (CpuHasPOPCNT)
		//	cout << "Good! CPU has hardware POPCNT." << endl;

		// Wait for a command from the user, and passes this command to
		// execute_uci_command() and also intercepts EOF from stdin to
		// ensure that we exit gracefully if the GUI dies unexpectedly.
		string cmd;
		while (getline(cin, cmd) && execute_uci_command(cmd)) {}
	}

	Threads.exit();
}
