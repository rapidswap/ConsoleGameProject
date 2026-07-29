#pragma once

namespace Craft 
{
	class Engine
	{
	public:
		Engine();
		virtual ~Engine();

		// 게임 루프 실행 함수.
		void Run();

		// 엔진 종료 함수.
		void Quit();

	protected:
		// 입력 처리 함수.
		void ProcessInput();

		// 초기화 함수.
		// 레벨 초기화 함수.
		void OnInitialized();

		// 액터 초기화 함수.
		void BeginPlay();

		// 업데이트 함수.
		void Tick(float deltaTime);

		// 화면에 그리는 함수.
		void Draw();

		// 이전입력을 저장하는 함수.
		void SavePreviousInputStates();

		// 정리 함수.
		void Shutdown();
	};
}

