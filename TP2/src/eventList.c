#include "eventList.h"


void insertEvento(evento* head, evento newEvento, int id){
	evento* lastEvent = head;

	while(lastEvent->next != (void *) NULL){
		lastEvent = lastEvent->next;
	}

	evento *ev;
	ev = (evento *) malloc(sizeof(evento));
	ev->id = id;
	ev->nodoIdentificador = newEvento.nodoIdentificador;
	ev->nodoIdentificado = newEvento.nodoIdentificado;
	ev->novoEstado = newEvento.novoEstado;
	ev->detectado = newEvento.detectado;
	ev->TestRound = newEvento.TestRound;
	ev->impresso = newEvento.impresso;
	ev->itwasdiagnosed = newEvento.itwasdiagnosed;
	ev->testNumberIdentificado = newEvento.testNumberIdentificado;



	ev->next = NULL;
	//insere referência:
	lastEvent->next = ev;

}

void printId(evento* head){
	evento* lastEvent = head;
	printf("id: ");
	while(lastEvent->next != (void *) NULL){
			printf("%d ", lastEvent->id);
			lastEvent = lastEvent->next;
		}
	printf("%d \n", lastEvent->id);
}

void removeEvento(evento* head, int idEvento){
	evento* eventToDelete = head;
	evento* previous = head;

	while(eventToDelete->id != idEvento){
		previous = eventToDelete;
		eventToDelete = eventToDelete->next;
		}

	if(eventToDelete->next != (void *) NULL){
		previous->next = eventToDelete->next;
	}
	else{
		previous->next = (void *) NULL;
	}

	free(eventToDelete);

}

int returnEventId(evento* head, int nodo, int timestamp){
	evento* lastEvent = head;

	while(lastEvent->next != (void *) NULL){
			if(lastEvent->nodoIdentificado == nodo && lastEvent->novoEstado == timestamp){
				return lastEvent->id;
			}
			lastEvent = lastEvent->next;
		}
	if(lastEvent->nodoIdentificado == nodo && lastEvent->novoEstado == timestamp){
		return lastEvent->id;
	}

	return(0); // se evento não encontrado
}

bool returnEventDiagnosed(evento* head, int idEvent){
	evento* lastEvent = head;
	while(lastEvent->next != (void *) NULL){
		if(lastEvent->id == idEvent){
			if(lastEvent->itwasdiagnosed){
				return true;
			}
		}
	}

	return false;
}

