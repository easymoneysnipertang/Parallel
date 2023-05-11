#include <iostream>
#include<fstream>
#include<string>
#include<vector>
#include<sstream>
#include<Windows.h>
#include<algorithm>
#include <smmintrin.h>
#include <nmmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>
#include <immintrin.h>
#include <omp.h>
#include <semaphore.h>
#include <pthread.h>
using namespace std;

#define NUM_THREADS 4
bool isParallel_out = false;
bool isParallel_in = true;

class InvertedIndex {// 倒排索引结构
public:
	int length = 0;
	vector<unsigned int> docIdList;
	
};
// 重载比较符，以长度排序各个索引
bool operator<(const InvertedIndex& i1, const InvertedIndex& i2) {
	return i1.length < i2.length;
}

static int query[1000][5] = { 0 };// 单个查询最多5个docId,全部读取
static vector<InvertedIndex>* invertedLists = new vector<InvertedIndex>();
pthread_mutex_t mutex;
sem_t sem_main, sem_worker[NUM_THREADS];
bool flag = true;

// 把倒排列表按长度排序
void sorted(int* list, vector<InvertedIndex>& idx, int num) {
	for (int i = 0; i < num - 1; i++) {
		for (int j = 0; j < num - i - 1; j++) {
			if (idx[list[j]].length > idx[list[j + 1]].length) {
				int tmp = list[j];
				list[j] = list[j + 1];
				list[j + 1] = tmp;
			}
		}
	}
}
// svs实现
InvertedIndex SVS(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 取最短的列表
	int count = 0;// s从头往后遍历一遍

	// 与剩余列表求交
#pragma omp parallel if(isParallel_in),num_threads(NUM_THREADS),shared(count)
	for (int i = 1; i < num; i++) {
		count = 0;// s从头往后遍历一遍
		int t = 0;

		// s列表中的每个元素都拿出来比较
#pragma omp for 
		for (int j = 0; j < s.length; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < index[queryList[i]].length; t++) {
				// 遍历i列表中所有元素
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
					break;
			}
#pragma omp critical
			if (isFind)// 覆盖
				s.docIdList[count++] = s.docIdList[j];
		}
#pragma omp single
		{
			if (count < s.length)// 最后才做删除
				s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
			s.length = count;
		}

	}
	return s;
}


// hash分段实现
class HashBucket {// 一个hash段，记录他在倒排列表中的起始位置
public:
	int begin = -1;
	int end = -1;
};
HashBucket** hashBucket;
// 预处理，将倒排列表放入hash段里
void preprocessing(vector<InvertedIndex>& index, int count) {
	hashBucket = new HashBucket * [count];
	for (int i = 0; i < count; i++)
		hashBucket[i] = new HashBucket[102000];// 26000000/512->256!v

	for (int i = 0; i < count; i++) {
		for (int j = 0; j < index[i].length; j++) {
			int value = index[i].docIdList[j];// 拿出当前列表第j个值
			int hashValue = value / 256;// TODO:check
			if (hashBucket[i][hashValue].begin == -1) // 该段内还没有元素
				hashBucket[i][hashValue].begin = j;
			hashBucket[i][hashValue].end = j;// 添加到该段尾部
		}
	}
}
InvertedIndex HASH(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 选出最短的倒排列表
	for (int i = 1; i < num; i++) {// 与剩余列表求交
		// SVS的思想，将s挨个按表求交
		int count = 0;
		int length = s.length;

		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 256;
			// 找到该hash值在当前待求交列表中对应的段
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// 该值肯定找不到，不用往后做了
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// 在该段中找到了当前值
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
						break;
					}
				}
				if (isFind) {
					// 覆盖
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
		if (count < length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}
	return s;
}




