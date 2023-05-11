#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include"InvertedIndex.h"
#include"Adp.h"
#include <pthread.h>
#include <semaphore.h>
#include "util.h"
#include"ThreadParam.h"
using namespace std;
extern vector<InvertedIndex> invertedLists;
extern pthread_mutex_t mutex;
extern bool staticFlag;
extern sem_t sem_main, sem_worker[NUM_THREADS];
extern int numFor;
int countForSVS;
InvertedIndex S_in_SVS;
pthread_barrier_t barrier_wait;
sem_t sem_barrier;


void* dynamicForSVS(void* param) {
	threadParam_t* p = (threadParam_t*)param;
	int t_id = p->t_id;
	int i = p->currentQuery;
	int length = S_in_SVS.docIdList.size() / 4;
	int begin = t_id * length, end = (t_id + 1) * length;
	int t = 0;

	if (length == 0)
		goto Out_SVS_dynamic;

	// Ԥ������ٴ�����¼�
	t = t_id * ((invertedLists)[i].docIdList.size() / NUM_THREADS);
	for (int j = t_id; j > 0; j--) {
		if (S_in_SVS.docIdList[begin] < (invertedLists)[i].docIdList[t])
			t -= ((invertedLists)[i].docIdList.size() / NUM_THREADS);
		else break;
	}

	// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
	for (; begin < end; begin++) {// ����Ԫ�ض��÷���һ��
		bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

		for (; t < (invertedLists)[i].docIdList.size(); t++) 
		{
			// ����i�б�������Ԫ��
			if (S_in_SVS.docIdList[begin] == (invertedLists)[i].docIdList[t]) 
			{
				isFind = true;
				break;
			}
			else if (S_in_SVS.docIdList[begin] < (invertedLists)[i].docIdList[t])// ��������
				break;
		}
		if (isFind) {
			// ����
			pthread_mutex_lock(&mutex);
			S_in_SVS.docIdList[countForSVS++] = S_in_SVS.docIdList[begin];
			pthread_mutex_unlock(&mutex);
		}
	}
Out_SVS_dynamic:
	pthread_exit(NULL);
	return p;
}
InvertedIndex SVS_dynamic(int* queryList, vector<InvertedIndex>& index, int num)
{
	pthread_mutex_init(&mutex, NULL);
	S_in_SVS = index[queryList[0]];// ȡ��̵��б�

	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		countForSVS = 0;// s��ͷ�������һ��

		threadParam_t param[NUM_THREADS];
		pthread_t thread[NUM_THREADS];
		for (int j = 0; j < NUM_THREADS; j++) {// �����̣߳��������
			param[j].t_id = j;
			param[j].currentQuery = queryList[i];

			pthread_create(&thread[j], NULL, dynamicForSVS, &param[j]);
		}
		for (int j = 0; j < NUM_THREADS; j++) {
			pthread_join(thread[j], NULL);
		}

		if (countForSVS < S_in_SVS.docIdList.size())// ������ɾ��
			S_in_SVS.docIdList.erase(S_in_SVS.docIdList.begin() + countForSVS, S_in_SVS.docIdList.end());
	}
	pthread_mutex_destroy(&mutex);
	return S_in_SVS;
}

int* queryList2;
int currentQuery;

void* staticForSVS(void* param) {
	threadParam_t* p = (threadParam_t*)param;
	int t_id = p->t_id;;

	while (true) {// ��̬���̣߳�һֱ������
		sem_wait(&sem_worker[t_id]);// �������ȴ����̻߳���
		if (staticFlag == false)
			break;

		int length = S_in_SVS.docIdList.size() / NUM_THREADS;
		int begin = t_id * length, end = (t_id + 1) * length;
		int t = 0;

		if (length == 0)
			goto Out_SVS_static;

		// Ԥ������ٴ�����¼�
		t = t_id * ((invertedLists)[queryList2[currentQuery]].docIdList.size() / NUM_THREADS);
		for (int j = t_id; j > 0; j--) {
			if (S_in_SVS.docIdList[begin] < (invertedLists)[queryList2[currentQuery]].docIdList[t])
				t -= ((invertedLists)[queryList2[currentQuery]].docIdList.size() / NUM_THREADS);
			else break;
		}

		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = begin; j < end; j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; t < (invertedLists)[queryList2[currentQuery]].docIdList.size(); t++) 
			{
				// ����i�б�������Ԫ��
				if (S_in_SVS.docIdList[j] == (invertedLists)[queryList2[currentQuery]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (S_in_SVS.docIdList[j] < (invertedLists)[queryList2[currentQuery]].docIdList[t])// ��������
					break;
			}
			// TODO:�ٽ�������
			if (isFind) {
				// ����
				pthread_mutex_lock(&mutex);
				S_in_SVS.docIdList[countForSVS++] = S_in_SVS.docIdList[j];
				pthread_mutex_unlock(&mutex);
			}
		}
	Out_SVS_static:
		sem_post(&sem_main);// �������߳�
	}

	pthread_exit(NULL);
	return p;
}

