#pragma once
#include<string>
#include<vector>
#include<sstream>
#include<Windows.h>
#include<algorithm>
using namespace std;
struct InvertedIndex {// ���������ṹ
	int length = 0;
	vector<unsigned int> docIdList;
};

// ���رȽϷ����Գ��������������
bool operator<(const InvertedIndex& i1, const InvertedIndex& i2) 
{
	return i1.length < i2.length;
}