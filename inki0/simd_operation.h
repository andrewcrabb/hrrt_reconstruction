#ifndef simd_operation_h
#define simd_operation_h
void mulprojection(float ***prj1,float ***prj2,int theta);
/*divideprojection not a simd operation because done only when denum not 0,
it is added to get a complete set 
*/

void divideprojection(float ***prj1,float ***prj2,int theta);
void addprojection(float ***prj1,float ***prj2,int theta);
void subprojection(float ***prj1,float ***prj2,int theta);
void setnegative2zero(float ***largeprj);
#endif


