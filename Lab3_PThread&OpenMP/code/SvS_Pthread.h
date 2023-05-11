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

	// 预处理加速大概率事件
	t = t_id * ((invertedLists)[i].docIdList.size() / NUM_THREADS);
	for (int j = t_id; j > 0; j--) {
		if (S_in_SVS.docIdList[begin] < (invertedLists)[i].docIdList[t])
			t -= ((invertedLists)[i].docIdList.size() / NUM_THREADS);
		else break;
	}

	// s列表中的每个元素都拿出来比较
	for (; begin < end; begin++) {// 所有元素都得访问一遍
		bool isFind = false;// 标志，判断当前count位是否能求交

		for (; t < (invertedLists)[i].docIdList.size(); t++) 
		{
			// 遍历i列表中所有元素
			if (S_in_SVS.docIdList[begin] == (invertedLists)[i].docIdList[t]) 
			{
				isFind = true;
				break;
			}
			else if (S_in_SVS.docIdList[begin] < (invertedLists)[i].docIdList[t])// 升序排列
				break;
		}
		if (isFind) {
			// 覆盖
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
	S_in_SVS = index[queryList[0]];// 取最短的列表

	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		countForSVS = 0;// s从头往后遍历一遍

		threadParam_t param[NUM_THREADS];
		pthread_t thread[NUM_THREADS];
		for (int j = 0; j < NUM_THREADS; j++) {// 创建线程，传入参数
			param[j].t_id = j;
			param[j].currentQuery = queryList[i];

			pthread_create(&thread[j], NULL, dynamicForSVS, &param[j]);
		}
		for (int j = 0; j < NUM_THREADS; j++) {
			pthread_join(thread[j], NULL);
		}

		if (countForSVS < S_in_SVS.docIdList.size())// 最后才做删除
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

	while (true) {// 静态多线程，一直做着呢
		sem_wait(&sem_worker[t_id]);// 阻塞，等待主线程唤醒
		if (staticFlag == false)
			break;

		int length = S_in_SVS.docIdList.size() / NUM_THREADS;
		int begin = t_id * length, end = (t_id + 1) * length;
		int t = 0;

		if (length == 0)
			goto Out_SVS_static;

		// 预处理加速大概率事件
		t = t_id * ((invertedLists)[queryList2[currentQuery]].docIdList.size() / NUM_THREADS);
		for (int j = t_id; j > 0; j--) {
			if (S_in_SVS.docIdList[begin] < (invertedLists)[queryList2[currentQuery]].docIdList[t])
				t -= ((invertedLists)[queryList2[currentQuery]].docIdList.size() / NUM_THREADS);
			else break;
		}

		// s列表中的每个元素都拿出来比较
		for (int j = begin; j < end; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < (invertedLists)[queryList2[currentQuery]].docIdList.size(); t++) 
			{
				// 遍历i列表中所有元素
				if (S_in_SVS.docIdList[j] == (invertedLists)[queryList2[currentQuery]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (S_in_SVS.docIdList[j] < (invertedLists)[queryList2[currentQuery]].docIdList[t])// 升序排列
					break;
			}
			// TODO:临界区处理
			if (isFind) {
				// 覆盖
				pthread_mutex_lock(&mutex);
				S_in_SVS.docIdList[countForSVS++] = S_in_SVS.docIdList[j];
				pthread_mutex_unlock(&mutex);
			}
		}
	Out_SVS_static:
		sem_post(&sem_main);// 唤醒主线程
	}

	pthread_exit(NULL);
	return p;
}

InvertedIndex SVS_static(int* queryList, vector<InvertedIndex>& index, int num) {
	
	S_in_SVS = index[queryList[0]];// 取最短的列表
	queryList2 = queryList;

	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		countForSVS = 0;// s从头往后遍历一遍
		currentQuery = i;

		// 唤醒工作线程start
		for (int j = 0; j < NUM_THREADS; j++)
			sem_post(&sem_worker[j]);
		// 主线程睡眠
		for (int j = 0; j < NUM_THREADS; j++)
			sem_wait(&sem_main);

		if (countForSVS < S_in_SVS.docIdList.size())// 最后才做删除
			S_in_SVS.docIdList.erase(S_in_SVS.docIdList.begin() + countForSVS, S_in_SVS.docIdList.end());
	}
	return S_in_SVS;
}


void* staticForSVS_barrier(void* param) {
	threadParam_t* p = (threadParam_t*)param;
	int t_id = p->t_id;;

	while (true) {// 静态多线程，一直做着呢
		sem_wait(&sem_worker[t_id]);// 阻塞，等待主线程唤醒
		if (staticFlag == false)
			break;
		// 与剩余列表求交
		for (int i = 1; i < numFor; i++) {
			countForSVS = 0;// s从头往后遍历一遍
			currentQuery = i;

			int length = S_in_SVS.docIdList.size() / NUM_THREADS;
			int begin = t_id * length, end = (t_id + 1) * length;
			int t = 0;

			if (length == 0)
				goto Out_SVS_static;

			// 预处理加速大概率事件
			t = t_id * ((invertedLists)[queryList2[currentQuery]].docIdList.size() / NUM_THREADS);
			for (int j = t_id; j > 0; j--) {
				if (S_in_SVS.docIdList[begin] < (invertedLists)[queryList2[currentQuery]].docIdList[t])
					t -= ((invertedLists)[queryList2[currentQuery]].docIdList.size() / NUM_THREADS);
				else break;
			}

			// s列表中的每个元素都拿出来比较
			for (int j = begin; j < end; j++) {// 所有元素都得访问一遍
				bool isFind = false;// 标志，判断当前count位是否能求交

				for (; t < (invertedLists)[queryList2[currentQuery]].docIdList.size(); t++) {
					// 遍历i列表中所有元素
					if (S_in_SVS.docIdList[j] == (invertedLists)[queryList2[currentQuery]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (S_in_SVS.docIdList[j] < (invertedLists)[queryList2[currentQuery]].docIdList[t])// 升序排列
						break;
				}
				// TODO:临界区处理
				if (isFind) {
					// 覆盖
					pthread_mutex_lock(&mutex);
					S_in_SVS.docIdList[countForSVS++] = S_in_SVS.docIdList[j];
					pthread_mutex_unlock(&mutex);
				}
			}
		
			// barrier
			if (t_id == 0) {
				// 线程0做删除操作，但需要等待其他线程做完
				for (int l = 1; l < NUM_THREADS; l++)
					sem_wait(&sem_barrier);
				if (countForSVS < S_in_SVS.docIdList.size())// 最后才做删除
					S_in_SVS.docIdList.erase(S_in_SVS.docIdList.begin() + countForSVS, S_in_SVS.docIdList.end());
			}
			else sem_post(&sem_barrier);
			pthread_barrier_wait(&barrier_wait);// 一起进入下一轮循环
		}
	Out_SVS_static:
		sem_post(&sem_main);// 唤醒主线程
	}

	pthread_exit(NULL);
	return p;
}

InvertedIndex SVS_static_barrier(int* queryList, vector<InvertedIndex>& index, int num) {

	S_in_SVS = index[queryList[0]];// 取最短的列表
	queryList2 = queryList;
	numFor = num;
	
	// barrier初始化
	sem_init(&sem_barrier, 0, 0);// init
	pthread_barrier_init(&barrier_wait, NULL, NUM_THREADS);

	// 唤醒工作线程start
	for (int j = 0; j < NUM_THREADS; j++)
		sem_post(&sem_worker[j]);
	// 主线程睡眠
	for (int j = 0; j < NUM_THREADS; j++)
		sem_wait(&sem_main);

	sem_destroy(&sem_barrier);
	pthread_barrier_destroy(&barrier_wait);
		
	return S_in_SVS;
}


void* dynamicForSVS_barrier(void* param) {
	threadParam_t* p = (threadParam_t*)param;
	int t_id = p->t_id;

	// 与剩余列表求交
	for (int i = 1; i < numFor; i++) {
		countForSVS = 0;// s从头往后遍历一遍

		int length = S_in_SVS.docIdList.size() / 4;
		int begin = t_id * length, end = (t_id + 1) * length;
		int t = 0;

		if (length == 0)
			goto Out_SVS_dynamic;

		// 预处理加速大概率事件
		t = t_id * ((invertedLists)[queryList2[i]].docIdList.size() / NUM_THREADS);
		for (int j = t_id; j > 0; j--) {
			if (S_in_SVS.docIdList[begin] < (invertedLists)[queryList2[i]].docIdList[t])
				t -= ((invertedLists)[queryList2[i]].docIdList.size() / NUM_THREADS);
			else break;
		}

		// s列表中的每个元素都拿出来比较
		for (; begin < end; begin++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < (invertedLists)[queryList2[i]].docIdList.size(); t++) {
				// 遍历i列表中所有元素
				if (S_in_SVS.docIdList[begin] == (invertedLists)[queryList2[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (S_in_SVS.docIdList[begin] < (invertedLists)[queryList2[i]].docIdList[t])// 升序排列
					break;
			}
			if (isFind) {
				// 覆盖
				pthread_mutex_lock(&mutex);
				S_in_SVS.docIdList[countForSVS++] = S_in_SVS.docIdList[begin];
				pthread_mutex_unlock(&mutex);
			}
		}
		// barrier
		if (t_id == 0) {
			// 线程0做删除操作，但需要等待其他线程做完
			for (int l = 1; l < NUM_THREADS; l++)
				sem_wait(&sem_barrier);
			if (countForSVS < S_in_SVS.docIdList.size())// 最后才做删除
				S_in_SVS.docIdList.erase(S_in_SVS.docIdList.begin() + countForSVS, S_in_SVS.docIdList.end());
		}
		else sem_post(&sem_barrier);
		pthread_barrier_wait(&barrier_wait);// 一起进入下一轮循环
	}
Out_SVS_dynamic:
	pthread_exit(NULL);
	return p;
}
InvertedIndex SVS_dynamic_barrier(int* queryList, vector<InvertedIndex>& index, int num)
{
	pthread_mutex_init(&mutex, NULL);
	S_in_SVS = index[queryList[0]];// 取最短的列表
	queryList2 = queryList;
	numFor = num;
	
	// barrier初始化
	sem_init(&sem_barrier, 0, 0);// init
	pthread_barrier_init(&barrier_wait, NULL, NUM_THREADS);

	threadParam_t param[NUM_THREADS];
	pthread_t thread[NUM_THREADS];
	for (int j = 0; j < NUM_THREADS; j++) {// 创建线程，传入参数
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