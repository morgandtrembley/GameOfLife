#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>

// Edit Number of Generations Here
int generations = 10;

#define BORN 1
#define DIED -1

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

void ProbeNeighbors(umap& board, uset& changes, const pair& coords, const int state) {
	int64_t left, up, right, down;

	// wrap around coords
	left = (coords.first > INT64_MIN) ? coords.first - 1 : INT64_MAX;
	up = (coords.second < INT64_MAX) ? coords.second + 1 : INT64_MIN;
	right = (coords.first < INT64_MAX) ? coords.first + 1 : INT64_MIN;
	down = (coords.second > INT64_MIN) ? coords.second - 1 : INT64_MAX;

	// upper row
	board[pair(left, up)].neighbors += state,
		changes.insert(pair(left, up));
	board[pair(coords.first, up)].neighbors += state,
		changes.insert(pair(coords.first, up));
	board[pair(right, up)].neighbors += state,
		changes.insert(pair(right, up));

	// middle row
	board[pair(left, coords.second)].neighbors += state,
		changes.insert(pair(left, coords.second));
	board[pair(right, coords.second)].neighbors += state,
		changes.insert(pair(right, coords.second));

	// lower row
	board[pair(left, down)].neighbors += state,
		changes.insert(pair(left, down));
	board[pair(coords.first, down)].neighbors += state,
		changes.insert(pair(coords.first, down));
	board[pair(right, down)].neighbors += state,
		changes.insert(pair(right, down));
}

void CalculateNextGeneration(umap& board, uset& changes) {
	std::vector<pair> changeQ;

	// evaluate current board state
	for (const pair& coords : changes) {
		const cellStruct& cell = board[coords];

		bool condition1 = !cell.state && cell.neighbors == 3;
		bool condition2 = cell.state && (cell.neighbors < 2 || cell.neighbors > 3);
		if (condition1 || condition2) {
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

void SaveResults(const umap& board) {
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

	SaveResults(board);

	return 0;
}