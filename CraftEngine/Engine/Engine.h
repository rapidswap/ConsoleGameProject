#pragma once
// CraftEngine 프로젝트 안의 클래스느 Craft 네임 스페이스 사용.

namespace Craft 
{
	// 메인 엔진 클래스.
	// 엔진 루프 제공.
	// 게임 엔진의 핵심 기능 제공.
	class Engine
	{
		// 엔진 설정 (데이터).
		struct Setting
		{
			// 목표 프레임 수 (초당 프레임).
			float framerate = 120.0f;
		};

	public:
		Engine();
		// 확장 염두해두고 virtual.
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

	protected:
		// 엔진 종료 요청 여부 플래그.
		bool isQuit = false;

		// 엔진 설정 함수.
		Setting setting;
	};
}

