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
// ��nλ����д�뵽int��������
void writeBitData(vector<unsigned int>& array, unsigned int index, unsigned int data, int n)
{
	int shift = index % 32;
	int elementIndex = index / 32;
	int remainingBits = 32 - shift;
	unsigned mask = (long long)pow(2, n) - 1;
	if (remainingBits >= n) {
		// ��Խһ������Ԫ�ص����
		array[elementIndex] &= ~(mask << shift);  // �����Ӧλ������
		array[elementIndex] |= (data & mask) << shift;  // д��������
	}
	else {
		// ��Խ��������Ԫ�ص����
		int bitsInFirstElement = remainingBits;
		int bitsInSecondElement = n - remainingBits;

		array[elementIndex] &= ~(mask << shift);  // �����һ��Ԫ���е�����
		array[elementIndex] |= (data & ((1 << bitsInFirstElement) - 1)) << shift;  // д���һ��Ԫ�ص�����

		array[elementIndex + 1] &= ~((1 << bitsInSecondElement) - 1);  // ����ڶ���Ԫ���е�����
		array[elementIndex + 1] |= (data >> bitsInFirstElement);  // д��ڶ���Ԫ�ص�����
	}
}

// ��unsigned int�������ж�ȡnλ����
unsigned int readBitData(const vector<unsigned int>& array, unsigned int index, int n) {
	unsigned int element_index = index / 32;
	unsigned int bit_position = index % 32;

	unsigned int mask = (unsigned)((long long)pow(2, n) - 1);
	unsigned int data = (array[element_index] >> bit_position) & mask;

	// ���Ҫ��ȡ��nλ��Խ������������
	if (bit_position > 32 - n) {
		unsigned int next_element = array[element_index + 1];
		unsigned int remaining_bits = n - (32 - bit_position);
		data |= (next_element & ((1 << remaining_bits) - 1)) << (n - remaining_bits);
	}

	return data;
}

void getIndex(vector<vector<unsigned>>& invertedLists)
{
	//��ȡ�������ļ�
	fstream file;
	file.open("ExpIndex", ios::binary | ios::in);
	if (!file.is_open()) {
		cout << "Wrong in opening file!";
		return;
	}
	unsigned int maxdocId = 0;
	double avgLen = 0;
	for (int i = 0; i < 2000; i++)		//�ܹ���ȡ2000����������
	{
		vector<unsigned> t;				//һ����������
		int len;
		file.read((char*)&len, sizeof(len));
		avgLen += len;
		for (int j = 0; j < len; j++)
		{
			unsigned int docId;			//�ļ�id
			file.read((char*)&docId, sizeof(docId));
			t.push_back(docId);//���������ű�
			if (docId > maxdocId)
				maxdocId = docId;
		}
		sort(t.begin(), t.end());//���ĵ��������
		invertedLists.push_back(t);		//����һ�����ű�
	}
	cout << maxdocId << endl;
	cout << avgLen / 2000 << endl;
	file.close();

	cout << "here" << endl;
}

void vectorToBin(const vector<unsigned int>& data, const string& filename)
{
	// �򿪶������ļ�����д��
	ofstream file(filename, ios::binary);
	if (!file.is_open()) {
		cerr << "Failed to open the file: " << filename << endl;
		return;
	}

	// �������е�ÿ��Ԫ��д���ļ�
	for (const unsigned int& element : data) {
		file.write(reinterpret_cast<const char*>(&element), sizeof(unsigned int));
	}

	// �ر��ļ�
	file.close();
}

