param([string]$ServerExecutable = (Join-Path $PSScriptRoot '..\Bin\x64\Debug\SK_Defense_Server.exe'))
$ErrorActionPreference = 'Stop'
$taskRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$testDirectory = Join-Path $taskRoot 'Intermediate\PathfindingTests'
New-Item -ItemType Directory -Force -Path $testDirectory | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;

public sealed class NavigationTestClient : IDisposable
{
    public TcpClient Client = new TcpClient();
    public NetworkStream Stream;
    public List<byte[]> Seen = new List<byte[]>();
    public NavigationTestClient()
    {
        Client.Connect("127.0.0.1", 7777);
        Stream = Client.GetStream();
        Stream.ReadTimeout = 40000;
    }
    public void Send(ushort id, byte[] payload)
    {
        byte[] packet = new byte[4 + payload.Length];
        Buffer.BlockCopy(BitConverter.GetBytes((ushort)packet.Length), 0, packet, 0, 2);
        Buffer.BlockCopy(BitConverter.GetBytes(id), 0, packet, 2, 2);
        Buffer.BlockCopy(payload, 0, packet, 4, payload.Length);
        Stream.Write(packet, 0, packet.Length);
    }
    public byte[] Read()
    {
        byte[] header = Exact(4);
        int length = BitConverter.ToUInt16(header, 0);
        if (length < 4) throw new Exception("Invalid frame length");
        byte[] packet = new byte[length];
        Buffer.BlockCopy(header, 0, packet, 0, 4);
        byte[] body = Exact(length - 4);
        Buffer.BlockCopy(body, 0, packet, 4, body.Length);
        Seen.Add(packet);
        return packet;
    }
    byte[] Exact(int length)
    {
        byte[] result = new byte[length];
        int offset = 0;
        while (offset < length)
        {
            int received = Stream.Read(result, offset, length - offset);
            if (received == 0) throw new Exception("Unexpected disconnect");
            offset += received;
        }
        return result;
    }
    public byte[] Until(ushort id, int monsterId = -1, uint afterRevision = 0)
    {
        DateTime deadline = DateTime.UtcNow.AddSeconds(40);
        while (DateTime.UtcNow < deadline)
        {
            byte[] packet = Read();
            if (BitConverter.ToUInt16(packet, 2) != id) continue;
            if (monsterId >= 0 && BitConverter.ToInt32(packet, 4) != monsterId) continue;
            if (afterRevision > 0 && BitConverter.ToUInt32(packet, 8) <= afterRevision) continue;
            return packet;
        }
        throw new Exception("Timeout waiting for " + id);
    }
    public void Dispose() { Client.Close(); }
}

