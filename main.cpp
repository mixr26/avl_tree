#include <iostream>

#include "avl_tree.h"


int main()
{
    avl::avl_tree<int> tree;
    tree.insert({6, 7});
    tree.insert({8, 9});

    tree[2] = 5;
    std::cout << tree[2] << std::endl;

    return 0;
}
