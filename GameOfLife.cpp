/*	Conways Game of Life with support for 64-bit cell coordinates.	*/
#pragma warning(disable : 4996)				// ctime warnings via boost
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <boost/gil.hpp>					// image editing
#include <boost/gil/io/io.hpp>				// ^^
#include <boost/gil/extension/io/bmp.hpp>	// .bmp write
#include <boost/functional/hash/hash.hpp>	// int pair hashing

struct cellStruct;
typedef std::pair<int64_t, int64_t> pair;
typedef std::unordered_set<pair, boost::hash<pair>> uset;
typedef std::unordered_map<pair, cellStruct, boost::hash<pair>> umap;
typedef boost::gil::rgb8_image_t image;
typedef boost::gil::rgb8_pixel_t color;

/* ---------- Editable Parameters ---------- */

const int generations = 10;

/* Image Parameters */
const bool genImages = true;		// set to true to generate image snapshots of each generation
const int cellWidth = 6;			// in pixels (even # for best results)
const int imgSizeX = 55;			// in cells (odd for best results)
const int imgSizeY = 55;			// ^^
static pair imgCenter = pair(0,0);	// coordinates for center of image

/* Colors */
const color darkGrey(10, 10, 10);
const color medGrey(25, 25, 25);
const color lightGrey(50, 50, 50);
const color green(0, 150, 0);
const color red(150, 0, 0);
const color blue(0, 0, 150);

/* ---------- End of Editable Parameters ---------- */

const int BORN = 1;
const int DIED = -1;
static pair minRange, maxRange;

/* ---------- Game of Life Simulation ---------- */

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

