/*
 * Autor: Fábio Engel de Camargo
 * Disciplina: Sistemas Distribuídos INFO7046
 * Data da última alteração: 17/04/2019
 * TP1: Implementação do algoritmo VCube no ambiente de simulação SMPL
 */

//#include <stdlib.h>
//#include <stdio.h>
#include <math.h>
#include "smpl.h"
#include "cisj.h"
#include <stdbool.h>

#define simulationTime 201.0 // tempo de duração da simulação
#define interval 10.0 //intervalo de testes


// Eventos
#define TEST 1
#define FAULT 2
#define REPAIR 3

/* descritor do nodo */
typedef struct{
	int id;
	int *timestamp;
}tnodo;

tnodo *nodo;

//utilizado para armanezenar informações de um evento e ser possível imprimir na tela.
typedef struct{
	bool novoEvento;
	bool detectado;
	bool impresso;
	int nodoIdentificador;
	int nodoIdentificado;
	int TestRound;
	int novoEstado;
	bool itwasdiagnosed;
	int testNumberDiagnosed;
}evento;

// armazena eventos que devem ocorrer
typedef struct{
	char type; // f para faulty e r para recover.
	real time; // momento em que ocorre evento.
	int node; // nodo que sofre event0.

}scheduleEvent;


int testnumber =1; //armazena número do teste realizado
int eventTestnumber = 0; // armazena o identificado do teste no momento em que um evento é diagnosticado

//Declaração de métodos
void executeTest(tnodo *nodos, int n, int tokenID, bool started, int roundTest, evento *newEv);
void printStatus(tnodo *nodos, int n, int tokenID);
void getDiagnosisInfo(tnodo *nodos, int n, int i, int j);
void whoIShouldTest(tnodo *nodos, int n, int i, int s, int *nList);
void printLogIntro(int n,scheduleEvent *eventos, int numEventos);
bool checkEventDiagnosed(tnodo *nodos, int n, int eventNode, int timestamp);
bool checkNodeStartDiagnosed(tnodo *nodos, int n);
void restartEvent(evento *newEv);

//reinicia/inicia váriavel que armazena informações sobre um evento
void restartEvent(evento *newEv){
	newEv->novoEvento = false;
	newEv->detectado = false;
	newEv->impresso = false;
	newEv->nodoIdentificador = -1;
	newEv->nodoIdentificado = -1;
	newEv->TestRound = -1;
	newEv->novoEstado = -1;
	newEv->itwasdiagnosed = false;
	newEv->testNumberDiagnosed = 0;
}

// verifica se todos nodos diagnosticaram a inicialização sem falhas de todos nodos do sistema. Apenas após esta verificação devem ocorrer eventos.
bool checkNodeStartDiagnosed(tnodo *nodos, int n){
	int i,j;
	for(i=0; i<n;i++){
		for(j=0;j<n;j++){
			if(nodos[i].timestamp[j] != 0){
				return false;
			}
		}
	}
	return true;
}

// verifica se todos nodos do sistema diagnosticam um evento
bool checkEventDiagnosed(tnodo *nodos, int n, int eventNode, int timestamp){
	int it;
	for(it=0; it < n; it++){
		if(status(nodos[it].id) == 0){ // considera-se apenas nodos livre de falha
			if(nodos[it].timestamp[eventNode] != timestamp){
				return(false);
			}
		}
	}
	return(true);
}

