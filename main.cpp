#include <chrono>
#include <iostream>
#include <map>

#include "avl_tree.h"

int main()
{
    avl::avl_tree<int> tree;
    std::map<int, int> map;

    constexpr int size = 1'000'000;
    auto start_tree = std::chrono::steady_clock::now();
    for (int i = 0; i < size; ++i)
        tree.insert({i, i});

    for (int i = 0; i < size; ++i)
        tree.erase(tree.begin());
    auto end_tree = std::chrono::steady_clock::now();

    auto start_map = std::chrono::steady_clock::now();
    for (int i = 0; i < size; ++i)
        map.insert({i, 1});

    for (int i = 0; i < size; ++i)
        map.erase(map.begin());
    auto end_map = std::chrono::steady_clock::now();

    auto elapsed_tree_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_tree - start_tree);
    auto elapsed_map_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_map - start_map);
    std::cout << "tree: " << elapsed_tree_ms.count() << "\n";
    std::cout << "map: " << elapsed_map_ms.count() << "\n";

    return 0;
}
