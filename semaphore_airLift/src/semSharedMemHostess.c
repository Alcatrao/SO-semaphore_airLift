/**
 *  \file semSharedMemHostess.c (implementation file)
 *
 *  \brief Problem name: Air Lift.
 *
 *  Synchronization based on semaphores and shared memory.
 *  Implementation with SVIPC.
 *
 *  Definition of the operations carried out by the hostess:
 *     \li waitForNextFlight
 *     \li waitForPassenger
 *     \li checkPassport
 *     \li signalReadyToFlight
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

/** \brief hostess waits for next flight */
static void waitForNextFlight ();

/** \brief hostess waits for passenger */
static void waitForPassenger();

/** \brief hostess checks passport */
static bool checkPassport ();

/** \brief hostess signals boarding is complete */
static void signalReadyToFlight ();


/** \brief getter for number of passengers flying */
static int nPassengersInFlight ();

/** \brief getter for number of passengers waiting */
static int nPassengersInQueue ();

/**
 *  \brief Main program.
 *
 *  Its role is to generate the life cycle of one of intervening entities in the problem: the hostess.
 */

int main (int argc, char *argv[])
{
    int key;                                                           /*access key to shared memory and semaphore set */
    char *tinp;                                                                      /* numerical parameters test flag */

    /* validation of command line parameters */

    if (argc != 4) { 
        freopen ("error_HT", "a", stderr);
        fprintf (stderr, "Number of parameters is incorrect!\n");
        return EXIT_FAILURE;
    }
    else freopen (argv[3], "w", stderr);

    strcpy (nFic, argv[1]);
    key = (unsigned int) strtol (argv[2], &tinp, 0);
    if (*tinp != '\0')
    { fprintf (stderr, "Error on the access key communication!\n");
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

    /* simulation of the life cycle of the hostess */

    int nPassengers=0;
    bool lastPassengerInFlight;

    while(nPassengers < N ) {
        waitForNextFlight();
        do { 
            waitForPassenger();
            lastPassengerInFlight = checkPassport();
            nPassengers++;
        } while (!lastPassengerInFlight);
        signalReadyToFlight();
    }

    /* unmapping the shared region off the process address space */

    if (shmemDettach (sh) == -1) {
        perror ("error on unmapping the shared region off the process address space");
        return EXIT_FAILURE;;
    }

    return EXIT_SUCCESS;
}

/**
 *  \brief wait for Next Flight.
 *
 *  Hostess updates its state and waits for plane to be ready for boarding
 *  The internal state should be saved.
 *
 */

static void waitForNextFlight ()
{
    if (semDown (semgid, sh->mutex) == -1)  {                                                  /* enter critical region */
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //hostess muda para o estado WAIT_FOR_FLIGHT. O estado é guardado
    sh->fSt.st.hostessStat=WAIT_FOR_FLIGHT;
    saveState(nFic, &sh->fSt);
    
    
    if (semUp (semgid, sh->mutex) == -1)                                                   /* exit critical region */
    { perror ("error on the down operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //hostess espera até que o piloto sinalize que o avião está pronto para boarding
    if (semDown (semgid, sh->readyForBoarding) == -1)                                                   /* exit critical region */
    { perror ("erro a bloquear semáforo que diz se se pode iniciar o embarque");
        exit (EXIT_FAILURE);
    }
}

/**
 *  \brief hostess waits for passenger
 *
 *  hostess waits for passengers to arrive at airport.
 *  The internal state should be saved.
 */

static void waitForPassenger ()
{
    if (semDown (semgid, sh->mutex) == -1)                                                      /* enter critical region */
    { perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //hostess muda o seu estado para WAIT_FOR_PASSENGER
    sh->fSt.st.hostessStat=WAIT_FOR_PASSENGER;
    saveState(nFic, &sh->fSt);
    

    if (semUp (semgid, sh->mutex) == -1) {                                                  /* exit critical region */
     perror ("error on the down operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //hostess espera pela chegada dos passageiros
     if (semDown (semgid, sh->passengersInQueue) == -1) {                                                  
     perror ("erro a desbloquear semáforo que faz a hostess esperar pelos passageiros");
        exit (EXIT_FAILURE);
    }
}

/**
 *  \brief passport check
 *
 *  The hostess checks passenger passport and waits for passenger to show id
 *  The internal state should be saved twice.
 *
 *  \return should be true if this is the last passenger for this flight
 *    that is: 
 *      - flight is at its maximum capacity 
 *      - flight is at or higher than minimum capacity and no passenger waiting 
 *      - no more passengers
 */

static bool checkPassport()
{
    bool last;

    /* insert your code here */
    //hostess atende o passageiro
    if (semUp (semgid, sh->passengersWaitInQueue) == -1)                                                     
    { perror ("erro a desbloquear semáforo que informa a hostess que há passageiros na fila à espera dela");
        exit (EXIT_FAILURE);
    }
    
    

    if (semDown (semgid, sh->mutex) == -1) {                                                     /* enter critical region */
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);

    }

    /* insert your code here */
    //hostess muda para o estado CHECK_PASSPORT. O estado é guardado
    sh->fSt.st.hostessStat=CHECK_PASSPORT;
    saveState(nFic, &sh->fSt);
    

    if (semUp (semgid, sh->mutex) == -1)     {                                                 /* exit critical region */
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //hostess espera que o passageiro lhe dê o passaporte para ela verificar
    if (semDown (semgid, sh->idShown) == -1)                                                     
    { perror ("erro a bloquear semáforo que permite a hostess verificar o passaporte");
        exit (EXIT_FAILURE);
    }
    
    
    

    if (semDown (semgid, sh->mutex) == -1)  {                                                 /* enter critical region */
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);

    }

    /* insert your code here */
    //atualização do nº de passageiros na fila de espera e no avião e devido registo
    
    sh->fSt.nPassInQueue-=1;
    sh->fSt.nPassInFlight+=1;
    sh->fSt.totalPassBoarded+=1;
    savePassengerChecked(nFic, &sh->fSt);
    
    
    //verificação das condições de voo, e respetivas mudanças de estado e logs
    if ((nPassengersInFlight()==MAXFC) || (nPassengersInFlight()>=MINFC && nPassengersInQueue()==0) || (sh->fSt.totalPassBoarded==N) ) {
    	last=true;
    	sh->fSt.nPassengersInFlight[sh->fSt.nFlight-1]=sh->fSt.nPassInFlight;
    	saveState(nFic, &sh->fSt);
    }
    else {
    	last=false;
    	saveState(nFic, &sh->fSt);
    }
    
    

    if (semUp (semgid, sh->mutex) == -1) {                                                     /* exit critical region */
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */


    return last;
}

static int nPassengersInFlight()
{
    return sh->fSt.nPassInFlight;
}

static int nPassengersInQueue()
{
    return sh->fSt.nPassInQueue;
}

/**
 *  \brief signal ready to flight 
 *
 *  The flight is ready to go.
 *  The hostess updates her state, registers the number of passengers in this flight 
 *  and checks if the airlift is finished (all passengers have boarded).
 *  Hostess informs pilot that plane is ready to flight.
 *  The internal state should be saved.
 */
void  signalReadyToFlight()
{
    if (semDown (semgid, sh->mutex) == -1) {                                                /* enter critical region */
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //hostess muda para o estado READY_TO_FLIGHT, regista o número de passageiros no voo, e o estado é guardado. Para que o output entre este programa e o pre-compilado sejam mais idênticos, o log do FlightDeparted é feito aqui.
    
    sh->fSt.st.hostessStat=READY_TO_FLIGHT;
    saveState(nFic, &sh->fSt);
    saveFlightDeparted(nFic, &sh->fSt);
    
    //verificar se todos os N passageiros já foram transportados (o ciclo de vida do piloto não acaba sem este passo). Descanso
    if (sh->fSt.totalPassBoarded==N) {
    	sh->fSt.finished=true;
    }
    
    

    if (semUp (semgid, sh->mutex) == -1) {                                                     /* exit critical region */
        perror ("error on the up operation for semaphore access (HT)");
        exit (EXIT_FAILURE);
    }

    /* insert your code here */
    //hostess informa piloto que embarque terminou
    if (semUp (semgid, sh->readyToFlight) == -1) {                                                     
        perror ("erro a desbloquear semáforo que faz o piloto esperar pelo término do embarque");
    exit (EXIT_FAILURE);
    }
}

