#include<vector>
#include<fstream>
#include<sstream>
#include<algorithm>
#include<windows.h>
#include<iostream>
#include<bitset>
#include <mpi.h>
#include <omp.h>
#include <xmmintrin.h>  //sse
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include <immintrin.h>
#define LIST_NUM 2000
#define NUM_THREADS 4
using namespace std;
// 将n位数据写入到int型数组中
void writeBitData(vector<unsigned int>& array, unsigned int index, unsigned int data, int n)
{
	int shift = index % 32;
	int elementIndex = index / 32;
	int remainingBits = 32 - shift;
	unsigned mask = (long long)pow(2, n) - 1;
	if (remainingBits >= n) {
		// 跨越一个数组元素的情况
		array[elementIndex] &= ~(mask << shift);  // 清除对应位的数据
		array[elementIndex] |= (data & mask) << shift;  // 写入新数据
	}
	else {
		// 跨越两个数组元素的情况
		int bitsInFirstElement = remainingBits;
		int bitsInSecondElement = n - remainingBits;

		array[elementIndex] &= ~(mask << shift);  // 清除第一个元素中的数据
		array[elementIndex] |= (data & ((1 << bitsInFirstElement) - 1)) << shift;  // 写入第一个元素的数据

		array[elementIndex + 1] &= ~((1 << bitsInSecondElement) - 1);  // 清除第二个元素中的数据
		array[elementIndex + 1] |= (data >> bitsInFirstElement);  // 写入第二个元素的数据
	}
}

// 从unsigned int型数组中读取n位数据
unsigned int readBitData(const vector<unsigned int>& array, unsigned int index, int n) {
	unsigned int element_index = index / 32;
	unsigned int bit_position = index % 32;

	unsigned int mask = (unsigned)((long long)pow(2, n) - 1);
	unsigned int data = (array[element_index] >> bit_position) & mask;

	// 如果要读取的n位跨越了两个数组项
	if (bit_position > 32 - n) {
		unsigned int next_element = array[element_index + 1];
		unsigned int remaining_bits = n - (32 - bit_position);
		data |= (next_element & ((1 << remaining_bits) - 1)) << (n - remaining_bits);
	}

	return data;
}

void getIndex(vector<vector<unsigned>>& invertedLists)
{
	//读取二进制文件
	fstream file;
	file.open("ExpIndex", ios::binary | ios::in);
	if (!file.is_open()) {
		cout << "Wrong in opening file!";
		return;
	}
	unsigned int maxdocId = 0;
	double avgLen = 0;
	for (int i = 0; i < 2000; i++)		//总共读取2000个倒排链表
	{
		vector<unsigned> t;				//一个倒排链表
		int len;
		file.read((char*)&len, sizeof(len));
		avgLen += len;
		for (int j = 0; j < len; j++)
		{
			unsigned int docId;			//文件id
			file.read((char*)&docId, sizeof(docId));
			t.push_back(docId);//加入至倒排表
			if (docId > maxdocId)
				maxdocId = docId;
		}
		sort(t.begin(), t.end());//对文档编号排序
		invertedLists.push_back(t);		//加入一个倒排表
	}
	cout << maxdocId << endl;
	cout << avgLen / 2000 << endl;
	file.close();

	cout << "here" << endl;
}

void vectorToBin(const vector<unsigned int>& data, const string& filename)
{
	// 打开二进制文件用于写入
	ofstream file(filename, ios::binary);
	if (!file.is_open()) {
		cerr << "Failed to open the file: " << filename << endl;
		return;
	}

	// 将向量中的每个元素写入文件
	for (const unsigned int& element : data) {
		file.write(reinterpret_cast<const char*>(&element), sizeof(unsigned int));
	}

	// 关闭文件
	file.close();
}

void binToVector(const string& filename, vector<unsigned int>& data)
{

	// 打开二进制文件用于读取
	ifstream file(filename, ios::binary);
	if (!file.is_open()) {
		cerr << "Failed to open the file: " << filename << endl;
		return;
	}

	// 获取文件大小
	file.seekg(0, ios::end);
	streampos fileSize = file.tellg();
	file.seekg(0, ios::beg);

	// 计算向量大小
	size_t numElements = fileSize / sizeof(unsigned int);
	data.resize(numElements);

	// 从文件读取数据到向量
	file.read(reinterpret_cast<char*>(data.data()), fileSize);

	// 关闭文件
	file.close();

	return;
}

