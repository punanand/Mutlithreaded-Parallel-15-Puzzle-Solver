#include <stdio.h>
#include <stdlib.h>

int a = 2;

void fun(int * tmp) {
	*tmp = 0;
} 

int main() {
	fun(&a);
	printf("%d\n", a);
}