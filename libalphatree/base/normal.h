//
// Created by yanyu on 2017/8/11.
//

#ifndef ALPHATREE_NORMAL_H
#define ALPHATREE_NORMAL_H

#include <math.h>

const float PI=3.1415926;

float normalFunction(float z)
{
    float temp;
    temp=exp((-1)*z*z/2)/sqrt(2*PI);
    return temp;

}
/***************************************************************/
/* 返回标准正态分布的累积函数，该分布的平均值为 0，标准偏差为 1。      */
/***************************************************************/
float normSDist(const float z)
{
    // this guards against overflow
    if(z > 6) return 1;
    if(z < -6) return 0;

    static const float gamma =  0.231641900,
            a1  =  0.319381530,
            a2  = -0.356563782,
            a3  =  1.781477973,
            a4  = -1.821255978,
            a5  =  1.330274429;

    float k = 1.0 / (1 + fabs(z) * gamma);
    float n = k * (a1 + k * (a2 + k * (a3 + k * (a4 + k * a5))));
    n = 1 - normalFunction(z) * n;
    if(z < 0)
        return 1.0 - n;

    return n;
}


/***************************************************************/
/* 返回标准正态分布累积函数的逆函数。该分布的平均值为 0，标准偏差为 1。 */
/***************************************************************/
float normsinv(const float p)
{
    static const float LOW  = 0.02425;
    static const float HIGH = 0.97575;

    /* Coefficients in rational approximations. */
    static const float a[] =
            {
                    -3.969683028665376e+01,
                    2.209460984245205e+02,
                    -2.759285104469687e+02,
                    1.383577518672690e+02,
                    -3.066479806614716e+01,
                    2.506628277459239e+00
            };

    static const float b[] =
            {
                    -5.447609879822406e+01,
                    1.615858368580409e+02,
                    -1.556989798598866e+02,
                    6.680131188771972e+01,
                    -1.328068155288572e+01
            };

    static const float c[] =
            {
                    -7.784894002430293e-03,
                    -3.223964580411365e-01,
                    -2.400758277161838e+00,
                    -2.549732539343734e+00,
                    4.374664141464968e+00,
                    2.938163982698783e+00
            };

    static const float d[] =
            {
                    7.784695709041462e-03,
                    3.224671290700398e-01,
                    2.445134137142996e+00,
                    3.754408661907416e+00
            };

    float q, r;

    //errno = 0;

    if (p < 0 || p > 1)
    {
        //errno = EDOM;
        return 0.0;
    }
    else if (p == 0)
    {
        //errno = ERANGE;
        return -HUGE_VAL /* minus "infinity" */;
    }
    else if (p == 1)
    {
        //errno = ERANGE;
        return HUGE_VAL /* "infinity" */;
    }
    else if (p < LOW)
    {
        /* Rational approximation for lower region */
        q = sqrt(-2*log(p));
        return (((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
               ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1);
    }
    else if (p > HIGH)
    {
        /* Rational approximation for upper region */
        q  = sqrt(-2*log(1-p));
        return -(((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
               ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1);
    }
    else
    {
        /* Rational approximation for central region */
        q = p - 0.5;
        r = q*q;
        return (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5])*q /
               (((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1);
    }
}

#endif //ALPHATREE_NORMAL_H
