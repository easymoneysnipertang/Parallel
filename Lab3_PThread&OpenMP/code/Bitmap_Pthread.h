#pragma once
#include"Bitmap.h"
#include<string>
#include<vector>
#include<algorithm>
#include"InvertedIndex.h"
#include"Adp.h"
#include <pthread.h>
#include <semaphore.h>
#include"ThreadParam.h"
extern pthread_mutex_t mutex;
extern int numFor;
extern bool staticFlag;
extern BitMap bitmapList[2000];//开空间
extern BitMap chosenList;
extern sem_t sem_main, sem_worker[NUM_THREADS];
int* qList;
void* dynamicForBITMAP(void* param) 
{
	threadParam_t* par = (threadParam_t*)param;
	int t_id = par->t_id;
	//二级索引的赋值起始点和结束点
	int skipSec = bitmapList[qList[0]].secondIndex.size() / NUM_THREADS;
	int startSec = skipSec * t_id;
	int endSec = skipSec * (t_id + 1);

	//一级索引的赋值起始点和结束点
	int skipFir = bitmapList[qList[0]].firstIndex.size() / NUM_THREADS;
	int startFir = skipFir * t_id;
	int endFir = skipFir * (t_id + 1);

	//索引的赋值起始点和结束点
	int skipBit = bitmapList[qList[0]].bits.size() / NUM_THREADS;
	int startBit = skipBit * t_id;
	int endBit = skipBit * (t_id + 1);

	//迭代器的真正结束点
	vector<int>::iterator trueEndSec = bitmapList[qList[0]].secondIndex.end();
	vector<int>::iterator trueEndFir = bitmapList[qList[0]].firstIndex.end();
	vector<int>::iterator trueEndBit = bitmapList[qList[0]].bits.end();

	//子线程迭代器的结束点
	vector<int>::iterator EndSec = t_id + 1 == NUM_THREADS ? trueEndSec : bitmapList[qList[0]].secondIndex.begin() + endSec;
	vector<int>::iterator EndFir = t_id + 1 == NUM_THREADS ? trueEndFir : bitmapList[qList[0]].firstIndex.begin() + endFir;
	vector<int>::iterator EndBit = t_id + 1 == NUM_THREADS ? trueEndBit : bitmapList[qList[0]].bits.begin() + endBit;

	copy(bitmapList[qList[0]].secondIndex.begin()+startSec,EndSec, chosenList.secondIndex.begin() + startSec);
	copy(bitmapList[qList[0]].firstIndex.begin()+startFir, EndFir, chosenList.firstIndex.begin() + startFir);
	copy(bitmapList[qList[0]].bits.begin()+startBit, EndBit, chosenList.bits.begin() + startBit);
	pthread_exit(NULL);
	return NULL;
}

InvertedIndex BITMAP_dynamic(int* list, vector<InvertedIndex>& idx, int num)
{
	//预处理
	qList = list;

	//创建线程,进行赋值
	threadParam_t param[NUM_THREADS];
	pthread_t thread[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++) // 创建线程，传入参数
	{
		param[i].t_id = i;
		param[i].num = num;
		pthread_create(&thread[i], NULL, dynamicForBITMAP, &param[i]);
	}
	for (int i = 0; i < NUM_THREADS; i++)
		pthread_join(thread[i], NULL);

	InvertedIndex res;
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < chosenList.secondIndex.size(); j++)
		{
			chosenList.secondIndex[j] &= bitmapList[list[i]].secondIndex[j];//按位与
			if (chosenList.secondIndex[j])
			{
				for (int t = j * 32; t < j * 32 + 32; t++)
				{
					chosenList.firstIndex[t] &= bitmapList[list[i]].firstIndex[t];//按位与
					if (chosenList.firstIndex[t])
					{
						for (int l = t * 32; l < t * 32 + 32; l++)
						{
							chosenList.bits[l] &= bitmapList[list[i]].bits[l];//按位与
							if (i == num - 1 && chosenList.bits[l])
							{
								int bit = chosenList.bits[l];//取出该段
								for (int m = 0; m < 32; m++)
								{
									if ((bit & 1) != 0)
										res.docIdList.push_back(32 * l + m);
									bit = bit >> 1;
								}
							}
						}
					}
				}
			}
		}
	}
	return res;
}

