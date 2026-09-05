#define NOMINMAX

#include "AStar.h"
#include "Node.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <Windows.h>

AStar::AStar()
	: startNode(nullptr), goalNode(nullptr)
{
}

AStar::~AStar()
{
	// 메모리 해제.
	for (Node* node : openList)
	{
		SafeDelete(node);
	}
	openList.clear();

	for (Node* node : closedList)
	{
		SafeDelete(node);
	}
	closedList.clear();
}

std::vector<Node*> AStar::FindPath(Node* startNode, Node* goalNode, std::vector<std::vector<int>>& grid)
{
	// 이전 탐색에서 남은 노드들 메모리 정리
	for (Node* node : openList)
	{
		SafeDelete(node);
	}
	openList.clear();

	for (Node* node : closedList)
	{
		SafeDelete(node);
	}
	closedList.clear();

	this->startNode = startNode;
	this->goalNode = goalNode;

	searchHistory.clear();

	// 입력 노드 또는 그리드가 유효하지 않으면 종료.
	if (!this->startNode || !this->goalNode || grid.empty() || grid[0].empty())
	{
		SafeDelete(startNode);
		// 빈 경로 반환.
		return {};
	}

	// 대각선 이동 비용 상수.
	const float diagonalCost = 1.41421356f;

	// 시작 노드를 열린 리스트(OpenList)에 추가.
	openList.emplace_back(this->startNode);

	// 상하좌우 및 대각선 이동 방향과 비용.
	std::vector<Direction> directions =
	{
		// 하상우좌 이동.
		{ 0, 1, 1.0f }, { 0, -1, 1.0f }, { 1, 0, 1.0f }, { -1, 0, 1.0f },

		// 대각선 이동.
		{ 1, 1, diagonalCost }, { 1, -1, diagonalCost }, { -1, 1, diagonalCost }, { -1, -1, diagonalCost }
	};

	// 이웃 노드 탐색 (열린 리스트가 비어 있지 않은 동안 반복).
	while (!openList.empty())
	{
		// 현재 열린 리스트에서 fCost가 가장 낮은 노드 검색.
		Node* lowestNode = openList[0];
		for (Node* node : openList)
		{
			if (node->fCost < lowestNode->fCost)
			{
				lowestNode = node;
			}
		}

		// fCost가 가장 낮은 노드를 현재 노드로 설정.
		Node* currentNode = lowestNode;

		// 현재 노드가 목표 노드인지 확인 후 맞으면 종료.
		if (IsDestination(currentNode))
		{
			return ConstructPath(currentNode);
		}

		// 방문 처리를 위해 현재 노드를 열린 리스트에서 제거.
		for (int ix = 0; ix < static_cast<int>(openList.size()); ++ix)
		{
			if (openList[ix] == currentNode)
			{
				openList.erase(openList.begin() + ix);
				break;
			}
		}

		// 방문 처리를 위해 현재 노드를 닫힌 리스트에 추가.
		closedList.emplace_back(currentNode);

		// 히스토리에 현재 탐색한 위치 저장 (애니메이션 용)
		searchHistory.push_back(currentNode->position);

		// 이웃 노드 방문(탐색). (하/상/우/좌 차례로 방문).
		for (const Direction& direction : directions)
		{
			// 다음에 이동할 위치 설정.
			int newX = currentNode->position.x + direction.x;
			int newY = currentNode->position.y + direction.y;

			// 그리드 밖이면 무시.
			if (!IsInRange(newX, newY, grid))
			{
				continue;
			}

			// 이동할 위치가 장애물(1: 벽, 2: 터렛)인 경우에는 무시.
			if (grid[newY][newX] == 1 || grid[newY][newX] == 2)
			{
				continue;
			}

			// 대각선 이동 시 모서리(장애물)를 통과하는지 확인.
			if (IsDiagonalBlocked(currentNode->position, direction, grid))
			{
				continue;
			}

			// 현재 노드를 기준으로 새 gCost 계산.
			float newGCost = currentNode->gCost + direction.cost;

			// 이미 더 좋은 비용으로 방문한 노드인 경우 무시.
			if (HasVisited(newX, newY, newGCost))
			{
				continue;
			}

			// 방문을 위한 이웃 노드 생성.
			Node* neighborNode = new Node(newX, newY, currentNode);
			// 방문할 노드의 gCost 계산.
			neighborNode->gCost = newGCost;
			// 방문할 노드의 hCost 계산.
			neighborNode->hCost = CalculateHeuristic(neighborNode, this->goalNode);
			// 방문할 노드의 fCost 계산.
			neighborNode->fCost = neighborNode->gCost + neighborNode->hCost;

			// 이웃 노드가 열린 리스트에 있는지 확인.
			Node* openListNode = nullptr;
			for (Node* node : openList)
			{
				if (*node == *neighborNode)
				{
					openListNode = node;
					break;
				}
			}

			// 이웃 노드가 열린 리스트에 있으면 더 좋은 경로인 경우 비용만 갱신.
			if (openListNode != nullptr)
			{
				if (neighborNode->gCost < openListNode->gCost || neighborNode->fCost < openListNode->fCost)
				{
					// 부모 노드 갱신.
					openListNode->parentNode = currentNode;
					// 비용 정보 갱신.
					openListNode->gCost = neighborNode->gCost;
					// 비용 정보 갱신.
					openListNode->hCost = neighborNode->hCost;
					// 비용 정보 갱신.
					openListNode->fCost = neighborNode->fCost;
				}
				// 임시 생성 노드 메모리 해제.
				SafeDelete(neighborNode);
				continue;
			}

			// 새로 방문 가능한 노드를 열린 리스트에 추가.
			if (grid[newY][newX] == 0)
			{
				// 방문 시각화 값 설정. (디버그용이므로 엔진에서는 꺼둠)
				// grid[newY][newX] = 5;
			}
			openList.emplace_back(neighborNode);

			// 콘솔 시각화 및 딜레이는 게임 루프를 정지시키므로 주석 처리
			// DisplayGrid(grid);
			// int delay = static_cast<int>(0.05f * 1000);
			// Sleep(delay);
		}
	}

	return {};
}

