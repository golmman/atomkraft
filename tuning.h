/*
 * tuning.h
 *
 *  Created on: 23.10.2011
 *      Author: golmman
 *      
 * Kalle 23/24.10.2011
 */

#ifndef TUNING_H_
#define TUNING_H_

#include "rkiss.h"
#include "pgn.h"

const int TUNE_HISTORY_SIZE = 200;
const int TUNE_DIM = 100;
const float DELTA_AXIS_FACTOR = 0.2f;
const float APPLY_FACTOR = 0.02f;

typedef double tune_t;



struct TunedVariable {
	tune_t* valueptr;			// value to be tuned
	tune_t value;
	tune_t startvalue;
	tune_t delta;			// radius around value
	tune_t deltaSdev;		// used to couple delta to sdev
	tune_t deltaAxisFactor;	// delta = deltaAxisFactor * value
	tune_t applyFactor;		//
	tune_t applyFactorSdev;	// used to couple applyFactor to sdev
	tune_t varSum;			// used to calculate mean
	tune_t sdevSum;			// used to calculated sdev
	tune_t mean;			// mean
	tune_t sdev;			// standard deviation
	
	int historyIndex;
	tune_t history[TUNE_HISTORY_SIZE];	// stores the last values
	tune_t historySdev;				// standard deviation of the history values
};

class VarTuning {
public:
	TunedVariable var[TUNE_DIM];
	int size;
	int updateCount;
	RKISS rkiss;
	
	VarTuning(int s, ...);
	void prepare_deltas();
	void prepare_vars(Color c);
	void update_vars(ResultPGN winner);
	
	void print_vars();
	void print_deltas();
	
	double rand_double();
	void rand_unit_vector(tune_t* vector, int size);
};



#endif /* TUNING_H_ */
