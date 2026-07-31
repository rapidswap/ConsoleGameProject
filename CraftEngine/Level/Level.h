#pragma once

namespace Craft
{
	class Level
	{
	public:
		Level();
		virtual ~Level();

		// 초기화 함수.
		virtual void OnInitialized();

		// 게임 플레이 이벤트 함수.
		virtual void BeginPlay();
		virtual void Tick(float deltaTime);
		virtual void Draw();

		// Getter
		inline bool HasInitialized() const { return hasInitialized; }

	protected:
		// 초기화 처리 여부 플래그.
		bool hasInitialized = false;

	};
}