std::vector<Node*> AStar::ConstructPath(Node* goalNode)
{
	// 목표 노드 부터, 부모 노드를 따라 역추적하면서 경로 노드 설정.
	std::vector<Node*> path;
	Node* currentNode = goalNode;
	while (currentNode != nullptr)
	{
		path.emplace_back(currentNode);
		currentNode = currentNode->parentNode;
	}

	std::reverse(path.begin(), path.end());
	return path;
}

float AStar::CalculateHeuristic(Node* currentNode, Node* goalNode)
{
	// 현재 노드의 위치에서 목표 위치까지의 거리를 휴리스틱 값으로 사용.
	Position diff = *currentNode - *goalNode;
	return static_cast<float>(std::sqrt(std::pow(diff.x, 2) + std::pow(diff.y, 2)));
}

bool AStar::IsInRange(int x, int y, const std::vector<std::vector<int>>& grid)
{
	// 빈 그리드면 탐색할 수 없으므로 false 반환.
	if (grid.empty() || grid[0].empty())
	{
		return false;
	}

	// x, y 범위가 벗어나면 false 반환.
	if (x < 0 || x >= static_cast<int>(grid[0].size()) || y < 0 || y >= static_cast<int>(grid.size()))
	{
		return false;
	}

	// 벗어나지 않았으면 true 반환.
	return true;
}

bool AStar::HasVisited(int x, int y, float gCost)
{
	// 열린 리스트에 이미 같은 위치가 있고 비용이 더 좋거나 같으면 방문했다고 판단.
	for (Node* const node : openList)
	{
		if (node->position.x == x && node->position.y == y && gCost >= node->gCost)
		{
			return true;
		}
	}

	// 닫힌 리스트에 이미 같은 위치가 있고 비용이 더 좋거나 같으면 방문했다고 판단.
	for (Node* const node : closedList)
	{
		if (node->position.x == x && node->position.y == y && gCost >= node->gCost)
		{
			return true;
		}
	}

	// 리스트에 없거나 더 좋은 비용이면 방문하지 않은 것으로 판단.
	return false;
}

bool AStar::IsDestination(const Node* node)
{
	// 노드의 위치가 서로 같은지 비교.
	return *node == *goalNode;
}

