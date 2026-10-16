#include "Common/Pathfinding.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <queue>
#include <random>
#include <stdexcept>

using namespace SKDefense;

static void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

// Independent Dijkstra oracle, with its own bounds and corner checks.
static float ShortestDistance(const Grid& grid, GridPoint start, GridPoint goal)
{
    const int width = static_cast<int>(grid.front().size());
    const int height = static_cast<int>(grid.size());
    const auto free = [&](int x, int y) {
        return x >= 0 && x < width && y >= 0 && y < height && grid[y][x] != 1 && grid[y][x] != 2;
    };
    const float infinity = std::numeric_limits<float>::infinity();
    if (!free(start.x, start.y) || !free(goal.x, goal.y)) return infinity;
    std::vector<float> distances(width * height, infinity);
    using Entry = std::pair<float, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
    const int first = start.y * width + start.x;
    distances[first] = 0;
    queue.emplace(0.0f, first);
    while (!queue.empty())
    {
        const auto [cost, index] = queue.top();
        queue.pop();
        if (cost != distances[index]) continue;
        const int x = index % width, y = index / width;
        if (GridPoint{x, y} == goal) return cost;
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if ((dx == 0 && dy == 0) || !free(x + dx, y + dy)) continue;
                if (dx != 0 && dy != 0 && (!free(x + dx, y) || !free(x, y + dy))) continue;
                const float nextCost = cost + ((dx && dy) ? std::sqrt(2.0f) : 1.0f);
                const int next = (y + dy) * width + x + dx;
                if (nextCost >= distances[next]) continue;
                distances[next] = nextCost;
                queue.emplace(nextCost, next);
            }
        }
    }
    return infinity;
}

static float PathCost(const std::vector<GridPoint>& path, const Grid& grid)
{
    float cost = 0;
    for (size_t i = 1; i < path.size(); ++i)
    {
        Check(AStar::CanStep(grid, path[i - 1], path[i]), "Invalid path step / diagonal corner crossing");
        cost += (path[i - 1].x != path[i].x && path[i - 1].y != path[i].y) ? std::sqrt(2.0f) : 1.0f;
    }
    return cost;
}

int main()
{
    try
    {
        AStar astar;
        Check(astar.FindPath({0,0}, {0,0}, {}).path.empty(), "Empty grid");
        Check(astar.FindPath({0,0}, {0,0}, {{0}, {0,0}}).path.empty(), "Ragged grid");
        Check(astar.FindPath({-1,0}, {0,0}, {{0}}).path.empty(), "Invalid start");
        Check(astar.FindPath({0,0}, {2,0}, {{0}}).path.empty(), "Invalid goal");
        Check(astar.FindPath({0,0}, {0,0}, {{2}}).path.empty(), "Blocked start/goal");
        Check(astar.FindPath({0,0}, {1,1}, {{0,1},{2,0}}).path.empty(), "No corner cutting");
        Check(astar.FindPath({0,0}, {0,0}, {{3}}).path.size() == 1, "Start equals goal");
        Check(!AStar::IsValidGrid(Grid(65, std::vector<int>(65))), "Map size cap");

        std::mt19937 random(20261014);
        size_t comparisons = 0;
        const auto began = std::chrono::steady_clock::now();
        for (int iteration = 0; iteration < 400; ++iteration)
        {
            const int width = 4 + random() % 30, height = 4 + random() % 20;
            Grid grid(height, std::vector<int>(width));
            for (auto& row : grid)
                for (auto& cell : row) cell = (random() % 100 < 28) ? 1 + random() % 2 : 0;
            const GridPoint goal{width - 1, height - 1};
            grid[goal.y][goal.x] = 3;
            const auto reachable = AStar::ReachableFrom(goal, grid);
            for (int query = 0; query < 20; ++query)
            {
                const GridPoint start{static_cast<int>(random() % width), static_cast<int>(random() % height)};
                const auto result = astar.FindPath(start, goal, grid, query % 2 == 0);
                const float distance = ShortestDistance(grid, start, goal);
                Check(result.path.empty() == !std::isfinite(distance), "Reachability differs from Dijkstra");
                Check(!result.path.empty() == (reachable[start.y * width + start.x] != 0), "Flood/A* disagree");
                if (!result.path.empty())
                {
                    Check(result.path.front() == start && result.path.back() == goal, "Wrong endpoints");
                    Check(std::abs(PathCost(result.path, grid) - distance) < 0.0002f, "A* path is not optimal");
                }
                if (query % 2 != 0) Check(result.history.empty(), "Disabled history allocates entries");
                ++comparisons;
            }
        }
        // Searches reuse scratch memory, while old returned paths remain valid.
        Grid grid(20, std::vector<int>(50));
        const auto saved = astar.FindPath({0,0}, {49,19}, grid);
        const auto original = saved.path;
        grid[0][1] = 2;
        Check(!astar.FindPath({0,0}, {49,19}, grid).path.empty(), "Dynamic obstacle reroute");
        Check(saved.path == original, "Returned path lifetime");
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - began).count();
        std::cout << "PASS: " << comparisons << " A*/Dijkstra comparisons, corner rules, flood validation, invalid grids and reusable storage (" << elapsed << " ms)\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
