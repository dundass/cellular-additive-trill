
#include <vector>

class CA1D {
	
	public:
	CA1D() {}
	CA1D(int numCells);
	CA1D(int numCells, std::vector<int>& ruleset);
	~CA1D() {}
	
	void update();
	void randomSeed(float p);
	int getNumCells();
	int getCell(int idx);
	void setCell(int idx, int val);
	
	private:
	int numStates_ = 2;	// todo - expand to any number of states 0-N
	std::vector<int> cells_;
	std::vector<int> cellsBuffer_;
	std::vector<int> ruleset_;
	
};