// adp实现
class QueryItem {
public:
	int cursor;// 当前读到哪了
	int length;// 倒排索引总长度
	int key;// 关键字值
};
bool operator<(const QueryItem& q1, const QueryItem& q2) {// 选剩余元素最少的元素
	return (q1.length - q1.cursor) < (q2.length - q2.cursor);
}
InvertedIndex ADP(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex S;
	
#pragma omp parallel if(isParallel_in),num_threads(NUM_THREADS/2)
	{
		QueryItem* list = new QueryItem[num]();// 每个线程都整一个list，然后我一个个取
		for (int i = 0; i < num; i++)// 预处理
		{
			list[i].cursor = 0;
			list[i].key = queryList[i];
			list[i].length = index[queryList[i]].length;
		}
#pragma omp for schedule(dynamic,1)
		for (int i = list[0].cursor; i < list[0].length; i++) {// 最短的列表非空
			bool isFind = true;
			unsigned int e = index[list[0].key].docIdList[i];
			for (int s=1; s != num && isFind == true; s++) {
				isFind = false;
				while (list[s].cursor < list[s].length) {// 检查s列表
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;

					list[s].cursor++;// 当前访问过，且没找到合适的，往后移
				}

			}
#pragma omp critical
			if (isFind) {
				S.docIdList.push_back(e);
				S.length++;

			}
		}
	}
	return S;
}

InvertedIndex S_in_ADP;

struct threadParam_t4 {
	int t_id;
};
threadParam_t4 param4[NUM_THREADS];

int numFor4;
QueryItem* queryListFor4;

