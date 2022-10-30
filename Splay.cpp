//
// Created by 10791 on 2022/10/25.
//

#include <queue>
#include <cmath>
#include <sstream>
#include "Splay.h"
Tree * Tree::Splay (T i, Tree *t)
{
    Tree N, *l, *r, *y;
    T comp,l_size, r_size;
    if (t == nullptr) return t;
    N.left = N.right = nullptr;
    l = r = &N;
    l_size = r_size = 0;

    for (;;) {
        comp = compare(i, t->key);
        if (comp < 0) {
            if (t->left == nullptr) break;
            if (compare(i, t->left->key) < 0) {
                y = t->left;                           /* rotate right */
                t->left = y->right;
                y->right = t;
                t->size = node_size(t->left) + node_size(t->right) + 1;
                t = y;
                if (t->left == nullptr) break;
            }
            r->left = t;                               /* link right */
            r = t;
            t = t->left;
            r_size += 1+node_size(r->right);
        } else if (comp > 0) {
            if (t->right == nullptr) break;
            if (compare(i, t->right->key) > 0) {
                y = t->right;                          /* rotate left */
                t->right = y->left;
                y->left = t;
                t->size = node_size(t->left) + node_size(t->right) + 1;
                t = y;
                if (t->right == nullptr) break;
            }
            l->right = t;                              /* link left */
            l = t;
            t = t->right;
            l_size += 1+node_size(l->left);
        } else {
            break;
        }
    }
    l_size += node_size(t->left);  /* Now l_size and r_size are the sizes of */
    r_size += node_size(t->right); /* the left and right trees we just built.*/
    t->size = l_size + r_size + 1;

    l->right = r->left = nullptr;

    /* The following two loops correct the size fields of the right path  */
    /* from the left child of the root and the right path from the left   */
    /* child of the root.                                                 */
    for (y = N.right; y != nullptr; y = y->right) {
        y->size = l_size;
        l_size -= 1+node_size(y->left);
    }
    for (y = N.left; y != nullptr; y = y->left) {
        y->size = r_size;
        r_size -= 1+node_size(y->right);
    }

    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;

    return t;
}

Tree * Tree::Insert(T i, Tree * t, T sector) {
/* Insert key i into the tree t, if it is not already there. */
/* Return a pointer to the resulting tree.                   */
    Tree * New;

    if (t != nullptr) {
        t = Splay(i,t);
        if (compare(i, t->key)==0) {
            return t;  /* it's already there */
        }
    }
    New = (Tree *) malloc (sizeof (Tree));
    if (New == nullptr) {printf("Ran out of space\n"); exit(1);}
    if (t == nullptr) {
        New->left = New->right = nullptr;
    } else if (compare(i, t->key) < 0) {
        New->left = t->left;
        New->right = t;
        t->left = nullptr;
        t->size = 1+node_size(t->right);
    } else {
        New->right = t->right;
        New->left = t;
        t->right = nullptr;
        t->size = 1+node_size(t->left);
    }
    New->key = i;
    New->sector[sector] = 1; //填充对应的sector
    New->size = 1 + node_size(New->left) + node_size(New->right);
    return New;
}

Tree * Tree::Insert(T i, Tree * t) { //函数重载
/* Insert key i into the tree t, if it is not already there. */
/* Return a pointer to the resulting tree.                   */
    Tree * New;

    if (t != nullptr) {
        t = Splay(i,t);
        if (compare(i, t->key)==0) {
            return t;  /* it's already there */
        }
    }
    New = (Tree *) malloc (sizeof (Tree));
    if (New == nullptr) {printf("Ran out of space\n"); exit(1);}
    if (t == nullptr) {
        New->left = New->right = nullptr;
    } else if (compare(i, t->key) < 0) {
        New->left = t->left;
        New->right = t;
        t->left = nullptr;
        t->size = 1+node_size(t->right);
    } else {
        New->right = t->right;
        New->left = t;
        t->right = nullptr;
        t->size = 1+node_size(t->left);
    }
    New->key = i;
    New->size = 1 + node_size(New->left) + node_size(New->right);
    return New;
}

Tree * Tree::Delete(T i, Tree *t) {
    Tree * x;
    T tsize;

    if (t==nullptr) return nullptr;
    tsize = t->size;
    t = Splay(i,t);
    if (compare(i, t->key) == 0) {               /* found it */
        if (t->left == nullptr) {
            x = t->right;
        } else {
            x = Splay(i, t->left);
            x->right = t->right;
        }
        free(t);
        if (x != nullptr) {
        x->size = tsize-1;
        }
        return x;
    } else {
        return t;                         /* It wasn't there */
    }
}

