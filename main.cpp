#include <iostream>
#include <map>
//#include <unordered_map>
#include <vector>
#include "Splay.h"
#include "sm.h"
//#include <semaphore>
//#include <chrono>
#include <thread>

#define L1_CACHE_ASSOC 64
#define LOOP 65536 //循环次数
#define sm_num 68 //下一步，分block执行 68 个sm

extern unsigned sector_num;
unsigned int LOGB2(unsigned int v) {
    unsigned int shift;
    unsigned int r;

    r = 0;

    shift = ((v & 0xFFFF0000) != 0) << 4;
    v >>= shift;
    r |= shift;
    shift = ((v & 0xFF00) != 0) << 3;
    v >>= shift;
    r |= shift;
    shift = ((v & 0xF0) != 0) << 2;
    v >>= shift;
    r |= shift;
    shift = ((v & 0xC) != 0) << 1;
    v >>= shift;
    r |= shift;
    shift = ((v & 0x2) != 0) << 0;
    v >>= shift;
    r |= shift;

    return r;
}

unsigned l1_cache_line_log2 = LOGB2(L1_CACHE_LINE);

unsigned get_set_id(unsigned addr) {
    return (addr >> l1_cache_line_log2) & (l1_cache_n_sets - 1); //l1_cache_n_sets log2
}

void execute_sms(int i, sm* sm1) {
    long long addr;
    int sector;
    int time = 0;
    for (auto it: sm1->coalesced_addresses) {
        time++;
        //long long rank = get_set_id(addr); //分 set
        addr = it.first;
        sector = it.second;//输入的第二个数
        long long rank = get_set_id(addr); //分 set
        // std::cout << "rank " << rank << std::endl;
        //seq[rank].push_back(addr);
        // printf("sm: %d addr: %lld sector:%d \n", i, addr, sector);
        if (sm1->hash_map[rank].find(addr) == sm1->hash_map[rank].end()) {
            sm1->hit[rank].push_back(false);//未命中
            // printf("miss addr: %lld\n", addr);
            sm1->nodes[rank]= Tree::Insert(time, sm1->nodes[rank], sector);//最多添加一个sector
            sm1->hash_map[rank][addr] = time;

            // printf("MISS\n");
            continue;
        }  //如果没有
        int now = sm1->hash_map[rank][addr];
        // printf( " now: %d " , now);
        sm1->nodes[rank] = Tree::Splay(now, sm1->nodes[rank]);//查找对应节点
        if (sm1->nodes[rank] == nullptr) {
            // printf("splay find error");//1079109002@qq.com 21371055
            return;
        }
        if (sm1->nodes[rank]->right == nullptr) {
            //sm1->stkDis[rank].push_back(0);//虽然距离为0，但未必命中
            if (sm1->nodes[rank]->sector[sector] == 1) {
                sm1->hit[rank].push_back(true);
                sm1->success++;
                // printf("HIT\n");
            } else {
                sm1->hit[rank].push_back(false);
                // printf("SECTOR_MISS\n");
                sm1->nodes[rank]->sector[sector] = 1; //一样原理 //看本节点命中情况
            }
        } else {
            if (sm1->nodes[rank]->right->size < L1_CACHE_ASSOC) { //cache line 被划分成type个set
                if (sm1->nodes[rank]->sector[sector] == 1) {
                    sm1->hit[rank].push_back(true);
                    sm1->success++;
                    // printf("HIT\n");
                } else {
                    // if (sm1->nodes[rank]->left != nullptr) {
                    //    Tree::freetree(sm1->nodes[rank]->left);
                    // }
                    sm1->hit[rank].push_back(false);
                    // printf("SECTOR_MISS\n");
                    sm1->nodes[rank]->sector[sector] = 1;
                }

            } else {
                for (int j = 0; j < sector_num; ++j) {
                    sm1->nodes[rank]->sector[sector] = 0; // 刷数组 注意是按 set 分
                }

                sm1->hit[rank].push_back(false);
                sm1->nodes[rank]->sector[sector] = 1;
                // printf("MISS\n");
            }
            //sm1->stkDis[rank].push_back(sm1->nodes[rank]->right->size);
        }
        sm1->nodes[rank] = Tree::Modify(now, sm1->nodes[rank], time);
        sm1->hash_map[rank][addr] = time;//输入的是访存数据
    }
}

void sm_cycle(int active_sm, std::vector<sm*> sms) {
    std::vector<std::thread> threads;
    for (int i = 0; i < active_sm; i++) { // 这样按顺序读取并拼接是有问题的
        threads.emplace_back(std::thread(execute_sms, i, sms[i]));
    }
    for (int i = 0; i < active_sm; i++) {
        threads[i].join();
    }
}


int main(int argc, char *argv[]) { //同时回顾下命名空间的使用->C++第八讲
    std::vector<sm*> sms(sm_num);
    for (int i = 0; i < sm_num; i++) {
        sms[i] = new sm();
    }
    long long addr;
    int sector;
    int time = 0;//从零开始记录时间

    string name = argv[1];
    //    string trace_path = "E:\\Science_research\\Flex-GPU-2022_12_22\\flex-gpusim\\benchmarks";
    //    string trace_path = "E:\\Science_research\\Flex-GPU-2022_12_22\\flex-gpusim\\benchmarks";
    string trace_path = "F:\\traces";
    // string des = "ans-" + name + "-mem-1.txt";
    int r;
    int block_cnt = 0;
    int active_sm = 0;
    printf("%d\n", sector_num);
    while (true) {
        r = Tree::read_mem(trace_path + "/" + name + "/traces", 128, block_cnt, std::stoi(argv[2]),
                           sms[block_cnt % (sm_num)]->coalesced_addresses); //地址聚合
        if (!r) {
            break; //判断文件是否正常打开
        }
        block_cnt++;
    }
    active_sm = sm_num <= (block_cnt + 1) ? sm_num : (block_cnt + 1);
    // std::ofstream out;
    // out.open("E:\\Science_research4\\splay\\ans\\" + des ,std::ios::out);
    // std::cout << out.is_open() << std::endl;
    // printf("type: %d\n", l1_cache_n_sets);
    std::thread t1(sm_cycle, active_sm, std::ref(sms));
    t1.join();
    printf("here\n");

    //fclose(fp);
    for (int i = 0; i < active_sm; i++) {
        for (int j = 0; j < l1_cache_n_sets; j++) {

            Tree::freetree(sms[i]->nodes[j]);
        }
    }
    unsigned final_hit = 0;
    unsigned total_hit = 0;
    for (int i = 0; i < active_sm; i++) {
        for (int j = 0; j < l1_cache_n_sets; j++) {
            total_hit += (sms[i]->hit[j].size() - 1); //从1开始计数
        }
        final_hit += sms[i]->success;
    }
    for (int i = 0; i < sm_num; i++) {
        delete(sms[i]);
    }

    double hit_rate = (double) final_hit / total_hit;
    printf("benchmark: %s kernel: %s hitrate: %.2f\n", argv[1], argv[2], hit_rate);
    printf("%u %u\n", final_hit, total_hit);
    // out << hit_rate << std::endl;
    return 0; //仅仅考虑l1
}
//第二阶段：多个set
//地址%4->4棵树 取模运算
//我的理解是每个set独立计算序列，重用距离和命中率
