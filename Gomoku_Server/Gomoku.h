#pragma once

#include <string>
#include <vector>

typedef std::pair<std::string, int> StrAndPos;

class Gomoku
{
private:
	int _boardSize = 0;
	std::vector<std::vector<int>> _board;
	std::vector<std::pair<int, int>> _fourDirection = { std::pair<int,int>(-1, 1), std::pair<int,int>(-1, -1), std::pair<int,int>(-1, 0), std::pair<int,int>(0, 1) };
	
	void Reset_Board();
	std::string Verify_Stone(int x, int y, int color);

public:
	Gomoku(int size);
	std::string Place_Stone(int x, int y, int color);
};