void binToVector(const string& filename, vector<unsigned int>& data)
{

	// �򿪶������ļ����ڶ�ȡ
	ifstream file(filename, ios::binary);
	if (!file.is_open()) {
		cerr << "Failed to open the file: " << filename << endl;
		return;
	}

	// ��ȡ�ļ���С
	file.seekg(0, ios::end);
	streampos fileSize = file.tellg();
	file.seekg(0, ios::beg);

	// ����������С
	size_t numElements = fileSize / sizeof(unsigned int);
	data.resize(numElements);

	// ���ļ���ȡ���ݵ�����
	file.read(reinterpret_cast<char*>(data.data()), fileSize);

	// �ر��ļ�
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
	unsigned maxDelta = invertedIndex[0];//�����

	// d-gapǰһ���ȥ��һ��,ͬʱ�������
	for (int i = 1; i < indexLen; i++)
	{
		unsigned delta = invertedIndex[i] - invertedIndex[i - 1];
		deltaId.push_back(delta);
		if (delta > maxDelta)
			maxDelta = delta;
	}

	unsigned maxBitNum = ceil(log2(maxDelta + 1));//�������ʹ��maxBitNumλ�洢

	long long bitCnt = usedBit + 32 + 6 + maxBitNum * indexLen;//�ܹ�ʹ��bit�� = ��ʹ�õ�bit��+�������Ҫʹ�õ�bit��
	result.resize((int)ceil(bitCnt / 32.0));

	writeBitData(result, idx, indexLen, 32);//д��Ԫ�ظ���
	writeBitData(result, idx + 32, maxBitNum, 6);//д��deltaԪ�س���

	idx += 38;//��index+38λ��ʼдѹ�����delta
	for (int i = 0; i < indexLen; i++)
	{
		writeBitData(result, idx, deltaId[i], maxBitNum);
		idx += maxBitNum;
	}
	return bitCnt;
}

// invertedIndex: dgapѹ���õĵ�������
// result�������������
// idx���ӵ�idx��bit��ʼ��ѹ
// return����ѹ���С��bit)
int dgapDecompress(const vector<unsigned>& compressedLists, vector<unsigned>& result, int& idx)
{
	//ǰ32λ�ǳ���
	unsigned len = readBitData(compressedLists, idx, 32);
	// cout << len << endl;
	if (len == 0)
		return -1;

	//6λ���õ�Bit��
	int bitNum = (int)readBitData(compressedLists, idx + 32, 6);
	idx += 38;

	unsigned delta = readBitData(compressedLists, idx, bitNum);
	idx += bitNum;
	result.push_back(delta);//��һ��deltaֱ�ӽ�ȥ
	for (unsigned i = 1; i < len; i++)
	{
		delta = readBitData(compressedLists, idx, bitNum);
		idx += bitNum;
		result.push_back(result[i - 1] + delta);//�����Ķ�Ҫ����ǰһ���Ž�ȥ
	}
	return idx;
}

