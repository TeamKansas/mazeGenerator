#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "tiff.h"

/* index macro */
#define I(W,M,X,Y) M[(Y)*(W) + (X)]

#define HEATMAP 1
#define SOLUTION 2

int check(int wid, int hei, int *map, int x, int y, char dir[2]); /* count wall segments forward and sideways from (x,y)*/
int checkUntilValid(int wid, int hei, int *map, int const pos[2], char dir[2],int direction); /* Check the four possible directions to move, stop at the first available one */
void drawLines(image *img, int wid, int hei, int *map, int res); /* draw wall lines */
void generateMaze(int wid, int hei, int *map); /* generate maze */
void printHelp(); /* print program description and a list of command line options*/
void rotate(char dir[2], char direction); /* rotate dir by 90 degrees in the specified direction */
void solveMaze(image *img, int wid, int hei, int *map, uint8_t solve, int res); /* solve maze: trace solution and draw colormap if specified */

int main(int argc, char **argv) {
    char *name = "maze.tiff";
    clock_t seed = 0;
    int wid, hei, res;
    uint8_t solve = 0;
    wid = 302;
    hei = 302;
    res = 5;
    for(int i = 1; i < argc; ++i) {
        if(!strcmp("-heatmap",argv[i]))
            solve |= HEATMAP;
        else if(!strcmp("-solution",argv[i]))
            solve |= SOLUTION;
        else if(!strcmp("-help",argv[i]))
            printHelp();
        else if(argv[i][0] == '-' && i < argc - 1 && strlen(argv[i]) == 2) {
            switch(argv[i][1]) {
                case 'S':
                    seed = atoi(argv[++i]);
                    break;
                case 'w':
                    wid = atoi(argv[++i]) + 2;
                    break;
                case 'h':
                    hei = atoi(argv[++i]) + 2;
                    break;
                case 'r':
                    res = atoi(argv[++i]);
                    break;
                case 'n':
                    name = argv[++i];

            }
        }
        else {
            printf("\033[1;31merror: \033[0mInvalid argument usage: '%s'\n\n",argv[i]);
            printHelp();
            return 1;
        }

    }
    if(wid < 5 || hei < 5) {
        printf("\033[1;31merror: \033[0m Maze dimensions smaller than 3 are invalid.\n");
        return 1;
    }
    image *imp = iopen((wid - 2) * res,(hei - 2) * res, FULLCOLOR);
    setColor(imp,255,255,255);
    fillColor(imp);
    setColor(imp, 0,0,0);
    int *map = malloc(sizeof(int) * wid * hei);
    seed = seed ? seed : clock() ;

    srand(seed);
    generateMaze(wid, hei, map);
    drawLines(imp, wid, hei, map,res);
    if(solve)
        solveMaze(imp,wid,hei,map,solve,res);
    free(map);
    setColor(imp,0,0,255);
    writeFile(imp, name);
    iclose(imp);
    printf("%lu\n",seed);
    return 0;
}

int check(int wid, int hei, int *map, int x, int y, char dir[2]) {
    if(x >= wid - 1 || x <= 0 || y >= hei - 1 || hei <= 0)
        return 1;
    return I(wid,map,x + dir[0],y + dir[1]) + I(wid,map,x - dir[1],y + dir[0]) + I(wid,map,x + dir[1],y - dir[0]) + I(wid,map,x + dir[0] + dir[1],y + dir[1] + dir[0]) + I(wid,map,x + dir[0] - dir[1],y + dir[1] - dir[0]);;
}

int checkUntilValid(int wid, int hei, int *map, int const pos[2], char dir[2],int direction) {
    int a;
    for(a = 0; a < 4 && check(wid,hei,map,dir[0] + pos[0],dir[1] + pos[1],dir); ++a) {
        rotate(dir,direction);
    }
    if(a == 4 || I(wid,map,pos[0] + dir[0],pos[1] + dir[1]))
        return 0;
    return a + 1;
}

/* draw the walls */
void drawLines(image *img, int wid, int hei, int *map, int res) {
    int xx, yy;
    int middle = res / 2;
    for(int x = wid - 2; x > 0; --x) {
        for(int y = hei - 2; y > 0; --y) {
            if(I(wid,map,x,y)) {
                xx = (x - 1) * res + middle;
                yy = (y - 1) * res + middle;
                if(I(wid,map,x+1,y))
                    orthoLine(img, xx, yy, 0, res);
                if(I(wid,map,x,y+1))
                    orthoLine(img, xx, yy, 1, res);
                if(I(wid,map,x-1,y))
                    orthoLine(img, xx, yy, 2, res);
                if(I(wid,map,x,y-1))
                    orthoLine(img, xx, yy, 3, res);
            }
        }
    }
}


