/*	Conways Game of Life with support for 64-bit cell coordinates.	*/
#pragma warning(disable : 4996)				// ctime warnings via boost
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <boost/gil.hpp>					// image editing
#include <boost/gil/io/io.hpp>				// ^^
#include <boost/gil/extension/io/bmp.hpp>	// .bmp write
#include <boost/functional/hash/hash.hpp>	// int pair hashing

#define BORN 1
#define DIED -1

struct cellStruct;
typedef std::pair<int64_t, int64_t> pair;
typedef std::unordered_set<pair, boost::hash<pair>> uset;
typedef std::unordered_map<pair, cellStruct, boost::hash<pair>> umap;
typedef boost::gil::rgb8_image_t image;
typedef boost::gil::rgb8_pixel_t color;

struct cellStruct {
	bool state = false;
	int neighbors = 0;
};

const int generations = 10;

// ---------- Edit Image Parameters Here ----------
const bool genImages = false;	// set to true to generate image representations of each generation
const int cellWidth = 6;	// in pixels (even # for best results)
const int imgSizeX = 55;	// in cells (odd for best results)
const int imgSizeY = 55;	// in cells (odd for best results)

// image center cell coordinates
pair imgCenter = pair(-4,7);

// image Colors
color darkGrey(10, 10, 10);
color medGrey(25, 25, 25);
color lightGrey(50, 50, 50);
color green(0, 255, 0);
color red(150, 0, 0);
color blue(0, 0, 100);

// ---------- Game Simulation ----------

bool GameRules(const cellStruct& cell) {
	const bool ruleA = !cell.state && cell.neighbors == 3;
	const bool ruleB = cell.state && (cell.neighbors < 2 || cell.neighbors > 3);

	if (ruleA || ruleB) {
		return true;
	}
	return false;
}

void GetNeighborCoordinates(int64_t *cols, int64_t *rows, const pair& coords) {
	// center|left|right
	cols[0] = coords.first;
	cols[1] = (coords.first > INT64_MIN) ? coords.first - 1 : INT64_MAX;
	cols[2] = (coords.first < INT64_MAX) ? coords.first + 1 : INT64_MIN;

	// center|up|down
	rows[0] = coords.second;
	rows[1] = (coords.second < INT64_MAX) ? coords.second + 1 : INT64_MIN;
	rows[2] = (coords.second > INT64_MIN) ? coords.second - 1 : INT64_MAX;
}

void ProcessCell(umap& board, uset& changes, const pair& coords) {
	int neighborModifier;
	int64_t cols[3], rows[3];

	GetNeighborCoordinates(cols, rows, coords);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			const pair& localCoords = pair(cols[i], rows[j]);
			changes.insert(localCoords);

			cellStruct& cell = board[localCoords];
			if (!(i + j)) {
				cell.state = !cell.state;
				neighborModifier = (cell.state) ? BORN : DIED;
			}
			else {
				cell.neighbors += neighborModifier;
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
		ProcessCell(board, changes, coords);
	}
}

// ---------- Image Processing ----------

void DrawCell(image& snapshot, pair coords) {
	int imageCenter = (snapshot.height() / 2) * snapshot.width() + snapshot.width() / 2;
	int centerShift = (imgCenter.second * snapshot.width() - imgCenter.first) * cellWidth;
	int coordinateShift = (coords.first - coords.second * snapshot.width()) * cellWidth;
	for (int i = 0; i < cellWidth - 1; i++) {
		for (int j = 0; j < cellWidth - 1; j++) {		
			int cellShift = (1 + i - cellWidth / 2) * snapshot.width() + j - cellWidth / 2 + 1;
			view(snapshot)[imageCenter + centerShift + coordinateShift + cellShift] = red;
		}
	}
}

bool InRange(const pair& coords, const pair& minRange, const pair& maxRange) {
	bool condition1 = coords.first >= minRange.first && coords.first <= maxRange.first;
	bool condition2 = coords.second >= minRange.second && coords.second <= maxRange.second;
	return (condition1 && condition2) ? true : false;
}

void DrawImage(const umap& board, const image base, const pair& minRange, const pair& maxRange, const std::string tag) {
	image snapshot = base;
	for (auto& cell:board) {
		if (cell.second.state) {
			if (InRange(cell.first, minRange, maxRange)) {
				DrawCell(snapshot, cell.first);
			}
		}
	}
	boost::gil::write_view("images/generation" + tag + ".bmp", view(snapshot), boost::gil::bmp_tag());
}

