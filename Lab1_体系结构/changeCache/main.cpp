#include <iostream>
#include <windows.h>
#include <stdlib.h>

using namespace std;

const int n=10000;
int a[n][n],b[n],sum[n];

int main()
{
    long long head,tail,freq;//计时器

    for (int i = 0; i < n; i++)//初始化
    {
        for (int j = 0; j < n; j++)
            a[i][j] = i + j;
    }

    for(int i=0;i<n;i++){
        sum[i]=0;
    }

    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    QueryPerformanceCounter((LARGE_INTEGER*)&head);
    //平凡算法
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++)
            {
                sum[i] += a[j][i] * b[j];
            }
        }
    //优化算法
//    for(int j=0;j<n;j++)
//        for(int i=0;i<n;i++)
//            sum[i]+=a[j][i]*b[j];


    QueryPerformanceCounter((LARGE_INTEGER*)&tail);

    cout<<"n="<<n<<" time:"<<(tail-head)*1000.0/freq<<"ms"<<endl;

    return 0;
}


