#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include "Splay.h"
#include "sm.h"

#define L1_CACHE_LINE 128
#define LOOP 65536 //循环次数
#define sm_num 64

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
    return (addr >> l1_cache_line_log2) & ((1 << (l1_cache_n_sets - 1)) - 1);
}

int main() { //同时回顾下命名空间的使用->C++第八讲
    //coalesced_address在头文件中已经声明，其作用相当于seq
    std::vector<sm> sms(sm_num);
    //std::vector<std::pair<long long, int> > coalesced_addresses;
    //std::vector<std::unordered_map<long long, int>> hash_map(1024);//地址到时间的映射
    //std::vector<std::vector<long long>> hit(1024, std::vector<long long>(1));//是否命中序列
    //std::vector<std::vector<long long>> seq(1024, std::vector<long long>(1));//序列
    //std::vector<std::vector<long long>> stkDis(1024, std::vector<long long>(1)); //栈的距离
    //std::vector<Tree*>nodes(1024);
    //FILE *fp = fopen("E:\\CUDA\\splay\\test2.txt", "r");
    //char line[105];
    long long addr;
    int sector;
    int time = 0;//从零开始记录时间
    //int type = 1;//set大小 type = 1为全相联？
    //int success = 0;//成功次数
//    if (fgets(line, 104, fp) != nullptr)
//    {
//        type = std::stoi(line);
//    } else {
//        printf("type error\n");
//        return 0;
//    }

    string name = "dwt2d";
//    string trace_path = "E:\\Science_research\\CUDA\\splay";
    string trace_path = "E:\\Science_research\\Flex-GPU-2022_12_22\\flex-gpusim\\benchmarks";
    string des = "ans-" + name + "-mem-1.txt";
    int r;
    int block_cnt = 0;
    int active_sm = 0;
    while (true) {
        r = Tree::read_mem(trace_path + "\\" + name + "\\traces", 128, block_cnt, 2,
                           sms[block_cnt % (sm_num)].coalesced_addresses); //地址聚合
        if (!r) {
            break; //判断文件是否正常打开
        }
        block_cnt++;
    }
    active_sm = sm_num <= (block_cnt  + 1) ? sm_num : (block_cnt + 1);
    //Tree::read_mem(trace_path + "\\" + name + "\\traces", 128, -1, 1,
    //               coalesced_addresses);
    std::ofstream out;
    //out.open("E:\\CUDA\\splay\\ans\\" + des ,std::ios_base::out);
    out.open("E:\\Science_research4\\splay\\ans\\" + des ,std::ios::out);
    std::cout << out.is_open() << std::endl;
    printf("type: %d\n", l1_cache_n_sets);
    //while (fgets(line, 104, fp) != nullptr) { //
    for (int i = 0; i < active_sm; i++) { // 这样按顺序读取并拼接是有问题的
        time = 0;
        for (auto it: sms[i].coalesced_addresses) {
            time++;
            long long rank = get_set_id(addr); //分 set
            //printf("%lld\n",rank);
            //num = std::stoi(line);
            addr = it.first;
            sector = it.second;//输入的第二个数
            //seq[rank].push_back(addr);
            printf("sm: %d addr: %lld sector:%d ", i, addr, sector);
            if (sms[i].hash_map[rank].find(addr) == sms[i].hash_map[rank].end()) {
                sms[i].hit[rank].push_back(false);//未命中
                sms[i].nodes[rank]= Tree::Insert(time, sms[i].nodes[rank], sector);//最多添加一个sector
                sms[i].hash_map[rank][addr] = time;
                //sms[i].stkDis[rank].push_back(INT_MAX);//初始化为无穷大

                printf("MISS\n");
                continue;
            }  //如果没有
            int now = sms[i].hash_map[rank][addr];
            printf( " now: %d " , now);
            sms[i].nodes[rank] = Tree::Splay(now, sms[i].nodes[rank]);//查找对应节点
            if (sms[i].nodes[rank] == nullptr) {
                printf("splay find error");//1079109002@qq.com 21371055
                return 0;
            }
            if (sms[i].nodes[rank]->right == nullptr) {
                //sms[i].stkDis[rank].push_back(0);//虽然距离为0，但未必命中
                if (sms[i].nodes[rank]->sector[sector] == 1) {
                    sms[i].hit[rank].push_back(true);
                    sms[i].success++;
                    printf("addr: %lld HIT\n", addr);
                } else {
                    sms[i].hit[rank].push_back(false);
                    printf("SECTOR_MISS\n");
                    sms[i].nodes[rank]->sector[sector] = 1; //一样原理 //看本节点命中情况
                }
            } else {
                if (l1_cache_n_sets * sms[i].nodes[rank]->right->size < L1_CACHE_LINE) { //cache line 被划分成type个set
                    if (sms[i].nodes[rank]->sector[sector] == 1) {
                        sms[i].hit[rank].push_back(true);
                        sms[i].success++;
                        printf("id: %d addr: %lld HIT\n", i, addr);
                    } else {
                        sms[i].hit[rank].push_back(false);
                        printf("SECTOR_MISS\n");
                        sms[i].nodes[rank]->sector[sector] = 1;
                    }

                } else {
                    for (int j = 0; j < SECTOR_SIZE; ++j) {
                        sms[i].nodes[rank]->sector[sector] = 0; // 刷数组 注意是按 set 分
                    }

                    sms[i].hit[rank].push_back(false);
                    sms[i].nodes[rank]->sector[sector] = 1;
                    printf("MISS\n");
                }
                //sms[i].stkDis[rank].push_back(sms[i].nodes[rank]->right->size);
            }
//        nodes[rank] = Tree::Delete(now, nodes[rank]);//删除原有的节点主要是二叉树排序树的性质
//        nodes[rank] = Tree::Insert(time, nodes[rank], sector);
            sms[i].nodes[rank] = Tree::Modify(now, sms[i].nodes[rank], time);
            sms[i].hash_map[rank][addr] = time;//输入的是访存数据
        }
    }

    //fclose(fp);
    for (int i = 0; i < active_sm; i++) {
        for (int j = 0; j < l1_cache_n_sets; j++) {
//        printf ("set:%d\n", i);
//        for (int j = 1; j < hit.size(); j++) {
//            //printf("set:%d j:%d seq:%lld stkDis:%lld hit:%lld\n", i, j, seq[i][j], stkDis[i][j], hit[i][j]);
//            printf("set:%d j:%d addr:%lld sector:%d stkDis:%lld hit:%lld\n", i, j, coalesced_addresses[j].first,
//                   coalesced_addresses[j].second, stkDis[i][j],
//                   hit[i][j]);
//            //if (hit[i][j]) success++;
//        }
            Tree::freetree(sms[i].nodes[j]);
        }
    }
    unsigned final_hit = 0;
    unsigned total_hit = 0;
    for (int i = 0; i < active_sm; i++) {
        for (int j = 0; j < l1_cache_n_sets; j++) {
            total_hit += (sms[i].hit[j].size() - 1); //从1开始计数
        }
        final_hit += sms[i].success;
    }
    double hit_rate = (double) final_hit / total_hit;
    printf("%.2f\n", hit_rate);
    printf("%u %u\n", final_hit, total_hit);
    out << hit_rate << std::endl;
        //    printf("%lf\n", (double) success / (hit[0].size() - 1));//注意匹配 //只有一个set
        //    out << std::to_string((double) success / (hit[0].size() - 1)) << std::endl;
        //    printf("%llu %d", hit[0].size() - 1, time);
    return 0; //仅仅考虑l1
}
//第二阶段：多个set
//地址%4->4棵树 取模运算
//我的理解是每个set独立计算序列，重用距离和命中率
