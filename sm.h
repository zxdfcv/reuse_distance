//
// Created by 10791 on 2023-07-02.
//

#ifndef SPLAY_SM_H
#define SPLAY_SM_H
#define l1_cache_n_sets 4


#include <vector>
#include <unordered_map>
#include "Splay.h"

class sm {
public:
    sm() : hash_map(1024), hit(1024, std::vector<long long>(1)), nodes
            (l1_cache_n_sets),
           success(0) {

    }
    std::vector<std::pair<long long, int> > coalesced_addresses;
    std::vector<std::unordered_map<long long, int>> hash_map;//地址到时间的映射
    std::vector<std::vector<long long>> hit;//是否命中
    std::vector<Tree*> nodes;
    unsigned success;
};


#endif //SPLAY_SM_H