void GetNeighborCoordinates(int64_t* cols, int64_t* rows, const pair& coords) {
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
			cellStruct& cell = board[localCoords];
			
			if (!(i + j)) {
				cell.state = !cell.state;
				neighborModifier = (cell.state) ? BORN : DIED;
			}
			else {
				cell.neighbors += neighborModifier;
			}
			changes.insert(localCoords);
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

/* ---------- Image Generation ---------- */

void DrawCell(image& snapshot, const pair& coords) {
	// find x pixel coordinate, then add y pixel coordinate then add offset of (1,1) pixels
	int64_t imgCoords = (maxRange.second - coords.second) * cellWidth * snapshot.width();	
	imgCoords += (coords.first - minRange.first) * cellWidth;								
	imgCoords += snapshot.width() + 1;														

	for (int64_t i = 0; i < cellWidth - 1; i++) {
		for (int64_t j = 0; j < cellWidth - 1; j++) {
			view(snapshot)[imgCoords + i*snapshot.width() + j] = red;
		}
	}
}

bool CellInImage(const pair& coords) {
	bool condition1 = coords.first >= minRange.first && coords.first <= maxRange.first;
	bool condition2 = coords.second >= minRange.second && coords.second <= maxRange.second;
	return (condition1 && condition2) ? true : false;
}

void DrawImage(const umap& board, const image base, const std::string tag) {
	std::string fileName = "images/generation" + tag + ".bmp";
	image snapshot = base;

	for (auto& cell : board) {
		if (cell.second.state) {
			if (CellInImage(cell.first)) {
				DrawCell(snapshot, cell.first);
			}
		}
	}
	boost::gil::write_view(fileName, view(snapshot), boost::gil::bmp_tag());
}

void DrawBounds(image& base, const int64_t range) {
	// lower boundary
	if (minRange.second == INT64_MIN) {
		for (int64_t y = base.height() * (base.width() - 1); y < base.height() * base.width(); y++) {
			view(base)[y] = blue;
		}
	}
	// upper boundary
	if (maxRange.second == INT64_MAX) {
		for (int64_t y = 0; y < base.width(); y++) {
			view(base)[y] = blue;
		}
	}
	// left boundary
	if (minRange.first == INT64_MIN) {
		for (int64_t x = 0; x < range; x += base.width()) {
			view(base)[x] = blue;
		}
	}
	// right boundary
	if (maxRange.first == INT64_MAX) {
		for (int64_t x = base.width() - 1; x < range; x += base.width()) {
			view(base)[x] = blue;
		}
	}
}

void DrawZeroes(image& base, const int64_t range) {
	// line x = 0
	if (minRange.first < 0 && maxRange.first > 0) {
		int64_t offset = base.width() / 2 - imgCenter.first * cellWidth;
		for (int64_t x = offset; x < range; x += base.width()) {
			view(base)[x] = green;
		}
	}
	// line y = 0
	if (minRange.second < 0 && maxRange.second > 0) {
		int64_t offset = (base.height() / 2 + imgCenter.second * cellWidth) * base.width();
		for (int64_t y = 0; y < base.width(); y++) {
			view(base)[offset + y] = green;
		}
	}
}

void DrawGridlines(image& base, const int64_t range) {
	// Vertical
	for (int64_t x = 0; x < range; x += base.width()) {
		for (int64_t y = 0; y < base.width(); y += cellWidth) {
			view(base)[x + y] = medGrey;
		}
	}
	// Horozontal
	int64_t stride = cellWidth * base.width();
	for (int64_t x = 0; x < range; x += stride) {
		for (int64_t y = 0; y < base.width(); y++) {
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

void GenerateCoordinateRange() {
	minRange.first = imgCenter.first - imgSizeX / 2;
	minRange.second = imgCenter.second - imgSizeY / 2;
	maxRange.first = imgCenter.first + imgSizeX / 2;
	maxRange.second = imgCenter.second + imgSizeY / 2;
}

void ClampImageCenter() {
	if (INT64_MAX - imgSizeX / 2 <= imgCenter.first) {
		imgCenter.first = INT64_MAX - imgSizeX / 2;
	}
	else if (INT64_MIN + imgSizeX / 2 >= imgCenter.first) {
		imgCenter.first = INT64_MIN + imgSizeX / 2;
	}

	if (INT64_MAX - imgSizeY / 2 <= imgCenter.second) {
		imgCenter.second = INT64_MAX - imgSizeY / 2;
	}
	else if (INT64_MIN + imgSizeY / 2 >= imgCenter.second) {
		imgCenter.second = INT64_MIN + imgSizeY / 2;
	}
}

void GenerateBaseImage(image& base) {
	int64_t range = base.height() * base.width();

	ClampImageCenter();
	GenerateCoordinateRange();

	fill_pixels(view(base), darkGrey);
	DrawGridlines(base, range);
	DrawZeroes(base, range);
	DrawBounds(base, range);
}

/* ---------- Input / Output ---------- */

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

void ReadUserInput(umap& board, uset& changes) {
	pair coords;
	std::string line;

	while (true) {
		std::getline(std::cin, line);
		if (sscanf(line.c_str(), "(%lli, %lli))", &coords.first, &coords.second)) {
			if (!board[coords].state) {
				ProcessCell(board, changes, coords);
			}
		}
		else {
			break;
		}
	}
}

void WriteResults(const umap& board) {
	std::fstream results;

	results.open("results.txt", std::ios::out);
	results << "#Life 1.06\n";
	std::cout << "#Life 1.06\n";
	for (auto& cell : board) {
		if (cell.second.state) {
			results << cell.first.first << " " << cell.first.second << std::endl;
			std::cout << cell.first.first << " " << cell.first.second << std::endl;
		}
	}
	results.close();
}

/* ---------- Main ---------- */

int main() {
	uset changes;
	umap board;
	image base(imgSizeX * cellWidth + 1, imgSizeY * cellWidth + 1);

	// ReadTestInput(board, changes);
	ReadUserInput(board, changes);

	if (genImages) { GenerateBaseImage(base); }

	for (int i = 0; i < generations; ++i) {
		if (genImages) { DrawImage(board, base, std::to_string(i)); }
		CalculateNextGeneration(board, changes);
	}
	if (genImages) { DrawImage(board, base, "Final"); }

	WriteResults(board);

	return 0;
}