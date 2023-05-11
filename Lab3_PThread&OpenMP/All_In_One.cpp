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

class InvertedIndex {// ���������ṹ
public:
	int length = 0;
	vector<unsigned int> docIdList;
	
};
// ���رȽϷ����Գ��������������
bool operator<(const InvertedIndex& i1, const InvertedIndex& i2) {
	return i1.length < i2.length;
}

static int query[1000][5] = { 0 };// ������ѯ���5��docId,ȫ����ȡ
static vector<InvertedIndex>* invertedLists = new vector<InvertedIndex>();
pthread_mutex_t mutex;
sem_t sem_main, sem_worker[NUM_THREADS];
bool flag = true;

// �ѵ����б���������
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
// svsʵ��
InvertedIndex SVS(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�
	int count = 0;// s��ͷ�������һ��

	// ��ʣ���б���
#pragma omp parallel if(isParallel_in),num_threads(NUM_THREADS),shared(count)
	for (int i = 1; i < num; i++) {
		count = 0;// s��ͷ�������һ��
		int t = 0;

		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
#pragma omp for 
		for (int j = 0; j < s.length; j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; t < index[queryList[i]].length; t++) {
				// ����i�б�������Ԫ��
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
					break;
			}
#pragma omp critical
			if (isFind)// ����
				s.docIdList[count++] = s.docIdList[j];
		}
#pragma omp single
		{
			if (count < s.length)// ������ɾ��
				s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
			s.length = count;
		}

	}
	return s;
}


// hash�ֶ�ʵ��
class HashBucket {// һ��hash�Σ���¼���ڵ����б��е���ʼλ��
public:
	int begin = -1;
	int end = -1;
};
HashBucket** hashBucket;
// Ԥ�����������б����hash����
void preprocessing(vector<InvertedIndex>& index, int count) {
	hashBucket = new HashBucket * [count];
	for (int i = 0; i < count; i++)
		hashBucket[i] = new HashBucket[102000];// 26000000/512->256!v

	for (int i = 0; i < count; i++) {
		for (int j = 0; j < index[i].length; j++) {
			int value = index[i].docIdList[j];// �ó���ǰ�б��j��ֵ
			int hashValue = value / 256;// TODO:check
			if (hashBucket[i][hashValue].begin == -1) // �ö��ڻ�û��Ԫ��
				hashBucket[i][hashValue].begin = j;
			hashBucket[i][hashValue].end = j;// ��ӵ��ö�β��
		}
	}
}
InvertedIndex HASH(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// ѡ����̵ĵ����б�
	for (int i = 1; i < num; i++) {// ��ʣ���б���
		// SVS��˼�룬��s����������
		int count = 0;
		int length = s.length;

		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 256;
			// �ҵ���hashֵ�ڵ�ǰ�����б��ж�Ӧ�Ķ�
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// ��ֵ�϶��Ҳ�����������������
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// �ڸö����ҵ��˵�ǰֵ
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
						break;
					}
				}
				if (isFind) {
					// ����
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
		if (count < length)// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}
	return s;
}




