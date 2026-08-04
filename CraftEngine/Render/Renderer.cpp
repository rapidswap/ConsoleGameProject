#include "Renderer.h"
#include <cassert>
#include <iostream>
#include <Windows.h>


namespace Craft
{
	// static 변수 초기화.
	Renderer* Renderer::instance = nullptr;

	Renderer::Renderer()
	{
		// 어서트.
		assert(!instance && "instance should be null.");
		instance = this;

		// 콘설 커서 안보이게 설정.
		CONSOLE_CURSOR_INFO info;
		GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

		// 보이기 옵션을 false로 설정.
		info.bVisible = FALSE;
		SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
	}

	Renderer::~Renderer()
	{
		instance = nullptr;

		// 콘설 커서 다시 보이게 설정(복구).
		CONSOLE_CURSOR_INFO info;
		GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

		// 보이기 옵션을 true로 설정.
		info.bVisible = TRUE;
		SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
	}

	void Renderer::Submit(const std::string& image, const Vector2& position, Color color, int sortingOrder)
	{
		// 렌더 명령 생성 및 값 설정.
		RenderCommand command;
		command.image = image;
		command.position = position;
		command.color = color;
		command.sortingOrder = sortingOrder;

		// 렌더 큐에 명령 추가.
		renderQueue.emplace_back(command);
	} 

	void Renderer::Draw()
	{
		// 화면 지우기.
		Clear();

		// 프레임 그리기.
		DrawrenderQueue();

		// 화면(이미지/프레임) 표시.
		Present();
	}

	Renderer& Renderer::Get()
	{
		assert(instance && "instance should not be null.");
		return *instance;
	}

	void Renderer::Clear()
	{
		// @Temp: 시스템 clear 함수 사용.
		system("cls");
	}

	void Renderer::DrawrenderQueue()
	{
		// 렌더 큐를 순회하면서 그리기 명령 실행.
		for (const RenderCommand& command : renderQueue)
		{
			// 윈도우 콘솔 핸들.
			HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

			// 그릴 위치로 이동(콘솔 좌표 이동).
			SetConsoleCursorPosition(handle, command.position);

			// 글자 색상 결정.
			SetConsoleTextAttribute(handle, static_cast<WORD>(command.color));

			// @Temp: 그리기.
			std::cout << command.image;

			// 콘솔 생상 복원.
			SetConsoleTextAttribute(handle, static_cast<WORD>(Color::White));
		}

		// 렌더큐 비우기.
		renderQueue.clear();
	}

	void Renderer::Present()
	{
		// Todo: 이중 버퍼링 구현할 때 내용 채우기.
	}
}