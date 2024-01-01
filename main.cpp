#include <stdlib.h>         
#include <stdio.h>
#include <malloc.h>
#include<string.h>
#include <cassert>
#include <iostream>
using namespace std;
/*
 * 内存块分为标记区(Header)和数据区(Data)
 */
struct BlockNode{//Header
	BlockNode* m_next;
	size_t m_size;
};
enum class SEARCH_MODE{
	FIRST_FIT,
	BEST_FIT
};
class MemAllocator {
public:
	MemAllocator(size_t pSize, SEARCH_MODE pMode) {
		m_size = pSize * 8;
		m_data = _aligned_malloc(m_size, 8);//申请一块8字节对齐的内存
		m_mode = pMode;
		memset(m_data, 0, m_size);
		InitFreeList(m_data, m_size);
		cout << "申请内存空间大小为：" << m_size << "B" << endl;
		cout << "内存空间实际剩余为：" << m_remainSize << "B" << endl;
		cout << "内存空间首地址为：0x" << m_data << endl;
	}
	//初始化FreeList
	void InitFreeList(void* pStart, size_t pSize){
		assert(pStart != nullptr);
		m_numAllocations = 0;
		m_remainSize = pSize - sizeof(BlockNode);
		m_freeList = static_cast<BlockNode*>(pStart);
		m_freeList->m_next = nullptr;
		m_freeList->m_size = m_remainSize;
	}
	bool GetProperBlock(size_t pSize, SEARCH_MODE pMode, BlockNode** pPrevNode, BlockNode** pFoundNode) {
		if (pMode == SEARCH_MODE::FIRST_FIT) {
			return GetFirstFitBlock(pSize, pPrevNode, pFoundNode);
		}
		return GetBestFitBlock(pSize, pPrevNode, pFoundNode);
	}
	bool GetFirstFitBlock(size_t pSize, BlockNode** pPrevNode, BlockNode** pFoundNode) {
		BlockNode* prev = nullptr;
		BlockNode* curr = m_freeList;
		bool success = false;

		while (curr != nullptr){
			if (curr->m_size >= pSize){
				success = true;
				break;
			}
			prev = curr;
			curr = curr->m_next;
		}
		*pPrevNode = prev;
		*pFoundNode = curr;
		return success;
	}
	bool GetBestFitBlock(size_t pSize, BlockNode** pPrevNode, BlockNode** pFoundNode){
		BlockNode* prev = nullptr;
		BlockNode* curr = m_freeList;
		bool success = false;

		BlockNode* best = nullptr;
		BlockNode* bestPrev = nullptr;
		size_t bestSize = m_size;
		while (curr != nullptr){
			size_t currSize = curr->m_size;
			if (currSize >= pSize){
				success = true;
				if (currSize < bestSize){
					bestSize = currSize;
					best = curr;
					bestPrev = prev;
				}
			}

			prev = curr;
			curr = curr->m_next;
		}

		*pPrevNode = bestPrev;
		*pFoundNode = best;
		return success;
	}
	/*若找到的空闲内存块的大小size > 所需内存大小 + sizeof(BlockNode)，
	则将该空闲块分裂为两个新的块，并将新分出的块插入到FreeList中：*/
	void SplitBlock(BlockNode* pOld, size_t pSize){
		assert(pOld != nullptr);
		size_t oldBlockSize = pOld->m_size;
		assert(oldBlockSize > pSize);
		BlockNode* newBlock = pOld + (pSize + sizeof(BlockNode)) / 8;
		newBlock->m_size = oldBlockSize - pSize - sizeof(BlockNode);
		pOld->m_size = pSize;
		InsertNode(pOld, newBlock);
		cout << "Split block" << endl;
	}
	void RemoveNode(BlockNode* pPrev, const BlockNode* pDelete){
		assert(pDelete != nullptr);
		if (pPrev != nullptr){
			pPrev->m_next = pDelete->m_next;
		}
		else{
			m_freeList = pDelete->m_next;
		}
	}

