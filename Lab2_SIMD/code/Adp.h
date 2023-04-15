#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include"InvertedIndex.h"
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