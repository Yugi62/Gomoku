#include "Gomoku.h"

Gomoku::Gomoku(int size) : _boardSize(size)
{
	_board = std::vector<std::vector<int>>(_boardSize, std::vector<int>(_boardSize, 0));
}

void Gomoku::Reset_Board()
{
	_board = std::vector<std::vector<int>>(_boardSize, std::vector<int>(_boardSize, 0));
}

std::string Gomoku::Place_Stone(int x, int y, int color)
{
	_board[y][x] = color;

	std::string result = Verify_Stone(x, y, color);

	return result;
}

std::string Gomoku::Verify_Stone(int x, int y, int color)
{
	//4방향을 기준으로 오목, 다목 검사
	for (auto direction : _fourDirection)
	{
		//연속되는 돌의 개수
		int cnt = 1;

		int xDir = direction.first;
		int yDir = direction.second;


		//한쪽으로 동일하지 않은 색 혹은 범위에서 벗어날 때까지 전진하여 cnt를 증가
		int temp = 1;
		while (true)
		{
			int xTemp = x + xDir * temp;
			int yTemp = y + yDir * temp;
			temp++;

			//범위 안에 있는 경우에만 
			if (xTemp >= 0 && xTemp < _board[0].size() && yTemp >= 0 && yTemp < _board.size())
			{
				if (_board[yTemp][xTemp] == color)
					cnt++;
				else
					break;
			}
			else
				break;
		}
		//반대 쪽도 똑같이 반복
		temp = 1;
		while (true)
		{
			int xTemp = x - xDir * temp;
			int yTemp = y - yDir * temp;
			temp++;

			//범위 안에 있는 경우에만 
			if (xTemp >= 0 && xTemp < _board[0].size() && yTemp >= 0 && yTemp < _board.size())
			{
				if (_board[yTemp][xTemp] == color)
					cnt++;
				else
					break;
			}
			else
				break;
		}

		//오목
		if (cnt == 5)
		{
			Reset_Board();
			return "Gomoku";
		}
		
		//다목
		else if (cnt > 5)
		{
			Reset_Board();
			return "Overline";
		}
		
	}

	int openThree = 0;
	int four = 0;


	//오목, 다목이 아니면서 흑돌인 경우 33,44 검사를 실시
	if (color == 1)
	{
		//4방향 중에서 열린3과 4가 각각 몇 개 있는지 검사 
		for (auto direction : _fourDirection)
		{
			int xDirection = direction.first;
			int yDirection = direction.second;

			int startX;
			int startY;
			int endX;
			int endY;

			//검사 가능한 범위의 사이즈
			int sz = 1;

			//검사할 시작과 끝 위치를 탐색 (착수 위치를 기준으로 최대 4칸 앞과 뒤)
			for (int i = 4; i >= 0; i--)
			{
				int xTemp = x + xDirection * i;
				int yTemp = y + yDirection * i;

				if (xTemp >= 0 && xTemp < _board[0].size() && yTemp >= 0 && yTemp < _board.size())
				{
					startX = xTemp;
					startY = yTemp;
					sz += i;
					break;
				}
			}
			for (int i = 4; i >= 0; i--)
			{
				int xTemp = x - xDirection * i;
				int yTemp = y - yDirection * i;

				if (xTemp >= 0 && xTemp < _board[0].size() && yTemp >= 0 && yTemp < _board.size())
				{
					endX = xTemp;
					endY = yTemp;
					sz += i;
					break;
				}
			}

			//검사 범위가 4칸 이하인 경우 스킵 (돌을 둘 수 있는 영역이 4칸 이하인 경우 열린3과 4이 될 수가 없다)
			if (sz < 5)
				continue;

			int cnt = 0;
			int sx = startX;
			int sy = startY;

			std::vector<int> count(3, 0);

			//start가 end 좌표와 겹친 이후에 반복문 종료
			while (true)
			{
				//cnt가 4이하일 때는 순수 개수를 증가
				if (cnt < 5)
				{
					count[_board[startY][startX]]++;
					cnt++;
				}
				//cnt가 5이상일 때는 
				else
				{
					count[_board[sy][sx]]--;
					sx -= xDirection;
					sy -= yDirection;
					count[_board[startY][startX]]++;
				}

				//cnt == 5부터는 열린3, 4가 존재하는지 계산
				if (cnt == 5)
				{
					//흑돌이 3개, 공백이 2개인 경우
					if (count[1] == 3 && count[0] == 2)
					{
						//양쪽 끝이 공백이면서 그 공백 한 칸 앞도 공백인 경우 열린 3이다
						if (_board[sy][sx] == 0 && _board[startY][startX] == 0)
						{
							int xTemp = startX - xDirection;
							int yTemp = startY - yDirection;

							if (xTemp >= 0 && xTemp < _board[0].size() && yTemp >= 0 && yTemp < _board.size())
							{
								//열린 3
								if (_board[yTemp][xTemp] == 0)
								{
									openThree++;
									break;
								}
							}
							xTemp = sx + xDirection;
							yTemp = sy + yDirection;

							if (xTemp >= 0 && xTemp < _board[0].size() && yTemp >= 0 && yTemp < _board.size())
							{
								//열린 3
								if (_board[yTemp][xTemp] == 0)
								{
									openThree++;
									break;
								}
							}
						}

						//한쪽만 공백인 경우 그 반대쪽의 한 칸 앞이 공백인 경우 열린 3이다
						else if (_board[sy][sx] == 0)
						{
							int xTemp = startX - xDirection;
							int yTemp = startY - yDirection;

							if (xTemp >= 0 && xTemp < _board[0].size() && yTemp >= 0 && yTemp < _board.size())
							{
								//열린 3
								if (_board[yTemp][xTemp] == 0)
								{
									openThree++;
									break;
								}
							}
						}
						//한쪽만 공백인 경우 그 반대쪽의 한 칸 앞이 공백인 경우 열린 3이다
						else if (_board[startY][startX] == 0)
						{
							int xTemp = sx + xDirection;
							int yTemp = sy + yDirection;

							if (xTemp >= 0 && xTemp < _board[0].size() && yTemp >= 0 && yTemp < _board.size())
							{
								//열린 3
								if (_board[yTemp][xTemp] == 0)
								{
									openThree++;
									break;
								}
							}
						}
					}

					//흑돌이 4개, 공백이 1개인 경우 4이다
					else if (count[1] == 4 && count[0] == 1)
					{
						four++;
						break;
					}
				}

				//start가 end와 겹친 경우 반복문 종료
				if (startX == endX && startY == endY)				
					break;				

				//start를 end 방향으로 이동
				startX -= xDirection;
				startY -= yDirection;
			}
		}
	}


	if (openThree >= 2)
	{
		Reset_Board();
		return "Three";
	}

	else if (four >= 2)
	{
		Reset_Board();
		return "Four";
	}

	else
		return "";
}