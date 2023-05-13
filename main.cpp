#include <iostream>

#include "avl_tree.h"


int main()
{
    avl::avl_tree<int> tree;
    tree.insert(5);
    tree.insert(8);
    tree.insert(3);
    tree.insert(4);
    tree.insert(2);
    tree.insert(1);
    tree.dump();
    return 0;
}
