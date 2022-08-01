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
color green(0, 10, 0);
color red(150, 0, 0);
color blue(0, 0, 100);

// Edit Number of Generations Here
int generations = 10;

// Edit Image Parameters Here
int cellSize = 5;	// pixels
// image size in # of cells must be odd
int imgSizeX = 55;
int imgSizeY = 55;
// image center coords
pair imgCenter = pair(0, 0);





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
			view(img)[(img.height() / 2 + i - cellSize / 2 - coords.second * cellSize) * img.width() + img.height() / 2 + j - cellSize / 2 + coords.first * cellSize] = red;
		}
	}
}

void DrawImage(const umap& board, const image img, const std::string tag) {
	image newImg = img;

	for (auto& cell:board) {
		// if cell is in image range (center +- imgsize/2)
		if (cell.second.state) {
			DrawCell(newImg, cell.first);
		}
	}
	boost::gil::write_view("images/img" + tag + ".bmp", view(newImg), boost::gil::bmp_tag());
}

void GenerateBaseImage(image& img) {
	fill_pixels(view(img), darkGrey);
	for (int x = 0; x < img.height(); x++) {
		for (int y = 0; y < img.width(); y++) {
			// gridlines
			if (x % cellSize == 0 && y % cellSize == 0) {
				view(img)[x * img.width() + y] = lightGrey;
			}
			// centerline
			// if x - center.x is within borders draw x 0
			// if y - center.y is within borders draw y 0
			if (x == (int)img.height() / 2 || y == (int)img.width() / 2) {
				view(img)[x * img.width() + y] = green;
			}
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

	image img(imgSizeX * cellSize + 1, imgSizeY * cellSize + 1);
	GenerateBaseImage(img);

	ReadTestInput(board, changes);

	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();

	for (int i = 0; i < generations; ++i) {
		DrawImage(board, img, std::to_string(i));
		CalculateNextGeneration(board, changes);
	}
	DrawImage(board, img, "Final");

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> runtime = end - start;
	std::cout << "Runtime: " << runtime.count() << " seconds\n";

	WriteResults(board);

	return 0;
}