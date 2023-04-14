#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<sstream>
#include<Windows.h>
#include<algorithm>
#include <smmintrin.h>
#include <nmmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>
#include <immintrin.h>

using namespace std;

class InvertedIndex {// 倒排索引结构
public:
	int length = 0;
	vector<unsigned int> docIdList;
};
// 重载比较符，以长度排序各个索引
bool operator<(const InvertedIndex& i1, const InvertedIndex& i2) {
	return i1.length < i2.length;
}


// 把倒排列表按长度排序
void sorted(int* list, vector<InvertedIndex>& idx, int num) {
	for (int i = 0; i < num - 1; i++) {
		for (int j = 0; j < num - i - 1; j++) {
			if (idx[list[j]].length > idx[list[j + 1]].length) {
				int tmp = list[j];
				list[j] = list[j + 1];
				list[j + 1] = tmp;
			}
		}
	}
}
// svs实现
InvertedIndex SVS(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 取最短的列表

	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		int count = 0;// s从头往后遍历一遍
		int t = 0;
		// s列表中的每个元素都拿出来比较
		for(int j=0;j<s.length;j++){// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < index[queryList[i]].length; t++) {
				// 遍历i列表中所有元素
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
					break;
			}
			if (isFind)// 覆盖
				s.docIdList[count++] = s.docIdList[j];
		}
		if(count<s.length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}
	return s;
}
// svs_sse
InvertedIndex SVS_SSE(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 取最短的列表

	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		int count = 0;// s从头往后遍历一遍
		int t = 0;
		// s列表中的每个元素都拿出来比较
		for (int j = 0; j < s.length; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			__m128i ss = _mm_set1_epi32(s.docIdList[j]);// 填充32位

			for (; t < index[queryList[i]].length - 3; t+=4) {// 同时与四个元素比较
				// 遍历i列表中所有元素
				__m128i ii;
				ii = _mm_load_si128((__m128i*)&index[queryList[i]].docIdList[t]);// 一次取四个
				__m128i tmp = _mm_set1_epi32(0);
				tmp = _mm_cmpeq_epi32(ss, ii);// 比较向量每一位
				int mask = _mm_movemask_epi8(tmp);// 转为掩码，如果有一个数相等,就不会是全0
				
				if (mask!=0){// 查看比较结果
					isFind=true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 及时break，避免超过，下一元素需重头再来
					break;
			}
			if (!isFind&& (t>=index[queryList[i]].length - 3)) {// 处理剩余元素
				for (; t < index[queryList[i]].length; t++)
				{
					if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
						break;
				}
			}
			if (isFind)// 覆盖
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}
	return s;
}
// svs_sse_notAlign
InvertedIndex SVS_NotAlign(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 取最短的列表

	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		int count = 0;// s从头往后遍历一遍
		int t = 0;
		// s列表中的每个元素都拿出来比较
		for (int j = 0; j < s.length; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			__m128i ss = _mm_set1_epi32(s.docIdList[j]);// 填充32位

			// 测试不对齐的表现
			// 在开始把模4剩余的部分处理掉，以后每次加四保证都能访问
			for (; t < 2; t++) {
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
					break;
			}

			for (; t < index[queryList[i]].length-3; t += 4) {// 同时与四个元素比较
				// 遍历i列表中所有元素
				__m128i ii;
				ii = _mm_loadu_epi32(&index[queryList[i]].docIdList[t]);// 一次取四个
				__m128i tmp = _mm_set1_epi32(0);
				tmp = _mm_cmpeq_epi32(ss, ii);// 比较向量每一位
				int mask = _mm_movemask_epi8(tmp);// 转为掩码，如果有一个数相等,就不会是全0

				if (mask != 0) {// 查看比较结果
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 及时break，避免超过，下一元素需重头再来
					break;
			}
			if (!isFind && (t >= index[queryList[i]].length - 3)) {// 处理剩余元素
				for (; t < index[queryList[i]].length; t++)
				{
					if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
						break;
				}
			}
			if (isFind)// 覆盖
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}
	return s;
}
// svs_avx
InvertedIndex SVS_AVX(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 取最短的列表

	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		int count = 0;// s从头往后遍历一遍
		int t = 0;
		// s列表中的每个元素都拿出来比较
		for (int j = 0; j < s.length; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			__m256i ss = _mm256_set1_epi32(s.docIdList[j]);// 填充32位

			for (; t < index[queryList[i]].length - 7; t += 8) {// 同时与八个元素比较
				// 遍历i列表中所有元素
				__m256i ii;
				ii = _mm256_loadu_epi32(&index[queryList[i]].docIdList[t]);// 一次取八个
				__m256i tmp = _mm256_set1_epi32(0);
				tmp = _mm256_cmpeq_epi32(ss, ii);// 比较向量每一位
				int mask = _mm256_movemask_epi8(tmp);// 转为掩码，如果有一个数相等,就不会是全0

				if (mask != 0) {// 查看比较结果
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 及时break，避免超过，下一元素需重头再来
					break;
			}
			if (!isFind && (t >= index[queryList[i]].length - 7)) {// 处理剩余元素
				for (; t < index[queryList[i]].length; t++)
				{
					if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
						break;
				}
			}
			if (isFind)// 覆盖
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}
	return s;
}


// adp实现
class QueryItem {
public:
	int cursor;// 当前读到哪了
	int length;// 倒排索引总长度
	int key;// 关键字值
};
bool operator<(const QueryItem& q1, const QueryItem& q2) {// 选剩余元素最少的元素
	return (q1.length-q1.cursor) < (q2.length-q2.cursor);
}
InvertedIndex ADP(int* queryList, vector<InvertedIndex>& index, int num)
{
	/*InvertedIndex S;
	int s = 1;
	bool found = true;
	vector<InvertedIndex> idx_;
	for (int i = 0; i < num; i++)
	{
		idx_.push_back(index[queryList[i]]);
	}

	for (int t = 0; t < idx_[0].length; t++)
	{
		unsigned int e = idx_[0].docIdList[t];
		while (s != num && found == true)
		{
			for (int i = 0; i < idx_[s].length; i++)
			{
				found = false;
				if (e == idx_[s].docIdList[i])
				{
					found = true;
					break;
				}
			}
			s = s + 1;
		}
		if (s == num && found == true)
			S.docIdList.push_back(e);
	}
	return S;*/

	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}

	while (list[0].cursor < list[0].length) {// 最短的列表非空
		bool isFind = true;
		int s = 1;
		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
		while (s != num && isFind == true) {
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
			s++;// 下一个链表
		}
		list[0].cursor++;// 当前元素已被访问过
		if (s == num && isFind) {
			S.docIdList.push_back(e);
			S.length++;
		}
		//sort(list, list + num);// 重排，将未探查元素少的列表前移
	}
	return S;
}
// adp_sse
InvertedIndex ADP_SSE(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}

	while (list[0].cursor < list[0].length) {// 最短的列表非空
		bool isFind = true;
		int s = 1;
		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
		__m128i ee = _mm_set1_epi32(e);

		while (s != num && isFind == true) {
			isFind = false;
			while (list[s].cursor < list[s].length-3) {// 检查s列表
				// 构造向量
				__m128i ii = _mm_load_si128((__m128i*)&index[list[s].key].docIdList[list[s].cursor]);
				__m128i result = _mm_set1_epi32(0);
				result = _mm_cmpeq_epi32(ee, ii);
				int mask = _mm_movemask_epi8(result);// 得到掩码
				if (mask != 0) {// 查看比较结果
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])// 及时break，避免重置cursor
					break;
				list[s].cursor += 4;
			}
			if (!isFind) {
				while (list[s].cursor < list[s].length) {// 剩余字段
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;
					list[s].cursor++;// 当前访问过，且没找到合适的，往后移
				}
			}
			s++;// 下一个链表
		}
		list[0].cursor++;// 当前元素已被访问过
		if (s == num && isFind) {
			S.docIdList.push_back(e);
			S.length++;
		}
		//sort(list, list + num);// 重排，将未探查元素少的列表前移
	}
	return S;
}
// adp_avx
InvertedIndex ADP_AVX(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}

	while (list[0].cursor < list[0].length) {// 最短的列表非空
		bool isFind = true;
		int s = 1;
		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
		__m256i ee = _mm256_set1_epi32(e);

		while (s != num && isFind == true) {
			isFind = false;
			while (list[s].cursor < list[s].length - 7) {// 检查s列表
				// 构造向量
				__m256i ii = _mm256_loadu_epi32(&index[list[s].key].docIdList[list[s].cursor]);
				__m256i result = _mm256_set1_epi32(0);
				result = _mm256_cmpeq_epi32(ee, ii);
				int mask = _mm256_movemask_epi8(result);// 得到掩码
				if (mask != 0) {// 查看比较结果
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])// 及时break，避免重置cursor
					break;
				list[s].cursor += 8;
			}
			if (!isFind) {
				while (list[s].cursor < list[s].length) {// 剩余字段
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;
					list[s].cursor++;// 当前访问过，且没找到合适的，往后移
				}
			}
			s++;// 下一个链表
		}
		list[0].cursor++;// 当前元素已被访问过
		if (s == num && isFind) {
			S.docIdList.push_back(e);
			S.length++;
		}
		//sort(list, list + num);// 重排，将未探查元素少的列表前移
	}
	return S;
}
// adp_sse_notAlign
InvertedIndex ADP_NotAlign(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}

	while (list[0].cursor < list[0].length) {// 最短的列表非空
		bool isFind = true;
		int s = 1;
		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
		__m128i ee = _mm_set1_epi32(e);

		while (s != num && isFind == true) {
			isFind = false;
			// 测试不对齐的表现
			// 在开始把模4剩余的部分处理掉，以后每次加四保证都能访问
			while (list[s].cursor < 2) {// 剩余字段
				if (e == index[list[s].key].docIdList[list[s].cursor]) {
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// 当前访问过，且没找到合适的，往后移
			}

			while (list[s].cursor < list[s].length-3) {// 检查s列表
				// 构造向量
				__m128i ii = _mm_loadu_epi32(&index[list[s].key].docIdList[list[s].cursor]);
				__m128i result = _mm_set1_epi32(0);
				result = _mm_cmpeq_epi32(ee, ii);
				int mask = _mm_movemask_epi8(result);// 得到掩码
				if (mask != 0) {// 查看比较结果
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])// 及时break，避免重置cursor
					break;
				list[s].cursor += 4;
			}
			if (!isFind) {
				while (list[s].cursor < list[s].length) {// 剩余字段
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;
					list[s].cursor++;// 当前访问过，且没找到合适的，往后移
				}
			}
			
			s++;// 下一个链表
		}
		list[0].cursor++;// 当前元素已被访问过
		if (s == num && isFind) {
			S.docIdList.push_back(e);
			S.length++;
		}
		//sort(list, list + num);// 重排，将未探查元素少的列表前移
	}
	return S;
}