void DrawBounds(image& base, const pair& minRange, const pair& maxRange, const int range) {
	// lower boundary
	if (minRange.second == INT64_MIN) {
		for (int y = base.height() * (base.width() - 1); y < base.height() * base.width(); y++) {
			view(base)[y] = blue;
		}
	}
	// upper boundary
	if (maxRange.second == INT64_MAX) {
		for (int y = 0; y < base.width(); y++) {
			view(base)[y] = blue;
		}
	}
	// left boundary
	if (minRange.first == INT64_MIN) {
		for (int x = 0; x < range; x += base.width()) {
			view(base)[x] = blue;
		}
	}
	// right boundary
	if (maxRange.first == INT64_MAX) {
		for (int x = base.width() - 1; x < range; x += base.width()) {
			view(base)[x] = blue;
		}
	}
}

void DrawZeroes(image& base, const pair& minRange, const pair& maxRange, const int range) {
	// line x = 0
	if (minRange.first < 0 && maxRange.first > 0) {
		int offset = base.width() / 2 - imgCenter.first * cellWidth;
		for (int x = offset; x < range; x += base.width()) {
			view(base)[x] = green;
		}
	}
	// line y = 0
	if (minRange.second < 0 && maxRange.second > 0) {
		int offset = (base.height() / 2 + imgCenter.second * cellWidth) * base.width();
		for (int y = 0; y < base.width(); y++) {
			view(base)[offset + y] = green;
		}
	}
}

void DrawGridlines(image& base, const int range) {
	// Vertical
	for (int x = 0; x < range; x += base.width()) {
		for (int y = 0; y < base.width(); y += cellWidth) {
			view(base)[x + y] = medGrey;
		}
	}
	// Horozontal
	int stride = cellWidth * base.width();
	for (int x = 0; x < range; x += stride) {
		for (int y = 0; y < base.width(); y++) {
			// if overlapping make cell brighter
			if (view(base)[x + y] == medGrey) {
				view(base)[x + y] = lightGrey;
			}
			else {
				view(base)[x + y] = medGrey;
			}
		}
	}
}

void GenerateBaseImage(image& base, const pair& minRange, const pair& maxRange) {
	int range = base.height() * base.width();

	fill_pixels(view(base), darkGrey);
	DrawGridlines(base, range);
	DrawZeroes(base, minRange, maxRange, range);
	DrawBounds(base, minRange, maxRange, range);
}

void GenerateCoordinateRange(pair& minRange, pair& maxRange) {
	minRange.first = imgCenter.first - imgSizeX/2;
	minRange.second = imgCenter.second - imgSizeY/2;
	maxRange.first = imgCenter.first + imgSizeX/2;
	maxRange.second = imgCenter.second + imgSizeY/2;
}

void ClampImageCenter() {
	if (imgCenter.first > 0) {
		if (INT64_MAX - imgSizeX/2 <= imgCenter.first) {
			imgCenter.first = INT64_MAX - imgSizeX / 2;
		}
	}
	else {
		if (INT64_MIN + imgSizeX / 2 >= imgCenter.first) {
			imgCenter.first = INT64_MIN + imgSizeX / 2;
		}
	}
	if (imgCenter.second > 0) {
		if (INT64_MAX - imgSizeY / 2 <= imgCenter.second) {
			imgCenter.second = INT64_MAX - imgSizeY / 2;
		}
	}
	else {
		if (INT64_MIN + imgSizeY / 2 >= imgCenter.second) {
			imgCenter.second = INT64_MIN + imgSizeY / 2;
		}
	}
}

// ---------- Input/Output ----------

void ReadTestInput(umap& board, uset& changes) {
	pair coords;
	std::fstream testFile;

	testFile.open("testFile.txt", std::ios::in);
	while (testFile >> coords.first >> coords.second) {
		if (!board[coords].state) {
			ProcessCell(board, changes, coords);
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

// ---------- Main ----------

int main() {
	uset changes;
	umap board;
	pair minRange, MaxRange;
	image base(imgSizeX * cellWidth + 1, imgSizeY * cellWidth + 1);

	ReadTestInput(board, changes);

	if (genImages) {
		ClampImageCenter();
		GenerateCoordinateRange(minRange, MaxRange);
		GenerateBaseImage(base, minRange, MaxRange);
	}

	for (int i = 0; i < generations; ++i) {
		if (genImages) { DrawImage(board, base, minRange, MaxRange, std::to_string(i)); }
		CalculateNextGeneration(board, changes);
	}
	if (genImages) { DrawImage(board, base, minRange, MaxRange, "Final"); }

	WriteResults(board);

	return 0;
}