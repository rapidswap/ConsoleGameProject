#include "Engine.h"
#include <Level/Level.h>

#include <iostream>
#include <Windows.h>
#include <cassert>


namespace Craft
{
	// 전역 변수 초기화.
	Engine* Engine::instance = nullptr;

	Engine::Engine()
	{
		// instance 초기화.
		assert(!instance && "instance is not null");
		instance = this;
	}

	Engine::~Engine()
	{
		instance = nullptr;
	}

	void Engine::Run()
	{
		// 고해상도 타이머 사용.

		// 밀리세컨드 - 1/1000초 -> 해상도 1000.
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);

		// 현재 시간 읽기.
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);

		// 프레임 계산을 위한 변수.
		int64_t current = counter.QuadPart;
		int64_t previous = current;

		// 고정 프레임으로 만들기 위한 값.
		float oneFrameTime = 1.0f / setting.framerate;

		// 엔진 루프.
		while (true)
		{
			// 종료 조건 처리.
			if (isQuit)
			{
				break;
			}

			// 프레임 처리.

			// 입력 처리.
			ProcessInput();

			// 프레임 시간 계산.
			// 1. 현재 시간 읽기.
			QueryPerformanceCounter(&counter);
			current = counter.QuadPart;

			// 2. (현재 시간 - 이전 시간) / 시간 단위(해상도).
			//    -> 초단위로 변환.
			//    예) 밀리세컨드(1/1000초). 200밀리세컨드 -> 초?
			float deltaTime
				= static_cast<float>(current - previous)
				/ static_cast<float>(frequency.QuadPart);

			// 고정 프레임 처리.
			// 프레임 사이에 걸린 시간이 목표 시간보다 더 많이 지났으면
			// 프레임 처리.
			if (deltaTime >= oneFrameTime)
			{
				// 게임 이벤트 함수 호출.
				OnInitialized();

				// 게임 이벤트의 초기화 함수(1번만 호출).
				BeginPlay();

				// 게임 업데이트.
				Tick(deltaTime);

				// 화면 그리기.
				Draw();

				// 이 위까지 호출이 완료되면 프레임 처리 완료됨.

				// 레벨 전환 처리.
				if (nextLevel)
				{
					// 기존 레벨 정리.
					if (mainLevel)
					{
						mainLevel.reset();
					}
					
					// 추가 요청된 레벨을 메인 레벨로 설정.
					mainLevel = nextLevel;

					// 포인터 정리.
					nextLevel.reset();
				}

				// 추가 또는 제거 요청된 액터 정리.
				if (mainLevel)
				{
					mainLevel->ProcessAddAndDestroyActors();
				}

				// 입력 상태 저장.
				SavePreviousInputStates();

				// 현재 시간을 이전 시간으로 저장.
				previous = current;
			}
		}

		// 종료 처리 함수 호출.
		Shutdown();
	}

	void Engine::Quit()
	{
		// 엔진 종료 플래그 설정.
		isQuit = true;
	}

	Engine& Engine::Get()
	{
		// 검증 - 어서트(어써트/assert).
		// 무조건(필수로) 통화해야하는 조건이 있을 때 사용.
		// 디버그 모드에서만 동작.
		assert(instance && "instance is null");
		return *instance;
	}

	void Engine::ProcessInput()
	{
	}

	void Engine::OnInitialized()
	{
		// 레벨 초기화 처리.
		// 예외 처리.

		if (!mainLevel || mainLevel->HasInitialized())
		{
			return;
		}

		mainLevel->OnInitialized();
	}

	void Engine::BeginPlay()
	{
		if (!mainLevel)
		{
			return;
		}

		// 레벨의 이벤트 전달.
		mainLevel->BeginPlay();
	}

	void Engine::Tick(float deltaTime)
	{
		if (!mainLevel)
		{
			return;
		}

		mainLevel->Tick(deltaTime);
	}

	void Engine::Draw()
	{
		if (!mainLevel)
		{
			return;
		}

		mainLevel->Draw();
	}
	void Engine::SavePreviousInputStates()
	{
	}
	void Engine::Shutdown()
	{
	}
}