void generateMaze(int wid, int hei, int *map) {
    for(int x = wid - 1; x >= 0; --x) {
        for(int y = hei - 1; y >= 0; --y) {
            if((x < 2 || y < 2 || x > wid - 3 || y > hei - 3))
                I(wid,map,x,y) = 1;
            else
                I(wid,map,x,y) = 0;

        }
    }
    char dir[2] = {1,0};
    int pos[2];
    int iterations = wid*hei / 25;

    /* make paths */
    int x,y,tmp;
    int avg;
    int a;
    for(int i = 0; i < iterations; ++i) {
        dir[0] = rand()%3 - 1;
        dir[1] = dir[0] ? 0 : (rand()%2) * 2 - 1;
        pos[0] = rand()%(wid - 2) + 1;
        pos[1] = rand()%(hei - 2) + 1;
        for(x = 2; x < wid - 2 && checkUntilValid(wid,hei,map,pos,dir,1) == 0; ++x) { /* find a pixel not already occupied by a wall */
            for(y = 2; y < hei - 2 && checkUntilValid(wid,hei,map,pos,dir,1) == 0; ++y) {
                pos[1] = (pos[1])%(hei - 2) + 1;
            }
            pos[0] = (pos[0])%(wid - 2) + 1;
        }
        if(checkUntilValid(wid,hei,map,pos,dir,1) == 0) /* this indicates that the entire map is filled up */
            return;
        while(!I(wid,map,pos[0],pos[1])) { /* find the nearest wall */
            pos[0] += dir[0];
            pos[1] += dir[1];
        }
        /* turn around */
        dir[0] *= -1;
        dir[1] *= -1;
        avg = 4;// (wid + hei) / 8;
        while(2) {
            if(!(tmp = checkUntilValid(wid,hei,map,pos,dir,2*(rand()%2) - 1)))
                break;
            avg -= tmp * 1.5;
            if(avg < 7)
                avg = 7;
            assert(pos[0]+dir[0] < wid);
            assert(pos[1]+dir[1] < hei);
            pos[0] += dir[0];
            pos[1] += dir[1];
            I(wid,map,pos[0],pos[1]) = 1;
            if(rand()%avg == 0)
                rotate(dir,2*(rand()%2) - 1);
        }
        avg *= 0.9;
        // printf("\033cGenerating: %.1f\n",(double) i / (double) iterations * 100);
    }
}

void printHelp() {
    printf("Random Maze generator: This program generates random maze puzzles and draws them into a .tiff file.\nThe user can specify the height, width, and resolution of the maze.\nThe user can also choose to include a step count heatmap or a solution in the image.\nThe program prints the randomizer seed used after completion.\n\n");
    printf("options:\n-h <value> \tHeight (in wall segments) of maze. Default of 300\n-r <value>\tSpecify resolution (effective pixel width of a floor section), default of 5\n-S <value>\tSpecify andomizer seed default value of clock()\n-w <value>\tWidth (in wall segments) of maze. Default of 300\n");
    printf("\n-heatmap\tColor floor tiles based on number of steps to get to that tile from the top left corner. Green is lowest, Blue is highest.\n-help\t\tPrint this very helpful help section\n-solution\tTrace the solution to the maze in red\n\n");
}

void rotate(char dir[2], char direction) { /* direction: 1 = clockwise, -1 = counterclockwise*/
    char tmp = dir[0];
    dir[0] = dir[1] * direction;
    dir[1] = -tmp * direction;
}

void solveMaze(image *img, int wid, int hei, int *map, uint8_t solve, int res) {
    I(wid,map,2,2) = -1;
    int x;
    int y;
    int current = -1;
    int change = 1;
    int middle = res / 2;
    while(change /*I(wid,map,wid-3,hei-3) == 0*/) {
        change = 0;
        for(x = wid - 2; x > 0; --x) {
            for(y = hei - 2; y > 0; --y) {
                if(I(wid,map,x,y) == current) {
                    if(I(wid,map,x+1,y) == 0) {
                        I(wid,map,x+1,y) = current - 1;
                        change = 1;
                    }
                    if(I(wid,map,x,y+1) == 0) {
                        I(wid,map,x,y+1) = current - 1;
                        change = 1;
                    }
                    if(I(wid,map,x-1,y) == 0) {
                        I(wid,map,x-1,y) = current - 1;
                        change = 1;
                    }
                    if(I(wid,map,x,y-1) == 0) {
                        I(wid,map,x,y-1) = current - 1;
                        change = 1;
                    }
                }
            }
        }
        current -= 1;
    }
    int gradient;
    for(x = 0; x < wid && (solve & HEATMAP); ++x) {
        for(y = 0; y < hei; ++y) {
            if(I(wid,map,x,y) < 0) {
                gradient = I(wid,map,x,y) * 255 / current;
                setColor(img, 0, -gradient + 255, gradient);
                fillRect(img, (x - 1) * res, (y - 1) * res, (x - 1) * res + 2*middle, (y - 1) * res + 2*middle);
            }

        }
    }
    
    setColor(img,255,0,0);
    x = wid - 3;
    y = hei - 3;
    int tmp,i;
    char dir[2] = {1,0};
    while((x != 2 || y != 2) && (solve & SOLUTION)) {
        dir[0] = 1;
        dir[1] = 0;
        for(i = 0; i < 4 && (I(wid,map,x+dir[0],y+dir[1]) > -1 || I(wid,map,x+dir[0],y+dir[1]) <= I(wid,map,x,y)); ++i) {
            rotate(dir,-1);
        }
        // orthoLine(img, (x - 1) * res + middle, (y - 1) * res + middle, i, res);
        fillRect(img, (x - 1) * res, (y - 1) * res, (x - 1) * res + 2*middle, (y - 1) * res + 2*middle);
        x += dir[0];
        y += dir[1];
    }
    fillRect(img, res, res, res + 2*middle, res + 2*middle);
}