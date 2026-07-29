#include "Engine.h"
#include <windows.h>
#include<stdint.h>
#include <iostream>

namespace Craft
{
	Engine::Engine()
	{
	}

	Engine::~Engine()
	{
	}

	void Engine::Run()
	{
		// 윈도우가 제공하는 고해상도 타이머 (하드웨어 타이머).

		// QueryPerformanceFrequency: 타이머의 해상도
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);

		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);

		// 프레임 시간 계산을 위한 변수
		int64_t currentTime = counter.QuadPart;
		int64_t previousTime = currentTime;

		// 프레임 고정.
		float oneFrameTime = 1.0f / 120.0f;

		while (true)
		{
			// 입력 처리.
			ProcessInput();

			// 현재 시간 확인.
			QueryPerformanceCounter(&counter);

			// 현재 시간 저장.
			currentTime = counter.QuadPart;

			// 프레임 시간 계산.
			float deltaTime = static_cast<float>(currentTime - previousTime)
				/ static_cast<float>(frequency.QuadPart);

			// 고정 프레임 처리
			if (deltaTime >= oneFrameTime)
			{
				// 레벨 초기화 이벤트 함수.
				OnInitialized();

				// 레벨의 액터 초기화 이벤트 함수.
				BeginPlay();

				// 레벨의 액터 업데이트 함수.
				Tick(deltaTime);

				// 업데이트된 결과를 화면에 그리는 함수.
				Draw();

				//처리된 입력을 이전 프레임 입력으로 저장.
				SavePreviousInputStates();

				// 이전 프레임 시간 기록.
				previousTime = currentTime;
			}
		}
	}

	void Engine::Quit()
	{
	}

	void Engine::ProcessInput()
	{
	}

	void Engine::OnInitialized()
	{
	}

	void Engine::BeginPlay()
	{
	}

	void Engine::Tick(float deltaTime)
	{
		std::cout << "DeltaTime: " << deltaTime
			<< " | FPS: " << (1.0f / deltaTime)
			<< "\n";
	}

	void Engine::Draw()
	{
	}

	void Engine::SavePreviousInputStates()
	{
	}

	void Engine::Shutdown()
	{
	}

}