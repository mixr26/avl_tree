#include <iostream>

#include "avl_tree.h"


int main()
{
    avl::avl_tree<int> tree;
    tree.insert(6);
    tree.insert(8);
    auto it = tree.find(9);

    return 0;
}
