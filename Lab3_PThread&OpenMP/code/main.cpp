#include<iostream>
#include<vector>
#include<string>
#include<Windows.h>
#include<algorithm>
#include <pthread.h>
#include <semaphore.h>
#include"InvertedIndex.h"
#include"util.h"

#include"Adp.h"
#include"Adp_SSE.h"
#include"Adp_AVX.h"
#include"Adp_Pthread.h"

#include"SvS.h"
#include"SvS_SSE.h"
#include"SvS_AVX.h"
#include"SvS_Pthread.h"

#include"NotAligned.h"

#include"Hash.h"
#include"Hash_SSE.h"
#include"Hash_AVX.h"

#include"Bitmap.h"
//#include"BitMap_SSE.h"
//#include"Bitmap_AVX.h"
#include"Bitmap_Pthread.h"
using namespace std;

alignas(16) static vector<InvertedIndex> invertedLists;//��������
alignas(16) static int query[1000][5] = { 0 };// ������ѯ���5��docId,ȫ����ȡ
bool isParallel_out = true;//����query�䲢��

struct method
{
	InvertedIndex(*f)(int*, vector<InvertedIndex>&, int);
	string name;
	method(InvertedIndex(*f)(int*, vector<InvertedIndex>&, int),
		string name)
	{
		this->f = f;
		this->name = name;
	}
};

//method intersec[12] = {
//	{SVS,"SVS"},
//	{ADP,"ADP"},
//	{S_BITMAP,"Bitmap"},
//	{HASH,"Hash"},
//	{SVS_SSE,"SVS_SSE"},
//	{ADP_SSE,"ADP_SSE"},
//	{BITMAP_SSE,"BITMAP_SSE"},
//	{HASH_SSE,"HASH_SSE"},
//	{SVS_AVX,"SVS_AVX"},
//	{ADP_AVX,"ADP_AVX"},
//	{BITMAP_AVX,"BITMAP_AVX"},
//	{HASH_AVX,"HASH_AVX"} };
int main() 
{
	//��֤
	//verify(S_BITMAP);
	
	//��ȡ�������ļ�
	getData(invertedLists, query);



	//ѡ�������в���
	//int i;
	//cout << "method id:";
	//cin >> i;
	
	//Ԥ����	����ָ��hash��bitmap�㷨ʱ��Ҫ����
	preprocessing(invertedLists, 2000);
	//bitMapProcessing(invertedLists, 2000);
	//bitMapSSEProcessing(invertedLists, 2000);
	//bitMapAVXProcessing(invertedLists, 2000);
	//if(i==11||i==7||i==3)
	//	preprocessing(invertedLists, 2000);
	//else if(i==2)
	//	bitMapProcessing(invertedLists, 2000);
	//else if(i==6)
	//	bitMapSSEProcessing(invertedLists, 2000);
	//else if(i==10)
	//	bitMapAVXProcessing(invertedLists, 2000);

	//���в���
	//cout << intersec[i].name << endl;
	//���̲߳���
	test(HASH_omp,query, invertedLists);
	//���̲߳���
	//testQueryDynamic(S_BITMAP, query, invertedLists);
	//testQueryStatic(S_BITMAP, query, invertedLists);
	//testInStatic(S_BITMAP, staticForBITMAP, query, invertedLists);
	return 0;
}