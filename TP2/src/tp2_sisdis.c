	/*
 * Autor: Fábio Engel de Camargo
 * Disciplina: Sistemas Distribuídos INFO7046
 * Data da última alteração: 17/04/2019
 * TP2: Implementação do algoritmo best-effort broadcast sobre o VCube
 */

//#include <stdlib.h>
//#include <stdio.h>
#include <math.h>
#include "smpl.h"
#include "cisj.h"
#include <stdbool.h>
#include "eventList.h"
#include <string.h>

#define simulationTime 142.0 // tempo de duração da simulação
#define interval 10.0 //intervalo de testes


// Eventos
#define TEST 1
#define FAULT 2
#define REPAIR 3

/* descritor do nodo */
typedef struct{
	int id;
	int *timestamp;
	int msgId;
	char msgRecev[16];
	char msgSend[16];
	int s; // valor de s recebido;

}tnodo;

tnodo *nodo;

// armazena eventos que devem ocorrer
typedef struct{
	char type; // f para faulty e r para recover.
	real time; // momento em que ocorre evento.
	int node; // nodo que sofre event0.

}scheduleEvent;


typedef struct{
	int nodeId; //nodo emissor do broadcast
	real time; //momento do envio
	bool alreadytransmitted;
}scheduleBroadcast;

int testnumber =1; //armazena número do teste realizado
int eventTestnumber = 0; // armazena o identificado do teste no momento em que um evento é diagnosticado
int eventCounter = 0; // armanzena o número do evento do sistema

//Declaração de métodos
void executeTest(tnodo *nodos, int n, int tokenID, bool started, int roundTest, evento *newEv);
void printStatus(tnodo *nodos, int n, int tokenID);
void getDiagnosisInfo(tnodo *nodos, int n, int i, int j);
void whoIShouldTest(tnodo *nodos, int n, int i, int s, int *nList);
void printLogIntro(int n,scheduleEvent *eventos, int numEventos, int numBroadc, scheduleBroadcast* sbroadc);
void checkEventDiagnosed(tnodo *nodos, int n, evento* head, int testnum);
bool checkNodeStartDiagnosed(tnodo *nodos, int n);
bool firstTimeEventDetected(evento *head, int nodo, int newtimestamp);
void printReport(tnodo *nodos, int n, evento* head, int tround, real time);

void broadcastschedule(tnodo *nodos, int n, evento* head, int qtdEv, scheduleEvent * sEvent, scheduleBroadcast* sbroadc,int qtdBroadcast,real time);
void sendMessage(tnodo *nodos, int n, int emissor, real tempo);
void createMsg(int idEmissor,int idMessage, char* msg);
void forwardMsg(tnodo *nodos, int n, char* msg, int num_clusters, int e);
int firstnodefaultyfree(tnodo *nodos, int e, int s);
bool sendcheck(tnodo *nodos, int n);

//insere na variavel msg a mensagem recem gerada
void createMsg(int idEmissor,int idMessage, char* msg){
	char emissor[6];
	sprintf(emissor,"%02d", idEmissor);
	char id[6];
	sprintf(id,"%02d", idMessage);
	strcat(msg,emissor);
	strcat(msg,"_");
	strcat(msg,id);
}

//verifica se nodos possuem mensagens a serem encaminhadas
bool sendcheck(tnodo *nodos, int n){
	int i;

	for(i=0;i<n;i++){
		if(nodos[i].s > 0){
			return(true); // se ainda existem nodos que precisam encaminhar msg, retorna true
		}
	}

	return(false); // caso contrário retorna false;

}

//encaminha mensagem recebida
void forwardMsg(tnodo *nodos, int n, char* msg, int num_clusters, int e){

	int s;
	int dest;
	int i, tab;

	for(s=1; s<= log2(n); s++){
		dest = firstnodefaultyfree(nodos, e, s);
		if(dest != -1){
			printf("\t\t\tNode %02d send %s to Node %02d\n", e,msg,dest);
			strcpy(nodos[dest].msgRecev, msg); // nodo recebe msg;
			nodos[dest].s = s; // nodo recebe valor de s;
		}
	}

	while(sendcheck(nodos,n)){
		for(i=0; i < n; i++){
			if(nodos[i].s > 0){
				if(nodos[i].s == 1){// nodo é folha, não retransmite.
					nodos[i].s = 0;
					strcpy(nodos[i].msgRecev,"");
				}
				else{
					nodos[i].s -= 1;
					dest = firstnodefaultyfree(nodos, i, nodos[i].s);
					if(dest != -1){
						printf("\t\t\tNode %02d send %s to Node %02d\n",i,nodos[i].msgRecev,dest);
						strcpy(nodos[dest].msgRecev, msg); // nodo recebe msg;
						nodos[dest].s = nodos[i].s; // nodo recebe valor de s;
					}
				}
			}
		}
	}

}