//imprime as informações iniciais de log
void printLogIntro(int n,scheduleEvent *eventos, int numEventos){
	printf("==========================================================================================================================================\n");
	printf("\033[1;30;107m\t\t\t\t\tVCube algorithm log\033[0m\n");
	printf("\033[1;30;107m\t\tNode number:\033[0m %d\n",n);
	printf("\033[1;30;107m\t\tTest interval:\033[0m %4.1f seconds\n", interval);
	printf("\033[1;30;107m\t\tExecution time:\033[0m %4.1f seconds\n", simulationTime);
	printf("\033[1;30;107m\t\tAuthor:\033[0m Fábio Engel de Camargo\n");
	printf("\033[1;30;107m\t\tScheduled Events: \033[0m\n");
	int it;
	for(it=0;it<numEventos; it++){
		if(eventos[it].type== 'f'){
			printf("\t\t\t\t\033[0;31;107mNode %02d fails over time %4.1f\033[0m\n", eventos[it].node, eventos[it].time);
		}
		else if(eventos[it].type== 'r'){
			printf("\t\t\t\t\033[0;34;107mNode %02d recovers over time %4.1f\033[0m\n", eventos[it].node, eventos[it].time);
		}

	}
	//printf("Log structure:\n\n");
	printf("==========================================================================================================================================\n\n");
	printf("\033[1;30;107mTime \t\tTest number \tTest/Event \t\t\t\tNode status \t\t\t\t\tObservation\033[0m\n");
}

//Método retorna quem o nodo i deve testar
// i = nodo testador; s = cluster
void whoIShouldTest(tnodo *nodos, int n, int i, int s, int *nList){
	int alvoTest;
	int k,it,j,iesimoJ;
	for(it=0;it<n;it++){
		nList[it] = 0; // é necessário zerar vetor;
	}
	node_set* nodesCij;// armazena nodos obtidos a partir da função cisj
	node_set* nodesCjs;
	for(s=1; s <= log2(n); s++){ // clusters

		// Antes de i executar um teste em j pertencente a Cis, ele checa se é o primeiro livre de erro de 	Cjs
		nodesCij = cis(i, s);
		for(k=0; k < POW_2(s-1); k++){ // k -> iteração dos valores de j
			j = nodesCij->nodes[k];
			nodesCjs = cis(j, s);
			//bool firstfaultfree = false;

			for(it =0; it < POW_2(s-1); it++){
				iesimoJ = nodesCjs->nodes[it];
				if(nodos[i].timestamp[iesimoJ] % 2 == 0 || nodos[i].timestamp[iesimoJ] == -1 || iesimoJ == i){ //se livre de erro ou desconhecido
					if(iesimoJ == i){
						nList[j] = 1;
					}
					break;
				}
			}
		}
	}
}


//Realiza testes de acordo com o definido pelo algoritmo VCube
void executeTest(tnodo *nodos, int n, int tokenID, bool started, int roundTest, evento *newEv){
	int it;
	int i = tokenID; // indice do nodo tester;
	int nodesTotest[n]; // lista de nodos que o nodo i deve testar
	int j;// nodo a ser testado/
	int jStatus; //estado de j após test
	//int newEvent = 0; // indica a existência de um novo evento

	int s; // define cluster id;
	whoIShouldTest(nodos,n,i,s, nodesTotest); // esta função armazena o valor 1 nas posições que indicam necessidade de teste no vetor nodesTotest

	for(j=0;j<n;j++){
		if(nodesTotest[j] ==1){
			jStatus = status(nodos[j].id);
			if(jStatus == 0){ // j é fault-free
				printf("time:%4.1f\ttn:%03d\t\tNode %02d test Node %02d: \033[0;34;107mfault-free\033[0m.", time(),testnumber, i, j);

				if(nodos[i].timestamp[j] % 2 == 1){ //Se estado anterior correspondia a falha, atualiza timestamp
					nodos[i].timestamp[j]++;
					if(started && newEv->novoEvento == false){
						newEv->novoEvento = true;
						newEv->nodoIdentificador = i;
						newEv->nodoIdentificado = j;
						newEv->novoEstado = nodos[i].timestamp[j];
						newEv->detectado = true;
						newEv->TestRound = roundTest;
						newEv->impresso = false;
						newEv->itwasdiagnosed = false;
					}
				}
				testnumber++;
				getDiagnosisInfo(nodos, n, i,j); //Nodo testado possui informações recentes, então atualiza seu vetor de estado (timestamp)
				printf("\tNode %02d status: ",i);
				printStatus(nodos, n, tokenID);
				if(started && newEv->novoEvento && !newEv->impresso){// se é um novo evento e ainda não foi impresso na tela, deve imprimir
					printf("\tRecovery event detected for the first time.");
					newEv->impresso = true;
				}

			}else{
				printf("time:%4.1f\ttn:%03d\t\tNode %02d test Node %02d: \033[0;31;107mfaulty\033[0m.",time(),testnumber,i, j);
				testnumber++;
				printf("\t\tNode %02d status: ",i);

				if(nodos[i].timestamp[j] % 2 == 0){
					nodos[i].timestamp[j]++;
					if(started && newEv->novoEvento == false){
						newEv->novoEvento = true;
						newEv->nodoIdentificador = i;
						newEv->nodoIdentificado = j;
						newEv->novoEstado = nodos[i].timestamp[j];
						newEv->detectado = true;
						newEv->TestRound = roundTest;
						newEv->impresso = false;
						newEv->itwasdiagnosed = false;
					}
				}
				printStatus(nodos, n, tokenID);
				if(started && newEv->novoEvento && !newEv->impresso){
					printf("\tFailure event detected for the first time.");
					newEv->impresso = true;
				}

			}

			// testa se evento foi diagnosticado
			if(checkEventDiagnosed(nodos, n, newEv->nodoIdentificado, newEv->novoEstado) && newEv->itwasdiagnosed == false){
				printf("\tDiagnosis complete.");
				newEv->itwasdiagnosed = true;
				newEv->testNumberDiagnosed = testnumber -1;
			}

			printf("\n");
		}
	}
}



