/**
 *  \file semSharedMemPilot.c (implementation file)
 *
 *  \brief Problem name: Air Lift
 *
 *  Synchronization based on semaphores and shared memory.
 *  Implementation with SVIPC.
 *
 *  Definition of the operations carried out by the pilot:
 *     \li flight
 *     \li signalReadyForBoarding
 *     \li waitUntilReadyToFlight
 *     \li dropPassengersAtTarget
 *
 *  \author Nuno Lau - January 2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>

#include "probConst.h"
#include "probDataStruct.h"
#include "logging.h"
#include "sharedDataSync.h"
#include "semaphore.h"
#include "sharedMemory.h"


/** \brief logging file name */
static char nFic[51];

/** \brief shared memory block access identifier */
static int shmid;

/** \brief semaphore set access identifier */
static int semgid;

/** \brief pointer to shared memory region */
static SHARED_DATA *sh;

static void flight (bool go);
static void signalReadyForBoarding ();
static void waitUntilReadyToFlight ();
static void dropPassengersAtTarget ();
static bool isFinished ();

/**
 *  \brief Main program.
 *
 *  Its role is to generate the life cycle of one of intervening entities in the problem: the pilot.
 */

int main (int argc, char *argv[])
{
    int key;                                                           /*access key to shared memory and semaphore set */
    char *tinp;                                                                      /* numerical parameters test flag */

    /* validation of command line parameters */

    if (argc != 4) { 
        freopen ("error_PT", "a", stderr);
        fprintf (stderr, "Number of parameters is incorrect!\n");
        return EXIT_FAILURE;
    }
    else freopen (argv[3], "w", stderr);
    strcpy (nFic, argv[1]);
    key = (unsigned int) strtol (argv[2], &tinp, 0);
    if (*tinp != '\0') {
        fprintf (stderr, "Error on the access key communication!\n");
        return EXIT_FAILURE;
    }

    /* connection to the semaphore set and the shared memory region and mapping the shared region onto the
       process address space */

    if ((semgid = semConnect (key)) == -1) { 
        perror ("error on connecting to the semaphore set");
        return EXIT_FAILURE;
    }
    if ((shmid = shmemConnect (key)) == -1) { 
        perror ("error on connecting to the shared memory region");
        return EXIT_FAILURE;
    }
    if (shmemAttach (shmid, (void **) &sh) == -1) { 
        perror ("error on mapping the shared region on the process address space");
        return EXIT_FAILURE;
    }

    srandom ((unsigned int) getpid ());                                                 /* initialize random generator */

    /* simulation of the life cycle of the pilot */

    while(!isFinished()) {
        flight(false); // from target to origin
        signalReadyForBoarding();
        waitUntilReadyToFlight();
        flight(true); // from origin to target
        dropPassengersAtTarget();
    }

    /* unmapping the shared region off the process address space */

    if (shmemDettach (sh) == -1) { 
        perror ("error on unmapping the shared region off the process address space");
        return EXIT_FAILURE;;
    }

    return EXIT_SUCCESS;
}

/**
 *  \brief test if air lift finished
 */
static bool isFinished ()
{
    return sh->fSt.finished; //não devia ser St em vez de fSt?
}

/**
 *  \brief flight.
 *
 *  The pilot takes passenger to destination (go) or 
 *  plane back to starting airport (return)
 *  state should be saved.
 *
 *  \param go true if going to destination
 */

