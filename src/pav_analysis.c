        #include <math.h>
#include <stdlib.h>
#include "pav_analysis.h"

float compute_power(const float *x, unsigned int N) {
    float t=0;
    for (int i = 0; i<N ; i++){
        t += x[i]*x[i];
    }
    t = t/N;
    float p = 10*log10(t);

    return p;
}

float compute_am(const float *x, unsigned int N) {
    float t=0;
    for (int i = 0; i<N;i++){
        t += fabs(x[i]);
    }
    t = t/N;
    return t;
}

float compute_zcr(const float *x, unsigned int N, float fm) {
   
    float sum=0;
    float ZCR = 0;
    float Ni = N;



    for(int i=1;i<N;i++){
        if((x[i]<0 && x[i-1]>0) || (x[i]>0 && x[i-1]<0)){//sgn(x[N]) = (x[N] > 0) - (x[N] < 0)
        sum++;
      //  printf("%f\n",sum);
        }
    }
    ZCR = fm/(2*(N-1))*sum;
    
    //printf("%f\n",ZCR);
    return ZCR;
}
