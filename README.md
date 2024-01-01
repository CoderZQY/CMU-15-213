# CMU-15-213
This repo consists of my implementation of malloc lab in CMU 15-213.

My blog：[CMU15-213](https://coderzqy.github.io/2024/01/01/CMU15-213/)

Main work：

- 用 malloc 申请一块内存空间作为自己的空闲内存；
- 实现自己的 `void *myalloc(int size)` 和 `int myfree(void *ptr)` 函数，用于分配和回收内存，替代 malloc 和 free 函数；
- 采用<u>最先适应</u>、<u>最优适应</u>或<u>最差适应</u>分配算法对空闲内存进行管理。