// hash分段实现
class HashBucket {// 一个hash段，记录他在倒排列表中的起始位置
public:
	int begin = -1;
	int end = -1;
};
HashBucket** hashBucket;
// 预处理，将倒排列表放入hash段里
void preprocessing(vector<InvertedIndex>& index, int count) {
	hashBucket = new HashBucket * [count];
	for (int i = 0; i < count; i++)
		hashBucket[i] = new HashBucket[50800];// 26000000/512->256!v

	for (int i = 0; i < count; i++) {
		for (int j = 0; j < index[i].length; j++) {
			int value = index[i].docIdList[j];// 拿出当前列表第j个值
			int hashValue = value / 512;// TODO:check
			if (hashBucket[i][hashValue].begin == -1) // 该段内还没有元素
				hashBucket[i][hashValue].begin = j;
			hashBucket[i][hashValue].end = j;// 添加到该段尾部
		}
	}
}
InvertedIndex HASH(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 选出最短的倒排列表
	for (int i = 1; i < num; i++) {// 与剩余列表求交
		// SVS的思想，将s挨个按表求交
		int count = 0;
		int length = s.length;
		
		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 512;
			// 找到该hash值在当前待求交列表中对应的段
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// 该值肯定找不到，不用往后做了
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// 在该段中找到了当前值
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
						break;
					}
				}
				if (isFind) {
					// 覆盖
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
		if (count < length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}
	return s;
}
// hash_sse
InvertedIndex HASH_SSE(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 选出最短的倒排列表
	for (int i = 1; i < num; i++) {// 与剩余列表求交
		// SVS的思想，将s挨个按表求交
		int count = 0;
		int length = s.length;

		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 512;
			// 找到该hash值在当前待求交列表中对应的段
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// 该值肯定找不到，不用往后做了
				continue;
			}
			else {// 在该段里进行查找，一次查4个
				__m128i ss = _mm_set1_epi32(s.docIdList[j]);// 填充32位

				for (begin; begin <= end-3; begin+=4) {
					__m128i ii;
					ii = _mm_loadu_epi32(&index[queryList[i]].docIdList[begin]);// 一次取四个
					__m128i tmp = _mm_set1_epi32(0);
					tmp = _mm_cmpeq_epi32(ss, ii);// 比较向量每一位
					int mask = _mm_movemask_epi8(tmp);// 转为掩码，如果有一个数相等,就不会是全0

					if (mask != 0) {// 查看比较结果
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin])// 及时break，避免超过，下一元素需重头再来
						break;
				}
				if (!isFind && (begin > end - 3)) {// 处理剩余元素
					for (; begin <= end; begin++)
					{
						if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
							isFind = true;
							break;
						}
						else if (s.docIdList[j] < index[queryList[i]].docIdList[begin])// 升序排列
							break;
					}
				}
				if (isFind) {
					// 覆盖
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
		if (count < length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}

	return s;
}


