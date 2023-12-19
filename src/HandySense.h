#ifndef __HANDY_SENSE__
#define __HANDY_SENSE__

void setGetTempFunction(int(*)(float*)) ;
void setGetHumiFunction(int(*)(float*)) ;
void setGetSoilFunction(int(*)(float*)) ;
void setGetLightFunction(int(*)(float*)) ;

void HandySenseSetup() ;
void HandySenseLoop() ;

#endif