	void InsertNode(BlockNode* pPrev, BlockNode* pNew){
		assert(pNew != nullptr);
		if (pPrev != nullptr){
			pNew->m_next = pPrev->m_next;
			pPrev->m_next = pNew;
		}
		else{
			pNew->m_next = m_freeList;
			m_freeList = pNew;
		}
	}
	void* myalloc(size_t pSize){
		size_t dataSize = pSize * 8;
		if (dataSize <= m_remainSize){
			BlockNode* prev = nullptr;
			BlockNode* found = nullptr;
			if (GetProperBlock(dataSize, m_mode, &prev, &found)){
				assert(found != nullptr);
				size_t founBlockSize = found->m_size;
				size_t allocateSize = founBlockSize + sizeof(BlockNode);
				if (founBlockSize > (dataSize + sizeof(BlockNode))){
					SplitBlock(found, dataSize);
					allocateSize = dataSize + sizeof(BlockNode);
				}
				RemoveNode(prev, found);
				m_remainSize -= allocateSize;
				++m_numAllocations;
				cout << "Malloc success, num allocations: " << m_numAllocations << endl;
				cout << "Remain size: " << m_remainSize << "B" << endl;
				return found + 1;
			}
		}
		cout << "Malloc failed!" << endl;
		cout << "Remain size: " << m_remainSize << endl;
		return nullptr;
	}
	bool GetPrevNode(BlockNode** pPrev, BlockNode* pNode){
		assert(pNode != nullptr);
		bool success = false;
		BlockNode* prev = nullptr;
		BlockNode* curr = m_freeList;
		while (curr != nullptr){
			if (pNode <= curr){
				success = true;
				break;
			}

			prev = curr;
			curr = curr->m_next;
		}

		*pPrev = prev;
		return success;
	}
	void MergeBlock(BlockNode* pPrev, BlockNode* pMerge){
		assert(pMerge != nullptr);
		size_t mergeNodeSize = pMerge->m_size;
		m_remainSize += mergeNodeSize;

		if (pMerge->m_next != nullptr){
			size_t nextNodeSize = pMerge->m_next->m_size;
			if ((pMerge + (mergeNodeSize + sizeof(BlockNode)) / 8) == pMerge->m_next) {
				RemoveNode(pMerge, pMerge->m_next);
				cout << "Merge block with next block, size after merge :" << nextNodeSize << endl;
				mergeNodeSize += sizeof(BlockNode) + nextNodeSize;//更新mergeNodeSize
				pMerge->m_size = mergeNodeSize;
				m_remainSize += sizeof(BlockNode);
			}
		}

		if (pPrev != nullptr){
			size_t prevNodeSize = pPrev->m_size;
			if ((pPrev + (prevNodeSize + sizeof(BlockNode)) / 8) == pMerge) {
				cout << "Merge block with prev block, size before merge :" << prevNodeSize << endl;
				RemoveNode(pPrev, pMerge);
				prevNodeSize += mergeNodeSize + sizeof(BlockNode);
				pPrev->m_size = prevNodeSize;
				m_remainSize += sizeof(BlockNode);
			}
		}
	}
	void myfree(void* pPtr){
		if (pPtr != nullptr){
			BlockNode* freeBlock = (BlockNode*)pPtr - 1;
			BlockNode* prev = nullptr;
			GetPrevNode(&prev, freeBlock);
			InsertNode(prev, freeBlock);
			MergeBlock(prev, freeBlock);
			--m_numAllocations;
			cout << "Free success, num allocations: " << m_numAllocations << endl;
			cout << "Remain size: " << m_remainSize << endl;
		}
	}
	~MemAllocator() {
		if (m_data != nullptr) {
			_aligned_free(m_data);
		}
	}
private:
	BlockNode* m_freeList;//指向所有空闲内存块
	void* m_data;
	size_t m_size;
	size_t m_numAllocations; //记录申请和释放内存的次数
	size_t m_remainSize; //记录空闲块数据区的总大小
	SEARCH_MODE m_mode;
};
struct block {
	string name;	//要分配的内存块名，便于删除想要操作的内存卡
	size_t size;	//分配的大小
	BlockNode* ptr;
}block[100];

int main() {
	char choice = ' ';
	cout << "请从下列选项中进行选择" << endl;
	cout << "1.最先适应算法" << endl;
	cout << "2.最优适应算法" << endl;
	cout << "3.退出" << endl;
	cout << ">>";
	cin >> choice;
	SEARCH_MODE mode;
	switch (choice) {
		case '1':mode = SEARCH_MODE::FIRST_FIT; break;
		case '2':mode = SEARCH_MODE::BEST_FIT; break;
		case '3':
		default:return 0;
	}
	MemAllocator ma(1024 / 8, mode);//申请一块1K字节的内存空间作为空闲内存
	int num = 0;
	while (true) {
		do {
			cout << "请从下列选项中进行选择:" << endl;
			cout << "1.分配内存" << endl;
			cout << "2.回收内存" << endl;
			cout << "3.结束操作" << endl;
			cout << ">>";
			cin >> choice;
		} while (choice != '1' && choice != '2' && choice != '3');
		switch (choice) {
		case '1': {
			cout << "请输入要分配的内存名称：";
			string name;
			cin >> name;
			block[num].name = name;
			cout << "请输入要分配的内存大小：";
			int size;
			cin >> size;
			BlockNode* ptr = (BlockNode*)ma.myalloc(size / 8);
			cout << "分配的地址空间为：0x" << ptr << "--" << "0x" << ptr + size / 8 << endl;
			block[num].ptr = ptr;
			block[num].size = size;
			num++;
			break;
		}
		case '2': {
			string name;
			bool success = false;
			cout << "请输入要释放的内存名称：";
			cin >> name;
			for (int i = 0; i < num; i++) {
				if (block[i].name == name) {
					success = true;
					ma.myfree(block[i].ptr);
				}
			}
			if (!success) {
				cout << "名称不存在！" << endl;
			}
			break;
		}
		case '3':return 0; break;
		}
	}
}