//método executado pelo nodo emissor da mensagem em broadcast
void sendMessage(tnodo *nodos, int n, int emissor, real tempo){
	int i;
	int num_clusters = log2(n);
	node_set* nodesToSend;
	nodos[emissor].msgId += 1; // nova mensagem a ser enviada.
	char msg[16];

	strcpy(msg,"msg_");
	createMsg(emissor,nodos[emissor].msgId,msg);
	strcpy(nodos[emissor].msgSend, msg);

	printf("\n\033[1;32;107mtime:%4.1f\tNode %02d broadcast: %s\033[0m\n", tempo,emissor, msg);

	for(i=0; i < n; i++){
		nodo[i].s = 0;
	}
	forwardMsg(nodos, n, msg, num_clusters, emissor);
	printf("\n");
}

//retorna o primeiro nodo livre de erros para o envio de msg
int firstnodefaultyfree(tnodo *nodos, int e, int s){
	node_set* nodesToCheck;
	nodesToCheck = cis(e,s);
	int i,k,nodo;

	for(k=0; k < POW_2(s-1); k++){
		nodo = nodesToCheck->nodes[k];
		if(nodos[e].timestamp[nodo] % 2 == 0){//se livre de erro;
			return nodo;

		}
	}
	return -1;
}
//verifica a cada final de rodada de testes se algum evento de broadcast deve ser realizado
void broadcastschedule(tnodo *nodos, int n, evento* head, int qtdEv,scheduleEvent * sEvent, scheduleBroadcast* sbroadc,int qtdBroadcast,real time){

	//verifica se há agendamento de broadcast
	int i,j,node, estado;
	real tempoNodo;
	bool eventInthisInterval = false;
	for(i=0; i< qtdBroadcast; i++){
		tempoNodo = sbroadc[i].time;

		// verifica-se se há evento no mesmo intervalo
		for(j=0;j<qtdEv; j++){
			if((sEvent[j].time >= time) &&  sEvent[j].time < (time + interval)){ // Existe evento no intervalo em que esta função é chamada
				if(tempoNodo > sEvent[j].time){// se broadcast ocorre após evento, função deve retornar;
					if(time < sEvent[j].time){
						eventInthisInterval = true;
					}
				}
			}
		}

		if(!sbroadc[i].alreadytransmitted){ // se broadcast ainda não foi enviado
			if((tempoNodo >= time) && (tempoNodo < (time + interval)) && !eventInthisInterval){// haverá transmissão de broadcast neste intervalo
				node = sbroadc[i].nodeId;
				estado = status(nodos[node].id);
				if(head->next == (void *) NULL){
					if(estado == 0){
						sendMessage(nodos, n, node, tempoNodo);
					}
					else{
						printf("\n\033[1;97;41mWarning:\033[0m \tScheduled node %02d for broadcast is faulty.\n\n", sbroadc[i].nodeId);
					}

				}
				else{
					printf("\n\033[1;97;41mWarning:\033[0m \tThere are events not yet diagnosed. Node %02d Broadcast not allowed.\n\n", node);
				}

				sbroadc[i].alreadytransmitted = true;
			}
		}

	}




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


void printReport(tnodo *nodos, int n, evento* head, int tround, real time){
	evento* lastEvent = head;
	int it;
	int eventNode;
	int timestamp;
	bool reportprinted = false;

	bool allfaulty = true;
	//verifica de todos nodos estão falhos
	for(it=0; it<n;it++){
		if(status(nodos[it].id) == 0){
			allfaulty = false;
			break; // indica que existe ao menos um nodo não falho;
		}
	}
	if(allfaulty){ // indica que todos nodos estão falhos
		printf("\ntime:%4.1f\tTest round report:\n",time);
		printf("\t\t\tAll nodes are faulty\n");
		return;
	}

	if(lastEvent->next == (void *) NULL){
		//No TP2 não é necessário exibir os testes
		//printf("\t\tNo events\n");
		return;
	}else{
		do{
			lastEvent = lastEvent->next;
			if(lastEvent->itwasdiagnosed){
				if(lastEvent->novoEstado % 2 == 0){// evento de recover
					if(!reportprinted){
						printf("\ntime:%4.1f\tTest round report:\n",time);
						reportprinted = true;
					}
					printf("\t\t\tDiagnosis over event_id %02d complete: Node %02d recovered\n",lastEvent->id, lastEvent->nodoIdentificado);
				}
				else{
					if(!reportprinted){
						printf("\ntime:%4.1f\tTest round report:\n",time);
						reportprinted = true;
					}
					printf("\t\t\tDiagnosis over event_id %02d complete: Node %02d faulty\n",lastEvent->id, lastEvent->nodoIdentificado);
				}
				printf("\t\t\tLatency: %02d round(s)\n", tround+1 - lastEvent->TestRound);
				printf("\t\t\tTotal tests: %02d\n", lastEvent->testNumberDiagnosed - lastEvent->testNumberIdentificado);
				//printf("\t\tCurrent time: %4.1f\n",time);
				printf("\n");
				removeEvento(head, lastEvent->id);
			}
			}while(lastEvent->next != (void *) NULL);
	}



}

// verifica se todos nodos do sistema diagnosticam um evento
void checkEventDiagnosed(tnodo *nodos, int n, evento* head, int testnum){
	evento* lastEvent = head;
	int it;
	int eventNode;
	int timestamp;

	if(lastEvent->next == (void *) NULL){
		return;
	}else{
		do{
			lastEvent = lastEvent->next;
			eventNode = lastEvent->nodoIdentificado;
			timestamp = lastEvent->novoEstado;
			bool eventoregistro = true;
			//TEsta para cada evento:
				for(it=0; it < n; it++){
					if(status(nodos[it].id) == 0){ // considera-se apenas nodos livre de falha
						if(nodos[it].timestamp[eventNode] != timestamp){
							eventoregistro = false;
							break;
							//Do nothing
						}
					}
				}
				if(eventoregistro &&!lastEvent->itwasdiagnosed){
					lastEvent->itwasdiagnosed = true;
					lastEvent->testNumberDiagnosed = testnum;
					//return(true);
					//No TP2 não é necessário exibir os testes
					//printf("\tDiagnosis event_id: %02d",lastEvent->id);
				}

			}while(lastEvent->next != (void *) NULL);

	}

}

//imprime as informações iniciais de log
void printLogIntro(int n,scheduleEvent *eventos, int numEventos, int numBroadc, scheduleBroadcast* sbroadc){
	printf("==========================================================================================================================================\n");
	printf("\033[1;30;107m\t\t\t\t\tBest-effort broadcast over VCbube algorith - Log\033[0m\n");
	printf("\033[1;30;107m\t\tNode number:\033[0m %d\n",n);
	printf("\033[1;30;107m\t\tTest interval:\033[0m %4.1f seconds\n", interval);
	printf("\033[1;30;107m\t\tExecution time:\033[0m %4.1f seconds\n", simulationTime);
	printf("\033[1;30;107m\t\tAuthor:\033[0m Fábio Engel de Camargo\n");
	printf("\033[1;30;107m\t\tScheduled Events: \033[0m");
	if(numEventos == 0){
		printf("No scheduled events\n");
	}
	else{
		printf("\n");
	}

	int it;
	for(it=0;it<numEventos; it++){
		if(eventos[it].type== 'f'){
			printf("\t\t\t\t\033[0;31;107mNode %02d fails at time %4.1f\033[0m\n", eventos[it].node, eventos[it].time);
		}
		else if(eventos[it].type== 'r'){
			printf("\t\t\t\t\033[0;34;107mNode %02d recovers at time %4.1f\033[0m\n", eventos[it].node, eventos[it].time);
		}

	}

	printf("\033[1;30;107m\t\tScheduled Broadcast: \033[0m");
	if(numBroadc == 0){
		printf("No broadcast\n");
	}
	else{
		printf("\n");
	}
	for(it=0;it<numBroadc; it++){
		printf("\t\t\t\t\033[0;32;107mNode %02d broadcast at time %4.1f\033[0m\n", sbroadc[it].nodeId, sbroadc[it].time);
	}


	//printf("Log structure:\n\n");
	printf("==========================================================================================================================================\n\n");
	//printf("\033[1;30;107mTime \t\t\t \tTest/Event \t\t\t\tNode status \t\t\t\t\tObservation\033[0m\n");

	if(numEventos != 0){
		printf("\033[1;30;107mTime \t\tEvent\033[0m\n");
	}

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

bool firstTimeEventDetected(evento *head, int nodo, int newtimestamp){
	evento* lastEvent = head;
	if(lastEvent->next == (void *) NULL){
			return(true);

	}else{
		do{
			lastEvent = lastEvent->next;
			if(lastEvent->nodoIdentificado == nodo && lastEvent->novoEstado == newtimestamp){ //se existe evento recente com o nodo
				return(false);
			}
		}while(lastEvent->next != (void *) NULL);
	}
	//se não existe evento com nodo:
	return(true);
}




//Realiza testes de acordo com o definido pelo algoritmo VCube
void executeTest(tnodo *nodos, int n, int tokenID, bool started, int roundTest, evento *headEvento){
	int it;
	int i = tokenID; // indice do nodo tester;
	int nodesTotest[n]; // lista de nodos que o nodo i deve testar
	int j;// nodo a ser testado/
	int jStatus; //estado de j após test

	int s; // define cluster id;
	whoIShouldTest(nodos,n,i,s, nodesTotest); // esta função armazena o valor 1 nas posições que indicam necessidade de teste no vetor nodesTotest

	for(j=0;j<n;j++){
		if(nodesTotest[j] ==1){
			jStatus = status(nodos[j].id);
			if(jStatus == 0){ // é fault-free
				//No TP2 não é necessário exibir os testes
				//printf("time:%4.1f\ttn:%03d\t\tNode %02d test Node %02d: \033[0;34;107mfault-free\033[0m.", time(),testnumber, i, j);
				bool printEvent = false;
				if(nodos[i].timestamp[j] % 2 == 1){ //Se estado anterior correspondia a falha, atualiza timestamp

					nodos[i].timestamp[j]++;
					if(started && firstTimeEventDetected(headEvento, j, nodos[i].timestamp[j])){

						evento newEv;
						newEv.nodoIdentificador = i;
						newEv.nodoIdentificado = j;
						newEv.novoEstado = nodos[i].timestamp[j];
						newEv.detectado = true;
						newEv.TestRound = roundTest;
						newEv.impresso = false;
						newEv.itwasdiagnosed = false;
						newEv.testNumberIdentificado = testnumber -1;

						eventCounter++;
						insertEvento(headEvento, newEv, eventCounter);
						printEvent = true;

					}
				}
				testnumber++;
				getDiagnosisInfo(nodos, n, i,j); //Nodo testado possui informações recentes, então atualiza seu vetor de estado (timestamp)
				//No TP2 não é necessário exibir os testes
				//printf("\tNode %02d status: ",i);
				//No TP2 não é necessário exibir os testes
				//printStatus(nodos, n, tokenID);

				if(started && printEvent){// se é um novo evento e ainda não foi impresso na tela, deve imprimir
					//No TP2 não é necessário exibir os testes
					//printf("\tRecovery event detected: event_id %02d",eventCounter);
					printEvent = false;
				}

			}else{
				//No TP2 não é necessário exibir os testes
				//printf("time:%4.1f\ttn:%03d\t\tNode %02d test Node %02d: \033[0;31;107mfaulty\033[0m.",time(),testnumber,i, j);
				testnumber++;
				//No TP2 não é necessário exibir os testes
				//printf("\t\tNode %02d status: ",i);
				bool printEvent = false;

				if(nodos[i].timestamp[j] % 2 == 0){
					nodos[i].timestamp[j]++;
					if(started && firstTimeEventDetected(headEvento, j, nodos[i].timestamp[j])){
						evento newEv;
						newEv.nodoIdentificador = i;
						newEv.nodoIdentificado = j;
						newEv.novoEstado = nodos[i].timestamp[j];
						newEv.detectado = true;
						newEv.TestRound = roundTest;
						newEv.impresso = false;
						newEv.itwasdiagnosed = false;
						newEv.testNumberIdentificado = testnumber -1;
						eventCounter++;
						insertEvento(headEvento, newEv, eventCounter);
						printEvent = true;

					}
				}
				//No TP2 não é necessário exibir os testes
				//printStatus(nodos, n, tokenID);
				if(started && printEvent){// se é um novo evento e ainda não foi impresso na tela, deve imprimir
					//printf("\tFailure event detected: event_id %02d",eventCounter);
					printEvent = false;
				}

			}

			// testa se evento foi diagnosticado
			if(started){
				checkEventDiagnosed(nodos, n, headEvento, testnumber);

//				newEv->itwasdiagnosed = true;
//				newEv->testNumberDiagnosed = testnumber -1;
			}
			//No TP2 não é necessário exibir os testes
			//printf("\n");
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
	//restartEvent(&newEvento); // esta função inicializa/reinicializa strutura de armazenamento de eventos.
	// inicializa o vetor de eventos
	evento *headList;
	// inicializa primeiro elemento como head
	headList = (evento *) malloc(sizeof(evento));
	headList->id = 0;
	headList->next = NULL;

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
			nodo[it].msgId = 0; //inicia contador de mensagens de cada nodo
			nodo[it].s = 0;
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

	sEvent[0].type = 'f'; sEvent[0].time = 45.0; sEvent[0].node = 0;
	sEvent[1].type = 'f'; sEvent[1].time = 65.0; sEvent[1].node = 1;
	sEvent[2].type = 'f'; sEvent[2].time = 79.0; sEvent[2].node = 2;
	sEvent[3].type = 'f'; sEvent[3].time = 95.0; sEvent[3].node = 3;
//	sEvent[4].type = 'r'; sEvent[4].time = 85.0; sEvent[4].node = 0;
	//sEvent[].type = ''; sEvent[].time = .0; sEvent[].node = ;
	/*----------------------------------------------------------------------------------------------------------------------*/

	//Eventods de broadcast devem ser definidos em ordem temporal
	int numeroBroadcast = 2;
	scheduleBroadcast * sBroadcast;
	sBroadcast = (scheduleBroadcast*) malloc(sizeof(scheduleBroadcast) * numeroBroadcast);

	 sBroadcast[0].alreadytransmitted = false; sBroadcast[0].time = 35.0; sBroadcast[0].nodeId = 5;
	 sBroadcast[0].alreadytransmitted = false; sBroadcast[1].time = 125.0; sBroadcast[1].nodeId = 5;
//	 sBroadcast[0].alreadytransmitted = false; sBroadcast[2].time = 97.0; sBroadcast[2].nodeId = 5;
//   sBroadcast[0].alreadytransmitted = false; sBroadcast[1].time = 85.0; sBroadcast[1].nodeId = 0;


	for(it=0; it<numeroDeEventos; it++){
		if(sEvent[it].type== 'f'){
			schedule(FAULT, sEvent[it].time, sEvent[it].node);
		}
		else if(sEvent[it].type== 'r'){
			schedule(REPAIR, sEvent[it].time, sEvent[it].node);
		}
	}

	printLogIntro(N,sEvent,numeroDeEventos, numeroBroadcast, sBroadcast); // imprime na tela cabeçalho do log

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
				//No TP2 não é necessário exibir os testes
				//printf("\n================================================ Start test round %02d ================================================\n", tr);
				t = time();
				tr++;
			}
			if(status(nodo[token].id) != 0){
				//No TP2 não é necessário exibir os testes
				//printf("time:%4.1f\t\t\tNode %02d is faulty.\n",time(), token);
			}
			else{

				executeTest(nodo, N, token, nodesStarted, tr, headList);
			}

			schedule(TEST, interval, token);

			if(token == N-1){
				//No TP2 não é necessário exibir os testes
				//printf("============================================== End of the test round %02d ============================================== \n\n",iTest+1);

				if(nodesStarted == false){
					nodesStarted = checkNodeStartDiagnosed(nodo,N);
					if(nodesStarted == true){
						printf("\ntime:%4.1f\tTest round report:\n", time());
								printf("\t\t\tAll Nodes were initialized and diagnosed.\n\n");
					}

				}
				else{
					printReport(nodo,N, headList,tr,time());
				}

				//Envio de broadcast ocorrem após as rodadas de teste, apenas quando não existem eventos ainda não diagnosticados.
				broadcastschedule(nodo, N, headList,numeroDeEventos,sEvent, sBroadcast, numeroBroadcast, time());

			}

			break;

		case FAULT:
			r = request(nodo[token].id, token, 0);
			if(r != 0){
				puts("Não foi possível falhar o nodo");
				exit(1);
			}
			printf("\033[1;31;107mtime:%4.1f\tNode %02d failed.\033[0m\n",time(), token );
			eventTestnumber = testnumber -1;
			broadcastschedule(nodo, N, headList,numeroDeEventos,sEvent, sBroadcast, numeroBroadcast, time());
			break;

		case REPAIR:
			release(nodo[token].id, token);
			//No TP2 não é necessário exibir os testes
			//printf("\033[1;34;107mtime:%4.1f\ttNode %02d recovered.\033[0m\t\t\tNode %02d status: ",time(), token, token);

			printf("\033[1;34;107mtime:%4.1f\tNode %02d recovered.\033[0m",time(), token);
			for(it =0; it < N; it++){
				if(it == token){
					nodo[token].timestamp[it] = 0;
				}
				else{
					nodo[token].timestamp[it] = -1;
				}
			}
			//No TP2 não é necessário exibir os testes
			//printStatus(nodo, N, token);
			printf("\n");
			eventTestnumber = testnumber -1;

			break;
		}

	}


}
