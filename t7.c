#include <stdio.h>
#include <stdlib.h>

/*
* Intra-procedural analysis
* There is a leak address through implicit flow between malloc and all printfs
*/



int main (int argc, char **argv) {
	int *i, input;
	
	i = (int *) malloc (8);

	if ((int *)i==0) 
		goto label1;
	else if ((int*)i>0) 
		    goto label2;
		else 
			goto label3; 	

	label1:
		printf ("1");
		return(0);
	label2:
		printf ("2");
		return(0);
	label3:
		printf ("3");
		return(0);
}
