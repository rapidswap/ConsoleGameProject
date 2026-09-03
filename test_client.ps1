# SK Defense 실시간 멀티플레이 대화형 클라이언트
param(
    [string]$ServerIP = "127.0.0.1",
    [int]$Port = 7777
)

# 1. 초고속 백그라운드 패킷 수신용 C# 헬퍼 컴파일
$csharpCode = @"
using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;

public class PacketReceiver
{
    private NetworkStream _stream;
    private bool _running = true;

    public void Start(NetworkStream stream)
    {
        _stream = stream;
        Thread thread = new Thread(ReceiveLoop);
        thread.IsBackground = true;
        thread.Start();
    }

    private void ReceiveLoop()
    {
        byte[] buffer = new byte[4096];
        while (_running)
        {
            try
            {
                if (_stream.DataAvailable)
                {
                    int read = _stream.Read(buffer, 0, buffer.Length);
                    if (read <= 0) break;

                    int offset = 0;
                    while (offset + 4 <= read)
                    {
                        ushort size = BitConverter.ToUInt16(buffer, offset);
                        ushort id = BitConverter.ToUInt16(buffer, offset + 2);

                        if (offset + size > read) break;

                        if (id == 1002) // S_LOGIN_OK
                        {
                            uint playerId = BitConverter.ToUInt32(buffer, offset + 4);
                            int gold = BitConverter.ToInt32(buffer, offset + 8);
                            Console.ForegroundColor = ConsoleColor.Cyan;
                            Console.WriteLine("\n>>> [로그인 성공] Player ID: " + playerId + ", 시작 골드: " + gold + " G");
                            Console.ResetColor();
                        }
                        else if (id == 2002) // S_CHAT
                        {
                            uint senderId = BitConverter.ToUInt32(buffer, offset + 4);
                            string msg = Encoding.ASCII.GetString(buffer, offset + 8, 128).TrimEnd('\0');
                            if (senderId == 0)
                            {
                                Console.ForegroundColor = ConsoleColor.Magenta;
                                Console.WriteLine("\n📢 " + msg);
                            }
                            else
                            {
                                Console.ForegroundColor = ConsoleColor.Yellow;
                                Console.WriteLine("\n💬 [Player " + senderId + "]: " + msg);
                            }
                            Console.ResetColor();
                        }
                        else if (id == 3002) // S_BUILD_TURRET
                        {
                            bool success = BitConverter.ToBoolean(buffer, offset + 4);
                            uint builderId = BitConverter.ToUInt32(buffer, offset + 5);
                            int bx = BitConverter.ToInt32(buffer, offset + 9);
                            int by = BitConverter.ToInt32(buffer, offset + 13);
                            int btype = BitConverter.ToInt32(buffer, offset + 17);
                            int remGold = BitConverter.ToInt32(buffer, offset + 21);
                            Console.ForegroundColor = ConsoleColor.Green;
                            Console.WriteLine("\n🏰 [타워 동기화] Player " + builderId + "님이 (" + bx + ", " + by + ")에 " + btype + "번 타워를 건설했습니다! (잔여 골드: " + remGold + " G)");
                            Console.ResetColor();
                        }

                        offset += size;
                    }
                }
                Thread.Sleep(50);
            }
            catch { break; }
        }
    }

    public void Stop() { _running = false; }
}
"@

if (-not ([System.Management.Automation.PSTypeName]'PacketReceiver').Type) {
    Add-Type -TypeDefinition $csharpCode
}

# 2. 서버 연결
$client = New-Object System.Net.Sockets.TcpClient
try {
    $client.Connect($ServerIP, $Port)
} catch {
    Write-Host "서버($($ServerIP):$($Port))에 연결할 수 없습니다. 서버가 켜져있는지 확인해주세요!" -ForegroundColor Red
    return
}

$stream = $client.GetStream()

Write-Host "========================================" -ForegroundColor Green
Write-Host "  SK Defense 멀티플레이어 클라이언트 연결 성공! " -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

# 3. 실시간 백그라운드 수신 가동!
$receiver = New-Object PacketReceiver
$receiver.Start($stream)

# 4. 메인 입력 루프
while ($true) {
    Write-Host ""
    Write-Host "--- [메뉴] ---" -ForegroundColor DarkGray
    Write-Host "1. 로그인 (C_LOGIN)"
    Write-Host "2. 타워 건설 (C_BUILD_TURRET)"
    Write-Host "3. 채팅 전송 (C_CHAT)"
    Write-Host "4. 종료"
    $choice = Read-Host "선택 (1~4)"

    if ($choice -eq "1") {
        $name = Read-Host "닉네임 입력"
        $loginPkt = New-Object byte[] 36
        [System.BitConverter]::GetBytes([uint16]36).CopyTo($loginPkt, 0)
        [System.BitConverter]::GetBytes([uint16]1001).CopyTo($loginPkt, 2)
        $nb = [System.Text.Encoding]::ASCII.GetBytes($name)
        [System.Array]::Copy($nb, 0, $loginPkt, 4, [Math]::Min($nb.Length, 31))
        $stream.Write($loginPkt, 0, 36)
        $stream.Flush()
    }
    elseif ($choice -eq "2") {
        $posX = [int](Read-Host "X 좌표")
        $posY = [int](Read-Host "Y 좌표")
        $type = [int](Read-Host "타워 종류(0:일반, 1:매직, 2:레어)")
        
        $buildPkt = New-Object byte[] 16
        [System.BitConverter]::GetBytes([uint16]16).CopyTo($buildPkt, 0)
        [System.BitConverter]::GetBytes([uint16]3001).CopyTo($buildPkt, 2)
        [System.BitConverter]::GetBytes([int32]$posX).CopyTo($buildPkt, 4)
        [System.BitConverter]::GetBytes([int32]$posY).CopyTo($buildPkt, 8)
        [System.BitConverter]::GetBytes([int32]$type).CopyTo($buildPkt, 12)
        $stream.Write($buildPkt, 0, 16)
        $stream.Flush()
    }
    elseif ($choice -eq "3") {
        $msg = Read-Host "채팅 메시지"
        $chatPkt = New-Object byte[] 136
        [System.BitConverter]::GetBytes([uint16]136).CopyTo($chatPkt, 0)
        [System.BitConverter]::GetBytes([uint16]2001).CopyTo($chatPkt, 2)
        $mb = [System.Text.Encoding]::ASCII.GetBytes($msg)
        [System.Array]::Copy($mb, 0, $chatPkt, 4, [Math]::Min($mb.Length, 127))
        $stream.Write($chatPkt, 0, 136)
        $stream.Flush()
    }
    elseif ($choice -eq "4") {
        break
    }
}

$receiver.Stop()
$client.Close()
Write-Host "연결을 종료했습니다." -ForegroundColor Red