public static class ServerNavigationTest
{
    static void Check(bool condition, string message) { if (!condition) throw new Exception(message); }
    static byte[] Ints(params int[] values)
    {
        byte[] data = new byte[values.Length * 4];
        for (int i = 0; i < values.Length; ++i) Buffer.BlockCopy(BitConverter.GetBytes(values[i]), 0, data, i * 4, 4);
        return data;
    }
    static void Equal(byte[] a, byte[] b, string message)
    {
        Check(a.Length == b.Length, message);
        for (int i = 0; i < a.Length; ++i) Check(a[i] == b[i], message);
    }
    static byte[] Text(string text, int length)
    {
        byte[] data = new byte[length];
        byte[] bytes = Encoding.ASCII.GetBytes(text);
        Buffer.BlockCopy(bytes, 0, data, 0, Math.Min(bytes.Length, length - 1));
        return data;
    }
    static void Barrier(NavigationTestClient a, NavigationTestClient b, string text)
    {
        a.Send(2001, Text(text, 128));
        a.Until(2002); b.Until(2002);
    }
    static void Reject(NavigationTestClient client, int x, int y, int gold)
    {
        client.Send(3001, Ints(x, y, 0));
        byte[] packet = client.Until(3002);
        Check(packet[4] == 0 && BitConverter.ToInt32(packet, 21) == gold, "Rejected build changed gold");
    }
    static void ValidatePath(byte[] packet, char[][] map)
    {
        int count = BitConverter.ToUInt16(packet, 12), history = BitConverter.ToUInt16(packet, 14);
        Check(packet.Length == 16 + (count + history) * 4 && count > 0, "Path payload length");
        int previousX = -1, previousY = -1;
        for (int i = 0; i < count; ++i)
        {
            int x = BitConverter.ToInt16(packet, 16 + i * 4), y = BitConverter.ToInt16(packet, 18 + i * 4);
            Check(x >= 0 && x < map[0].Length && y >= 0 && y < map.Length && map[y][x] != '#' && map[y][x] != 'T', "Path crosses obstacle");
            if (i > 0)
            {
                Check(Math.Abs(x - previousX) <= 1 && Math.Abs(y - previousY) <= 1, "Non-adjacent path step");
                if (x != previousX && y != previousY)
                    Check(map[y][previousX] != '#' && map[y][previousX] != 'T' && map[previousY][x] != '#' && map[previousY][x] != 'T', "Corner crossing");
            }
            previousX = x; previousY = y;
        }
        Check(map[previousY][previousX] == 'D', "Path misses destination");
    }
    public static void Run(string mapPath)
    {
        char[][] map = Array.ConvertAll(File.ReadAllLines(mapPath), line => line.ToCharArray());
        using (var a = new NavigationTestClient())
        using (var b = new NavigationTestClient())
        {
            a.Send(1001, Text("PathTestA", 32)); a.Until(1002);
            b.Send(1001, Text("PathTestB", 32)); b.Until(1002);
            Reject(a, 10, 10, 100); // Requests before PLAYING are ignored/rejected.
            a.Send(1003, new byte[]{0}); b.Send(1003, new byte[]{0});
            a.Until(1004); b.Until(1004);
            // Truncated fixed packet must not be decoded or disconnect healthy peers.
            a.Send(3001, new byte[0]);
            Reject(a, -1, -1, 200); Reject(a, int.MaxValue, int.MaxValue, 200);
            Reject(a, 0, 0, 200); Reject(a, 1, 1, 200);
            a.Send(3001, Ints(2, 1, 0));
            Check(a.Until(3002)[4] == 1 && b.Until(3002)[4] == 1, "Valid turret rejected");
            Reject(a, 1, 3, 150); // Would enclose the spawn; must roll back.
            Reject(a, 2, 1, 150); // Overlap.
            a.Send(3003, Ints(2, 1, 3));
            Check(BitConverter.ToInt32(a.Until(3004), 17) == 25, "Server trusts forged sell tier"); b.Until(3004);

            Console.WriteLine("PASS: bounds, spawn protection, path blocking, rollback, overlap, sell tier and malformed packet");
            // Wait for the real 30s preparation timer; this also exercises ordered framing.
            byte[] spawnA = a.Until(4001), spawnB = b.Until(4001);
            Equal(spawnA, spawnB, "Spawn differs between clients");
            int monsterId = BitConverter.ToInt32(spawnA, 4);
            byte[] pathA = a.Until(4002, monsterId), pathB = b.Until(4002, monsterId);
            Equal(pathA, pathB, "Server paths differ between clients"); ValidatePath(pathA, map);
            uint revision = BitConverter.ToUInt32(pathA, 8);
            byte[] moveA = a.Until(4003, monsterId), moveB = b.Until(4003, monsterId);
            Equal(moveA, moveB, "Server moves differ between clients");
            Check(BitConverter.ToUInt16(moveA, 12) > 1, "Monster did not move");

            int buildX = BitConverter.ToInt16(pathA, 16 + 10 * 4), buildY = BitConverter.ToInt16(pathA, 18 + 10 * 4);
            a.Send(3001, Ints(buildX, buildY, 0));
            Check(a.Until(3002)[4] == 1 && b.Until(3002)[4] == 1, "Route obstacle rejected");
            for (int dy = 0; dy < 2; ++dy) for (int dx = 0; dx < 2; ++dx) map[buildY + dy][buildX + dx] = 'T';
            pathA = a.Until(4002, monsterId, revision); pathB = b.Until(4002, monsterId, revision);
            Equal(pathA, pathB, "Reroute differs"); ValidatePath(pathA, map);
            revision = BitConverter.ToUInt32(pathA, 8);
            a.Send(3003, Ints(buildX, buildY, 1)); a.Until(3004); b.Until(3004);
            for (int dy = 0; dy < 2; ++dy) for (int dx = 0; dx < 2; ++dx) map[buildY + dy][buildX + dx] = '.';
            pathA = a.Until(4002, monsterId, revision); pathB = b.Until(4002, monsterId, revision);
            Equal(pathA, pathB, "Sell reroute differs"); ValidatePath(pathA, map);
            a.Send(3011, Ints(monsterId, 10)); a.Send(3011, Ints(monsterId, 10));
            Equal(a.Until(3012, monsterId), b.Until(3012, monsterId), "Kill differs");
            Barrier(a, b, "kill-barrier");
            int kills = 0;
            foreach (var frame in a.Seen) if (BitConverter.ToUInt16(frame, 2) == 3012 && BitConverter.ToInt32(frame, 4) == monsterId) ++kills;
            Check(kills == 1, "Duplicate kill reward");
            Console.WriteLine("PASS: identical server paths/moves, build/sell reroute, revision changes and duplicate kill");

            a.Send(3006, Ints(10000));
            for (int i = 0; i < 7; ++i)
            {
                a.Send(3001, Ints(20 + i * 3, 2, 0));
                Check(a.Until(3002)[4] == 1 && b.Until(3002)[4] == 1, "Merge fixture build failed");
                Barrier(a, b, "merge-barrier");
            }
            var mergesA = a.Seen.FindAll(frame => BitConverter.ToUInt16(frame, 2) == 4004);
            var mergesB = b.Seen.FindAll(frame => BitConverter.ToUInt16(frame, 2) == 4004);
            Check(mergesA.Count > 0 && mergesA.Count == mergesB.Count, "Merge not synchronized");
            for (int i = 0; i < mergesA.Count; ++i) Equal(mergesA[i], mergesB[i], "Merge differs");
            byte[] merge = mergesA[mergesA.Count - 1];
            a.Send(3003, Ints(BitConverter.ToInt16(merge, 4), BitConverter.ToInt16(merge, 6), 1));
            Check(BitConverter.ToInt32(a.Until(3004), 17) == 75, "Merged turret tier not tracked by server"); b.Until(3004);
            Console.WriteLine("PASS: server merge synchronization and authoritative merged-tier refund");

            // Burst responses must retain packet storage and FIFO order during IOCP sends.
            for (int i = 0; i < 1000; ++i) a.Send(2001, Text("burst-" + i, 128));
            for (int i = 0; i < 1000; ++i)
            {
                byte[] chatA = a.Until(2002), chatB = b.Until(2002);
                Equal(chatA, chatB, "Broadcast burst differs");
                string actual = Encoding.ASCII.GetString(chatA, 8, 128).Split(new char[]{'\0'})[0];
                Check(actual == "burst-" + i, "Send queue ordering/corruption: expected burst-" + i + ", got " + actual);
            }
            Console.WriteLine("PASS: 1000 ordered broadcasts to two clients");
            a.Send(3009, Ints(99)); // Client arrival/damage reports must not affect server HP.
            byte[] arrivalA, arrivalB;
            do { arrivalA = a.Until(4003, monsterId + 1); } while (arrivalA[18] != 0);
            do { arrivalB = b.Until(4003, monsterId + 1); } while (arrivalB[18] != 0);
            Equal(arrivalA, arrivalB, "Arrival differs between clients");
            byte[] healthA = a.Until(3010), healthB = b.Until(3010);
            Equal(healthA, healthB, "Agit health differs");
            Check(BitConverter.ToInt32(healthA, 4) == 99, "Arrival damaged agit more than once / client damage trusted");
            Console.WriteLine("PASS: server arrival removes monster and damages agit exactly once");
        }
    }
}
'@

$serverProcess = Start-Process -FilePath ([IO.Path]::GetFullPath($ServerExecutable)) -WorkingDirectory $taskRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $testDirectory 'server-navigation.log') -RedirectStandardError (Join-Path $testDirectory 'server-navigation-error.log')
try {
    Start-Sleep -Milliseconds 800
    if ($serverProcess.HasExited) { throw 'Test server failed to start.' }
    [ServerNavigationTest]::Run((Join-Path $taskRoot 'Assets\SK_Defense_Map.txt'))
} finally {
    if (!$serverProcess.HasExited) { Stop-Process -Id $serverProcess.Id }
}
