#include <iostream>

#include "avl_tree.h"


int main()
{
    avl::avl_tree<int> tree;
    tree.insert(6);
    tree.insert(8);
    tree.insert(2);
    tree.insert(1);
    tree.insert(4);
    tree.insert(5);
    tree.dump();

//    auto it = tree.end();
//    --it;
//    for (; it != tree.begin(); --it) {
//        std::cout << it->first << std::endl;
//    }
    return 0;
}
