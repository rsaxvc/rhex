#include <iostream>
#include "bin2hex.h"
#include <stdint.h>

using namespace std;

int main()
{
bin2hex b;
const int32_t test = 0x12345678;

cout<<"test="<<test<<endl;
cout<<"test=0x"<<hex<<test<<endl;
cout<<"test[0]="<<b( test>>24 & 0x000000FF )<<endl;
cout<<"test[1]="<<b( test>>16 & 0x000000FF )<<endl;
cout<<"test[2]="<<b( test>>8  & 0x000000FF )<<endl;
cout<<"test[3]="<<b( test>>0  & 0x000000FF )<<endl;
}
