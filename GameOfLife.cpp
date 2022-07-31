#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>

// Edit Number of Generations Here
int generations = 10;

struct cellStruct;
struct HashCoords;
typedef std::pair<int64_t, int64_t> pair;
typedef std::unordered_set<pair, HashCoords> uset;
typedef std::unordered_map<pair, cellStruct, HashCoords> umap;

struct cellStruct {
	bool state = false;
	int neighbors = 0;
};

// REPLACE !!!!!!!!!!!!!!
struct HashCoords {
	template <typename T, typename U> std::size_t operator()(const std::pair<T, U>& pair) const {
		return std::hash<T>()(pair.first) ^ std::hash<U>()(pair.second);
	}
};

void ProbeNeighbors(umap& board, uset& changes, pair coords, int state) {
	int64_t up, down, left, right;

	// wrap around coords
	if (coords.second < INT64_MAX) {
		up = coords.second + 1;
	}
	else {
		up = INT64_MIN;
	}

	if (coords.second > INT64_MIN) {
		down = coords.second - 1;
	}
	else {
		down = INT64_MAX;
	}

	if (coords.first > INT64_MIN) {
		left = coords.first - 1;
	}
	else {
		left = INT64_MAX;
	}

	if (coords.first < INT64_MAX) {
		right = coords.first + 1;
	}
	else {
		right = INT64_MIN;
	}

	// upper row
	board[pair(left, up)].neighbors += state;
	changes.insert(pair(left, up));
	board[pair(coords.first, up)].neighbors += state;
	changes.insert(pair(coords.first, up));
	board[pair(right, up)].neighbors += state;
	changes.insert(pair(right, up));

	// middle row
	board[pair(left, coords.second)].neighbors += state;
	changes.insert(pair(left, coords.second));
	board[pair(right, coords.second)].neighbors += state;
	changes.insert(pair(right, coords.second));

	// lower row
	board[pair(left, down)].neighbors += state;
	changes.insert(pair(left, down));
	board[pair(coords.first, down)].neighbors += state;
	changes.insert(pair(coords.first, down));
	board[pair(right, down)].neighbors += state;
	changes.insert(pair(right, down));
}

void CalculateNextGeneration(umap& board, uset& changes) {
	std::vector<pair> changeQ;

	// evaluate current board state
	for (auto& coords : changes) {
		cellStruct cell = board[coords];

		if ((!cell.state && cell.neighbors == 3) || 
			(cell.state && (cell.neighbors < 2 || cell.neighbors > 3))) {
			changeQ.push_back(coords);
		}
	}
	changes.clear();

	// apply and record changes
	for (auto& coords : changeQ) {
		cellStruct& cell = board[coords];

		cell.state = !cell.state;
		changes.insert(coords);
		ProbeNeighbors(board, changes, coords, (cell.state) ? 1 : -1);
	}
	changeQ.clear();
}

void ReadTestInput(umap& board, uset& changes) {
	pair coords;
	std::fstream testFile;

	testFile.open("testFile.txt", std::ios::in);
	while (testFile >> coords.first >> coords.second) {
		board[coords].state = true;
		changes.insert(coords);
		ProbeNeighbors(board, changes, coords, 1);
	}
	testFile.close();
}

void SaveResults(umap& board) {
	std::fstream results;

	results.open("results.txt", std::ios::out);
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

	for (int i = 0; i < generations; i++) {
		CalculateNextGeneration(board, changes);
	}

	SaveResults(board);

	return 0;
}