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


    int gap=100;
    for(int k=100;k<=n;k+=gap){
        for(int i=0;i<k;i++){
            sum[i]=0;
        }

        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        QueryPerformanceCounter((LARGE_INTEGER*)&head);
        //平凡算法
//        for (int i = 0; i < k; i++) {
//            for (int j = 0; j < k; j++)
//            {
//                sum[i] += a[j][i] * b[j];
//            }
//        }
        //优化算法
        for(int j=0;j<k;j++)
            for(int i=0;i<k;i++)
                sum[i]+=a[j][i]*b[j];

        //循环展开
//        for(int i=0;i<k;i++)
//            for(int j=0;j<k;j+=2)
//                sum[i]+=a[j][i]*b[j]+a[j+1][i]*b[j+1];

        QueryPerformanceCounter((LARGE_INTEGER*)&tail);
//        cout<<(tail-head)*1000.0/freq<<endl;
        cout<<"n="<<k<<" time:"<<(tail-head)*1000.0/freq<<"ms"<<endl;
        if(k==1000)gap=1500;
        if(k==5500)gap=4500;


    }

    return 0;
}