//Imprime na tela vetor status
void printStatus(tnodo *nodos, int n, int tokenID){
	int it;
	int stat;
	printf("[");
	for (it = 0; it < n; ++it){
		stat = nodo[tokenID].timestamp[it];
		if(stat == -1){
			printf("\033[1;39;32m%3d\033[0m", stat);
		}else if(stat % 2 == 0)
			printf("\033[1;39;34m%3d\033[0m", stat);
		else
			printf("\033[1;39;31m%3d\033[0m", stat);
	}
	printf("  ]");
}

//Obtém diagnóstico a partir do nodo j fault-free
void getDiagnosisInfo(tnodo *nodos, int n, int i, int j){
	int it;
	for(it=0; it < n; it++){
		if(nodos[j].timestamp[it] > nodos[i].timestamp[it]){ //Se informação obtida ao testar é mais recente, atualiza timestamp
			nodos[i].timestamp[it] = nodos[j].timestamp[it];
		}
	}
}



int main(int argc, char * argv[]){
	static int N, //Número de nodos
	token, event,r,it,j,tr,t;
	tr = 1; // indica a rodada de testes;
	t = -1; // utilizada para verificar se houve mudança no tempo de execução e imprimir mensagem de nova rodada de testes
	int maxTestround = (int) simulationTime/interval; //Os testes são executados a cada 10 segundos, portanto é possível obter a quantidade máxima de rodadas de testes em função do tempo de simulação
	evento newEvento; // armazena informaçoes sobre um novo evento detectado
	restartEvent(&newEvento); // esta função inicializa/reinicializa strutura de armazenamento de eventos.
	bool nodesStarted = false;
	int qtdTest[maxTestround]; // armazena quantidade de testes por rodada.
	int iTest = -1;
	static char fa_name[5];

	if(argc != 2){
		puts("Wrong input, use: ./tempo <number_of_nodes>");
		exit(1);
	}

	N = atoi(argv[1]);
	smpl(0, "VCube");
	reset();
	stream(1);

	// Inicializacao
	nodo = (tnodo *) malloc(sizeof(tnodo) * N);

	for(it = 0; it < N; ++it) {
		memset(fa_name, '\0', 5);
		sprintf(fa_name, "%d", it);
		nodo[it].id = facility(fa_name, 1);

		//Inicialização do timestamp de cada nó.
		nodo[it].timestamp= malloc(N*sizeof(int)); //aloca espaço para vetor que armazena estado dos demais nodos;
		for(j = 0; j < N; ++j){
			if(it == j){
				nodo[it].timestamp[j] = 0; //
			}
			else{
				nodo[it].timestamp[j] = -1; //
			}
		}
	}

	// Schedule de eventos
	for (it = 0; it < N; it++) {
		schedule(TEST, interval, it);
	}



	/*----------------------------------------------------------------------------------------------------------------------*/
	// Atenção: Aqui devem ser inseridos os eventos que devem ocorrer. Atenção ao inserir número de eventos e indices.
	int numeroDeEventos = 4;

	scheduleEvent * sEvent;
	sEvent = (scheduleEvent*) malloc(sizeof(scheduleEvent) * numeroDeEventos);

	sEvent[0].type = 'f'; sEvent[0].time = 42.0; sEvent[0].node = 0;
	sEvent[1].type = 'f'; sEvent[1].time = 73.0; sEvent[1].node = 3;

	sEvent[2].type = 'r'; sEvent[2].time = 105.0; sEvent[2].node = 0;
	sEvent[3].type = 'r'; sEvent[3].time = 138.0; sEvent[3].node = 3;


	//sEvent[].type = ''; sEvent[].time = .0; sEvent[].node = ;
	/*----------------------------------------------------------------------------------------------------------------------*/


	for(it=0; it<numeroDeEventos; it++){
		if(sEvent[it].type== 'f'){
			schedule(FAULT, sEvent[it].time, sEvent[it].node);
		}
		else if(sEvent[it].type== 'r'){
			schedule(REPAIR, sEvent[it].time, sEvent[it].node);
		}
	}

	printLogIntro(N,sEvent,numeroDeEventos); // imprime na tela cabeçalho do log

//	schedule(FAULT, 105.0, 7);


	while(time() < simulationTime) {
		cause(&event, &token);
		switch(event){
		case TEST:
			if(token == 0){
				iTest++; // Se o tester equivale ao Nodo zero, considera-se nova rodada de testes;
			}

			if(time() > simulationTime){break;}
			if(t < time()){
				printf("\n================================================ Start test round %02d ================================================\n", tr);
				t = time();
				tr++;
			}
			if(status(nodo[token].id) != 0){
				printf("time:%4.1f\t\t\tNode %02d is faulty.\n",time(), token);
			}
			else{

				executeTest(nodo, N, token, nodesStarted, tr, &newEvento);
			}

			schedule(TEST, interval, token);

			if(token == N-1){
				printf("============================================== End of the test round %02d ============================================== \n\n",iTest+1);
				if(nodesStarted == false){
					nodesStarted = checkNodeStartDiagnosed(nodo,N);
				}
				else{
					if(newEvento.novoEvento){ // é necessário verificar se evento já foi diagnosticado
						if(checkEventDiagnosed(nodo, N, newEvento.nodoIdentificado, newEvento.novoEstado)){ //se evento foi diagnosticado

							printf("Test round report:\n");
							if(newEvento.novoEstado % 2 == 0){ // evento de recover

								printf("\t\tDiagnosis complete: Node %02d recovered.\n",newEvento.nodoIdentificado);
							}
							else{
								printf("\t\tDiagnosis complete: Node %02d faulty.\n",newEvento.nodoIdentificado);
							}

							printf("\t\tLatency: %02d round(s).\n", tr+1 - newEvento.TestRound);
							printf("\t\tTotal tests: %02d.\n", newEvento.testNumberDiagnosed - eventTestnumber);
							printf("\n");

							restartEvent(&newEvento); //reinicia variavel de armazena evento;
						}
						else{

						}
					}
				}

			}

			break;

		case FAULT:
			r = request(nodo[token].id, token, 0);
			if(r != 0){
				puts("Não foi possível falhar o nodo");
				exit(1);
			}
			printf("\033[1;31;107mtime:%4.1f\t\t\tNode %02d failed.\033[0m\n",time(), token );
			eventTestnumber = testnumber -1;
			break;

		case REPAIR:
			release(nodo[token].id, token);
			printf("\033[1;34;107mtime:%4.1f\t\t\tNode %02d recovered.\033[0m\t\t\tNode %02d status: ",time(), token, token);
			for(it =0; it < N; it++){
				if(it == token){
					nodo[token].timestamp[it] = 0;
				}
				else{
					nodo[token].timestamp[it] = -1;
				}
			}
			printStatus(nodo, N, token);
			eventTestnumber = testnumber -1;
			break;
		}

	}


}
