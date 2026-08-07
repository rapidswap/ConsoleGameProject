#include "GameLevel.h"
#include <Actor/Box.h>
#include <Actor/Player.h>
#include <Actor/Ground.h>
#include <Actor/Wall.h>
#include <Actor/Target.h>

#include <iostream>
#include <cassert>

using namespace Craft;
bool GameLevel::CanMove(
	const Vector2& playerPosition,
	const Vector2& nextPosition)
{
	// 게임 클리어인 경우 처리 안함.
	if (isGameClear)
	{
		return false;
	}

	// 이동하려는 위치에 어떤 액터가 있는지를 확인할 때 타입을 활용.
	// 박스가 있을 때 로직이 더 복잡하기 때문에 이 처리를 위한 배열.
	std::vector<std::shared_ptr<Actor>> boxList;

	// 레벨을 순회하면서 박스 타입을 boxList에 저장.
	for (const std::shared_ptr<Actor>& actor : actorList)
	{
		if (actor->IsTypeOf<Box>())
		{
			boxList.emplace_back(actor);
			continue;
		}
	}
	// 이동하려는 위치에 박스가 있는지 검증을 위한 변수.
	std::shared_ptr<Actor> boxActor = nullptr;

	// 위치 값 비교를 통해 해당 위치에 박스가 있는지 확인.
	for (const std::shared_ptr<Actor>& box : boxList)
	{
		if (box->GetPosition() == nextPosition)
		{
			boxActor = box;
			break;
		}
	}

	// #1. 이동하려는 위치에 박스가 있는 경우.
	if (boxActor)
	{
		// 박스는 밀 수 있지만, 밀리는 위치가 이동 가능해야 함.
		// 1. 두 위치 값이 있을 때 이동 방향 구하기.
		// 플레이어 위치에서 다음 위치로 향하는 방향 구하기.
		// (x1, y1, 1) - (x2, y2, 1) = (x1-x2, y1-y2, 0)
		Vector2 direction = nextPosition - playerPosition;
		
		// 박스가 밀리는 위치 구하기.
		// 위치(좌표) + 벡터(크기, 방향을 가지는 데이터).
		// 벡터의 정의에 따르면 위치 정보는 없음.
		// 하지만 이 덧셈은 수학적으로 정의되지 않음.
		// 동차 좌표계(위치, 벡터)  |  아핀 변환.
		// (x, y, w): w가 0이면 벡터  |  w 1이면 위치
		// (x, y, z, w): w가 0이면 벡터  |  w 1이면 위치
		// (x1, y1, 1) + (x2, y2, 0) = (x1+x2, y1+y2, 1)
		// (x1, y1, 1) + (x2, y2, 1) = (x1+x2, y1+y2, 2) -> 정의가 되어있지 않음.
		// 위치를 바꾸고 싶으면 위치와 벡터를 더해야함.
		Vector2 newPosition = boxActor->GetPosition() + direction;

		// 박스가 밀리는 위치에 다른 박스가 있는지 확인.
		for (const std::shared_ptr<Actor>& otherBox : boxList)
		{

			// 같은 액터를 검사 중이면 건너뛰기.
			if (otherBox == boxActor)
			{
				continue;
			}

			// 위치 확인.
			if (otherBox->GetPosition() == newPosition)
			{
				// 박스를 이동시킬 위치에 다른 박스가 있으면 이동 불가.
				return false;
			}

		}

		// 박스가 밀릴 위치가 이동 가능한지 다시 확인.
		for (const std::shared_ptr<Actor>& actor : actorList)
		{
			// 박스가 밀리는 위치의 액터 검색.
			if (actor->GetPosition() == newPosition)
			{
				// 벽이면 이동 불가.
				if (actor->IsTypeOf<Wall>())
				{
					return false;
				}

				// 땅이거나 목표 지점이면 이동 가능.
				if (actor->IsTypeOf<Ground>() || actor->IsTypeOf<Target>())
				{
					// 박스 밀림 처리.
					boxActor->SetPosition(newPosition);

					return true;
				}
			}
		}
	}

	// #2. 플레이어가 이동하려는 곳에 박스가 없는 경우.
	for (const std::shared_ptr<Actor>& actor : actorList)
	{
		// 플레이어가 이동하려는 위치의 액터 검색.
		if (actor->GetPosition() == nextPosition)
		{
			// 벽이면 이동 불가.
			if (actor->IsTypeOf<Wall>())
			{
				return false;
			}

			// 벽이 아니라면 이동 가능.
			// 1. 앞에서 박스는 아니라고 검증함.
			// 2. 바로 위에서 벽도 아니라고 검증함.
			// 3. 박스도 벽도 아니기 때문에 플레이어/타겟/땅만 남음.
			// 4. 플레이어는 검증 필요하지 않음.
			return true;
		}
	}

	// 예상치 못한 처리 - 이동 불가.
	return false;
}
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

	// 읽은 데이터를 기반으로 로직 제작.
	// 1. 화면에 액터를 그리기.
	
	// 문자열에 저장된 값을 접근할 때 사용할 인덱스.
	int index = 0;

	// 액터 생성에 사용할 위치 값.
	Vector2 position;
	// 종료 조건 - 읽는 위치가 파일의 크기를 넘으면 종료.
	while (index < fileSize)
	{
		// 이번에 확인할 문자 값.
		char mapCharacter = buffer[index];

		// 인덱스 증가 처리.
		++index;

		// 현재 문자가 개행 문자라면 로직은 건너뛰고,
		// 위치 값만 설정.
		if (mapCharacter == '\n')
		{
			++position.y;
			position.x = 0;
			continue;
		}

		// 읽은 문자 별로 처리.
		switch (mapCharacter)
		{
			// 벽.
		case '#':
			// 벽 액터 생성.
			SpawnActor<Wall>(position);
			break;

			// 바닥(땅).
		case '.':
			// 바닥 액터 생성.
			SpawnActor<Ground>(position);
			break;

			// 플레이어.
		case 'p':
			// 플레이어는 이동이 가능하기 때문에 같은 위치에 땅 생성.
			SpawnActor<Ground>(position);

			// 플레이어 액터 생성.
			SpawnActor<Player>(position);
			break;

			// 박스.
		case 'b':
			// 박스는 이동이 가능하기 때문에 같은 위치에 땅 생성.
			SpawnActor<Ground>(position);

			// 박스 액터 생성.
			SpawnActor<Box>(position);
			break;

			// 타겟(목표 위치)
		case 't':
			// 타겟 액터 생성.
			SpawnActor<Target>(position);

			// 목표 스코어 증가 처리.
			++targetScore;
			break;
		}

		// x 위치 업데이트.
		++position.x;
	}
	
	// 모두 사용한 버퍼 해제.
	delete[] buffer;
	buffer = nullptr;

	// 파일 닫기.
	fclose(file);  
	// 파일을 닫아줄 때 file을 다른 곳에서 사용 하지 않아서
	// 닫아주기만 하고 nullptr로 초기화를 않아도 상관없지만
	// nullptr로 초기화 시켜주는것은 좋은습관.
	file = nullptr;
}