void* staticForBITMAP(void* param)
{
	threadParam_t* par = (threadParam_t*)param;
	int t_id = par->t_id;
	while (true)
	{
		sem_wait(&sem_worker[t_id]);// 阻塞，等待主线程唤醒
		if (staticFlag == false)
			break;
		//二级索引的赋值起始点和结束点
		int skipSec = bitmapList[qList[0]].secondIndex.size() / NUM_THREADS;
		int startSec = skipSec * t_id;
		int endSec = skipSec * (t_id + 1);

		//一级索引的赋值起始点和结束点
		int skipFir = bitmapList[qList[0]].firstIndex.size() / NUM_THREADS;
		int startFir = skipFir * t_id;
		int endFir = skipFir * (t_id + 1);

		//索引的赋值起始点和结束点
		int skipBit = bitmapList[qList[0]].bits.size() / NUM_THREADS;
		int startBit = skipBit * t_id;
		int endBit = skipBit * (t_id + 1);

		//迭代器的真正结束点
		vector<int>::iterator trueEndSec = bitmapList[qList[0]].secondIndex.end();
		vector<int>::iterator trueEndFir = bitmapList[qList[0]].firstIndex.end();
		vector<int>::iterator trueEndBit = bitmapList[qList[0]].bits.end();

		//子线程迭代器的结束点
		vector<int>::iterator EndSec = t_id + 1 == NUM_THREADS ? trueEndSec : bitmapList[qList[0]].secondIndex.begin() + endSec;
		vector<int>::iterator EndFir = t_id + 1 == NUM_THREADS ? trueEndFir : bitmapList[qList[0]].firstIndex.begin() + endFir;
		vector<int>::iterator EndBit = t_id + 1 == NUM_THREADS ? trueEndBit : bitmapList[qList[0]].bits.begin() + endBit;

		copy(bitmapList[qList[0]].secondIndex.begin() + startSec, EndSec, chosenList.secondIndex.begin() + startSec);
		copy(bitmapList[qList[0]].firstIndex.begin() + startFir, EndFir, chosenList.firstIndex.begin() + startFir);
		copy(bitmapList[qList[0]].bits.begin() + startBit, EndBit, chosenList.bits.begin() + startBit);

		sem_post(&sem_main);// 唤醒主线程
	}
	pthread_exit(NULL);
	return NULL;
}

InvertedIndex BITMAP_static(int* list, vector<InvertedIndex>& idx, int num)
{
	//子线程所需变量
	numFor = num;
	task = 0;
	subWait = false;
	qList = list;

	// 唤醒工作线程start
	for (int j = 0; j < NUM_THREADS; j++)
		sem_post(&sem_worker[j]);
	// 主线程睡眠
	for (int j = 0; j < NUM_THREADS; j++)
		sem_wait(&sem_main);
	InvertedIndex res;
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < chosenList.secondIndex.size(); j++)
		{
			chosenList.secondIndex[j] &= bitmapList[list[i]].secondIndex[j];//按位与
			if (chosenList.secondIndex[j])
			{
				for (int t = j * 32; t < j * 32 + 32; t++)
				{
					chosenList.firstIndex[t] &= bitmapList[list[i]].firstIndex[t];//按位与
					if (chosenList.firstIndex[t])
					{
						for (int l = t * 32; l < t * 32 + 32; l++)
						{
							chosenList.bits[l] &= bitmapList[list[i]].bits[l];//按位与
							if (i == num - 1 && chosenList.bits[l])
							{
								int bit = chosenList.bits[l];//取出该段
								for (int m = 0; m < 32; m++)
								{
									if ((bit & 1) != 0)
										res.docIdList.push_back(32 * l + m);
									bit = bit >> 1;
								}
							}
						}
					}
				}
			}
		}
	}
	return res;
}

void* staticForBITMAP_TaskPool(void* param)
{
	threadParam_t* par = (threadParam_t*)param;
	int t_id = par->t_id;
	int skip = 0;
	int start = 0;
	int end ;
	while (true)
	{

		if (subWait||start>= bitmapList[0].secondIndex.size())//说明任务已经完成
		{	
			if(!subWait)
				sem_post(&sem_main);// 唤醒主线程
			sem_wait(&sem_worker[t_id]);
			if (staticFlag == false)
				break;
		}
		skip = bitmapList[0].secondIndex.size()/10 ;

		pthread_mutex_lock(&mutex);
		start = skip * task;
		task++;
		end = skip * task > bitmapList[0].secondIndex.size() ? bitmapList[0].secondIndex.size() : skip * task;
		pthread_mutex_unlock(&mutex);

		for (int i = 1; i < numFor; i++)
		{
			for (int j = start; j < end; j++)
			{
				chosenList.secondIndex[j] &= bitmapList[qList[i]].secondIndex[j];//按位与
				if (chosenList.secondIndex[j])
				{
					for (int t = j * 32; t < j * 32 + 32; t++)
					{
						chosenList.firstIndex[t] &= bitmapList[qList[i]].firstIndex[t];//按位与
						if (chosenList.firstIndex[t])
						{
							for (int l = t * 32; l < t * 32 + 32; l++)
								chosenList.bits[l] &= bitmapList[qList[i]].bits[l];//按位与
						}
					}
				}
			}
		}
	}

	pthread_exit(NULL);
	return NULL;
}