#include "GameLevel.h"
#include <iostream>
#include <cassert>
void GameLevel::OnInitialized()
{
	Level::OnInitialized();

	// 파일을 읽어서 맵 로드.
	LoadMap("Map.txt");
}

void GameLevel::Draw()
{
	Level::Draw();
}

void GameLevel::LoadMap(const std::string& filename)
{
	// 최종 경로 조립.
	std::string path = std::string("../Assets/") + filename;

	// 파일 열기 (C-Style).
	FILE* file = nullptr;
	fopen_s(&file, path.c_str(), "rb");
	if (!file)
	{
		assert(false && "failed to open sokoban stage file.");
		return;
	}

	// 파일의 내용을 저장할 버퍼(데이터 저장공간) 확인.
	// 파일 길이 확인 -> 파일 위치를 제일 뒤로 이동 시킨 다음, 해당 위치 값 읽기.
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	

	// 파일 제일 끝위치를 구한 다음에는 다시 처음으로 되돌리기.
	//fseek(file, 0, SEEK_SET);
	rewind(file);

	// 앞에서 구한 위치를 사용해서 버퍼 생성.
	char* buffer = new char[fileSize] {};
	
	// 데이터 읽기(파일 읽기).
	size_t readSize = fread(buffer, sizeof(char), fileSize, file);


	// 어서트.
	assert(readSize > 0 && "No data is in the stage file.");

	// Todo: 읽은 데이터를 기반으로 로직 제작.

	// 모두 사용한 버퍼 해제.
	delete[] buffer;
	buffer = nullptr;



	
	// 파일 닫기.
	fclose(file);
	file = nullptr;
}
