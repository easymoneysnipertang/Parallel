#pragma once
#include"Adp.h"
#include"util.h"
using namespace std;
#define NUM_THREADS 4
bool subWait;
int numFor;//query中list的元素数量，静态必须
bool staticFlag;//静态算法退出标志
pthread_mutex_t mutex;//锁

int task;//任务池现在的任务指针

sem_t sem_main, sem_worker[NUM_THREADS];
struct threadParam_t 
{
	int t_id;
	QueryItem* list;
	int num;//查询中的数量
	int currentQuery;//当前查询
};