#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include "Splay.h"

#define CACHE_LINE 128
#define LOOP 65536 //循环次数


int main() { //同时回顾下命名空间的使用->C++第八讲
    //coalesced_address在头文件中已经声明，其作用相当于seq
    std::vector<std::pair<long long, int> > coalesced_address;
    std::vector<std::unordered_map<long long, int>> hash_map(1024);//地址到时间的映射
    std::vector<std::vector<long long>> hit(1024, std::vector<long long>(1));//是否命中
    //std::vector<std::vector<long long>> seq(1024, std::vector<long long>(1));//序列
    std::vector<std::vector<long long>> stkDis(1024, std::vector<long long>(1));
    std::vector<Tree*>nodes(1024);
    //FILE *fp = fopen("E:\\CUDA\\splay\\test2.txt", "r");
    char line[105];
    long long addr;
    int sector;
    int time = 0;//从零开始记录时间
    int type = 1;//set大小
    int success = 0;//成功次数
//    if (fgets(line, 104, fp) != nullptr)
//    {
//        type = std::stoi(line);
//    } else {
//        printf("type error\n");
//        return 0;
//    }

    Tree::read_mem("E:\\CUDA\\splay\\backprop4096\\traces", 128, -1, 1,
                   coalesced_address);
    printf("type: %d\n", type);
    //while (fgets(line, 104, fp) != nullptr) {
    for (auto it : coalesced_address) {
        time++;
        long long rank = addr % type;
        //num = std::stoi(line);
        addr = it.first;
        sector = it.second;//输入的第二个数
        //seq[rank].push_back(addr);
        printf("addr:%lld sector:%d ", addr, sector);
        if (hash_map[rank].find(addr) == hash_map[rank].end()) {
            hit[rank].push_back(false);//未命中
            nodes[rank] = Tree::Insert(time, nodes[rank], sector);//最多添加一个sector
            hash_map[rank][addr] = time;
            stkDis[rank].push_back(INT_MAX);//初始化为无穷大

            printf("MISS\n");
            continue;
        }  //如果没有
        int now = hash_map[rank][addr];
        nodes[rank] = Tree::Splay(now, nodes[rank]);//查找对应节点
        if (nodes[rank] == nullptr) {
            printf("splay find error");
            return 0;
        }
        if (nodes[rank]->right == nullptr) {
            stkDis[rank].push_back(0);//虽然距离为0，但未必命中
            if (nodes[rank]->sector[sector] == 1)
            {
                hit[rank].push_back(true);
                success++;
                printf("HIT\n");
            } else {
                hit[rank].push_back(false);
                printf("SECTOR_MISS\n");
                nodes[rank]->sector[sector] = 1; //一样原理 //看本节点命中情况
            }
        } else {
            if (type * nodes[rank]->right->size < CACHE_LINE) { //cache line 被划分成type个set
                if (nodes[rank]->sector[sector] == 1)
                {
                    hit[rank].push_back(true);
                    success++;
                    printf("HIT\n");
                } else {
                    hit[rank].push_back(false);
                    printf("SECTOR_MISS\n");
                    nodes[rank]->sector[sector] = 1;
                }

            } else {
                hit[rank].push_back(false);
                nodes[rank]->sector[sector] = 1;
                printf("MISS\n");
            }
            stkDis[rank].push_back(nodes[rank]->right->size);
        }
//        nodes[rank] = Tree::Delete(now, nodes[rank]);//删除原有的节点主要是二叉树排序树的性质
//        nodes[rank] = Tree::Insert(time, nodes[rank], sector);
        nodes[rank] = Tree::Modify(now, nodes[rank], time);
        hash_map[rank][addr] = time;//输入的是访存数据
    }

    //fclose(fp);
    for (int i = 0; i < type; i++) {
//        printf ("set:%d\n", i);
//        for (int j = 1; j < hit.size(); j++) {
//            //printf("set:%d j:%d seq:%lld stkDis:%lld hit:%lld\n", i, j, seq[i][j], stkDis[i][j], hit[i][j]);
//            printf("set:%d j:%d addr:%lld sector:%d stkDis:%lld hit:%lld\n", i, j, coalesced_address[j].first,
//                   coalesced_address[j].second, stkDis[i][j],
//                   hit[i][j]);
//            //if (hit[i][j]) success++;
//        }
        Tree::freetree(nodes[i]);
    }
    printf("%lf\n", (double) success / (hit[0].size() - 1));//注意匹配 //只有一个set
    printf("%d %d", hit[0].size() - 1, time);
    return 0;
}
//第二阶段：多个set
//地址%4->4棵树 取模运算
//我的理解是每个set独立计算序列，重用距离和命中率
