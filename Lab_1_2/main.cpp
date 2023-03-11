#include <iostream>
#include <windows.h>
#include <stdlib.h>

using namespace std;

const int n=1048576;
int a[n],sum;

void recur(int*a,int n){
    if(n==1)return;
    else{
        for(int i=0;i<n/2;i++)
            a[i]+=a[n-i-1];//后面的加到前面去
        n=n/2;
        recur(a,n);
    }
}

int main()
{
    long long head,tail,freq;//计时器



    for(int k=64;k<=n;k*=2){

        for (int i = 0; i < k; i++)//初始化
        {
            a[i]=1;
        }
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        QueryPerformanceCounter((LARGE_INTEGER*)&head);


        //平凡算法
            sum=0;
            for (int i = 0; i < k; i++) {
                sum+=a[i];
            }
        //优化算法
        //多路链式
//            int sum1=0,sum2=0;
//            for(int i=0;i<k;i+=2){
//                sum1+=a[i];
//                sum2+=a[i+1];
//            }
//            sum=sum1+sum2;
        //二重循环实现两两相加
//            for(int j=k;j>1;j/=2){
//                for(int i=0;i<j/2;i++){
//                    a[i]=a[i*2]+a[i*2+1];
//                    //a[i+1]=a[(i+1)*2]+a[(i+1)*2+1];
//                }
//            }
//            sum=a[0];
        //递归实现两两相加
//        recur(a,k);
//        sum=a[0];


        QueryPerformanceCounter((LARGE_INTEGER*)&tail);
//        cout<<k<<endl;
        cout<<(tail-head)*1000.0/freq<<endl;
        //cout<<sum<<endl;
    }
    return 0;
}