// adpʵ��
class QueryItem {
public:
	int cursor;// ��ǰ��������
	int length;// ���������ܳ���
	int key;// �ؼ���ֵ
};
bool operator<(const QueryItem& q1, const QueryItem& q2) {// ѡʣ��Ԫ�����ٵ�Ԫ��
	return (q1.length - q1.cursor) < (q2.length - q2.cursor);
}
InvertedIndex ADP(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex S;
	
#pragma omp parallel if(isParallel_in),num_threads(NUM_THREADS/2)
	{
		QueryItem* list = new QueryItem[num]();// ÿ���̶߳���һ��list��Ȼ����һ����ȡ
		for (int i = 0; i < num; i++)// Ԥ����
		{
			list[i].cursor = 0;
			list[i].key = queryList[i];
			list[i].length = index[queryList[i]].length;
		}
#pragma omp for schedule(dynamic,1)
		for (int i = list[0].cursor; i < list[0].length; i++) {// ��̵��б�ǿ�
			bool isFind = true;
			unsigned int e = index[list[0].key].docIdList[i];
			for (int s=1; s != num && isFind == true; s++) {
				isFind = false;
				while (list[s].cursor < list[s].length) {// ���s�б�
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;

					list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
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

	while (true) {// ��̬���̣߳�һֱ������
		sem_wait(&sem_worker[t_id]);// �������ȴ����̻߳���
		if (flag == false)break;

		QueryItem* list = new QueryItem[numFor4]();
		for (int i = 0; i < numFor4; i++)// Ԥ����
			list[i] = queryListFor4[i];

		int length = list[0].length / NUM_THREADS;
		int begin = t_id * length, end = (t_id + 1) * length;// ��list[0]���4����

		for (int i = 1; i < numFor4; i++)
		{// Ԥ����cursor�����ٴ�����¼�
			list[i].cursor = t_id * (list[i].length / NUM_THREADS);
			for (int j = t_id; j > 0; j--) {
				if ((*invertedLists)[list[0].key].docIdList[begin] < (*invertedLists)[list[i].key].docIdList[list[i].cursor])
					list[i].cursor -= list[i].length / NUM_THREADS;
				else break;
			}
			//for (int j = numOfThreads; j > 0; j--) {
			//	if ((*invertedLists)[list[0].key].docIdList[begin] < (*invertedLists)[list[i].key].docIdList[list[i].cursor]) {
			//		list[i].cursor /= 2;// �۰����
			//		if (j == 1)list[i].cursor = 0;// û�취�ˣ�ֻ�ܴ�ͷ������
			//	}
			//	else break;
			//}
		}

		for (; begin < end; begin++) {
			bool isFind = true;
			unsigned int e = (*invertedLists)[list[0].key].docIdList[begin];
			for (int s = 1; s != numFor4; s++) {
				isFind = false;
				for (; list[s].cursor < list[s].length; list[s].cursor++) {// ���s�б�
					if (e == (*invertedLists)[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < (*invertedLists)[list[s].key].docIdList[list[s].cursor])
						break;
					// ��ǰ���ʹ�����û�ҵ����ʵģ�������
				}
				if (!isFind)break;
			}
			// �ٽ��� ����
			// ��ǰԪ���ѱ����ʹ�
			if (isFind) {
				pthread_mutex_lock(&mutex);
				S_in_ADP.docIdList.push_back(e);
				pthread_mutex_unlock(&mutex);
			}
		}
		sem_post(&sem_main);// �������߳�
	}
	pthread_exit(NULL);
	return p;
}
InvertedIndex ADP_static(int* queryList, vector<InvertedIndex>& index, int num)
{
	S_in_ADP.docIdList.clear();
	numFor4 = num;
	
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}
	queryListFor4 = list;

	// ���ѹ����߳�start
	for (int j = 0; j < NUM_THREADS; j++)
		sem_post(&sem_worker[j]);
	// ���߳�˯��
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
	for (int i = 0; i < num; i++)// ����һ��list������cursor++ͬ�������Ѿ�
		list[i] = (p->list)[i];

	int length = list[0].length / NUM_THREADS;
	int begin = t_id * length, end = (t_id + 1) * length;// ��list[0]���4����

	for (int i = 1; i < num; i++)
	{// Ԥ����cursor�����ٴ�����¼�
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
			for (; list[s].cursor < list[s].length; list[s].cursor++) {// ���s�б�
				if (e == (*invertedLists)[list[s].key].docIdList[list[s].cursor]) {
					isFind = true;
					break;
				}
				else if (e < (*invertedLists)[list[s].key].docIdList[list[s].cursor])
					break;
				// ��ǰ���ʹ�����û�ҵ����ʵģ�������
			}
			if (!isFind)break;
		}

		// �ٽ��� ����
		// ��ǰԪ���ѱ����ʹ�
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
	for (int i = 0; i < num; i++)// Ԥ����
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}

	pthread_t thread[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++) {// �����̣߳��������
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

	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		countForSVS = 0;// s��ͷ�������һ��

		int t = 0;
		if (t == 0)goto outSVS_dynamic;
		// Ԥ������ٴ�����¼�
		t = t_id * ((*invertedLists)[list[i]].length / NUM_THREADS);
		for (int j = t_id; j > 0; j--) {
			if (S_in_SVS.docIdList[begin] < (*invertedLists)[list[i]].docIdList[t])
				t -= ((*invertedLists)[list[i]].length / NUM_THREADS);
			else break;
		}

		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j=begin; j < end; j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; t < (*invertedLists)[list[i]].length; t++) {
				// ����i�б�������Ԫ��
				if (S_in_SVS.docIdList[j] == (*invertedLists)[list[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (S_in_SVS.docIdList[j] < (*invertedLists)[list[i]].docIdList[t])// ��������
					break;
			}
			if (isFind) {
				// ����
				pthread_mutex_lock(&mutex);
				S_in_SVS.docIdList[countForSVS++] = S_in_SVS.docIdList[j];
				pthread_mutex_unlock(&mutex);
			}

		}

		if (t_id == 0) {
			// barrier һ������
			for (int l = 1; l < NUM_THREADS; l++)
				sem_wait(&sem_barrier);
			if (countForSVS < S_in_SVS.length)// ������ɾ��
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
	S_in_SVS = index[queryList[0]];// ȡ��̵��б�
		
	pthread_t thread[NUM_THREADS];
	sem_init(&sem_barrier, 0, 0);// init
	pthread_barrier_init(&barrier_wait, NULL, NUM_THREADS);
	for (int j = 0; j < NUM_THREADS; j++) {// �����̣߳��������
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

	while (true) {// ��̬���̣߳�һֱ������
		sem_wait(&sem_worker[t_id]);// �������ȴ����̻߳���
		if (flag == false)break;
		// ��ʣ���б���
		for (int i = 1; i < numFor2; i++) {
			countForSVS = 0;// s��ͷ�������һ��
			int length = S_in_SVS.docIdList.size() / NUM_THREADS;
			int begin = t_id * length, end = (t_id + 1) * length;
			int t = 0;
			if (length == 0)goto outSVS_static;
			// Ԥ������ٴ�����¼�
			t = t_id * ((*invertedLists)[queryList2[i]].length / NUM_THREADS);
			for (int j = t_id; j > 0; j--) {
				if (S_in_SVS.docIdList[begin] < (*invertedLists)[queryList2[i]].docIdList[t]) //TODO:��仰���±곬�ˣ�
					t -= ((*invertedLists)[queryList2[i]].length / NUM_THREADS);
				else break;
			}

			// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
			for (int j = begin; j < end; j++) {// ����Ԫ�ض��÷���һ��
				bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

				for (; t < (*invertedLists)[queryList2[i]].length; t++) {
					// ����i�б�������Ԫ��
					if (S_in_SVS.docIdList[j] == (*invertedLists)[queryList2[i]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (S_in_SVS.docIdList[j] < (*invertedLists)[queryList2[i]].docIdList[t])// ��������
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
				// barrier һ������
				for (int l = 1; l < NUM_THREADS; l++)
					sem_wait(&sem_barrier);
				if (countForSVS < S_in_SVS.length)// ������ɾ��
					S_in_SVS.docIdList.erase(S_in_SVS.docIdList.begin() + countForSVS, S_in_SVS.docIdList.end());
				S_in_SVS.length = countForSVS;
			}
			else sem_post(&sem_barrier);
			pthread_barrier_wait(&barrier_wait);

		}
	outSVS_static:
		sem_post(&sem_main);// �������߳�
	}

	pthread_exit(NULL);
	return p;
}

InvertedIndex SVS_static(int* queryList, vector<InvertedIndex>& index, int num) {
	S_in_SVS = index[queryList[0]];// ȡ��̵��б�
	queryList2 = queryList;
	numFor2 = num;

	// ���ѹ����߳�start
	sem_init(&sem_barrier, 0, 0);// init
	pthread_barrier_init(&barrier_wait, NULL, NUM_THREADS);
	for (int j = 0; j < NUM_THREADS; j++)
		sem_post(&sem_worker[j]);
	// ���߳�˯��
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
//	// ��̬query�䲢��
//	threadParam_t1* p = (threadParam_t1*)param;
//	int t_id = p->t_id;
//	int count = p->queryCount / 4;
//	int begin = t_id * count, end = (t_id + 1) * count;
//	for (int i = begin; i < end; i++) {
//			int num = 0;// query��ѯ����
//			for (int j = 0; j < 5; j++) {
//				if (query[i][j] != 0) {
//					num++;
//				}
//			}
//			int* list = new int[num];// Ҫ�����list
//			for (int j = 0; j < num; j++) {
//				list[j] = query[i][j];
//			}
//			sorted(list, *(invertedLists), num);// ����������
//			InvertedIndex res = SVS(list, (*invertedLists), num);
//			cout << i<< endl;// cout��ʱ��Ҳ�ڲ��У����������i�п�������һ��
//	}
//	pthread_exit(NULL);
//	return p;
//
//}


void mainFunc() {
	// ��ȡ�������ļ�
	fstream file;
	file.open("ExpIndex", ios::binary | ios::in);
	if (!file.is_open()) {
		cout << "Wrong in opening file!";
		return;

	}
	//static vector<InvertedIndex>* invertedLists = new vector<InvertedIndex>();
	for (int i = 0; i < 2000; i++)		//�ܹ���ȡ2000����������
	{
		InvertedIndex* t = new InvertedIndex();				//һ����������
		file.read((char*)&t->length, sizeof(t->length));
		for (int j = 0; j < t->length; j++)
		{
			unsigned int docId;			//�ļ�id
			file.read((char*)&docId, sizeof(docId));
			t->docIdList.push_back(docId);//���������ű�
		}
		sort(t->docIdList.begin(), t->docIdList.end());//���ĵ��������
		invertedLists->push_back(*t);		//����һ�����ű�
	}
	file.close();
	cout << "here" << endl;

	// ��ȡ��ѯ����
	file.open("ExpQuery", ios::in);
	//static int query[1000][5] = { 0 };// ������ѯ���5��docId,ȫ����ȡ
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

	//------��------
	long long head, tail, freq;

	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
	//pthread_mutex_init(&mutex, NULL);
	preprocessing(*invertedLists, 2000);

	//omp
#pragma omp parallel if(isParallel_out),num_threads(NUM_THREADS),shared(query,invertedLists)
#pragma omp for 
	for (int i = 0; i < count; i++) {// count����ѯ
		int num = 0;// query��ѯ����
		for (int j = 0; j < 5; j++) {
			if (query[i][j] != 0) {
				num++;
			}
		}
		int* list = new int[num];// Ҫ�����list
		for (int j = 0; j < num; j++) {
			list[j] = query[i][j];
		}
		sorted(list, *(invertedLists), num);// ����������
		InvertedIndex res = HASH(list, *invertedLists, num);
		cout << i << endl;
	}

	
	// Func1/3---��̬query�ڲ���svs adp
	//for (int i = 0; i < count; i++) {// count����ѯ
	//	int num = 0;// query��ѯ����
	//	for (int j = 0; j < 5; j++) {
	//		if (query[i][j] != 0) {
	//			num++;
	//		}
	//	}
	//	int* list = new int[num];// Ҫ�����list
	//	for (int j = 0; j < num; j++) {
	//		list[j] = query[i][j];
	//	}
	//	sorted(list, *(invertedLists), num);// ����������
	//	InvertedIndex res = SVS_dynamic(list, *invertedLists, num);
	//	cout << i << endl;
	//}


	// Func2/4---��̬query�ڲ���svs adp
	//sem_init(&sem_main, 0, 0);// init
	//for (int i = 0; i < NUM_THREADS; i++) {
	//	sem_init(&sem_worker[i], 0, 0);
	//}
	//
	//pthread_t thread[NUM_THREADS];// handle
	//for (int j = 0; j < NUM_THREADS; j++) {// �����̣߳��������
	//		param2[j].t_id = j;
	//		pthread_create(&thread[j], NULL, threadFunc2, &param2[j]);
	//}
	// 
	//for (int i = 0; i < count; i++) {// count����ѯ
	//	int num = 0;// query��ѯ����
	//	for (int j = 0; j < 5; j++) {
	//		if (query[i][j] != 0) {
	//			num++;
	//		}
	//	}
	//	int* list = new int[num];// Ҫ�����list
	//	for (int j = 0; j < num; j++) {
	//		list[j] = query[i][j];
	//	}
	//	sorted(list, *(invertedLists), num);// ����������
	//	InvertedIndex res = SVS_static(list, *invertedLists, num);
	//	cout << i << endl;
	// 
	//}
	// 
	//flag = false;// �����߳̽���
	//// ���ѹ����߳�end->�ö໽��һ�Σ���Ȼ���ڿ�ͷ��sem_wait
	//for (int j = 0; j < NUM_THREADS; j++)
	//	sem_post(&sem_worker[j]);
	//for (int i = 0; i < NUM_THREADS; i++) {
	//	pthread_join(thread[i], NULL);
	//}
	// 
	//// �ݻ��ź���
	//sem_destroy(&sem_main);
	//for (int i = 0; i < NUM_THREADS; i++) {
	//	sem_destroy(&sem_worker[i]);
	//}

	
	// Func1---��̬query�䲢��svs
	// ���ޱ�Ҫ������̬query��
	//pthread_t thread[4];
	//threadParam_t1 param[4];
	//for (int i = 0; i < 4; i++) {// �����̣߳������߳�id�Լ��������query��
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