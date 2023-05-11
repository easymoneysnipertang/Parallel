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
#include"ThreadParam.h"
#include"SVS_Pthread.h"
#include"SvS_Neon.h"
#include"Bitmap.h"
#include<omp.h>
#include <pthread.h>
#include <semaphore.h>
using namespace std;
extern int query[1000][5];
extern vector<InvertedIndex> invertedLists;
extern bool staticFlag;
extern pthread_mutex_t mutex;
extern sem_t sem_main, sem_worker[NUM_THREADS];
extern bool isParallel_out;
int queryCount;
void verify(InvertedIndex(*f)(int*, vector<InvertedIndex>&, int))
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
		//cin >> t.docIdList.size();
		for (int j = 0; j < docIdLen[i]; j++)
		{
			unsigned int docId;
			//cin >> docId;
			docId = docIdList[i][j];
			t.docIdList.push_back(docId);
		}
		sort(t.docIdList.begin(), t.docIdList.end());//���ĵ��������
		testInvertedLists.push_back(t);
	}
	bitMapProcessing(testInvertedLists, 3); //bitmap��Ҫ
	InvertedIndex res = f(testQuery, testInvertedLists, 3);
	for (auto i : res.docIdList)
		cout << i << ' ';

}

void getData(vector<InvertedIndex>& invertedLists,int query[1000][5])
{
	//��ȡ�������ļ�
	fstream file;
	file.open("ExpIndex.bin", ios::binary | ios::in);
	if (!file.is_open()) {
		cout << "Wrong in opening file!";
		return;
	}
	unsigned int maxdocId=0;
	double avgLen = 0;
	for (int i = 0; i < 2000; i++)		//�ܹ���ȡ2000����������
	{
		InvertedIndex* t = new InvertedIndex();				//һ����������
		int len;
		file.read((char*)&len, sizeof(len));
		avgLen += len;
		for (int j = 0; j < len; j++)
		{
			unsigned int docId;			//�ļ�id
			file.read((char*)&docId, sizeof(docId));
			t->docIdList.push_back(docId);//���������ű�
			if (docId > maxdocId)
				maxdocId = docId;
		}
		sort(t->docIdList.begin(), t->docIdList.end());//���ĵ��������
		invertedLists.push_back(*t);		//����һ�����ű�
	}
	cout << maxdocId << endl;
	cout << avgLen / 2000 << endl;
	file.close();

	// ��ȡ��ѯ����
	file.open("ExpQuery.txt", ios::in);
	for (int i = 0; i < 1000; i++)
		for (int j = 0; j < 5; j++)
			query[i][j] = 0;
	string line;
	int count = 0;
	while (getline(file, line)) {// ��ȡһ��
		stringstream ss; // �ַ���������
		ss << line; // ������һ��
		int i = 0;
		int temp;
		ss >> temp;
		while (!ss.eof()) {
			query[count][i] = temp;
			i++;
			ss >> temp;
		}
		count++;// �ܲ�ѯ��
	}
	cout << "here" << endl;
}

// �ѵ����б���������
void sorted(int* list, vector<InvertedIndex>& idx, int num) {
	for (int i = 0; i < num - 1; i++) {
		for (int j = 0; j < num - i - 1; j++) {
			if (idx[list[j]].docIdList.size() > idx[list[j + 1]].docIdList.size()) {
				int tmp = list[j];
				list[j] = list[j + 1];
				list[j + 1] = tmp;
			}
		}
	}
}

