Conway's Game of life with a 64-bit integer playing field.
This program requires Boost libraries for hashing and image production! (https://www.boost.org/)

This version of Game of Life uses a wrap-around coordinate system (similar to pac-man) 
so life can teleport from one side of the field to the other in a single generation.

Image snapshots are created for the first and final generations automatically but 
you can enable image creation for every generation in the "GameOfLife.cpp" file. (This is defaulted to off)

You can customize the image parameters as you like but following the comments in the file will produce the best quality images.

There are other editable parameters in the "GameOfLife.cpp" file.

Inputs should take the form "(x, y)" and anything else will cause the simulation to start running.
Results are printed directly to console and stored in "results.txt." Both follow Life 1.06 formatting instructions. (https://conwaylife.com/wiki/Life_1.06)
