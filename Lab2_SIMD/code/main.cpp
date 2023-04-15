#include<iostream>
#include<vector>
#include<Windows.h>
#include<algorithm>
#include"InvertedIndex.h"
#include"util.h"

#include"SvS.h"
#include"Adp.h"

#include"SvS_SSE.h"
#include"Adp_SSE.h"
#include"Adp_AVX.h"
#include"SvS_AVX.h"

#include"NotAligned.h"

//#include"Hash.h"
//#include"Hash_SSE.h"
#include"Hash_AVX.h"

//#include"Bitmap.h"
//#include"Hash_SSE.h"
//#include"BitMap_SSE.h"
#include"Bitmap_AVX.h"
using namespace std;

	
//TODO:�޸�Ϊnlogn-------------------------------------------
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

int main() {
	//����
	//verify();
	//��ȡ�������ļ�
	alignas(16) static vector<InvertedIndex> invertedLists;
	alignas(16) static int query[1000][5] = { 0 };// ������ѯ���5��docId,ȫ����ȡ
	int count;
	getData(invertedLists, query,count);
	//------svs��------
	long long head, tail, freq;

	//Ԥ����
	//preprocessing(invertedLists, 2000);
	//bitMapProcessing(invertedLists, 2000);
	//bitMapSSEProcessing(invertedLists, 2000);
	bitMapAVXProcessing(invertedLists, 2000);
	int step = 50;
	for (int k = 50; k <= count; k += step)
	{	
		float totalTime = 0;
		//��ʼ��ʱ���
		for (int t = 0; t < 5; t++)
		{
			QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
			QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
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
				BITMAP_AVX(list, invertedLists, num);
			}
			QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
			totalTime += (tail - head) * 1000.0 / freq;
		}
		cout << totalTime/5<<endl;
		if (k == 100)
			step = 100;
		if (k == 400)
			step = 200;
	}

	return 0;
}