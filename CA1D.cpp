
#include <cmath>
#include <stdlib.h>
#include <time.h>
#include "CA1D.h"

CA1D::CA1D(int numCells, std::vector<int>& ruleset) {
	cells_.resize(numCells, 0);
	cellsBuffer_.resize(numCells, 0);
	ruleset_ = ruleset;
	srand(time(NULL));
}

void CA1D::update() {
	cellsBuffer_.clear();
	cellsBuffer_.resize(cells_.size(), 0);
	int l = 0;
	int r = 0;
	
	for(unsigned int i = 0; i < cells_.size(); i++) {
		l = i - 1;
		if(l < 0) l = cells_.size() - 1;
		r = (i + 1) % cells_.size();
		
		for(unsigned int j = 0; j < ruleset_.size(); j++) {
			if(cells_[l] == j / (numStates_ * numStates_) && cells_[i] == (j / numStates_) % numStates_ && cells_[r] == j % numStates_) {
				cellsBuffer_[i] = ruleset_[j];
			}
		}
	}
	
	cells_ = cellsBuffer_;
}

void CA1D::randomSeed(float p) {
	for(unsigned int i = 0; i < cells_.size(); i++) {
		cells_[i] = rand() % 100 < p * 100 ? 1 : 0;
	}
}

int CA1D::getNumCells() {
	return cells_.size();
}

int CA1D::getCell(int idx) {
	return cells_[idx];
}

void CA1D::setCell(int idx, int val) {
	cells_[idx] = val;
}