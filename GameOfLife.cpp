#pragma warning(disable : 4996)
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <boost/functional/hash/hash.hpp>	// pair hashing
#include <boost/gil.hpp>					// image support
#include <boost/gil/io/io.hpp>				// image support
#include <boost/gil/extension/io/bmp.hpp>	// bmp write support

#define BORN 1
#define DIED -1

struct cellStruct;
typedef std::pair<int64_t, int64_t> pair;
typedef std::unordered_set<pair, boost::hash<pair>> uset;
typedef std::unordered_map<pair, cellStruct, boost::hash<pair>> umap;
typedef boost::gil::rgb8_image_t image;
typedef boost::gil::rgb8_pixel_t color;

color darkGrey(10, 10, 10);
color medGrey(25, 25, 25);
color lightGrey(50, 50, 50);
color green(0, 100, 0);
color red(150, 0, 0);
color blue(0, 0, 100);

// Edit Number of Generations Here
const int generations = 10;

// Edit Image Parameters Here
const int cellSize = 6;	// pixels (Should be even)
// image size in # of cells. Should be odd
const int imgSizeX = 105;
const int imgSizeY = 55;
// image center coords
// Image CAN NOT EXCEED int64 Bounds! imgCenter will be changed iff out of range
pair imgCenter = pair(10,-2);

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

void DrawCell(image& img, pair coords) {
	for (int i = 0; i < cellSize - 1; i++) {
		for (int j = 0; j < cellSize - 1; j++) {
			view(img)[(img.height() / 2)*img.width() + img.width()/2 + imgCenter.second*cellSize*img.width() - imgCenter.first*cellSize - cellSize/2*img.width() - cellSize / 2 + img.width() + 1 + i*img.width() + j - coords.second*cellSize*img.width() + coords.first * cellSize] = red;
		}
	}
}

bool InRange(const pair& coords, const pair& minRange, const pair& maxRange) {
	if (coords.first >= minRange.first && coords.first <= maxRange.first &&
			coords.second >= minRange.second && coords.second <= maxRange.second) {
		return true;
	}
	return false;
}

void DrawImage(const umap& board, const image img, const pair& minRange, const pair& maxRange, const std::string tag) {
	image newImg = img;

	for (auto& cell:board) {
		if (cell.second.state) {
			if (InRange(cell.first, minRange, maxRange)) {
				DrawCell(newImg, cell.first);
			}
		}
	}
	boost::gil::write_view("images/img" + tag + ".bmp", view(newImg), boost::gil::bmp_tag());
}

void GenerateBaseImage(image& img, const pair& minRange, const pair& maxRange) {
	fill_pixels(view(img), darkGrey);
	// line x = 0
	bool drawXZero = (minRange.first < 0 && maxRange.first > 0) ? true : false;
	// line y = 0
	bool drawYZero = (minRange.second < 0 && maxRange.second > 0) ? true : false;
	
	bool lowerBound = (minRange.second == INT64_MIN) ? true : false;
	bool upperBound = (maxRange.second == INT64_MAX) ? true : false;
	bool leftBound = (minRange.first == INT64_MIN) ? true : false;
	bool rightBound = (maxRange.first == INT64_MAX) ? true : false;

	for (int x = 0; x < img.height(); x++) {
		for (int y = 0; y < img.width(); y++) {
			// gridlines
			if (x % cellSize == 0 && y % cellSize == 0) {
				view(img)[x * img.width() + y] = lightGrey;
			}
			else if (x % cellSize == 0 || y % cellSize == 0) {
				view(img)[x * img.width() + y] = medGrey;
			}
			
			// center lines
			if (drawXZero) {
				if (x == img.height() / 2 + imgCenter.second*cellSize) {
					view(img)[x * img.width() + y] = green;
				}
			}
			if (drawYZero) {
				if (y == img.width() / 2 - imgCenter.first * cellSize) {
					view(img)[x * img.width() + y] = green;
				}
			}

			// bounds
			if (lowerBound) {
				if (x == img.height() - 1) {
					view(img)[x * img.width() + y] = blue;
				}
			}
			else if (upperBound) {
				if (x == 0) {
					view(img)[x * img.width() + y] = blue;
				}
			}
			if (leftBound) {
				if (y == 0) {
					view(img)[x * img.width() + y] = blue;
				}
			}
			else if (rightBound) {
				if (y == img.width() - 1) {
					view(img)[x * img.width() + y] = blue;
				}
			}
		}
	}
	
}

void GenerateCoordinateRange(pair& minRange, pair& maxRange) {
	minRange.first = imgCenter.first - imgSizeX/2;
	minRange.second = imgCenter.second - imgSizeY/2;
	maxRange.first = imgCenter.first + imgSizeX/2;
	maxRange.second = imgCenter.second + imgSizeY/2;
}

void ValidateImageCenter() {
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

int main() {
	uset changes;
	umap board;
	pair minRange, MaxRange;


	image img(imgSizeX * cellSize + 1, imgSizeY * cellSize + 1);
	ValidateImageCenter();
	GenerateCoordinateRange(minRange,MaxRange);
	GenerateBaseImage(img, minRange, MaxRange);

	ReadTestInput(board, changes);

	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();

	for (int i = 0; i < generations; ++i) {
		DrawImage(board, img, minRange, MaxRange, std::to_string(i));
		CalculateNextGeneration(board, changes);
	}
	DrawImage(board, img, minRange, MaxRange, "Final");


	end = std::chrono::system_clock::now();
	std::chrono::duration<double> runtime = end - start;
	std::cout << "Runtime: " << runtime.count() << " seconds\n";

	WriteResults(board);

	return 0;
}