//
// Created by 10791 on 2022/10/25.
//

#ifndef SPLAY_SPLAY_H
#define SPLAY_SPLAY_H
#define SECTOR_SIZE 32

#include <cstdio>
#include <cstdlib>
#include "algorithm"
#include <string>
#include <fstream>
#include <map>
#include <cstring>

using std::string;
typedef int T;

class Tree {
public:
    Tree *left, *right;
    T key;
    T size;   /* maintained to be the number of nodes rooted here */
    T sector[256];
    Tree() {
        this->left = nullptr;
        this->right = nullptr;
        this->key = -1;
        this->size = 1;
        memset(this->sector, 0, sizeof(this->sector)); //初始化
//        for (int i = 0; i <= 255; i++) {
//            this->sector[i] = 0;
//        }
    }

    Tree(int key) {
        this->left = nullptr;
        this->right = nullptr;
        this->key = key;
        this->size = 1;
        memset(this->sector, 0, sizeof(this->sector));
    }

    Tree(int key, Tree *l, Tree *r) {
        this->left = l;
        this->right = r;
        this->key = key;
        this->size = 1;
        memset(this->sector, 0, sizeof(this->sector));
    }

    static Tree *Splay(T i, Tree *t);

    static Tree *Insert(T i, Tree *t, T sector);

    static Tree *Insert(T i, Tree *t);

    static Tree *Delete(T i, Tree *t);

    static Tree *Modify(T i, Tree *t, T v);

    static Tree *find_rank(T r, Tree *t);

    static void read_mem(const string &trace_path, int l1_cache_line_size, int block_id, int m_kernel_id);

    static void printtree(Tree *t, int d);

    static void freetree(Tree *t);
};

//std::map<int, std::vector<mem_inst *> > mem_inst_map;
std::vector<std::pair<long long, int> > coalesced_address;

#define compare(i, j) ((i)-(j))

#define node_size(x) (((x)==NULL) ? 0 : ((x)->size))


#endif //SPLAY_SPLAY_H
