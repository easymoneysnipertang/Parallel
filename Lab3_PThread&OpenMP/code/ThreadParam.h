#pragma once
#include"Adp.h"
#include"util.h"
using namespace std;
#define NUM_THREADS 4
bool subWait;
int numFor;//query��list��Ԫ����������̬����
bool staticFlag;//��̬�㷨�˳���־
pthread_mutex_t mutex;//��

int task;//��������ڵ�����ָ��

sem_t sem_main, sem_worker[NUM_THREADS];
struct threadParam_t 
{
	int t_id;
	QueryItem* list;
	int num;//��ѯ�е�����
	int currentQuery;//��ǰ��ѯ
};