#pragma once
#include<string>
#include<vector>
#include<sstream>
#include<Windows.h>
#include<algorithm>
using namespace std;
struct InvertedIndex {// 倒排索引结构
	int length = 0;
	vector<unsigned int> docIdList;
};

// 重载比较符，以长度排序各个索引
bool operator<(const InvertedIndex& i1, const InvertedIndex& i2) 
{
	return i1.length < i2.length;
}