void test01() {
	vector<InvertedIndex> testInvertedLists;
	int testQuery[3] = { 0,1,2 };
	for (int i = 0; i < 3; i++)
	{
		InvertedIndex t;
		cin >> t.length;
		for (int j = 0; j < t.length; j++)
		{
			unsigned int docId;
			cin >> docId;
			t.docIdList.push_back(docId);
		}
		sort(t.docIdList.begin(), t.docIdList.end());//对文档编号排序
		testInvertedLists.push_back(t);
	}
	sorted(testQuery, testInvertedLists, 3);
	//preprocessing(testInvertedLists, 3);
	InvertedIndex res = SVS_SSE(testQuery, testInvertedLists, 3);
	for (auto i : res.docIdList)
		cout << i << " ";
}
void test02() {
	srand(time(0));

	vector<InvertedIndex> testInvertedLists;

	for (int i = 0; i < 500; i++)
	{
		InvertedIndex t;// 生成500个倒排列表
		t.length = rand() % (100 - 30 + 1) + 30;
		//cout << t.length << " :";

		vector<int> forRandom;
		for (int j = 0; j < t.length * 5; j++)//为了不重复
			forRandom.push_back(j);
		random_shuffle(forRandom.begin(), forRandom.end());//随机打乱

		for (int j = 0; j < t.length; j++)
		{
			int docId = forRandom[j];
			//cout << docId << " ";
			t.docIdList.push_back(docId);
		}
		sort(t.docIdList.begin(), t.docIdList.end());//对文档编号排序
		testInvertedLists.push_back(t);
		//cout << endl;
	}
	//preprocessing(testInvertedLists, 500);
	double sum = 0;
	for (int time = 0; time < 20; time++) {
		long long head, tail, freq;
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

		for (int i = 0; i < 200; i++) {// 做200次查询
			int testQuery[5];
			for (int i = 0; i < 5; i++)testQuery[i] = rand() % 500;

			sorted(testQuery, testInvertedLists, 5);
			InvertedIndex res = SVS_SSE(testQuery, testInvertedLists, 5);
			for (auto i : res.docIdList)
				cout << i << " ";
			if (res.docIdList.size() != 0)
				cout << endl;
		}
		QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
		sum += (tail - head) * 1000.0 / freq;
		//cout << endl << (tail - head) * 1000.0 / freq;
	}
	cout << endl << (double)sum / 20;
}