void test(InvertedIndex (*f)(int*, vector<InvertedIndex>&, int),
	int query[1000][5], vector<InvertedIndex>& invertedLists)
{
	long long head, tail, freq;
	int step = 50;
	for (int k = 50; k <= 1000; k += step)
	{
		double totalTime = 0;
		//��ʼ��ʱ���
		for (int t = 0; t < 5; t++)
		{
			QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
			QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
#pragma omp parallel if(isParallel_out),num_threads(NUM_THREADS),shared(query,invertedLists)
#pragma omp for
			for (int i = 0; i < k; i++) // k����ѯ
			{
				int num = 0;// query��ѯ����
				for (int j = 0; j < 5; j++)
					if (query[i][j] != 0)
						num++;
				int* list = new int[num];// Ҫ�����list
				for (int j = 0; j < num; j++)
					list[j] = query[i][j];
				sorted(list, (invertedLists), num);//����������
				f(list, invertedLists, num);
			}
			QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
			totalTime += (tail - head) * 1000.0 / freq;
		}
		cout << totalTime / 5 << endl;
		if (k == 100)
			step = 100;
		if (k == 400)
			step = 200;
	}
}
struct threadParamOut
{
	int start, end, id;
	InvertedIndex(*f)(int*, vector<InvertedIndex>&, int);
};
//��̬query��
void* dynamicForTest(void* param)
{
	threadParamOut* par = (threadParamOut*)param;
	int start = par->start;
	int end = par->end;
	int id = par->id;
	InvertedIndex(*f)(int*, vector<InvertedIndex>&, int) = par->f;
	for (int i = start; i < end; i++) // start��end����ѯ
	{
		int num = 0;// query��ѯ����
		for (int j = 0; j < 5; j++)
			if (query[i][j] != 0)
				num++;
		int* list = new int[num];// Ҫ�����list
		for (int j = 0; j < num; j++)
			list[j] = query[i][j];
		sorted(list, invertedLists, num);//����������
		f(list, invertedLists, num);
	}
	pthread_exit(NULL);
	return NULL;
}
void testQueryDynamic(InvertedIndex(*f)(int*, vector<InvertedIndex>&, int),
	int query[1000][5], vector<InvertedIndex>& invertedLists)
{
	long long head, tail, freq;
	int step = 50;

	for (int k = 50; k <= 1000; k += step)
	{
		
		double totalTime = 0;
		//��ʼ��ʱ���
		for (int t = 0; t < 5; t++)
		{
			QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
			QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
			pthread_t thread[NUM_THREADS];
			int skip = k / NUM_THREADS;
			threadParamOut paramOut[NUM_THREADS];
			for (int i = 0; i < NUM_THREADS; i++) {// �����̣߳������߳�id�Լ��������query��
				paramOut[i].id = i;
				paramOut[i].start = i*skip;
				paramOut[i].end = i + 1 == NUM_THREADS ? k : (i + 1) * skip;
				paramOut[i].f = f;
				pthread_create(&thread[i], NULL, dynamicForTest, &paramOut[i]);
			}
			for (int i = 0; i < NUM_THREADS; i++) {
				pthread_join(thread[i], NULL);
			}
			QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
			totalTime += (tail - head) * 1000.0 / freq;
		}
		cout << totalTime / 5 << endl;
		if (k == 100)
			step = 100;
		if (k == 400)
			step = 200;
	}
}


