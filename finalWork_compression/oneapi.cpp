#include <sycl/sycl.hpp>
#include <vector>
#include <iostream>
#include <string>
#include<iostream>
#include<fstream>
#include<vector>
#include<algorithm>
#include<sstream>
#if FPGA || FPGA_EMULATOR
#include <sycl/ext/intel/fpga_extensions.hpp>
#endif

using namespace sycl;
using namespace std;
struct InvertedIndex {// 倒排索引结构
	vector<unsigned int> docIdList;
	int length;
};
extern SYCL_EXTERNAL int find1stGreaterEqual(unsigned int* arr,int left,int right, unsigned int target) {

	while (left < right)
	{
		int mid = left + (right - left) / 2;
		if (arr[mid] < target)
		{
			left = mid + 1;
		}
		else
			right = mid;
	}
	return left;
}
// adp实现
class QueryItem {
public:
	int cursor;// 当前读到哪了
	int end;// 倒排索引总长度
	int key;// 关键字值
};
bool operator<(const QueryItem& q1, const QueryItem& q2) {// 选剩余元素最少的元素
	return (q1.end - q1.cursor) < (q2.end - q2.cursor);
}
vector<unsigned int> ADP(unsigned int* index,int* len)
{
	vector<unsigned int>S;
	QueryItem* list = new QueryItem[len[0]]();
	for (int i = 0; i < len[0]; i++)// 预处理
	{
		list[i].cursor = len[i];
		list[i].end = len[i+1];
	}
	list[0].cursor = 0;

	for (int i = list[0].cursor; i < len[1]; i++) {// 最短的列表非空
		bool isFind = true;
		// todo等会改auto
		auto e = index[i];
		for (int s = 1; s != len[0] && isFind == true; s++) {
			isFind = false;
			while (list[s].cursor < list[s].end) {// 检查s列表
				if (e == index[len[s+1]+list[s].cursor]) {
					isFind = true;
					break;
				}
				else if (e < index[len[s + 1] + list[s].cursor])
					break;
				list[s].cursor++;// 当前访问过，且没找到合适的，往后移
			}
			// 下一个链表
		}
		// 当前元素已被访问过
		if (isFind)
			S.push_back(e);
		//sort(list, list + num);// 重排，将未探查元素少的列表前移
	}
	delete list;
	return S;
}
#define group 1024
double gpu_kernel(unsigned int* docId, int* lArr, sycl::queue& q)
{
	auto global_ndrange = range<1>(lArr[1]/group);
	auto local_ndrange = range<1>(5);
	int num = lArr[0];

	double duration = 0.0f;

	auto e = q.submit([&](sycl::handler& h)
	{
		h.parallel_for<class k_name_t>(sycl::nd_range<1>(global_ndrange, local_ndrange), [=](sycl::nd_item<1> index)
		{
				int is = index.get_global_id() * group;
				int ie = lArr[1]>is + group?is+group:lArr[1];
				int rs[5];
				int re[5];

				rs[0] = is, re[0] = ie;
				for (int i = 1; i < num; i++)
				{
					rs[i] = find1stGreaterEqual(docId, lArr[i], lArr[i + 1], docId[is]);
					re[i] = lArr[i + 1];
				}

				QueryItem list[5];
				for (int i = 0; i < num; i++)// 预处理
				{
					list[i].cursor = rs[i];
					list[i].end = re[i];
				}

				int cnt = 0;
				for (int i = list[0].cursor; i < lArr[1]; i++) {// 最短的列表非空
					bool isFind = true;
					// todo等会改auto
					unsigned e = docId[i];
					for (int s = 1; s != num && isFind == true; s++) {
						isFind = false;
						while (list[s].cursor < list[s].end) {// 检查s列表
							if (e == docId[list[s].cursor]) {
								isFind = true;
								break;
							}
							else if (e < docId[list[s].cursor])
								break;
							list[s].cursor++;// 当前访问过，且没找到合适的，往后移
						}
						// 下一个链表
					}
					// 当前元素已被访问过
					if (isFind)
						docId[cnt++];
					//sort(list, list + num);// 重排，将未探查元素少的列表前移
				}
		});
	});
	e.wait();
	duration += (e.get_profiling_info<info::event_profiling::command_end>() -
		e.get_profiling_info<info::event_profiling::command_start>()) / 1000.0f / 1000.0f;
	return duration;
}
int main() {
	// 读取二进制文件
	fstream file;
	file.open("ExpIndex", ios::binary | ios::in);
	if (!file.is_open()) {
		cout << "Wrong in opening file!";
		return;

	}
	static vector<InvertedIndex>* invertedLists = new vector<InvertedIndex>();
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

	// 读取查询数据
	file.open("ExpQuery", ios::in);
	static int query[1000][5] = { 0 };// 单个查询最多5个docId,全部读取
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
	file.close();

	//------求交------
	cout << "------intersection begin-----" << std::endl;
	// 开始计时
	double duration = 0.0;
	for (int i = 0; i < count; i++) {// count个查询
		int num = 0;// query查询项数
		for (int j = 0; j < 5; j++) {
			if (query[i][j] != 0) {
				num++;
			}
		}

		auto propList = sycl::property_list{ sycl::property::queue::enable_profiling() };
		queue my_gpu_queue(sycl::default_selector_v, propList);

		int* gpuLenArr = malloc_shared<int>(num + 1, my_gpu_queue);
		if (gpuLenArr == NULL) 
		{
			fprintf(stderr, "Malloc failed\n");
			return -1;
		}

		int totalLength = 0;// 获取各段长度
		gpuLenArr[0] = num;// 用0号位置来保存num
		for (int j = 0; j < num; j++) 
		{
			int length = (*invertedLists)[query[i][j]].length;
			totalLength += length;
			gpuLenArr[j + 1] = totalLength;
		}

		unsigned int* gpuIndex = malloc_shared<unsigned int>(totalLength, my_gpu_queue);
		if (gpuIndex == NULL) 
		{
			fprintf(stderr, "Malloc failed\n");
			return -1;
		}

		for (int j = 0; j < num; j++) 
		{
			// 倒排链表全部放进一个二维数组->一维数组表示二维数组
			// 复制到数组当中去
			int s = j == 0 ? 0 : gpuLenArr[j];
			copy((*invertedLists)[query[i][j]].docIdList.begin(), (*invertedLists)[query[i][j]].docIdList.end(), gpuIndex + s);
		}
		duration += gpu_kernel(gpuIndex, gpuLenArr, my_gpu_queue);

		free(gpuIndex,my_gpu_queue);
		free(gpuLenArr,my_gpu_queue);
		cout << "i:" << i << " time: "<<duration<<std::endl;
	}

	// 停止计时...

	printf("GPU: ms\n");

	return 0;
}