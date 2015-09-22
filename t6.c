#include <stdio.h>
#include <stdlib.h>

/*
* Inter-procedural analysis 
* There is a leak address through implicit flow between malloc and printf inside the function foo ()
*/

void foo (int *i) {
	int *x;
	x = i++;
	printf ("%d", (int)x);
}


int main (int argc, char **argv) {
	int *i, input;
	
	i = (int *) malloc (8);

	foo(i);
}
