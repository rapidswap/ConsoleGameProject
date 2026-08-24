#pragma once

#include <iostream>

class Position
{
public:
	Position(int x, int y)
		: x(x), y(y)
	{
	}

public:
	int x;
	int y;
};

class Node
{
public:
	Node(int x, int y, Node* parentNode = nullptr)
		: position(x, y), gCost(0.0f), hCost(0.0f), fCost(0.0f), parentNode(parentNode)
	{
	}

	~Node()
	{
	}

	Position operator-(const Node& otherNode)
	{
		return Position(position.x - otherNode.position.x, position.y - otherNode.position.y);
	}

	bool operator==(const Node& otherNode) const
	{
		return position.x == otherNode.position.x && position.y == otherNode.position.y;
	}

public:
	Position position;
	float gCost;
	float hCost;
	float fCost;
	Node* parentNode;
};