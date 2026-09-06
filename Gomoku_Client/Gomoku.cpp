#include "Gomoku.h"

void Gomoku::SetTurn(bool isMyTurn)
{
	_isMyTurn = isMyTurn;
}

void Gomoku::SetColor(bool isMyTurn)
{
	_black = isMyTurn;
}

bool Gomoku::ShowTurn()
{
	return _isMyTurn;
}

int Gomoku::ShowColor()
{
	//흑돌이면 1, 백돌이면 2
	if (_black)
		return 1;
	else
		return 2;
}

void Gomoku::CreateBoard(int size)
{
	if (gomokuBoard.empty())
		gomokuBoard = std::vector<std::vector<int>>(size, std::vector<int>(size, 0));
}

void Gomoku::ResetBoard(int size)
{
	_isMyTurn = false;
	_black = false;
	gomokuBoard = std::vector<std::vector<int>>(size, std::vector<int>(size, 0));
}

const std::vector<std::vector<int>>& Gomoku::ShowBoard()
{
	return gomokuBoard;
}

void Gomoku::SetBoard(int y, int x, int color)
{
	gomokuBoard[y][x] = color;
}


void Gomoku::StartGame()
{
	_isGameStarted = true;
}
void Gomoku::EndGame()
{
	_isGameStarted = false;
	ResetBoard(15);
}

bool Gomoku::ShowGameStatus()
{
	return _isGameStarted;
}