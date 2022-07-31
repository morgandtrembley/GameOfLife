#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <boost/functional/hash/hash.hpp>	// pair hashing
#include <boost/gil.hpp>					// image support

// Edit Number of Generations Here
int generations = 10;

#define BORN 1
#define DIED -1

struct cellStruct;
typedef std::pair<int64_t, int64_t> pair;
typedef std::unordered_set<pair, boost::hash<pair>> uset;
typedef std::unordered_map<pair, cellStruct, boost::hash<pair>> umap;

struct cellStruct {
	bool state = false;
	int neighbors = 0;
};

bool GameRules(const cellStruct& cell) {
	const bool ruleA = !cell.state && cell.neighbors == 3;
	const bool ruleB = cell.state && (cell.neighbors < 2 || cell.neighbors > 3);

	if (ruleA || ruleB) {
		return true;
	}
	return false;
}

// wrap around coordinate system
void GetCoordinates(int64_t *cols, int64_t *rows, const pair& coords) {
	// left|center|right
	cols[0] = (coords.first > INT64_MIN) ? coords.first - 1 : INT64_MAX;
	cols[1] = coords.first;
	cols[2] = (coords.first < INT64_MAX) ? coords.first + 1 : INT64_MIN;

	// up|center|down
	rows[0] = (coords.second < INT64_MAX) ? coords.second + 1 : INT64_MIN;
	rows[1] = coords.second;
	rows[2] = (coords.second > INT64_MIN) ? coords.second - 1 : INT64_MAX;
}

void ProbeNeighbors(umap& board, uset& changes, const pair& coords, const int stateChange) {
	int64_t cols[3], rows[3];

	GetCoordinates(cols, rows, coords);

	for (const int64_t x : cols) {
		for (const int64_t y : rows) {
			// skip central cell
			if (!(x == coords.first && y == coords.second)) {
				const pair& localCoords = pair(x, y);
				board[localCoords].neighbors += stateChange;
				changes.insert(localCoords);
			}
		}
	}
}

void CalculateNextGeneration(umap& board, uset& changes) {
	std::vector<pair> changeQ;

	// evaluate current board state
	for (const pair& coords : changes) {
		if (GameRules(board[coords])) {
			changeQ.push_back(coords);
		}
	}
	changes.clear();

	// apply and record changes
	for (const pair& coords : changeQ) {
		cellStruct& cell = board[coords];

		cell.state = !cell.state;
		changes.insert(coords);
		ProbeNeighbors(board, changes, coords, (cell.state) ? BORN : DIED);
	}
	changeQ.clear();
}

void ReadTestInput(umap& board, uset& changes) {
	pair coords;
	std::fstream testFile;

	testFile.open("testFile.txt", std::ios::in);
	while (testFile >> coords.first >> coords.second) {
		if (!board[coords].state) {
			board[coords].state = true;
			changes.insert(coords);
			ProbeNeighbors(board, changes, coords, BORN);
		}
	}
	testFile.close();
}

void WriteResults(const umap& board) {
	std::fstream results;

	results.open("results.txt", std::ios::out);
	results << "#Life 1.06\n";
	for (auto& cell : board) {
		if (cell.second.state) {
			results << cell.first.first << " " << cell.first.second << std::endl;
		}
	}
	results.close();
}

int main() {
	uset changes;
	umap board;

	ReadTestInput(board, changes);

	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();

	for (int i = 0; i < generations; ++i) {
		CalculateNextGeneration(board, changes);
	}

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> runtime = end - start;
	std::cout << "Runtime: " << runtime.count() << " seconds\n";

	WriteResults(board);

	return 0;
}