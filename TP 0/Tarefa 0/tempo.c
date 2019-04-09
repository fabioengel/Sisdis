/*
 * Autor: Fábio Engel de Camargo
 * Disciplina: Sistemas Distribuídos INFO7046
 * Tarefa 0: Editar, compilar e executar tempo.c
 */

#include <stdlib.h>
#include <stdio.h>
#include "smpl.h"
/* Neste programa cada nodo conta seu tempo */

// Eventos
#define TEST 1
#define FAULT 2
#define REPAIR 3

/* descritor do nodo */
typedef struct{
	int id;
}tnodo;

tnodo *nodo;

/* é nesta estrutura que mantemos informações locais dos nodos */

int main(int argc, char * argv[]){
	static int N, //Número de nodos
				token, event,r,i;

	static char fa_name[5];

	if(argc != 2){
		puts("Uso correto: tempo <num-nodos>");
		exit(1);
	}

	N = atoi(argv[1]);
	smpl(0, "program tempo");
	reset();
	stream(1);

	// Inicializacao
	nodo = (tnodo *) malloc(sizeof(tnodo) * N);

	for(i = 0; i < N; ++i) {
		memset(fa_name, '\0', 5);
		sprintf(fa_name, "%d", i);
		nodo[i].id = facility(fa_name, 1);
	}

	// Schedule de eventos
	for (i = 0; i < N; ++i) {
		schedule(TEST, 30.0, i);
	}

	schedule(FAULT, 35.0, 0);
	schedule(REPAIR, 65.0, 0);

	while(time() < 120.0) {
		cause(&event, &token);
		switch(event){
			case TEST:
				if(status(nodo[token].id) != 0) break;
				printf("O nodo %d testa no tempo %4.1f\n", token, time());
				schedule(TEST, 30.0, token);
				break;

			case FAULT:
				r = request(nodo[token].id, token, 0);
				if(r != 0){
					puts("Não foi possível falhar o nodo");
					exit(1);
				}
				printf("O nodo %d falhou no tempo %4.1f\n", token, time());
				break;

			case REPAIR:
				release(nodo[token].id, token);
				schedule(TEST, 30.0, token);
				printf("O nodo %d recuperou no tempo %4.1f\n", token, time());
				break;
		}

	}

}
