/*
 * tuning.cpp
 *
 *  Created on: 23.10.2011
 *      Author: golmman
 * 
 * http://chessprogramming.wikispaces.com/Stockfish's+Tuning+Method
 */




#include "tuning.h"
#include "rkiss.h"
#include "types.h"

#include <iostream>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

using namespace std;



double VarTuning::rand_double() {
	// generate random number
	return (double)(rkiss.rand<unsigned>() % 16777216) / 16777216;
}

void VarTuning::rand_unit_vector(tune_t* vector, int size) {
	
	double norm;
	int64_t s = 0;
	int64_t ran;
	
	for (int k = 0; k < size; ++k) {
		ran = rkiss.rand<unsigned>() % 16777216;
		ran *= (rkiss.rand<unsigned>() % 2 ? 1 : -1);
		vector[k] = tune_t(ran);
		s += (ran * ran);
	}
	
	norm = sqrt(s);

	for (int k = 0; k < size; ++k) {
		vector[k] /= norm;
		cout << vector[k] << endl;
	}
}




VarTuning::VarTuning(int s, ...) {
	
	for (int k = abs(rand() % 10000); k > 0; --k) {
		rkiss.rand<unsigned>();
	}
	
	updateCount = 0;
	size = s;
	
	assert(size >= 0 && size < 100);
	
	va_list vl;
	va_start(vl, s);
	for (int k = 0; k < size; ++k) {
		var[k].valueptr = (tune_t*)va_arg(vl, double*);
		var[k].value = *var[k].valueptr;
		var[k].startvalue = *var[k].valueptr;
		var[k].deltaAxisFactor = DELTA_AXIS_FACTOR;
		var[k].sdevSum = 0.0;
		var[k].varSum = 0.0;
		var[k].applyFactor = APPLY_FACTOR;
		memset(var[k].history, 0, sizeof(tune_t) * TUNE_HISTORY_SIZE);
		var[k].historyIndex = 0;
	}
	va_end(vl);
	
}


void VarTuning::prepare_deltas() {
	double norm;
	int64_t s = 0;
	int64_t ran;
	
	for (int k = 0; k < size; ++k) {
		ran = rkiss.rand<unsigned>() % 16777216;
		ran *= (rkiss.rand<unsigned>() % 2 ? 1 : -1);
		var[k].delta = tune_t(ran);
		s += (ran * ran);
	}
	
	norm = sqrt(s);
	
	// normalize
	for (int k = 0; k < size; ++k) {
		var[k].delta *= (var[k].startvalue * var[k].deltaAxisFactor / norm);
	}
}


void VarTuning::prepare_vars(Color c) {
	tune_t sign = (c == WHITE ? 1 : -1);
	
	for (int k = 0; k < size; ++k) {
		*var[k].valueptr = var[k].value + (sign * var[k].delta);
	}
}


void VarTuning::update_vars(ResultPGN winner) {
	tune_t sign = (winner == WHITE_WINS ? 1 : -1);
	++updateCount;
	
	for (int k = 0; k < size; ++k) {
		*var[k].valueptr = var[k].value + (sign * var[k].delta * var[k].applyFactor);
		var[k].value = *var[k].valueptr;
		var[k].varSum += var[k].value;
		var[k].mean = var[k].varSum / updateCount;
		var[k].sdevSum += ((var[k].value - var[k].mean) * (var[k].value - var[k].mean));
		var[k].sdev = sqrt(var[k].sdevSum / updateCount);
		
		var[k].history[var[k].historyIndex] = var[k].value;
		++var[k].historyIndex;
		
		if (var[k].historyIndex >= TUNE_HISTORY_SIZE) {
			var[k].historyIndex = 0;
		}
		
		tune_t mean = 0;
		for (int i = 0; i < TUNE_HISTORY_SIZE; ++i) {
			mean += (var[k].history[i] / TUNE_HISTORY_SIZE);
		}
		
		if (k == 0) cout << "mean: " << mean << endl;
		
		tune_t sdev = 0;
		for (int i = 0; i < TUNE_HISTORY_SIZE; ++i) {
			sdev += ((var[k].history[i] - mean) * (var[k].history[i] - mean) / TUNE_HISTORY_SIZE); 
		}
		var[k].historySdev = sqrt(sdev);
	}
}

void VarTuning::print_vars() {
	for (int k = 0; k < size; ++k) {
		cout << *var[k].valueptr << " ";
	}
	cout << endl;
}

void VarTuning::print_deltas() {
	for (int k = 0; k < size; ++k) {
		cout << var[k].delta << " ";
	}
	cout << endl;
}















