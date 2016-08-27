#include <iostream>
#include <fstream>
#include <bitset>
#include <iomanip>
using namespace std;

unsigned int fadd (unsigned int a, unsigned int b);

int main()
{
    ifstream file;
    file.open("input.txt",ios::in);
    int testcounter;
    int passcounter = 0;
    int failcounter = 0;
    char comcheck;
    unsigned int a,b,c;

    while(file>>dec>>testcounter>>hex>>a>>hex>>b>>hex>>c){
    
   
    
    unsigned int ans = fadd(a,b);
    //cout<<"0x"<<setfill('0')<<setw(8)<<hex<<ans<<endl;
    //cout<<"0x"<<setfill('0')<<setw(8)<<hex<<c<<endl;

    if (ans == c){
        cout<<"Test "<<dec<<testcounter<<" PASSED"<<endl;
        passcounter++;
    }
    else {
        cout<<"Test "<<dec<<testcounter<<" FAILED"<<endl;
        failcounter++;
    }
   }
    cout<<"Total "<<dec<<passcounter<<" PASSED "<<dec<<failcounter<<" FAILED."<<endl;
    file.close();
    return 0;
}


unsigned int fadd (unsigned int a, unsigned int b) {
    
    //Taking care of zero cases
    if (a==0){
        return b;
    }
    if (b==0){
        return a;
    }

    //Extracting information from a and b
    unsigned int a_sign = (a & 0x80000000)>>31;
    unsigned int a_enc_exp = (a & 0x7f800000)>>23;
    unsigned int a_mantissa = (a & 0x7fffff);


    unsigned int b_sign = (b & 0x80000000)>>31;
    unsigned int b_enc_exp = (b & 0x7f800000)>>23;
    unsigned int b_mantissa = (b & 0x7fffff);


    unsigned int a_significand = (a_enc_exp >= 1) ? (a_mantissa | (1<<23)) : a_mantissa;
    unsigned int b_significand = (b_enc_exp >= 1) ? (b_mantissa | (1<<23)) : b_mantissa;


    //Initially shifting a and b 7 bits left to increase precison for rounding
    unsigned int a_shift_significand = (a_significand << 7);
    unsigned int b_shift_significand = (b_significand << 7);

    //Taking care of denormal numbers
    unsigned int a_exp = ((a_enc_exp == 0) ? 1 : a_enc_exp);
    unsigned int b_exp = ((b_enc_exp == 0) ? 1 : b_enc_exp);
    unsigned int ans_exp;
    unsigned int ans_significand;
    unsigned int ans_sign;
    bool ans_denormal = false;

    /* Special Cases */

    //Case when a is NaN
    if (a_exp == 255 && a_mantissa != 0){
        return 0x7fffffff;
    }

    //Case when b is NaN
    if (b_exp == 255 && b_mantissa != 0){
        return 0x7fffffff;
    }

    //Case when Infinity - Infinity
    if (a_exp == 255 && a_mantissa == 0 && b_exp == 255 && b_mantissa == 0 && a_sign != b_sign){
        return 0x7fffffff;
    }

    //Case when a is Infinity
    if (a_exp == 255 && a_mantissa == 0){
        return a;
    }

    //Case when b is Infinty
    if (b_exp == 255 && b_mantissa == 0){
        return b;
    }

    /* Making Exponent Same */


    if (a_exp >= b_exp){
        unsigned int shift = a_exp-b_exp;

        b_shift_significand = (b_shift_significand >> ((shift>31) ? 31 : shift) );
        ans_exp = a_exp;



    }
    else {
        unsigned int shift = b_exp-a_exp;
        a_shift_significand = (a_shift_significand >> ((shift>31) ? 31 : shift) );

        ans_exp = b_exp;
    }

    /* Adding Significands */
    if (a_sign == b_sign){
        ans_significand = a_shift_significand + b_shift_significand;
        ans_sign = a_sign;

    }
    else {
        if (a_shift_significand > b_shift_significand){
            ans_sign = a_sign;
            ans_significand = a_shift_significand - b_shift_significand;
        }
        else if (a_shift_significand < b_shift_significand){
            ans_sign = b_sign;
            ans_significand = b_shift_significand - a_shift_significand;
        }
        else if ((a_shift_significand == b_shift_significand)) {
            ans_sign = 0;
            ans_significand = a_shift_significand - b_shift_significand;

        }
    }

    /* Normalization */
    int i;
    for (i=31; i>0 && ((ans_significand>>i) == 0); i-- ){;}
    
    if (i>23){

        //Rounding
        unsigned int residual = ((ans_significand&(1<<(i-23-1)))>>(i-23-1));

        unsigned int sticky = 0;
        for(int j=0;j<i-23-1;j++){
            sticky = sticky | ((ans_significand & (1<<j))>>j);
        }

        if ((int(ans_exp) + (i-23) - 7) > 0 && (int(ans_exp) + (i-23) - 7) < 255){

            ans_significand = (ans_significand>>(i-23));

            ans_exp = ans_exp + (i-23) - 7;

            if (residual==1 && sticky == 1){
                ans_significand += 1;

            }
            else if ((ans_significand&1)==1 && residual ==1 && sticky == 0){
                ans_significand += 1;

            }

            if ((ans_significand>>24)==1){
                ans_significand = (ans_significand>>1);
                ans_exp += 1;

            }
        }

        //Denormal number
        else if (int(ans_exp) + (i-23) - 7 <= 0) {
            ans_denormal = true;
            ans_significand = ans_significand>>7;
            ans_significand = ans_significand<<(ans_exp-1);
            ans_exp = 0;
        }

        //Overflow
        else if (int(ans_exp) + (i-23) - 7 >= 255){
            ans_significand = (1<<23);
            ans_exp = 255;
        }

    }
    else if (i<=23 && i!=0){
        if ((int(ans_exp) - (23-i) - 7) > 0 && (int(ans_exp) - (23-i) - 7) < 255){
            ans_significand = (ans_significand<<(23-i));
            ans_exp = ans_exp - (23-i) - 7;
        }

        //Denormal Number
        else if (int(ans_exp) - (23-i) - 7 <= 0) {
            ans_denormal = true;
            ans_significand = ans_significand>>7;
            ans_significand = ans_significand<<(ans_exp-1);
            ans_exp = 0;
        }

        //Overflow
        else if ((int(ans_exp) - (23-i) - 7) >= 255){
            ans_significand = (1<<23);
            ans_exp = 255;
        }


    }

    //When answer is zero
    else if (i==0 && ans_exp < 255){
        ans_exp = 0;
    }

    /* Constructing floating point number from sign, exponent and significand */

    unsigned int ans = (ans_sign<<31) | (ans_exp<<23) | (ans_significand& (0x7FFFFF));
    return ans;
}