InvertedIndex SVS_static(int* queryList, vector<InvertedIndex>& index, int num) {
	
	S_in_SVS = index[queryList[0]];// ȡ��̵��б�
	queryList2 = queryList;

	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		countForSVS = 0;// s��ͷ�������һ��
		currentQuery = i;

		// ���ѹ����߳�start
		for (int j = 0; j < NUM_THREADS; j++)
			sem_post(&sem_worker[j]);
		// ���߳�˯��
		for (int j = 0; j < NUM_THREADS; j++)
			sem_wait(&sem_main);

		if (countForSVS < S_in_SVS.docIdList.size())// ������ɾ��
			S_in_SVS.docIdList.erase(S_in_SVS.docIdList.begin() + countForSVS, S_in_SVS.docIdList.end());
	}
	return S_in_SVS;
}


void* staticForSVS_barrier(void* param) {
	threadParam_t* p = (threadParam_t*)param;
	int t_id = p->t_id;;

	while (true) {// ��̬���̣߳�һֱ������
		sem_wait(&sem_worker[t_id]);// �������ȴ����̻߳���
		if (staticFlag == false)
			break;
		// ��ʣ���б���
		for (int i = 1; i < numFor; i++) {
			countForSVS = 0;// s��ͷ�������һ��
			currentQuery = i;

			int length = S_in_SVS.docIdList.size() / NUM_THREADS;
			int begin = t_id * length, end = (t_id + 1) * length;
			int t = 0;

			if (length == 0)
				goto Out_SVS_static;

			// Ԥ������ٴ�����¼�
			t = t_id * ((invertedLists)[queryList2[currentQuery]].docIdList.size() / NUM_THREADS);
			for (int j = t_id; j > 0; j--) {
				if (S_in_SVS.docIdList[begin] < (invertedLists)[queryList2[currentQuery]].docIdList[t])
					t -= ((invertedLists)[queryList2[currentQuery]].docIdList.size() / NUM_THREADS);
				else break;
			}

			// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
			for (int j = begin; j < end; j++) {// ����Ԫ�ض��÷���һ��
				bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

				for (; t < (invertedLists)[queryList2[currentQuery]].docIdList.size(); t++) {
					// ����i�б�������Ԫ��
					if (S_in_SVS.docIdList[j] == (invertedLists)[queryList2[currentQuery]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (S_in_SVS.docIdList[j] < (invertedLists)[queryList2[currentQuery]].docIdList[t])// ��������
						break;
				}
				// TODO:�ٽ�������
				if (isFind) {
					// ����
					pthread_mutex_lock(&mutex);
					S_in_SVS.docIdList[countForSVS++] = S_in_SVS.docIdList[j];
					pthread_mutex_unlock(&mutex);
				}
			}
		
			// barrier
			if (t_id == 0) {
				// �߳�0��ɾ������������Ҫ�ȴ������߳�����
				for (int l = 1; l < NUM_THREADS; l++)
					sem_wait(&sem_barrier);
				if (countForSVS < S_in_SVS.docIdList.size())// ������ɾ��
					S_in_SVS.docIdList.erase(S_in_SVS.docIdList.begin() + countForSVS, S_in_SVS.docIdList.end());
			}
			else sem_post(&sem_barrier);
			pthread_barrier_wait(&barrier_wait);// һ�������һ��ѭ��
		}
	Out_SVS_static:
		sem_post(&sem_main);// �������߳�
	}

	pthread_exit(NULL);
	return p;
}

InvertedIndex SVS_static_barrier(int* queryList, vector<InvertedIndex>& index, int num) {

	S_in_SVS = index[queryList[0]];// ȡ��̵��б�
	queryList2 = queryList;
	numFor = num;
	
	// barrier��ʼ��
	sem_init(&sem_barrier, 0, 0);// init
	pthread_barrier_init(&barrier_wait, NULL, NUM_THREADS);

	// ���ѹ����߳�start
	for (int j = 0; j < NUM_THREADS; j++)
		sem_post(&sem_worker[j]);
	// ���߳�˯��
	for (int j = 0; j < NUM_THREADS; j++)
		sem_wait(&sem_main);

	sem_destroy(&sem_barrier);
	pthread_barrier_destroy(&barrier_wait);
		
	return S_in_SVS;
}


void* dynamicForSVS_barrier(void* param) {
	threadParam_t* p = (threadParam_t*)param;
	int t_id = p->t_id;

	// ��ʣ���б���
	for (int i = 1; i < numFor; i++) {
		countForSVS = 0;// s��ͷ�������һ��

		int length = S_in_SVS.docIdList.size() / 4;
		int begin = t_id * length, end = (t_id + 1) * length;
		int t = 0;

		if (length == 0)
			goto Out_SVS_dynamic;

		// Ԥ������ٴ�����¼�
		t = t_id * ((invertedLists)[queryList2[i]].docIdList.size() / NUM_THREADS);
		for (int j = t_id; j > 0; j--) {
			if (S_in_SVS.docIdList[begin] < (invertedLists)[queryList2[i]].docIdList[t])
				t -= ((invertedLists)[queryList2[i]].docIdList.size() / NUM_THREADS);
			else break;
		}

		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (; begin < end; begin++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; t < (invertedLists)[queryList2[i]].docIdList.size(); t++) {
				// ����i�б�������Ԫ��
				if (S_in_SVS.docIdList[begin] == (invertedLists)[queryList2[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (S_in_SVS.docIdList[begin] < (invertedLists)[queryList2[i]].docIdList[t])// ��������
					break;
			}
			if (isFind) {
				// ����
				pthread_mutex_lock(&mutex);
				S_in_SVS.docIdList[countForSVS++] = S_in_SVS.docIdList[begin];
				pthread_mutex_unlock(&mutex);
			}
		}
		// barrier
		if (t_id == 0) {
			// �߳�0��ɾ������������Ҫ�ȴ������߳�����
			for (int l = 1; l < NUM_THREADS; l++)
				sem_wait(&sem_barrier);
			if (countForSVS < S_in_SVS.docIdList.size())// ������ɾ��
				S_in_SVS.docIdList.erase(S_in_SVS.docIdList.begin() + countForSVS, S_in_SVS.docIdList.end());
		}
		else sem_post(&sem_barrier);
		pthread_barrier_wait(&barrier_wait);// һ�������һ��ѭ��
	}
Out_SVS_dynamic:
	pthread_exit(NULL);
	return p;
}
InvertedIndex SVS_dynamic_barrier(int* queryList, vector<InvertedIndex>& index, int num)
{
	pthread_mutex_init(&mutex, NULL);
	S_in_SVS = index[queryList[0]];// ȡ��̵��б�
	queryList2 = queryList;
	numFor = num;
	
	// barrier��ʼ��
	sem_init(&sem_barrier, 0, 0);// init
	pthread_barrier_init(&barrier_wait, NULL, NUM_THREADS);

	threadParam_t param[NUM_THREADS];
	pthread_t thread[NUM_THREADS];
	for (int j = 0; j < NUM_THREADS; j++) {// �����̣߳��������
		param[j].t_id = j;
		pthread_create(&thread[j], NULL, dynamicForSVS_barrier, &param[j]);
	}
	for (int j = 0; j < NUM_THREADS; j++) {
		pthread_join(thread[j], NULL);
	}

	sem_destroy(&sem_barrier);
	pthread_barrier_destroy(&barrier_wait);
		
	pthread_mutex_destroy(&mutex);
	return S_in_SVS;
}