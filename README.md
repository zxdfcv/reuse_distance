# reuse_distance

这是使用 reuse distance 分析模型分析 GPU L1 缓存命中率的一个小工具，没有使用 mshr。

## 功能设计

代码分为数据结构和模拟算法两个部分。数据结构采用 `Splay` 树存储判断访存请求，结合 I/O 处理函数分析指令(STG/LDG)。
算法采用 reuse distance 的算法，如果两个相同地址访存请求相距超过 cache_line_size - 1，则miss，否则hit(或者sector miss)，然后cache line 的缓存情况。

这里利用了 `Splay` 树的一个性质：每访问一个节点 x 后都要强制将其旋转到根节点。 `Splay` 操作即对 x 做一系列的 Splay 步骤。每次对 x 做一次 Splay 步骤，x 到根节点的距离都会更近。定义 p 为 x 
的父节点。`Splay` 步骤有三种，具体分为六种情况：

假如新来了一个请求 req。首先将查找它并将它伸展到根部，然后判断判断它右子树的大小。如果找未到这个地址，MISS，同时插入这个地址请求；如果找到这个地址，且右子树的大小大于等于 
cache_line_size，也就是说比它新的请求对应的地址数超过了 cache_line_size - 
1，说明这个请求已经被淘汰，`MISS`。

如果如果右子树的大小小于于 cache_line_size，也就是说比它新的请求对应的地址数未超过 cache_line_size -
1，说明这个请求地址对应的数据存在与内存中，进而再判断 `SECTOR_MISS` 和 `HIT`。

最后修改更新这个地址请求。

注意，这个代码支持了了 sector 这个结构，具体表现在对每个节点添加了一个 `sector[256]`，用于判断对应的 sector 是否命中。但是这里没有考虑一行 cache line 能够装下多少个 sector。

## 代码实现

main.cpp：算法
Splay.cpp/Splay.h：数据结构