//��̬query��
void* staticForTest(void* param)
{

	threadParamOut* par = (threadParamOut*)param;
	int id = par->id;
	InvertedIndex(*f)(int*, vector<InvertedIndex>&, int) = par->f;


	while (true)
	{	
		sem_wait(&sem_worker[id]);// �������ȴ����̻߳���
		//long long head, tail, freq;
		//QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		//QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
		if (staticFlag == false)
			break;
		int skip = queryCount / NUM_THREADS;
		int start = id * skip;
		int end = id + 1 == NUM_THREADS ?  queryCount: (id + 1) * skip;
		for (int i = start; i < end; i++) // start��end����ѯ
		{
			int num = 0;// query��ѯ����
			for (int j = 0; j < 5; j++)
				if (query[i][j] != 0)
					num++;
			int* list = new int[num];// Ҫ�����list
			for (int j = 0; j < num; j++)
				list[j] = query[i][j];
			sorted(list, invertedLists, num);//����������
			f(list, invertedLists, num);

		}
		//QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
		//cout<<"id:"<<id<<" "<<(tail - head) * 1000.0 / freq<<" ms"<<endl;
		sem_post(&sem_main);// �������߳�
	}
	pthread_exit(NULL);
	return NULL;
}
void testQueryStatic(InvertedIndex(*f)(int*, vector<InvertedIndex>&, int),
	int query[1000][5], vector<InvertedIndex>& invertedLists)
{
	
	long long head, tail, freq;
	int step = 50;
	// �����̣߳������߳�id�Լ��������query��
	pthread_t thread[NUM_THREADS];
	threadParamOut paramOut[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++) 
	{
		paramOut[i].id = i;
		paramOut[i].f = f;
		pthread_create(&thread[i], NULL, staticForTest, &paramOut[i]);
	}
	// ��ʼ���ź���
	sem_init(&sem_main, 0, 0);
	for (int i = 0; i < NUM_THREADS; i++)
		sem_init(&sem_worker[i], 0, 0);

	for (int k = 50; k <= 1000; k += step)
	{
		double totalTime = 0;
		queryCount = k;
		//��ʼ��ʱ���

		for (int t = 0; t < 5; t++)
		{
			QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
			QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
			staticFlag = true;

			// ���ѹ����߳�start
			for (int i = 0; i < NUM_THREADS; i++)
				sem_post(&sem_worker[i]);
			// ���߳�˯��
			for (int i = 0; i < NUM_THREADS; i++)
				sem_wait(&sem_main);

			QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
			totalTime += (tail - head) * 1000.0 / freq;
		}
		cout << totalTime / 5 << endl;

		if (k == 100)
			step = 100;
		if (k == 400)
			step = 200;
	}
	// �����߳̽���
	staticFlag = false;
	// ���ѹ����߳�end->�ö໽��һ�Σ���Ȼ���ڿ�ͷ��sem_wait
	for (int i = 0; i < NUM_THREADS; i++)
		sem_post(&sem_worker[i]);
	for (int i = 0; i < NUM_THREADS; i++) 
		pthread_join(thread[i], NULL);
	// �ݻ��ź���
	sem_destroy(&sem_main);
	for (int i = 0; i < NUM_THREADS; i++) 
		sem_destroy(&sem_worker[i]);
}

//��̬query��

void testInStatic(InvertedIndex(*testFunc)(int*, vector<InvertedIndex>&, int),
	void* (*threadFunc)(void*),
	int query[1000][5], vector<InvertedIndex>& invertedLists)
{
	long long head, tail, freq;

	pthread_mutex_init(&mutex, NULL);
	// ---��̬query�ڲ���svs adp
	sem_init(&sem_main, 0, 0);// init
	for (int i = 0; i < NUM_THREADS; i++) 
		sem_init(&sem_worker[i], 0, 0);
	staticFlag = true;
	threadParam_t param[NUM_THREADS];
	pthread_t thread[NUM_THREADS];// handle
	
	// �����̣߳��������
	for (int j = 0; j < NUM_THREADS; j++) 
	{
			param[j].t_id = j;
			pthread_create(&thread[j], NULL, threadFunc, &param[j]);
	}

	int step = 50;
	for (int k = 50; k <= 1000; k += step)
	{
		double totalTime = 0;
		for (int t = 0; t < 5; t++)
		{
			QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
			QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
			for (int i = 0; i < k; i++) // k����ѯ
			{
				int num = 0;// query��ѯ����
				for (int j = 0; j < 5; j++) 
				{
					if (query[i][j] != 0) 
						num++;
				}
				int* list = new int[num];// Ҫ�����list
				for (int j = 0; j < num; j++) {
					list[j] = query[i][j];
				}
				sorted(list, invertedLists, num);// ����������
				subWait = true;
				testFunc(list, invertedLists, num);
			}
			QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
			totalTime += (tail - head) * 1000.0 / freq;
		}
		cout << totalTime / 5 << endl;
		if (k == 100)
			step = 100;
		if (k == 400)
			step = 200;
	}
 		staticFlag = false;// �����߳̽���
		// ���ѹ����߳�end->�ö໽��һ�Σ���Ȼ���ڿ�ͷ��sem_wait
		for (int j = 0; j < NUM_THREADS; j++)
			sem_post(&sem_worker[j]);
		for (int i = 0; i < NUM_THREADS; i++) 
			pthread_join(thread[i], NULL);
		// �ݻ��ź���
		sem_destroy(&sem_main);
		for (int i = 0; i < NUM_THREADS; i++) 
			sem_destroy(&sem_worker[i]);

		pthread_mutex_destroy(&mutex);
}