void dgapDecompressAll(const vector<unsigned>& compressedLists, vector<vector<unsigned>>& result)
{
	vector<unsigned> curList;
	int idx = 0;
	for (int i = 0; i < LIST_NUM; i++) //��ѹ2000������,�������decompressed
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

	//��ȡÿ������ĳ��Ⱥ�ÿ��docID���õ�bit��
	for (int i = 0; i < LIST_NUM; i++)
	{
		// ǰ32λ�ǳ���
		int len = readBitData(compressedLists, idx, 32);

		//6λ���õ�Bit��
		bitNum = (int)readBitData(compressedLists, idx + 32, 6);

		// ��һ�����ȵ�λ��Ϊ+=32 + 6 + len*bitNum,ͬʱ����
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
	//ǰ32λ�ǳ���
	unsigned len = readBitData(compressedLists, idx, 32);
	vector<unsigned> result;
	result.resize(len);
	//printf("%d\n", len);
	if (len == 0)
		return result;
	//6λ���õ�Bit��
	int bitNum = (int)readBitData(compressedLists, idx + 32, 6);
	idx += 38;

	int seq_num = len / NUM_THREADS;// ÿ���̴߳���Ԫ����Ŀ
	if (seq_num == 0) {// ���������ÿ��0��
		unsigned delta = readBitData(compressedLists, idx, bitNum);
		idx += bitNum;
		result[0] = delta;//��һ��deltaֱ�ӽ�ȥ
		for (int i = 1; i < len; i++) {
			delta = readBitData(compressedLists, idx, bitNum);
			idx += bitNum;
			result[i] = result[i - 1] + delta;//�����Ķ�Ҫ����ǰһ���Ž�ȥ
		}
		return result;
	}

#pragma omp parallel num_threads(NUM_THREADS)
	{
		int tid = omp_get_thread_num();// ��ǰ�߳�tid
		int localIdx = idx + bitNum * tid * seq_num;
		unsigned delta = readBitData(compressedLists, localIdx, bitNum);// �ҳ��öε�һ��Ԫ��
		localIdx += bitNum;// ����һ��delta
		result[tid * seq_num] = delta;
		for (int j = 0; j < seq_num - 1; j++) {// �߳��ڽ���ǰ׺��
			delta = readBitData(compressedLists, localIdx, bitNum);
			localIdx += bitNum;
			result[tid * seq_num + j + 1] = delta + result[tid * seq_num + j];
		}
#pragma omp barrier
	}
#pragma omp single
	// ����߽�λ�ã�������沢��ʹ��
	for (int i = 2; i <= NUM_THREADS; i++)// ��������ʽ·�ϣ�
		result[i * seq_num - 1] += result[(i - 1) * seq_num - 1];

#pragma omp parallel num_threads(NUM_THREADS)
	{
		int tid = omp_get_thread_num();
		if (tid != 0) {// 0���̲߳�����
			//for (int j = 0; j < seq_num - 1; j++)// �����߳�ÿ��Ԫ�ؼ�ǰ��ε�ĩβԪ��
			//	result[tid * seq_num + j] += result[tid * seq_num - 1];
			int j = 0;
			// ʹ��simd����һ�μ�4��Ԫ�أ�ע�����²��ּ��ص�����ִ��
			__m128i scalarValue = _mm_set1_epi32(result[tid * seq_num - 1]);
			for (; j < seq_num - 1 - 3; j += 4) {// ʹ��simd
				__m128i vector = _mm_loadu_si128((__m128i*) & result[tid * seq_num + j]);
				__m128i sum = _mm_add_epi32(scalarValue, vector);
				_mm_storeu_si128((__m128i*) & result[tid * seq_num + j], sum);
			}
			for (; j < seq_num - 1; j++)// ����ʣ��Ԫ��
				result[tid * seq_num + j] += result[tid * seq_num - 1];
		}
	}
	idx += bitNum * NUM_THREADS * seq_num;// ����idxָ��
	// ���д���ʣ��Ԫ��
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

	//��ȡÿ������ĳ��Ⱥ�ÿ��docID���õ�bit��
	for (int i = 0; i < LIST_NUM; i++)
	{
		// ǰ32λ�ǳ���
		int len = readBitData(compressedLists, idx, 32);

		//6λ���õ�Bit��
		bitNum = (int)readBitData(compressedLists, idx + 32, 6);

		// ��һ�����ȵ�λ��Ϊ+=32 + 6 + len*bitNum,ͬʱ����
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
	//---------------------------------------��ȡ---------------------------------------
	vector<vector<unsigned>> invertedLists;//��ȡ������
	vector<vector<unsigned>> decompressed;//��ѹ�������Ӧ�õ���invertedLists

	//��ʱ
	long long head, tail, freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

	//getIndex(invertedLists);//��ȡ��������

	//��ʱ��ֹ
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	cout << "Read(orignal) Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;

	vector<unsigned> compressedRes;//ѹ�����
	vector<unsigned> compressed; //��������ѹ������
	vector<unsigned> curList;//��ǰ��ѹ������

	//---------------------------------------ѹ��---------------------------------------------
	int idx = 0;
	//for (int i = 0; i < invertedLists.size(); i++) // ��ÿ���������ѹ�����浽һ��vector<unsigned> compressedRes��
	//    dgapCompress(invertedLists[i], compressedRes, idx, idx);
	//
	//vectorToBin(compressedRes, "compress.bin");

	//---------------------------------------��ѹ---------------------------------------------
	MPI_Init(&argc, &argv);
	int rank, proNum;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &proNum);

	//��ʱ
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

	//�ӵ�0��bit��ʼ�����ݣ���ѹ
	binToVector("compress.bin", compressed);//��ȡ��compressed�У�Ӧ��compressed����compressedRes

	//dgapDecompressAll(compressed, decompressed);
	dgapDeMpiOmp(compressed, decompressed,rank,proNum);

	//��ʱ��ֹ
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	cout << "Decompresss Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;

	MPI_Finalize();
	return 0;
}