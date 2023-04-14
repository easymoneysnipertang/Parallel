#include<iostream>
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

using namespace std;

class InvertedIndex {// ���������ṹ
public:
	int length = 0;
	vector<unsigned int> docIdList;
};
// ���رȽϷ����Գ��������������
bool operator<(const InvertedIndex& i1, const InvertedIndex& i2) {
	return i1.length < i2.length;
}


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

	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		int count = 0;// s��ͷ�������һ��
		int t = 0;
		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for(int j=0;j<s.length;j++){// ����Ԫ�ض��÷���һ��
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
			if (isFind)// ����
				s.docIdList[count++] = s.docIdList[j];
		}
		if(count<s.length)// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}
	return s;
}
// svs_sse
InvertedIndex SVS_SSE(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�

	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		int count = 0;// s��ͷ�������һ��
		int t = 0;
		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = 0; j < s.length; j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			__m128i ss = _mm_set1_epi32(s.docIdList[j]);// ���32λ

			for (; t < index[queryList[i]].length - 3; t+=4) {// ͬʱ���ĸ�Ԫ�رȽ�
				// ����i�б�������Ԫ��
				__m128i ii;
				ii = _mm_load_si128((__m128i*)&index[queryList[i]].docIdList[t]);// һ��ȡ�ĸ�
				__m128i tmp = _mm_set1_epi32(0);
				tmp = _mm_cmpeq_epi32(ss, ii);// �Ƚ�����ÿһλ
				int mask = _mm_movemask_epi8(tmp);// תΪ���룬�����һ�������,�Ͳ�����ȫ0
				
				if (mask!=0){// �鿴�ȽϽ��
					isFind=true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��ʱbreak�����ⳬ������һԪ������ͷ����
					break;
			}
			if (!isFind&& (t>=index[queryList[i]].length - 3)) {// ����ʣ��Ԫ��
				for (; t < index[queryList[i]].length; t++)
				{
					if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
						break;
				}
			}
			if (isFind)// ����
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}
	return s;
}
// svs_sse_notAlign
InvertedIndex SVS_NotAlign(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�

	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		int count = 0;// s��ͷ�������һ��
		int t = 0;
		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = 0; j < s.length; j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			__m128i ss = _mm_set1_epi32(s.docIdList[j]);// ���32λ

			// ���Բ�����ı���
			// �ڿ�ʼ��ģ4ʣ��Ĳ��ִ�������Ժ�ÿ�μ��ı�֤���ܷ���
			for (; t < 2; t++) {
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
					break;
			}

			for (; t < index[queryList[i]].length-3; t += 4) {// ͬʱ���ĸ�Ԫ�رȽ�
				// ����i�б�������Ԫ��
				__m128i ii;
				ii = _mm_loadu_epi32(&index[queryList[i]].docIdList[t]);// һ��ȡ�ĸ�
				__m128i tmp = _mm_set1_epi32(0);
				tmp = _mm_cmpeq_epi32(ss, ii);// �Ƚ�����ÿһλ
				int mask = _mm_movemask_epi8(tmp);// תΪ���룬�����һ�������,�Ͳ�����ȫ0

				if (mask != 0) {// �鿴�ȽϽ��
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��ʱbreak�����ⳬ������һԪ������ͷ����
					break;
			}
			if (!isFind && (t >= index[queryList[i]].length - 3)) {// ����ʣ��Ԫ��
				for (; t < index[queryList[i]].length; t++)
				{
					if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
						break;
				}
			}
			if (isFind)// ����
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}
	return s;
}
// svs_avx
InvertedIndex SVS_AVX(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�

	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		int count = 0;// s��ͷ�������һ��
		int t = 0;
		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = 0; j < s.length; j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			__m256i ss = _mm256_set1_epi32(s.docIdList[j]);// ���32λ

			for (; t < index[queryList[i]].length - 7; t += 8) {// ͬʱ��˸�Ԫ�رȽ�
				// ����i�б�������Ԫ��
				__m256i ii;
				ii = _mm256_loadu_epi32(&index[queryList[i]].docIdList[t]);// һ��ȡ�˸�
				__m256i tmp = _mm256_set1_epi32(0);
				tmp = _mm256_cmpeq_epi32(ss, ii);// �Ƚ�����ÿһλ
				int mask = _mm256_movemask_epi8(tmp);// תΪ���룬�����һ�������,�Ͳ�����ȫ0

				if (mask != 0) {// �鿴�ȽϽ��
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��ʱbreak�����ⳬ������һԪ������ͷ����
					break;
			}
			if (!isFind && (t >= index[queryList[i]].length - 7)) {// ����ʣ��Ԫ��
				for (; t < index[queryList[i]].length; t++)
				{
					if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
						break;
				}
			}
			if (isFind)// ����
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// ������ɾ��
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
	return (q1.length-q1.cursor) < (q2.length-q2.cursor);
}
InvertedIndex ADP(int* queryList, vector<InvertedIndex>& index, int num)
{
	/*InvertedIndex S;
	int s = 1;
	bool found = true;
	vector<InvertedIndex> idx_;
	for (int i = 0; i < num; i++)
	{
		idx_.push_back(index[queryList[i]]);
	}

	for (int t = 0; t < idx_[0].length; t++)
	{
		unsigned int e = idx_[0].docIdList[t];
		while (s != num && found == true)
		{
			for (int i = 0; i < idx_[s].length; i++)
			{
				found = false;
				if (e == idx_[s].docIdList[i])
				{
					found = true;
					break;
				}
			}
			s = s + 1;
		}
		if (s == num && found == true)
			S.docIdList.push_back(e);
	}
	return S;*/

	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}

	while (list[0].cursor < list[0].length) {// ��̵��б�ǿ�
		bool isFind = true;
		int s = 1;
		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
		while (s != num && isFind == true) {
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
			s++;// ��һ������
		}
		list[0].cursor++;// ��ǰԪ���ѱ����ʹ�
		if (s == num && isFind) {
			S.docIdList.push_back(e);
			S.length++;
		}
		//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
	}
	return S;
}
// adp_sse
InvertedIndex ADP_SSE(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}

	while (list[0].cursor < list[0].length) {// ��̵��б�ǿ�
		bool isFind = true;
		int s = 1;
		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
		__m128i ee = _mm_set1_epi32(e);

		while (s != num && isFind == true) {
			isFind = false;
			while (list[s].cursor < list[s].length-3) {// ���s�б�
				// ��������
				__m128i ii = _mm_load_si128((__m128i*)&index[list[s].key].docIdList[list[s].cursor]);
				__m128i result = _mm_set1_epi32(0);
				result = _mm_cmpeq_epi32(ee, ii);
				int mask = _mm_movemask_epi8(result);// �õ�����
				if (mask != 0) {// �鿴�ȽϽ��
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])// ��ʱbreak����������cursor
					break;
				list[s].cursor += 4;
			}
			if (!isFind) {
				while (list[s].cursor < list[s].length) {// ʣ���ֶ�
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;
					list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
				}
			}
			s++;// ��һ������
		}
		list[0].cursor++;// ��ǰԪ���ѱ����ʹ�
		if (s == num && isFind) {
			S.docIdList.push_back(e);
			S.length++;
		}
		//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
	}
	return S;
}
// adp_avx
InvertedIndex ADP_AVX(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}

	while (list[0].cursor < list[0].length) {// ��̵��б�ǿ�
		bool isFind = true;
		int s = 1;
		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
		__m256i ee = _mm256_set1_epi32(e);

		while (s != num && isFind == true) {
			isFind = false;
			while (list[s].cursor < list[s].length - 7) {// ���s�б�
				// ��������
				__m256i ii = _mm256_loadu_epi32(&index[list[s].key].docIdList[list[s].cursor]);
				__m256i result = _mm256_set1_epi32(0);
				result = _mm256_cmpeq_epi32(ee, ii);
				int mask = _mm256_movemask_epi8(result);// �õ�����
				if (mask != 0) {// �鿴�ȽϽ��
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])// ��ʱbreak����������cursor
					break;
				list[s].cursor += 8;
			}
			if (!isFind) {
				while (list[s].cursor < list[s].length) {// ʣ���ֶ�
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;
					list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
				}
			}
			s++;// ��һ������
		}
		list[0].cursor++;// ��ǰԪ���ѱ����ʹ�
		if (s == num && isFind) {
			S.docIdList.push_back(e);
			S.length++;
		}
		//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
	}
	return S;
}
// adp_sse_notAlign
InvertedIndex ADP_NotAlign(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}

	while (list[0].cursor < list[0].length) {// ��̵��б�ǿ�
		bool isFind = true;
		int s = 1;
		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
		__m128i ee = _mm_set1_epi32(e);

		while (s != num && isFind == true) {
			isFind = false;
			// ���Բ�����ı���
			// �ڿ�ʼ��ģ4ʣ��Ĳ��ִ�������Ժ�ÿ�μ��ı�֤���ܷ���
			while (list[s].cursor < 2) {// ʣ���ֶ�
				if (e == index[list[s].key].docIdList[list[s].cursor]) {
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
			}

			while (list[s].cursor < list[s].length-3) {// ���s�б�
				// ��������
				__m128i ii = _mm_loadu_epi32(&index[list[s].key].docIdList[list[s].cursor]);
				__m128i result = _mm_set1_epi32(0);
				result = _mm_cmpeq_epi32(ee, ii);
				int mask = _mm_movemask_epi8(result);// �õ�����
				if (mask != 0) {// �鿴�ȽϽ��
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])// ��ʱbreak����������cursor
					break;
				list[s].cursor += 4;
			}
			if (!isFind) {
				while (list[s].cursor < list[s].length) {// ʣ���ֶ�
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;
					list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
				}
			}
			
			s++;// ��һ������
		}
		list[0].cursor++;// ��ǰԪ���ѱ����ʹ�
		if (s == num && isFind) {
			S.docIdList.push_back(e);
			S.length++;
		}
		//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
	}
	return S;
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
		hashBucket[i] = new HashBucket[50800];// 26000000/512->256!v

	for (int i = 0; i < count; i++) {
		for (int j = 0; j < index[i].length; j++) {
			int value = index[i].docIdList[j];// �ó���ǰ�б��j��ֵ
			int hashValue = value / 512;// TODO:check
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
			int hashValue = s.docIdList[j] / 512;
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
// hash_sse
InvertedIndex HASH_SSE(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// ѡ����̵ĵ����б�
	for (int i = 1; i < num; i++) {// ��ʣ���б���
		// SVS��˼�룬��s����������
		int count = 0;
		int length = s.length;

		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 512;
			// �ҵ���hashֵ�ڵ�ǰ�����б��ж�Ӧ�Ķ�
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// ��ֵ�϶��Ҳ�����������������
				continue;
			}
			else {// �ڸö�����в��ң�һ�β�4��
				__m128i ss = _mm_set1_epi32(s.docIdList[j]);// ���32λ

				for (begin; begin <= end-3; begin+=4) {
					__m128i ii;
					ii = _mm_loadu_epi32(&index[queryList[i]].docIdList[begin]);// һ��ȡ�ĸ�
					__m128i tmp = _mm_set1_epi32(0);
					tmp = _mm_cmpeq_epi32(ss, ii);// �Ƚ�����ÿһλ
					int mask = _mm_movemask_epi8(tmp);// תΪ���룬�����һ�������,�Ͳ�����ȫ0

					if (mask != 0) {// �鿴�ȽϽ��
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin])// ��ʱbreak�����ⳬ������һԪ������ͷ����
						break;
				}
				if (!isFind && (begin > end - 3)) {// ����ʣ��Ԫ��
					for (; begin <= end; begin++)
					{
						if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
							isFind = true;
							break;
						}
						else if (s.docIdList[j] < index[queryList[i]].docIdList[begin])// ��������
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


void test01() {
	vector<InvertedIndex> testInvertedLists;
	int testQuery[3] = { 0,1,2 };
	for (int i = 0; i < 3; i++)
	{
		InvertedIndex t;
		cin >> t.length;
		for (int j = 0; j < t.length; j++)
		{
			unsigned int docId;
			cin >> docId;
			t.docIdList.push_back(docId);
		}
		sort(t.docIdList.begin(), t.docIdList.end());//���ĵ��������
		testInvertedLists.push_back(t);
	}
	sorted(testQuery, testInvertedLists, 3);
	//preprocessing(testInvertedLists, 3);
	InvertedIndex res = SVS_SSE(testQuery, testInvertedLists, 3);
	for (auto i : res.docIdList)
		cout << i << " ";
}
void test02() {
	srand(time(0));

	vector<InvertedIndex> testInvertedLists;

	for (int i = 0; i < 500; i++)
	{
		InvertedIndex t;// ����500�������б�
		t.length = rand() % (100 - 30 + 1) + 30;
		//cout << t.length << " :";

		vector<int> forRandom;
		for (int j = 0; j < t.length * 5; j++)//Ϊ�˲��ظ�
			forRandom.push_back(j);
		random_shuffle(forRandom.begin(), forRandom.end());//�������

		for (int j = 0; j < t.length; j++)
		{
			int docId = forRandom[j];
			//cout << docId << " ";
			t.docIdList.push_back(docId);
		}
		sort(t.docIdList.begin(), t.docIdList.end());//���ĵ��������
		testInvertedLists.push_back(t);
		//cout << endl;
	}
	//preprocessing(testInvertedLists, 500);
	double sum = 0;
	for (int time = 0; time < 20; time++) {
		long long head, tail, freq;
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

		for (int i = 0; i < 200; i++) {// ��200�β�ѯ
			int testQuery[5];
			for (int i = 0; i < 5; i++)testQuery[i] = rand() % 500;

			sorted(testQuery, testInvertedLists, 5);
			InvertedIndex res = SVS_SSE(testQuery, testInvertedLists, 5);
			for (auto i : res.docIdList)
				cout << i << " ";
			if (res.docIdList.size() != 0)
				cout << endl;
		}
		QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
		sum += (tail - head) * 1000.0 / freq;
		//cout << endl << (tail - head) * 1000.0 / freq;
	}
	cout << endl << (double)sum / 20;
}

void mainFunc() {
	// ��ȡ�������ļ�
	fstream file;
	file.open("ExpIndex", ios::binary | ios::in);
	if (!file.is_open()) {
		cout << "Wrong in opening file!";
		return ;

	}
	static vector<InvertedIndex>* invertedLists = new vector<InvertedIndex>();
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
	static int query[1000][5] = { 0 };// ������ѯ���5��docId,ȫ����ȡ
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

	//preprocessing(*invertedLists, 2000);

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
		InvertedIndex res = ADP_NotAlign(list, *invertedLists, num);
		cout << i << endl;
	}
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	cout << (tail - head) * 1000.0 / freq;
}


int main() {
	//test01();
	test02();
	
	//mainFunc();

	return 0;
}