#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include"InvertedIndex.h"
#include"Adp.h"
#include <pthread.h>
#include <semaphore.h>
#include"ThreadParam.h"
using namespace std;
extern pthread_mutex_t mutex;
extern int numFor;
extern bool staticFlag;
extern sem_t sem_main, sem_worker[NUM_THREADS];
// adp_dynamic
InvertedIndex S_in_ADP;
void* dynamicForADP(void* param) 
{
	threadParam_t* p = (threadParam_t*)param;
	int t_id = p->t_id;
	int num = p->num;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 复制一份list，后续cursor++同步起来费劲
		list[i] = (p->list)[i];

	int length = list[0].length / NUM_THREADS;
	int begin = t_id * length, end = (t_id + 1) * length;// 把list[0]拆成4部分

	for (int i = 1; i < num; i++)
	{// 预处理cursor，加速大概率事件
		list[i].cursor = t_id * (list[i].length / NUM_THREADS);
		for (int j = t_id; j > 0; j--) {
			if ((invertedLists)[list[0].key].docIdList[begin] < (invertedLists)[list[i].key].docIdList[list[i].cursor])
				list[i].cursor -= list[i].length / NUM_THREADS;
			else break;
		}
	}


	for (; begin < end; begin++) {
		bool isFind = true;
		unsigned int e = (invertedLists)[list[0].key].docIdList[begin];
		for (int s = 1; s != num; s++) {
			isFind = false;
			for (; list[s].cursor < list[s].length; list[s].cursor++) {// 检查s列表
				if (e == (invertedLists)[list[s].key].docIdList[list[s].cursor]) {
					isFind = true;
					break;
				}
				else if (e < (invertedLists)[list[s].key].docIdList[list[s].cursor])
					break;
				// 当前访问过，且没找到合适的，往后移
			}
			if (!isFind)break;
		}
		// 临界区 是吗？
		// 当前元素已被访问过
		if (isFind) {
			pthread_mutex_lock(&mutex);
			S_in_ADP.docIdList.push_back(e);
			pthread_mutex_unlock(&mutex);
		}
	}
	pthread_exit(NULL);
	return p;
}
// adp实现
InvertedIndex ADP_dynamic(int* queryList, vector<InvertedIndex>& index, int num)
{
	S_in_ADP.docIdList.clear();
	pthread_mutex_init(&mutex, NULL);

	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].docIdList.size();
	}

	threadParam_t param[NUM_THREADS];
	pthread_t thread[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++) {// 创建线程，传入参数
		param[i].t_id = i;
		param[i].num = num;
		param[i].list = list;
		pthread_create(&thread[i], NULL, dynamicForADP, &param[i]);
	}
	for (int i = 0; i < NUM_THREADS; i++) 
		pthread_join(thread[i], NULL);
	
	pthread_mutex_destroy(&mutex);
	return S_in_ADP;
}

QueryItem* queryListFor4;

void* staticForADP(void* param) {
	threadParam_t* p = (threadParam_t*)param;
	int t_id = p->t_id;

	while (true) 
	{
		// 静态多线程，一直做着
		sem_wait(&sem_worker[t_id]);// 阻塞，等待主线程唤醒
		if (staticFlag == false)
			break;

		QueryItem* list = new QueryItem[numFor]();
		for (int i = 0; i < numFor; i++)// 预处理
			list[i] = queryListFor4[i];

		int length = list[0].length / NUM_THREADS;
		int begin = t_id * length, end = (t_id + 1) * length;// 把list[0]拆成4部分
		for (int i = 1; i < numFor; i++)
		{// 预处理cursor，加速大概率事件
			list[i].cursor = t_id * (list[i].length / NUM_THREADS);
			for (int j = t_id; j > 0; j--) {
				if ((invertedLists)[list[0].key].docIdList[begin] < (invertedLists)[list[i].key].docIdList[list[i].cursor])
					list[i].cursor -= list[i].length / NUM_THREADS;
				else break;
			}
		}

		for (; begin < end; begin++) {
			bool isFind = true;
			unsigned int e = (invertedLists)[list[0].key].docIdList[begin];
			for (int s = 1; s != numFor; s++) {
				isFind = false;
				for (; list[s].cursor < list[s].length; list[s].cursor++) {// 检查s列表
					if (e == (invertedLists)[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < (invertedLists)[list[s].key].docIdList[list[s].cursor])
						break;
					// 当前访问过，且没找到合适的，往后移
				}
				if (!isFind)break;
			}
			// 临界区 是吗？
			// 当前元素已被访问过
			if (isFind) {
				pthread_mutex_lock(&mutex);
				S_in_ADP.docIdList.push_back(e);
				pthread_mutex_unlock(&mutex);
			}
		}
		sem_post(&sem_main);// 唤醒主线程
	}
	pthread_exit(NULL);
	return p;
}
InvertedIndex ADP_static(int* queryList, vector<InvertedIndex>& index, int num)
{
	S_in_ADP.docIdList.clear();
	numFor = num;
	task = 0;

	QueryItem* list = new QueryItem[num]();
	
	// 预处理
	for (int i = 0; i < num; i++)
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].docIdList.size();
	}
	queryListFor4 = list;
	subWait = false;

	// 唤醒工作线程start
	for (int j = 0; j < NUM_THREADS; j++)
		sem_post(&sem_worker[j]);
	// 主线程睡眠
	for (int j = 0; j < NUM_THREADS; j++)
		sem_wait(&sem_main);

	return S_in_ADP;
}

void* staticForADP_TaskPool(void* param) {
	threadParam_t* p = (threadParam_t*)param;
	int t_id = p->t_id;
	while (true) 
	{
		// 静态多线程，一直做着
		sem_wait(&sem_worker[t_id]);// 阻塞，等待主线程唤醒
		if (staticFlag == false)
			break;

		QueryItem* list = new QueryItem[numFor]();
		for (int i = 0; i < numFor; i++)// 预处理
			list[i] = queryListFor4[i];
		int begin = 0;
		int end = 0;
		int length = 
			list[0].length>=300?list[0].length/ (list[0].length/300):
			list[0].length>4?list[0].length/4:list[0].length;
		//cout << list[0].length << endl;
		while (end < list[0].length)
		{
			pthread_mutex_lock(&mutex);
			begin = length * task, end = min(length * (task + 1),list[0].length);// 把list[0]拆成若干部分
			task++;
			pthread_mutex_unlock(&mutex);
			if (begin > list[0].length)
				break;
			for (; begin < end; begin++) {
				bool isFind = true;
				unsigned int e = invertedLists[list[0].key].docIdList[begin];
				for (int s = 1; s != numFor; s++) {
					isFind = false;
					for (; list[s].cursor < list[s].length; list[s].cursor++) {// 检查s列表
						if (e == (invertedLists)[list[s].key].docIdList[list[s].cursor]) {
							isFind = true;
							break;
						}
						else if (e < (invertedLists)[list[s].key].docIdList[list[s].cursor])
							break;
						// 当前访问过，且没找到合适的，往后移
					}
					if (!isFind)break;
				}
				// 临界区 是吗？
				// 当前元素已被访问过
				if (isFind) {
					pthread_mutex_lock(&mutex);
					S_in_ADP.docIdList.push_back(e);
					pthread_mutex_unlock(&mutex);
				}
			}
		}
		sem_post(&sem_main);// 唤醒主线程
	}
	pthread_exit(NULL);
	return p;
}