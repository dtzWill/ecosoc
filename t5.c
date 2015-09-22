#include <stdio.h>
#include <stdlib.h>

/*
* Inter-procedural analysis
* The printf function in each functions foo() and bar() implicitly is influenced by the predicate (int)i>0 in the main function
*/

void foo () {
	printf ("1");
}

void bar () {
	printf ("2");
}

int main (int argc, char **argv) {
	int *i, input;
	
	i = (int *) malloc (8);

	if ((int)i>0) {
		foo();
	}else {
		bar();	
	}
}
