#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>


typedef struct evento{
	int id;
	bool detectado;
	bool impresso;
	int nodoIdentificador;
	int nodoIdentificado;
	int TestRound;
	int novoEstado;
	bool itwasdiagnosed;
	int testNumberDiagnosed;
	int testNumberIdentificado;
	struct evento* next;
}evento;


void insertEvento(evento* head, evento newEvento, int id);
void printId(evento* head);
void removeEvento(evento* head, int idEvento);
int returnEventId(evento* head, int nodo, int timestamp);
bool returnEventDiagnosed(evento* head, int idEvent);