long long dgapCompress(const vector<unsigned>& invertedIndex, vector<unsigned>& result, int& idx, int usedBit)
{
	unsigned indexLen = invertedIndex.size();
	if (indexLen == 0)
		return -1;

	vector<unsigned> deltaId;
	deltaId.push_back(invertedIndex[0]);
	unsigned maxDelta = invertedIndex[0];//最大间隔

	// d-gap前一项减去后一项,同时求最大间隔
	for (int i = 1; i < indexLen; i++)
	{
		unsigned delta = invertedIndex[i] - invertedIndex[i - 1];
		deltaId.push_back(delta);
		if (delta > maxDelta)
			maxDelta = delta;
	}

	unsigned maxBitNum = ceil(log2(maxDelta + 1));//整体最多使用maxBitNum位存储

	long long bitCnt = usedBit + 32 + 6 + maxBitNum * indexLen;//总共使用bit数 = 以使用的bit数+存该链表要使用的bit数
	result.resize((int)ceil(bitCnt / 32.0));

	writeBitData(result, idx, indexLen, 32);//写入元素个数
	writeBitData(result, idx + 32, maxBitNum, 6);//写入delta元素长度

	idx += 38;//从index+38位开始写压缩后的delta
	for (int i = 0; i < indexLen; i++)
	{
		writeBitData(result, idx, deltaId[i], maxBitNum);
		idx += maxBitNum;
	}
	return bitCnt;
}

// invertedIndex: dgap压缩好的倒排索引
// result：解缩后的索引
// idx：从第idx个bit开始解压
// return：解压后大小（bit)
int dgapDecompress(const vector<unsigned>& compressedLists, vector<unsigned>& result, int& idx)
{
	//前32位是长度
	unsigned len = readBitData(compressedLists, idx, 32);
	// cout << len << endl;
	if (len == 0)
		return -1;

	//6位是用的Bit数
	int bitNum = (int)readBitData(compressedLists, idx + 32, 6);
	idx += 38;

	unsigned delta = readBitData(compressedLists, idx, bitNum);
	idx += bitNum;
	result.push_back(delta);//第一个delta直接进去
	for (unsigned i = 1; i < len; i++)
	{
		delta = readBitData(compressedLists, idx, bitNum);
		idx += bitNum;
		result.push_back(result[i - 1] + delta);//后续的都要加上前一个放进去
	}
	return idx;
}

void dgapDecompressAll(const vector<unsigned>& compressedLists, vector<vector<unsigned>>& result)
{
	vector<unsigned> curList;
	int idx = 0;
	for (int i = 0; i < LIST_NUM; i++) //解压2000个链表,结果放在decompressed
	{
		dgapDecompress(compressedLists, curList, idx);
		result.push_back(curList);
		curList.clear();
	}
}

void dgapDeMpi(const vector<unsigned>& compressedLists, vector<vector<unsigned>>& result,int rank, int proNum)
{
	int idx = 0;
	int listIndex[LIST_NUM] = { 0 };
	int bitNum;

	//读取每个链表的长度和每个docID所用的bit数
	for (int i = 0; i < LIST_NUM; i++)
	{
		// 前32位是长度
		int len = readBitData(compressedLists, idx, 32);

		//6位是用的Bit数
		bitNum = (int)readBitData(compressedLists, idx + 32, 6);

		// 下一个长度的位置为+=32 + 6 + len*bitNum,同时保存
		idx += 32 + 6 + len * bitNum;
		if (i != LIST_NUM - 1)
			listIndex[i + 1] = idx;
	}

	int start = LIST_NUM / proNum * rank;
	int end = min(LIST_NUM / proNum * (rank + 1), LIST_NUM);
	for (int i = start; i < end; i++)
	{
		vector<unsigned> curList;
		dgapDecompress(compressedLists, curList, listIndex[i]);
		result.push_back(curList);
	}
}

