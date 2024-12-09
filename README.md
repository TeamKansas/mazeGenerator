Random Maze generator: This program generates random maze puzzles and draws them into a .tiff file.
The user can specify the height, width, and resolution of the maze.
The user can also choose to include a step count heatmap or a solution in the image.
The program prints the randomizer seed used after completion.

options:
-h <value>      Height (in wall segments) of maze. Default of 300
-r <value>      Specify resolution (effective pixel width of a floor section), default of 5
-S <value>      Specify andomizer seed default value of clock()
-w <value>      Width (in wall segments) of maze. Default of 300

-heatmap        Color floor tiles based on number of steps to get to that tile from the top left corner. Green is lowest, Blue is highest.
-help           Print this very helpful help section
-solution       Trace the solution to the maze in red