void AStar::DisplayGrid(std::vector<std::vector<int>>& grid)
{
	static COORD position = { 0, 0 };
	static HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(handle, position);

	int white = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	int green = FOREGROUND_GREEN;

	for (int y = 0; y < static_cast<int>(grid.size()); ++y)
	{
		for (int x = 0; x < static_cast<int>(grid[0].size()); ++x)
		{
			// 시작 위치.
			if (grid[y][x] == 2)
			{
				SetConsoleTextAttribute(handle, FOREGROUND_RED);
				std::cout << "S ";
			}

			// 목표 위치.
			if (grid[y][x] == 3)
			{
				SetConsoleTextAttribute(handle, FOREGROUND_RED);
				std::cout << "G ";
			}

			// 장애물.
			if (grid[y][x] == 1)
			{
				SetConsoleTextAttribute(handle, white);
				std::cout << "1 ";
			}

			// 경로.
			else if (grid[y][x] == 5)
			{
				SetConsoleTextAttribute(handle, green);
				std::cout << "+ ";
			}

			// 빈 공간.
			else if (grid[y][x] == 0)
			{
				SetConsoleTextAttribute(handle, white);
				std::cout << "0 ";
			}
		}

		std::cout << "\n";
	}
}

bool AStar::IsDiagonalBlocked(const Position& current, const Direction& direction, const std::vector<std::vector<int>>& grid) const
{
	// 이동하려는 방향에 장애물이 있는지 확인
	// 대각선 방향의 x, y 성분이 모두 0이 아니어야 대각선임.
	if (direction.x == 0 || direction.y == 0)
	{
		return false;
	}

	// 대각선으로 이동하려는 새로운 위치의 x, y 성분을 분해.
	int sideX = current.x + direction.x;
	int sideY = current.y + direction.y;

	// 벽(1) 이나 터렛(2)이 있으면 대각선 이동 불가로 판정
	return (grid[current.y][sideX] == 1 || grid[current.y][sideX] == 2) || 
		   (grid[sideY][current.x] == 1 || grid[sideY][current.x] == 2);
}

void AStar::DisplayGridWithPath(std::vector<std::vector<int>>& grid, const std::vector<Node*>& path)
{
	static COORD position = { 0, 0 };
	static HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(handle, position);

	int white = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	int green = FOREGROUND_GREEN;
	int red = FOREGROUND_RED;

	// 구분을 위해 설정했던 데이터 초기화.
	for (int y = 0; y < static_cast<int>(grid.size()); ++y)
	{
		for (int x = 0; x < static_cast<int>(grid[0].size()); ++x)
		{
			int& value = grid[y][x];
			if (value == 5)
			{
				value = 0;
			}
		}
	}

	// 경로를 제외한 맵 출력.
	for (int y = 0; y < static_cast<int>(grid.size()); ++y)
	{
		for (int x = 0; x < static_cast<int>(grid[0].size()); ++x)
		{
			// 시작 위치.
			if (grid[y][x] == 2)
			{
				SetConsoleTextAttribute(handle, red);
				std::cout << "S ";
			}

			// 목표 위치.
			else if (grid[y][x] == 3)
			{
				SetConsoleTextAttribute(handle, red);
				std::cout << "G ";
			}

			// 장애물.
			else if (grid[y][x] == 1)
			{
				SetConsoleTextAttribute(handle, white);
				std::cout << "1 ";
			}

			// 빈 공간.
			else if (grid[y][x] == 0)
			{
				SetConsoleTextAttribute(handle, white);
				std::cout << "0 ";
			}
		}

		std::cout << "\n";
	}

	// 경로 출력.
	for (const Node* node : path)
	{
		// 시작/목표 위치는 원래 표기를 유지.
		//if (grid[node->position.y][node->position.x] == 2 || grid[node->position.y][node->position.x] == 3)
		//{
		//	continue;
		//}

		// 경로는 '*'로 표시.
		COORD position{ static_cast<short>(node->position.x * 2), static_cast<short>(node->position.y) };
		SetConsoleCursorPosition(handle, position);
		SetConsoleTextAttribute(handle, green);

		std::cout << "* ";
		int delay = static_cast<int>(0.05f * 1000);
		Sleep(delay);
	}
}