void* threadFunc4(void* param) {
	threadParam_t4* p = (threadParam_t4*)param;
	int t_id = p->t_id;

	while (true) {// 静态多线程，一直做着呢
		sem_wait(&sem_worker[t_id]);// 阻塞，等待主线程唤醒
		if (flag == false)break;

		QueryItem* list = new QueryItem[numFor4]();
		for (int i = 0; i < numFor4; i++)// 预处理
			list[i] = queryListFor4[i];

		int length = list[0].length / NUM_THREADS;
		int begin = t_id * length, end = (t_id + 1) * length;// 把list[0]拆成4部分

		for (int i = 1; i < numFor4; i++)
		{// 预处理cursor，加速大概率事件
			list[i].cursor = t_id * (list[i].length / NUM_THREADS);
			for (int j = t_id; j > 0; j--) {
				if ((*invertedLists)[list[0].key].docIdList[begin] < (*invertedLists)[list[i].key].docIdList[list[i].cursor])
					list[i].cursor -= list[i].length / NUM_THREADS;
				else break;
			}
			//for (int j = numOfThreads; j > 0; j--) {
			//	if ((*invertedLists)[list[0].key].docIdList[begin] < (*invertedLists)[list[i].key].docIdList[list[i].cursor]) {
			//		list[i].cursor /= 2;// 折半查找
			//		if (j == 1)list[i].cursor = 0;// 没办法了，只能从头开找了
			//	}
			//	else break;
			//}
		}

		for (; begin < end; begin++) {
			bool isFind = true;
			unsigned int e = (*invertedLists)[list[0].key].docIdList[begin];
			for (int s = 1; s != numFor4; s++) {
				isFind = false;
				for (; list[s].cursor < list[s].length; list[s].cursor++) {// 检查s列表
					if (e == (*invertedLists)[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < (*invertedLists)[list[s].key].docIdList[list[s].cursor])
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
	numFor4 = num;
	
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}
	queryListFor4 = list;

	// 唤醒工作线程start
	for (int j = 0; j < NUM_THREADS; j++)
		sem_post(&sem_worker[j]);
	// 主线程睡眠
	for (int j = 0; j < NUM_THREADS; j++)
		sem_wait(&sem_main);
	
	return S_in_ADP;
}


struct threadParam_t3 {
	int t_id;
	QueryItem *list;
	int num;
};
threadParam_t3 param3[NUM_THREADS];

// adp_dynamic
void* threadFunc3(void* param) {
	threadParam_t3* p = (threadParam_t3*)param;
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
			if ((*invertedLists)[list[0].key].docIdList[begin] < (*invertedLists)[list[i].key].docIdList[list[i].cursor])
				list[i].cursor -= list[i].length / NUM_THREADS;
			else break;
		}
	}

	for (; begin < end; begin++) {
		bool isFind = true;
		unsigned int e = (*invertedLists)[list[0].key].docIdList[begin];
		for (int s = 1; s != num; s++) {
			isFind = false;
			for (; list[s].cursor < list[s].length; list[s].cursor++) {// 检查s列表
				if (e == (*invertedLists)[list[s].key].docIdList[list[s].cursor]) {
					isFind = true;
					break;
				}
				else if (e < (*invertedLists)[list[s].key].docIdList[list[s].cursor])
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
;
	pthread_exit(NULL);
	return p;
}
InvertedIndex ADP_dynamic(int* queryList, vector<InvertedIndex>& index, int num)
{
	S_in_ADP.docIdList.clear();
	
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}

	pthread_t thread[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++) {// 创建线程，传入参数
		param3[i].t_id = i;
		param3[i].num = num;
		param3[i].list = list;
		pthread_create(&thread[i], NULL, threadFunc3, &param3[i]);
	}
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(thread[i], NULL);
	}

	return S_in_ADP;
}


int countForSVS;
InvertedIndex S_in_SVS;
sem_t sem_barrier;
pthread_barrier_t barrier_wait;

struct threadParam_t1 {
	int t_id;
	int* list;
	int num;
};
threadParam_t1 param1[NUM_THREADS];


void* threadFunc1(void* param) {
	threadParam_t1* p = (threadParam_t1*)param;
	int t_id = p->t_id;
	int num = p->num;
	int* list = p->list;
	int length = S_in_SVS.length / NUM_THREADS;
	int begin = t_id * length, end = (t_id + 1) * length;

	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		countForSVS = 0;// s从头往后遍历一遍

		int t = 0;
		if (t == 0)goto outSVS_dynamic;
		// 预处理加速大概率事件
		t = t_id * ((*invertedLists)[list[i]].length / NUM_THREADS);
		for (int j = t_id; j > 0; j--) {
			if (S_in_SVS.docIdList[begin] < (*invertedLists)[list[i]].docIdList[t])
				t -= ((*invertedLists)[list[i]].length / NUM_THREADS);
			else break;
		}

		// s列表中的每个元素都拿出来比较
		for (int j=begin; j < end; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < (*invertedLists)[list[i]].length; t++) {
				// 遍历i列表中所有元素
				if (S_in_SVS.docIdList[j] == (*invertedLists)[list[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (S_in_SVS.docIdList[j] < (*invertedLists)[list[i]].docIdList[t])// 升序排列
					break;
			}
			if (isFind) {
				// 覆盖
				pthread_mutex_lock(&mutex);
				S_in_SVS.docIdList[countForSVS++] = S_in_SVS.docIdList[j];
				pthread_mutex_unlock(&mutex);
			}

		}

		if (t_id == 0) {
			// barrier 一个人做
			for (int l = 1; l < NUM_THREADS; l++)
				sem_wait(&sem_barrier);
			if (countForSVS < S_in_SVS.length)// 最后才做删除
				S_in_SVS.docIdList.erase(S_in_SVS.docIdList.begin() + countForSVS, S_in_SVS.docIdList.end());
			S_in_SVS.length = countForSVS;
		}
		else
			sem_post(&sem_barrier);
		pthread_barrier_wait(&barrier_wait);
	}

outSVS_dynamic:
	pthread_exit(NULL);
	return p;
}
InvertedIndex SVS_dynamic(int* queryList, vector<InvertedIndex>& index, int num)
{
	S_in_SVS = index[queryList[0]];// 取最短的列表
		
	pthread_t thread[NUM_THREADS];
	sem_init(&sem_barrier, 0, 0);// init
	pthread_barrier_init(&barrier_wait, NULL, NUM_THREADS);
	for (int j = 0; j < NUM_THREADS; j++) {// 创建线程，传入参数
		param1[j].t_id = j;
		param1[j].num = num;
		param1[j].list = queryList;

		pthread_create(&thread[j], NULL, threadFunc1, &param1[j]);
	}
	for (int j = 0; j < NUM_THREADS; j++) {
		pthread_join(thread[j], NULL);
	}
	sem_destroy(&sem_barrier);
	pthread_barrier_destroy(&barrier_wait);
	return S_in_SVS;
}


typedef struct {
	int t_id;
}threadParam_t2;
threadParam_t2 param2[NUM_THREADS];

int* queryList2;
int numFor2;

void* threadFunc2(void* param) {
	threadParam_t2* p = (threadParam_t2*)param;
	int t_id = p->t_id;;

	while (true) {// 静态多线程，一直做着呢
		sem_wait(&sem_worker[t_id]);// 阻塞，等待主线程唤醒
		if (flag == false)break;
		// 与剩余列表求交
		for (int i = 1; i < numFor2; i++) {
			countForSVS = 0;// s从头往后遍历一遍
			int length = S_in_SVS.docIdList.size() / NUM_THREADS;
			int begin = t_id * length, end = (t_id + 1) * length;
			int t = 0;
			if (length == 0)goto outSVS_static;
			// 预处理加速大概率事件
			t = t_id * ((*invertedLists)[queryList2[i]].length / NUM_THREADS);
			for (int j = t_id; j > 0; j--) {
				if (S_in_SVS.docIdList[begin] < (*invertedLists)[queryList2[i]].docIdList[t]) //TODO:这句话让下标超了？
					t -= ((*invertedLists)[queryList2[i]].length / NUM_THREADS);
				else break;
			}

			// s列表中的每个元素都拿出来比较
			for (int j = begin; j < end; j++) {// 所有元素都得访问一遍
				bool isFind = false;// 标志，判断当前count位是否能求交

				for (; t < (*invertedLists)[queryList2[i]].length; t++) {
					// 遍历i列表中所有元素
					if (S_in_SVS.docIdList[j] == (*invertedLists)[queryList2[i]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (S_in_SVS.docIdList[j] < (*invertedLists)[queryList2[i]].docIdList[t])// 升序排列
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
				// barrier 一个人做
				for (int l = 1; l < NUM_THREADS; l++)
					sem_wait(&sem_barrier);
				if (countForSVS < S_in_SVS.length)// 最后才做删除
					S_in_SVS.docIdList.erase(S_in_SVS.docIdList.begin() + countForSVS, S_in_SVS.docIdList.end());
				S_in_SVS.length = countForSVS;
			}
			else sem_post(&sem_barrier);
			pthread_barrier_wait(&barrier_wait);

		}
	outSVS_static:
		sem_post(&sem_main);// 唤醒主线程
	}

	pthread_exit(NULL);
	return p;
}

InvertedIndex SVS_static(int* queryList, vector<InvertedIndex>& index, int num) {
	S_in_SVS = index[queryList[0]];// 取最短的列表
	queryList2 = queryList;
	numFor2 = num;

	// 唤醒工作线程start
	sem_init(&sem_barrier, 0, 0);// init
	pthread_barrier_init(&barrier_wait, NULL, NUM_THREADS);
	for (int j = 0; j < NUM_THREADS; j++)
		sem_post(&sem_worker[j]);
	// 主线程睡眠
	for (int j = 0; j < NUM_THREADS; j++)
		sem_wait(&sem_main);
	sem_destroy(&sem_barrier);
	pthread_barrier_destroy(&barrier_wait);
		
	return S_in_SVS;
}

//typedef struct {
//	int t_id;
//	int queryCount;
//}threadParam_t1;
//
//void* threadFunc1(void* param) {
//	// 动态query间并行
//	threadParam_t1* p = (threadParam_t1*)param;
//	int t_id = p->t_id;
//	int count = p->queryCount / 4;
//	int begin = t_id * count, end = (t_id + 1) * count;
//	for (int i = begin; i < end; i++) {
//			int num = 0;// query查询项数
//			for (int j = 0; j < 5; j++) {
//				if (query[i][j] != 0) {
//					num++;
//				}
//			}
//			int* list = new int[num];// 要传入的list
//			for (int j = 0; j < num; j++) {
//				list[j] = query[i][j];
//			}
//			sorted(list, *(invertedLists), num);// 按长度排序
//			InvertedIndex res = SVS(list, (*invertedLists), num);
//			cout << i<< endl;// cout的时候也在并行，所以输出的i有可能连在一起
//	}
//	pthread_exit(NULL);
//	return p;
//
//}


void mainFunc() {
	// 读取二进制文件
	fstream file;
	file.open("ExpIndex", ios::binary | ios::in);
	if (!file.is_open()) {
		cout << "Wrong in opening file!";
		return;

	}
	//static vector<InvertedIndex>* invertedLists = new vector<InvertedIndex>();
	for (int i = 0; i < 2000; i++)		//总共读取2000个倒排链表
	{
		InvertedIndex* t = new InvertedIndex();				//一个倒排链表
		file.read((char*)&t->length, sizeof(t->length));
		for (int j = 0; j < t->length; j++)
		{
			unsigned int docId;			//文件id
			file.read((char*)&docId, sizeof(docId));
			t->docIdList.push_back(docId);//加入至倒排表
		}
		sort(t->docIdList.begin(), t->docIdList.end());//对文档编号排序
		invertedLists->push_back(*t);		//加入一个倒排表
	}
	file.close();
	cout << "here" << endl;

	// 读取查询数据
	file.open("ExpQuery", ios::in);
	//static int query[1000][5] = { 0 };// 单个查询最多5个docId,全部读取
	string line;
	int count = 0;

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

	//------求交------
	long long head, tail, freq;

	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
	//pthread_mutex_init(&mutex, NULL);
	preprocessing(*invertedLists, 2000);

	//omp
#pragma omp parallel if(isParallel_out),num_threads(NUM_THREADS),shared(query,invertedLists)
#pragma omp for 
	for (int i = 0; i < count; i++) {// count个查询
		int num = 0;// query查询项数
		for (int j = 0; j < 5; j++) {
			if (query[i][j] != 0) {
				num++;
			}
		}
		int* list = new int[num];// 要传入的list
		for (int j = 0; j < num; j++) {
			list[j] = query[i][j];
		}
		sorted(list, *(invertedLists), num);// 按长度排序
		InvertedIndex res = HASH(list, *invertedLists, num);
		cout << i << endl;
	}

	
	// Func1/3---动态query内并行svs adp
	//for (int i = 0; i < count; i++) {// count个查询
	//	int num = 0;// query查询项数
	//	for (int j = 0; j < 5; j++) {
	//		if (query[i][j] != 0) {
	//			num++;
	//		}
	//	}
	//	int* list = new int[num];// 要传入的list
	//	for (int j = 0; j < num; j++) {
	//		list[j] = query[i][j];
	//	}
	//	sorted(list, *(invertedLists), num);// 按长度排序
	//	InvertedIndex res = SVS_dynamic(list, *invertedLists, num);
	//	cout << i << endl;
	//}


	// Func2/4---静态query内并行svs adp
	//sem_init(&sem_main, 0, 0);// init
	//for (int i = 0; i < NUM_THREADS; i++) {
	//	sem_init(&sem_worker[i], 0, 0);
	//}
	//
	//pthread_t thread[NUM_THREADS];// handle
	//for (int j = 0; j < NUM_THREADS; j++) {// 创建线程，传入参数
	//		param2[j].t_id = j;
	//		pthread_create(&thread[j], NULL, threadFunc2, &param2[j]);
	//}
	// 
	//for (int i = 0; i < count; i++) {// count个查询
	//	int num = 0;// query查询项数
	//	for (int j = 0; j < 5; j++) {
	//		if (query[i][j] != 0) {
	//			num++;
	//		}
	//	}
	//	int* list = new int[num];// 要传入的list
	//	for (int j = 0; j < num; j++) {
	//		list[j] = query[i][j];
	//	}
	//	sorted(list, *(invertedLists), num);// 按长度排序
	//	InvertedIndex res = SVS_static(list, *invertedLists, num);
	//	cout << i << endl;
	// 
	//}
	// 
	//flag = false;// 工作线程结束
	//// 唤醒工作线程end->得多唤醒一次，不然卡在开头的sem_wait
	//for (int j = 0; j < NUM_THREADS; j++)
	//	sem_post(&sem_worker[j]);
	//for (int i = 0; i < NUM_THREADS; i++) {
	//	pthread_join(thread[i], NULL);
	//}
	// 
	//// 摧毁信号量
	//sem_destroy(&sem_main);
	//for (int i = 0; i < NUM_THREADS; i++) {
	//	sem_destroy(&sem_worker[i]);
	//}

	
	// Func1---动态query间并行svs
	// 已无必要再做动态query间
	//pthread_t thread[4];
	//threadParam_t1 param[4];
	//for (int i = 0; i < 4; i++) {// 创建线程，传入线程id以及处理的总query数
	//	param[i].t_id = i;
	//	param[i].queryCount = count;
	//	pthread_create(&thread[i], NULL, threadFunc1, &param[i]);
	//}
	//for (int i = 0; i < 4; i++) {
	//	pthread_join(thread[i], NULL);
	//}

	//pthread_mutex_destroy(&mutex);
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	cout << (tail - head) * 1000.0 / freq;
}

int main()
{
	mainFunc();
    return 0;
}