vector<unsigned> dgapDecompressOmp(const vector<unsigned>& compressedLists, int& idx)
{
	//前32位是长度
	unsigned len = readBitData(compressedLists, idx, 32);
	vector<unsigned> result;
	result.resize(len);
	//printf("%d\n", len);
	if (len == 0)
		return result;
	//6位是用的Bit数
	int bitNum = (int)readBitData(compressedLists, idx + 32, 6);
	idx += 38;

	int seq_num = len / NUM_THREADS;// 每个线程处理元素数目
	if (seq_num == 0) {// 特殊情况，每段0个
		unsigned delta = readBitData(compressedLists, idx, bitNum);
		idx += bitNum;
		result[0] = delta;//第一个delta直接进去
		for (int i = 1; i < len; i++) {
			delta = readBitData(compressedLists, idx, bitNum);
			idx += bitNum;
			result[i] = result[i - 1] + delta;//后续的都要加上前一个放进去
		}
		return result;
	}

#pragma omp parallel num_threads(NUM_THREADS)
	{
		int tid = omp_get_thread_num();// 当前线程tid
		int localIdx = idx + bitNum * tid * seq_num;
		unsigned delta = readBitData(compressedLists, localIdx, bitNum);// 找出该段第一个元素
		localIdx += bitNum;// 读走一个delta
		result[tid * seq_num] = delta;
		for (int j = 0; j < seq_num - 1; j++) {// 线程内进行前缀和
			delta = readBitData(compressedLists, localIdx, bitNum);
			localIdx += bitNum;
			result[tid * seq_num + j + 1] = delta + result[tid * seq_num + j];
		}
#pragma omp barrier
	}
#pragma omp single
	// 处理边界位置，方便后面并行使用
	for (int i = 2; i <= NUM_THREADS; i++)// 好像有隐式路障？
		result[i * seq_num - 1] += result[(i - 1) * seq_num - 1];

#pragma omp parallel num_threads(NUM_THREADS)
	{
		int tid = omp_get_thread_num();
		if (tid != 0) {// 0号线程不用做
			//for (int j = 0; j < seq_num - 1; j++)// 其余线程每个元素加前面段的末尾元素
			//	result[tid * seq_num + j] += result[tid * seq_num - 1];
			int j = 0;
			// 使用simd做，一次加4个元素，注释以下部分即回到串行执行
			__m128i scalarValue = _mm_set1_epi32(result[tid * seq_num - 1]);
			for (; j < seq_num - 1 - 3; j += 4) {// 使用simd
				__m128i vector = _mm_loadu_si128((__m128i*) & result[tid * seq_num + j]);
				__m128i sum = _mm_add_epi32(scalarValue, vector);
				_mm_storeu_si128((__m128i*) & result[tid * seq_num + j], sum);
			}
			for (; j < seq_num - 1; j++)// 处理剩余元素
				result[tid * seq_num + j] += result[tid * seq_num - 1];
		}
	}
	idx += bitNum * NUM_THREADS * seq_num;// 调整idx指针
	// 串行处理剩余元素
#pragma single
	if (len % NUM_THREADS != 0) {
		for (int i = NUM_THREADS * seq_num; i < len; i++) {
			unsigned delta = readBitData(compressedLists, idx, bitNum);
			idx += bitNum;
			result[i] = result[i - 1] + delta;
		}
	}
	return result;
}

void dgapDeMpiOmp(const vector<unsigned>& compressedLists, vector<vector<unsigned>>& result, int rank, int proNum)
{
	int idx = 0;
	int listIndex[LIST_NUM] = { 0 };
	int bitNum;

	//读取每个链表的长度和每个docID所用的bit数
	for (int i = 0; i < LIST_NUM; i++)
	{
		// 前32位是长度
		int len = readBitData(compressedLists, idx, 32);

		//6位是用的Bit数
		bitNum = (int)readBitData(compressedLists, idx + 32, 6);

		// 下一个长度的位置为+=32 + 6 + len*bitNum,同时保存
		idx += 32 + 6 + len * bitNum;
		if (i != LIST_NUM - 1)
			listIndex[i + 1] = idx;
	}

	int start = LIST_NUM / proNum * rank;
	int end = min(LIST_NUM / proNum * (rank + 1), LIST_NUM);
	for (int i = start; i < end; i++)
	{
		vector<unsigned> curList;
		curList = dgapDecompressOmp(compressedLists, listIndex[i]);
		result.push_back(curList);
	}
}
int main(int argc, char* argv[])
{
	//---------------------------------------读取---------------------------------------
	vector<vector<unsigned>> invertedLists;//读取的链表
	vector<vector<unsigned>> decompressed;//解压后的链表应该等于invertedLists

	//计时
	long long head, tail, freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

	//getIndex(invertedLists);//读取倒排链表

	//计时终止
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	cout << "Read(orignal) Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;

	vector<unsigned> compressedRes;//压缩结果
	vector<unsigned> compressed; //读出来的压缩链表
	vector<unsigned> curList;//当前解压的链表

	//---------------------------------------压缩---------------------------------------------
	int idx = 0;
	//for (int i = 0; i < invertedLists.size(); i++) // 对每个链表进行压缩，存到一个vector<unsigned> compressedRes中
	//    dgapCompress(invertedLists[i], compressedRes, idx, idx);
	//
	//vectorToBin(compressedRes, "compress.bin");

	//---------------------------------------解压---------------------------------------------
	MPI_Init(&argc, &argv);
	int rank, proNum;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &proNum);

	//计时
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

	//从第0个bit开始读数据，解压
	binToVector("compress.bin", compressed);//读取到compressed中，应该compressed等于compressedRes

	//dgapDecompressAll(compressed, decompressed);
	dgapDeMpiOmp(compressed, decompressed,rank,proNum);

	//计时终止
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	cout << "Decompresss Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;

	MPI_Finalize();
	return 0;
}