Tree* Tree::Modify(T i, Tree *t, T v) {
    Tree * x;
    T tsize;

    if (t==nullptr) return nullptr;
    tsize = t->size;
    t = Splay(i,t);
    Tree* temp = t;
    if (compare(i, t->key) == 0) {               /* found it */
        if (t->left == nullptr) { //分离x
            x = t->right;
        } else {
            x = Splay(i, t->left);
            x->right = t->right;
        }
        x = Insert(v, x);//修改访问时间->删旧添新
        for (int j = 0; j < SECTOR_SIZE; j++) {
            x->sector[j] = temp->sector[j];
        }
        free(t);
        x->size = tsize-1;
        return x;
    } else {
        return t;                         /* It wasn't there */
    }
}

Tree* Tree::find_rank(T r, Tree *t) {
/* Returns a pointer to the node in the tree with the given rank.  */
/* Returns NULL if there is no such node.                          */
/* Does not change the tree.  To guarantee logarithmic behavior,   */
/* the node found here should be splayed to the root.              */
    T lsize;
    if ((r < 0) || (r >= node_size(t))) return nullptr;
    for (;;) {
        lsize = node_size(t->left);
        if (r < lsize) {
            t = t->left;
        } else if (r > lsize) {
            r = r - lsize -1;
            t = t->right;
        } else {
            return t;
        }
    }
}

void Tree::freetree(Tree* t)
{
    if(t==nullptr) return;
    freetree(t->right);
    freetree(t->left);
    free(t);
}

void Tree::printtree(Tree * t, int d) {
//printf("%p\n",t);
    int i;
    if (t == nullptr) return;
    printtree(t->right, d+1);
    for (i=0; i<d; i++) printf("  ");
    printf("%d(%d)\n", t->key, t->size);
    printtree(t->left, d+1);
}

void Tree::read_mem(const string &trace_path, int l1_cache_line_size, int block_id, int m_kernel_id) {
    std::ifstream mem_trace(
            trace_path + "/kernel-" + std::to_string(m_kernel_id) + "-block-" + std::to_string(block_id) + ".mem",
            std::ios::in); //打开文件
    string line;
    unsigned sector_num = 0;
    std::map<int, std::map<long long, int> > block_pc_num;  // warp_id:{pc:pc_num}
    while (std::getline(mem_trace, line)) {
        if (line != "\n") {
            if (line != "====") {
                std::istringstream is(line);
                std::string str;
                std::queue<std::string> tmp_str;
                while (is >> str) {
                    tmp_str.push(str);
                }
                int warp_id = std::stoi(tmp_str.front());
                tmp_str.pop();

                char *stop;
                long long pc = std::strtoll(tmp_str.front().c_str(), &stop, 16);
                tmp_str.pop();

                string opcode = tmp_str.front();
                tmp_str.pop();

                int pc_index = 0;
                if (block_pc_num.find(warp_id) != block_pc_num.end()) { //find常规操作
                    if (block_pc_num[warp_id].find(pc) != block_pc_num[warp_id].end()) {
                        pc_index = block_pc_num[warp_id][pc] + 1;
                        block_pc_num[warp_id][pc] = pc_index;
                    } else {
                        block_pc_num[warp_id][pc] = 0;
                    }
                } else {
                    block_pc_num[warp_id][pc] = 0;
                }

                while (!tmp_str.empty()) {//coalescing the addresses of the warp
                    string warp_address = tmp_str.front();
                    tmp_str.pop();

                    long long i_warp_address = std::strtoll(warp_address.c_str(), &stop, 16);

                    auto cache_line = (long long) (i_warp_address >> (int) log2(l1_cache_line_size));
                    long long offset = i_warp_address - (cache_line << (int) log2(l1_cache_line_size)); //取对数
                    long long cache_line_addr = cache_line << (int) log2(l1_cache_line_size);//获取地址大小
                    int sector_mask = (int) (offset / SECTOR_SIZE); //偏移量除以段的大小 //获取sector大小
                    if (std::find(coalesced_address.begin(), coalesced_address.end(),
                                  std::pair<long long, int>(cache_line_addr, sector_mask)) ==
                        coalesced_address.end()) {
                        coalesced_address.emplace_back(std::pair<long long, int>(cache_line_addr, sector_mask));
                    }
                }
                //sector_num += coalesced_address.size();
                //mem_inst_map[warp_id].push_back(new mem_inst(opcode, coalesced_address, pc, pc_index));
            }
        }
    }
}

