#include <unistd.h>
#include <signal.h>
#include <stdio.h>

int flag = 1, conta = 1;

// atende alarme
void atende() {
	printf("alarme # %d\n", conta);
	flag = 1;
	conta++;
}


main() {
	(void) signal(SIGALRM, atende); // instala  rotina que atende interrupcao
	while(conta < 4) {
		if(flag) {
			alarm(3); //ativa alarme de 3s
			flag = 0;
		}
	}
	printf("Vou terminar.\n");
}
