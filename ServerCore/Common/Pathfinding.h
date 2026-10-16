#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace SKDefense
{
struct GridPoint
{
    int x = 0;
    int y = 0;
    bool operator==(const GridPoint&) const = default;
};

using Grid = std::vector<std::vector<int>>;

// Reuse one instance per room (under its lock), or per offline level.
// Returned coordinates own their storage; they never point into search scratch memory.
class AStar
{
public:
    struct Result
    {
        std::vector<GridPoint> path; // Includes the start and destination.
        std::vector<GridPoint> history;
    };

    static bool IsValidGrid(const Grid& grid)
    {
        if (grid.empty() || grid.front().empty()) return false;
        const size_t width = grid.front().size();
        if (width > 32767 || grid.size() > 32767 || grid.size() > 4096 / width) return false;
        return std::all_of(grid.begin(), grid.end(), [width](const auto& row) { return row.size() == width; });
    }

    static bool IsWalkable(const Grid& grid, GridPoint p)
    {
        return p.y >= 0 && p.y < static_cast<int>(grid.size()) &&
            p.x >= 0 && p.x < static_cast<int>(grid[p.y].size()) &&
            grid[p.y][p.x] != 1 && grid[p.y][p.x] != 2;
    }

    static bool CanStep(const Grid& grid, GridPoint from, GridPoint to)
    {
        if (!IsWalkable(grid, to)) return false;
        const int dx = to.x - from.x, dy = to.y - from.y;
        if (std::abs(dx) > 1 || std::abs(dy) > 1 || (dx == 0 && dy == 0)) return false;
        return dx == 0 || dy == 0 ||
            (IsWalkable(grid, {to.x, from.y}) && IsWalkable(grid, {from.x, to.y}));
    }

    Result FindPath(GridPoint start, GridPoint goal, const Grid& grid, bool recordHistory = false)
    {
        Result result;
        if (!IsValidGrid(grid) || !IsWalkable(grid, start) || !IsWalkable(grid, goal)) return result;
        const int width = static_cast<int>(grid.front().size());
        const size_t count = grid.size() * width;
        records.resize(count);
        // Generation stamps avoid clearing the whole grid on every search.
        if (++generation == 0)
        {
            for (auto& record : records) record.generation = 0;
            ++generation;
        }
        open.clear();
        open.reserve(count);
        const int startIndex = start.y * width + start.x;
        const int goalIndex = goal.y * width + goal.x;
        Record& first = Touch(startIndex);
        first.g = 0.0f;
        Push({startIndex, 0.0f, Heuristic(start, goal)});

        while (!open.empty())
        {
            std::pop_heap(open.begin(), open.end(), Compare{});
            const Entry entry = open.back();
            open.pop_back();
            Record& current = records[entry.index];
            // A decrease in cost adds a new heap entry; ignore old entries lazily.
            if (current.closed || entry.g != current.g) continue;
            current.closed = true;
            const GridPoint point{entry.index % width, entry.index / width};
            if (recordHistory) result.history.push_back(point);
            if (entry.index == goalIndex)
            {
                for (int index = goalIndex; index != -1; index = records[index].parent)
                    result.path.push_back({index % width, index / width});
                std::reverse(result.path.begin(), result.path.end());
                return result;
            }

            for (const auto& direction : directions)
            {
                const GridPoint next{point.x + direction.x, point.y + direction.y};
                if (!CanStep(grid, point, next)) continue;
                const int index = next.y * width + next.x;
                Record& neighbor = Touch(index);
                const float cost = current.g + ((direction.x != 0 && direction.y != 0) ? diagonalCost : 1.0f);
                if (neighbor.closed || cost >= neighbor.g) continue;
                neighbor.g = cost;
                neighbor.parent = entry.index;
                Push({index, cost, cost + Heuristic(next, goal)});
            }
        }
        return result;
    }

    // One flood from the goal validates all spawn points and live monsters in O(V+E).
    // It uses exactly the same obstacle/corner rules as A*.
    static std::vector<uint8_t> ReachableFrom(GridPoint goal, const Grid& grid)
    {
        if (!IsValidGrid(grid) || !IsWalkable(grid, goal)) return {};
        const int width = static_cast<int>(grid.front().size());
        std::vector<uint8_t> reachable(grid.size() * width, 0);
        std::vector<GridPoint> queue;
        queue.reserve(reachable.size());
        queue.push_back(goal);
        reachable[goal.y * width + goal.x] = 1;
        for (size_t head = 0; head < queue.size(); ++head)
        {
            const GridPoint point = queue[head];
            for (const auto& direction : directions)
            {
                const GridPoint next{point.x + direction.x, point.y + direction.y};
                if (!CanStep(grid, point, next)) continue;
                const int index = next.y * width + next.x;
                if (reachable[index]) continue;
                reachable[index] = 1;
                queue.push_back(next);
            }
        }
        return reachable;
    }

private:
    static constexpr float diagonalCost = 1.41421356237f;
    static constexpr GridPoint directions[] = {{0,1}, {0,-1}, {1,0}, {-1,0}, {1,1}, {1,-1}, {-1,1}, {-1,-1}};
    struct Record
    {
        uint32_t generation = 0;
        float g = std::numeric_limits<float>::infinity();
        int parent = -1;
        bool closed = false;
    };
    struct Entry { int index; float g; float f; };
    struct Compare
    {
        bool operator()(const Entry& a, const Entry& b) const
        {
            if (a.f != b.f) return a.f > b.f;
            if (a.g != b.g) return a.g < b.g; // Prefer nearer the goal on ties.
            return a.index > b.index;
        }
    };
    static float Heuristic(GridPoint a, GridPoint b)
    {
        const int dx = std::abs(a.x - b.x), dy = std::abs(a.y - b.y);
        return static_cast<float>((std::max)(dx, dy)) + (diagonalCost - 1.0f) * (std::min)(dx, dy);
    }
    Record& Touch(int index)
    {
        Record& record = records[index];
        if (record.generation != generation)
        {
            record = Record{};
            record.generation = generation;
        }
        return record;
    }
    void Push(Entry entry)
    {
        open.push_back(entry);
        std::push_heap(open.begin(), open.end(), Compare{});
    }
    uint32_t generation = 0;
    std::vector<Record> records;
    std::vector<Entry> open;
};
}
