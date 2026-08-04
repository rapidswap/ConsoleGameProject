#pragma once

#include <Core/Core.h>
#include <Math/Vector2.h>
#include <Math/Color.h>
#include <string>
#include <vector>

namespace Craft
{
	// 그리기 기능을 전담하는 전문 객체.
	class CRAFT_API Renderer
	{
		// 화면에 그릴 데이터를 명령 단위로 저장하기 위한 구조체.
		struct RenderCommand
		{
			// 화면에 그릴 문자값.
			std::string image;

			// 위치.
			Vector2 position = Vector2::Zero;

			// 색상.
			Color color = Color::White;

			// 그리기 정렬 순서.
			int sortingOrder = -1;
		};

	public:
		Renderer();
		~Renderer();

		// 화면에 그릴 데이터를 제출(전달)하는 함수.
		void Submit(
			const std::string& image,
			const Vector2& position,
			Color color = Color::White,
			int sortingOrder = 0
		);

		// Draw 이벤트 함수 - Engine에서 호출.
		void Draw();

		// 전역 접근 함수.
		static Renderer& Get();

	private:
		// 그리기 작업을 시작할 때 프레임(화면)을 지우는 함수.
		void Clear();

		// 전달 받은 렌더 명령을 활용해 화면을 그리는 함수.
		void DrawrenderQueue();

		// 그린 결과를 화면에 표시하는 함수.
		void Present();

	private:
		
		// 전역 접근이 가능하도록 변수 선언.
		static Renderer* instance;
		
		// 이번 프레임에 그릴 렌더 명령을 모아두는 배열.
		// 큐(Queue) 처럼 사용.
		std::vector<RenderCommand> renderQueue;

	};
}
