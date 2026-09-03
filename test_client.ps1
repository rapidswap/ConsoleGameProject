$client = New-Object System.Net.Sockets.TcpClient
$client.Connect("127.0.0.1", 7777)
$stream = $client.GetStream()
$resBuffer = New-Object byte[] 1024

Write-Host "========================================" -ForegroundColor Green
Write-Host "  SK Defense 게임 패킷 테스터 연결 성공!  " -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

while ($true) {
    Write-Host ""
    Write-Host "[원하는 행동을 선택하세요]" -ForegroundColor Cyan
    Write-Host "1. 로그인 요청 (C_LOGIN)"
    Write-Host "2. 타워 설치 요청 (C_BUILD_TURRET)"
    Write-Host "3. 채팅 전송 (C_CHAT)"
    Write-Host "4. 종료"
    
    $choice = Read-Host "선택 (1~4)"

    if ($choice -eq "1") {
        $name = Read-Host "플레이어 닉네임 입력"
        $loginBytes = New-Object byte[] 36
        [System.BitConverter]::GetBytes([uint16]36).CopyTo($loginBytes, 0)
        [System.BitConverter]::GetBytes([uint16]1001).CopyTo($loginBytes, 2)
        $nameBytes = [System.Text.Encoding]::ASCII.GetBytes($name)
        [System.Array]::Copy($nameBytes, 0, $loginBytes, 4, [Math]::Min($nameBytes.Length, 31))
        
        $stream.Write($loginBytes, 0, 36)
        $stream.Flush()

        # 서버 응답 수신
        $null = $stream.Read($resBuffer, 0, 1024)
        $playerId = [System.BitConverter]::ToUInt32($resBuffer, 4)
        $gold = [System.BitConverter]::ToInt32($resBuffer, 8)
        Write-Host ">> [서버 응답] 로그인 성공! 내 ID: $playerId, 시작 골드: $gold G" -ForegroundColor Yellow
    }
    elseif ($choice -eq "2") {
        $rawX = Read-Host "설치할 X 좌표 (예: 5)"
        $rawY = Read-Host "설치할 Y 좌표 (예: 3)"
        $rawType = Read-Host "타워 종류 (0:일반, 1:매직, 2:레어)"

        $posX = [int]$rawX
        $posY = [int]$rawY
        $type = [int]$rawType

        $buildBytes = New-Object byte[] 16
        [System.BitConverter]::GetBytes([uint16]16).CopyTo($buildBytes, 0)
        [System.BitConverter]::GetBytes([uint16]3001).CopyTo($buildBytes, 2)
        [System.BitConverter]::GetBytes([int32]$posX).CopyTo($buildBytes, 4)
        [System.BitConverter]::GetBytes([int32]$posY).CopyTo($buildBytes, 8)
        [System.BitConverter]::GetBytes([int32]$type).CopyTo($buildBytes, 12)

        $stream.Write($buildBytes, 0, 16)
        $stream.Flush()

        # 서버 응답 수신
        $null = $stream.Read($resBuffer, 0, 1024)
        $success = [System.BitConverter]::ToBoolean($resBuffer, 4)
        $remGold = [System.BitConverter]::ToInt32($resBuffer, 21)
        Write-Host ">> [서버 응답] 타워 설치 승인(Success: $success)! 남은 골드: $remGold G" -ForegroundColor Yellow
    }
    elseif ($choice -eq "3") {
        $msg = Read-Host "보낼 채팅 내용"
        $chatBytes = New-Object byte[] 132
        [System.BitConverter]::GetBytes([uint16]132).CopyTo($chatBytes, 0)
        [System.BitConverter]::GetBytes([uint16]2001).CopyTo($chatBytes, 2)
        $msgBytes = [System.Text.Encoding]::ASCII.GetBytes($msg)
        [System.Array]::Copy($msgBytes, 0, $chatBytes, 4, [Math]::Min($msgBytes.Length, 127))

        $stream.Write($chatBytes, 0, 132)
        $stream.Flush()

        # 서버 응답 수신
        $null = $stream.Read($resBuffer, 0, 1024)
        $recvMsg = [System.Text.Encoding]::ASCII.GetString($resBuffer, 8, 128).TrimEnd("`0")
        Write-Host ">> [서버 응답] 채팅 에코 수신: $recvMsg" -ForegroundColor Yellow
    }
    elseif ($choice -eq "4") {
        break
    }
}

$client.Close()
Write-Host "연결을 종료했습니다." -ForegroundColor Red