static void flight (bool go)
{
    if (semDown (semgid, sh->mutex) == -1) {                                                      /* enter critical region */
        perror ("error on the up operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //se "go" for falso, mudar estado do piloto para 0 (voo para origem). Caso contrário, mudar para 3 (voo para o destino). Escrever no log as alterações
    if(go==false) {
   	sh->fSt.st.pilotStat=FLYING_BACK;
   	//Modificação final: logo ao início, podem ocorrer dois ou três logs idênticos; quando um ou mais passageiros fazem log, e quando de seguida, o piloto e a hospedeira (ou vice versa) fazem log seguidos. Tanto a Hostess como o Pilot são iniciados no estado 0, e quando o saveState inicial de cada é executado ambos escrevem um log que regista a mudança do estado inicial (0) para o estado 0. Isto causa uma repetição de logs, que embora não tenha impacto na correta execução do programa (ocorre também nas versões pré-compiladas), é um pequeno pormenor que assim, se pode evitar em grande parte das vezes (mas ainda pode acontecer) ao assegurar que um deles não ocorre ao início.
   	if(sh->fSt.totalPassBoarded>0) { //verificação apenas com o propósito único de evitar cerca de metade dos logs repetidos
   		//saveFlightReturning(nFic, &sh->fSt); //foi movido para a última função, de modo a obter um output semelhante à versão pre-compilada
   		saveState(nFic, &sh->fSt);
   	}
    }
    else {
    	sh->fSt.st.pilotStat=FLYING;
    	//saveFlightDeparted(nFic, &sh->fSt); //a hostess passa a dar esta informação, de modo a obter a resultados semelhantes aos do programa pré-compilado
    	saveState(nFic, &sh->fSt);
    }

    if (semUp (semgid, sh->mutex) == -1) {                                                      /* exit critical region */
        perror ("error on the up operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }

    usleep((unsigned int) floor ((MAXFLIGHT * random ()) / RAND_MAX + 100.0));
    
}

/**
 *  \brief pilot informs hostess that plane is ready for boarding
 *
 *  The pilot updates its state and signals the hostess that boarding may start
 *  The flight number should be updated.
 *  The internal state should be saved.
 */

static void signalReadyForBoarding ()
{
    if (semDown (semgid, sh->mutex) == -1) {                                                      /* enter critical region */
        perror ("error on the up operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //o piloto atualiza o seu estado para 1 (READY_FOR_BOARDING). O número do voo é incrementado por 1. O estado é guardado, e é apresentada uma mensagem de início o embarque
    sh->fSt.st.pilotStat=READY_FOR_BOARDING;
    sh->fSt.nFlight+=1; //meter .st entre fSt e nPass, secalhar
    saveState(nFic, &sh->fSt);
    saveStartBoarding(nFic, &sh->fSt);
    

    if (semUp (semgid, sh->mutex) == -1) {                                                      /* exit critical region */
        perror ("error on the up operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //A hostess pode começar operações de embarque
    if (semUp (semgid, sh->readyForBoarding) == -1) {                                                      
        perror ("erro a desbloquear semáforo que indica a hostess que o embarque pode começar");
    exit (EXIT_FAILURE);
    }

}

/**
 *  \brief pilot waits for plane to get filled with passengers.
 *
 *  The pilot updates its state and wait for Boarding to finish 
 *  The internal state should be saved.
 */

static void waitUntilReadyToFlight ()
{
    if (semDown (semgid, sh->mutex) == -1) {                                                      /* enter critical region */
        perror ("error on the up operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //piloto muda de estado para 2 (WAITING_FOR_BOARDING)
    sh->fSt.st.pilotStat=WAITING_FOR_BOARDING;
    saveState(nFic, &sh->fSt);

    if (semUp (semgid, sh->mutex) == -1) {                                                      /* exit critical region */
        perror ("error on the up operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //piloto tem de esperar que o embarque termine
    if (semDown (semgid, sh->readyToFlight) == -1) {                                                     
        perror ("erro a bloquear semáforo que faz o piloto esperar pelo término do embarque");
    exit (EXIT_FAILURE);
    }
    
}

/**
 *  \brief pilot drops passengers at destination.
 *
 *  Pilot update its state and allows passengers to leave plane
 *  Pilot must wait for all passengers to leave plane before starting to return.
 *  The internal state should not be saved twice (after allowing passengeres to leave and after the plane is empty). //(should not? Logo a seguir estão 2 instâncias em que o estado deve ser guardado. Também existem 2 insert codes dentro de mutexes)
 */

static void dropPassengersAtTarget ()
{
    if (semDown (semgid, sh->mutex) == -1) {                                                  /* enter critical region */
        perror ("error on the down operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //é apresentada a informação de que o voo chegou. O piloto muda para o estado 4 (DROPING_PASSENGERS), e o estado é guardado
    saveFlightArrived(nFic, &sh->fSt);
    sh->fSt.st.pilotStat=DROPING_PASSENGERS;
    saveState(nFic, &sh->fSt);
    
    
    
    if (semUp (semgid, sh->mutex) == -1)  {                                                   /* exit critical region */
        perror ("error on the up operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }

	
    /* insert your code here */
        //o piloto sinaliza os passageiros que podem sair do avião. Como cada passageiro faz um Down(), também temos que fazer um Up() por passageiro. Para evitar deadlocks e condições de corrida, esta secção inteira deve estar dentro da região crítica, para que o nPassInFlight não seja inconsistente entre interações. Alternativamente, simplesmete fixa-se o nº inicial de passageiros em voo antes de se fazerem as iterações
        
        int passageiros=sh->fSt.nPassInFlight;
        
        for (int i=0; i<passageiros; i++) {
	    if (semUp (semgid, sh->passengersWaitInFlight) == -1)  {                                                 
		perror ("erro ao desbloquear semáforo que sinaliza passageiros que podem abandonar o avião");
		exit (EXIT_FAILURE);
	    }
	    //printf("Up %d\n", i);
     }


	//o piloto só parte quando o semáfoto planeEmpty estiver desbloqueado. Se a thread do piloto chegar aqui primeiro do que a thread do último passageiro, o piloto bloqueia à espera do último passageiro, e depois deste desbloquear o semáforo, o piloto prossegue. Se a thread do último passageiro chegar primeiro, o piloto quando chegar não espera, mas também já não tem que esperar. De qualquer das maneiras, apõs cada voo, o valor do semáforo será sempre 0 e consistente. 
     	    //printf("planeEmpty\n");
	    if (semDown (semgid, sh->planeEmpty) == -1)  {                                                 
		perror ("erro ao bloquear semáforo que faz o piloto esperar pelo último passageiro");
		exit (EXIT_FAILURE);
	    }
	
 

    if (semDown (semgid, sh->mutex) == -1) {                                                  /* enter critical region */
        perror ("error on the down operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //o piloto muda para o estado 0 (FLYING_BACK). É apresentada a informação de que o avião está a regressar, e o estado não é guardado
    sh->fSt.st.pilotStat=FLYING_BACK;
    saveFlightReturning(nFic, &sh->fSt);
    //saveState(nFic, &sh->fSt);

    if (semUp (semgid, sh->mutex) == -1)  {                                                   /* exit critical region */
        perror ("error on the up operation for semaphore access (PT)");
        exit (EXIT_FAILURE);
    }
}

