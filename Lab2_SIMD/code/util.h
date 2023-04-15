#pragma once
#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<sstream>
#include<Windows.h>
#include<algorithm>
#include"InvertedIndex.h"
#include"SvS.h"
#include"Adp.h"
#include"SvS_Neon.h"
using namespace std;
void verify()
{
	vector<InvertedIndex> testInvertedLists;
	int testQuery[3] = { 0,1,2 };
	int docIdList[3][13] = { {11, 13, 14, 16, 17, 39, 8, 11, 40, 42, 50, 4},
						   {5, 13, 17, 40, 50, 16},
						   {12, 1, 9, 10, 16, 18, 20, 40, 50, 13, 2, 3, 5} };
	int docIdLen[3] = { 12,6,13 };
	for (int i = 0; i < 3; i++)
	{
		InvertedIndex t;
		//cin >> t.length;
		t.length = docIdLen[i];
		for (int j = 0; j < t.length; j++)
		{
			unsigned int docId;
			//cin >> docId;
			docId = docIdList[i][j];
			t.docIdList.push_back(docId);
		}
		sort(t.docIdList.begin(), t.docIdList.end());//对文档编号排序
		testInvertedLists.push_back(t);
	}
	InvertedIndex res = SVS(testQuery, testInvertedLists, 3);
	for (auto i : res.docIdList)
		cout << i << ' ';

}
void getData(vector<InvertedIndex>& invertedLists,int query[1000][5],int& count)
{
	//读取二进制文件
	fstream file;
	file.open("ExpIndex.bin", ios::binary | ios::in);
	if (!file.is_open()) {
		cout << "Wrong in opening file!";
		return;
	}
	unsigned int maxdocId=0;
	for (int i = 0; i < 2000; i++)		//总共读取2000个倒排链表
	{
		InvertedIndex* t = new InvertedIndex();				//一个倒排链表
		file.read((char*)&t->length, sizeof(t->length));
		for (int j = 0; j < t->length; j++)
		{
			unsigned int docId;			//文件id
			file.read((char*)&docId, sizeof(docId));
			t->docIdList.push_back(docId);//加入至倒排表
			if (docId > maxdocId)
				maxdocId = docId;
		}
		sort(t->docIdList.begin(), t->docIdList.end());//对文档编号排序
		invertedLists.push_back(*t);		//加入一个倒排表
	}
	cout << maxdocId << endl;
	file.close();

	// 读取查询数据
	file.open("ExpQuery.txt", ios::in);
	for (int i = 0; i < 1000; i++)
		for (int j = 0; j < 5; j++)
			query[i][j] = 0;
	string line;
	count = 0;
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
	cout << "here" << endl;
}
//int bits[2] = { 0 };
//int x;
//for (int i = 0; i < 4; i++)
//{
//	cin >> x;
//	long long b = 1;
//	if (x > 32)
//	{
//		b <<= x - 32;
//		bits[1] |= b;
//	}
//	else
//	{
//		b <<= x;
//		bits[0] |= b;
//	}
//}	
//for (int i = 0; i < 2; i++)
//{
//	int cnt = i*32;
//	int bit = bits[i];
//	while (bit != 0)
//	{
//		if((bit&1)!=0)
//			cout << cnt<< ' ';
//		bit = bit >> 1;
//		++cnt;
//	}
//}