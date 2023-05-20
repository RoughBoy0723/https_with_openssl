#include <stdio.h>
void aa(int* a){
	*a=20;
}
int main(){
	int a;
	a=10;
	aa(&a);
	printf("%d",a);
}




