#pragma once

#include <vector>

class Gomoku
{
private:
	std::vector<std::vector<int>> gomokuBoard;

	bool _isMyTurn = false;
	bool _black = false;
	bool _isGameStarted = false;

	void ResetBoard(int size);

public:
	void SetTurn(bool isMyTurn);
	void SetColor(bool isMyTurn);

	bool ShowTurn();
	int  ShowColor();
	void CreateBoard(int size);
	const std::vector<std::vector<int>>& ShowBoard();
	void SetBoard(int y, int x, int color);

	void StartGame();
	void EndGame();
	bool ShowGameStatus();



};