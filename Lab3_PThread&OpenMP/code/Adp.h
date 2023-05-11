#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include<omp.h>
#include"InvertedIndex.h"
#include"ThreadParam.h"
using namespace std;

//InvertedIndex ADP(int* list, vector<InvertedIndex>& idx, int num)
//{
//	InvertedIndex S;
//	int s = 1;
//	bool found = true;
//	vector<InvertedIndex> idx_;
//	for (int i = 0; i < num; i++)
//		idx_.push_back(idx[list[i]]);
//	sort(idx_.begin(), idx_.end());
//	for (int t = 0; t < idx_[0].length; t++)
//	{
//		unsigned int e = idx_[0].docIdList[t];
//		while (s != num && found == true)//û�е������
//		{
//			found = false;
//			for (int i = 0; i < idx_[s].length; i++)
//			{
//				if (e == idx_[s].docIdList[i])
//				{
//					found = true;
//					break;
//				}
//			}
//			s = s + 1;
//		}
//		if (s == num && found == true)
//			S.docIdList.push_back(s);
//	}
//	return S;
//}

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
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].docIdList.size();
	}
	for (int i = list[0].cursor; i < list[0].length; i++) {// ��̵��б�ǿ�
		bool isFind = true;
		unsigned int e = index[list[0].key].docIdList[i];
		for (int s = 1; s != num && isFind == true; s++) {
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
			// ��һ������
		}
		// ��ǰԪ���ѱ����ʹ�
		if (isFind)
			S.docIdList.push_back(e);
		//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
	}
	return S;
}

InvertedIndex ADP_omp(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex S;
#pragma omp parallel num_threads(4)
	{
		QueryItem* list = new QueryItem[num]();
		for (int i = 0; i < num; i++)// Ԥ����
		{
			list[i].cursor = 0;
			list[i].key = queryList[i];
			list[i].length = index[queryList[i]].docIdList.size();
		}

#pragma omp for 
		for (int i = list[0].cursor; i < list[0].length; i++) {// ��̵��б�ǿ�
			bool isFind = true;
			unsigned int e = index[list[0].key].docIdList[i];
			for (int s = 1; s != num && isFind == true; s++) {
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
				// ��һ������
			}
#pragma omp critical
			// ��ǰԪ���ѱ����ʹ�
			if (isFind)
				S.docIdList.push_back(e);
			//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
		}
	}
	return S;
}