#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include<omp.h>
#include"InvertedIndex.h"
#include"ThreadParam.h"
using namespace std;

//InvertedIndex ADP(int* list, vector<InvertedIndex>& idx, int num)
//{
//	InvertedIndex S;
//	int s = 1;
//	bool found = true;
//	vector<InvertedIndex> idx_;
//	for (int i = 0; i < num; i++)
//		idx_.push_back(idx[list[i]]);
//	sort(idx_.begin(), idx_.end());
//	for (int t = 0; t < idx_[0].length; t++)
//	{
//		unsigned int e = idx_[0].docIdList[t];
//		while (s != num && found == true)//没有到达最后
//		{
//			found = false;
//			for (int i = 0; i < idx_[s].length; i++)
//			{
//				if (e == idx_[s].docIdList[i])
//				{
//					found = true;
//					break;
//				}
//			}
//			s = s + 1;
//		}
//		if (s == num && found == true)
//			S.docIdList.push_back(s);
//	}
//	return S;
//}

// adp实现
class QueryItem {
public:
	int cursor;// 当前读到哪了
	int length;// 倒排索引总长度
	int key;// 关键字值
};
bool operator<(const QueryItem& q1, const QueryItem& q2) {// 选剩余元素最少的元素
	return (q1.length - q1.cursor) < (q2.length - q2.cursor);
}
InvertedIndex ADP(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].docIdList.size();
	}
	for (int i = list[0].cursor; i < list[0].length; i++) {// 最短的列表非空
		bool isFind = true;
		unsigned int e = index[list[0].key].docIdList[i];
		for (int s = 1; s != num && isFind == true; s++) {
			isFind = false;
			while (list[s].cursor < list[s].length) {// 检查s列表
				if (e == index[list[s].key].docIdList[list[s].cursor]) {
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// 当前访问过，且没找到合适的，往后移
			}
			// 下一个链表
		}
		// 当前元素已被访问过
		if (isFind)
			S.docIdList.push_back(e);
		//sort(list, list + num);// 重排，将未探查元素少的列表前移
	}
	return S;
}

InvertedIndex ADP_omp(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex S;
#pragma omp parallel num_threads(4)
	{
		QueryItem* list = new QueryItem[num]();
		for (int i = 0; i < num; i++)// 预处理
		{
			list[i].cursor = 0;
			list[i].key = queryList[i];
			list[i].length = index[queryList[i]].docIdList.size();
		}

#pragma omp for 
		for (int i = list[0].cursor; i < list[0].length; i++) {// 最短的列表非空
			bool isFind = true;
			unsigned int e = index[list[0].key].docIdList[i];
			for (int s = 1; s != num && isFind == true; s++) {
				isFind = false;
				while (list[s].cursor < list[s].length) {// 检查s列表
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;
					list[s].cursor++;// 当前访问过，且没找到合适的，往后移
				}
				// 下一个链表
			}
#pragma omp critical
			// 当前元素已被访问过
			if (isFind)
				S.docIdList.push_back(e);
			//sort(list, list + num);// 重排，将未探查元素少的列表前移
		}
	}
	return S;
}