void mainFunc() {
	// 读取二进制文件
	fstream file;
	file.open("ExpIndex", ios::binary | ios::in);
	if (!file.is_open()) {
		cout << "Wrong in opening file!";
		return ;

	}
	static vector<InvertedIndex>* invertedLists = new vector<InvertedIndex>();
	for (int i = 0; i < 2000; i++)		//总共读取2000个倒排链表
	{
		InvertedIndex* t = new InvertedIndex();				//一个倒排链表
		file.read((char*)&t->length, sizeof(t->length));
		for (int j = 0; j < t->length; j++)
		{
			unsigned int docId;			//文件id
			file.read((char*)&docId, sizeof(docId));
			t->docIdList.push_back(docId);//加入至倒排表
		}
		sort(t->docIdList.begin(), t->docIdList.end());//对文档编号排序
		invertedLists->push_back(*t);		//加入一个倒排表
	}
	file.close();
	cout << "here" << endl;

	// 读取查询数据
	file.open("ExpQuery", ios::in);
	static int query[1000][5] = { 0 };// 单个查询最多5个docId,全部读取
	string line;
	int count = 0;
	while (getline(file, line)) {// 读取一行
		stringstream ss; // 字符串输入流
		ss << line; // 传入这一行
		int i = 0;
		int temp;
		ss >> temp;
		while (!ss.eof()) {
			query[count][i] = temp;
			i++;
			ss >> temp;
		}
		count++;// 总查询数
	}

	//------求交------
	long long head, tail, freq;

	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

	//preprocessing(*invertedLists, 2000);

	for (int i = 0; i < count; i++) {// count个查询
		int num = 0;// query查询项数
		for (int j = 0; j < 5; j++) {
			if (query[i][j] != 0) {
				num++;
			}
		}
		int* list = new int[num];// 要传入的list
		for (int j = 0; j < num; j++) {
			list[j] = query[i][j];
		}
		sorted(list, *(invertedLists), num);// 按长度排序
		InvertedIndex res = ADP_NotAlign(list, *invertedLists, num);
		cout << i << endl;
	}
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	cout << (tail - head) * 1000.0 / freq;
}


int main() {
	//test01();
	test02();
	
	//mainFunc();

	return 0;
}