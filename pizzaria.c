#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include "pizzaria.h"

//GENERAL TO-DO RC CODES/CHECK FOR ERRORS IN THREADS.

void rcCheckerMutex(int rc,int type){
    if (rc != 0) {  
            if(type) printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
            else printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
            pthread_exit(&rc);
        }
}

void *order(void *args) {

    ORDER_ARGUMENTS *orderArgs;
    orderArgs = (ORDER_ARGUMENTS *)args;

    int threadId = orderArgs->threadId;
    unsigned int seedOfThread = orderArgs->seed + threadId;
    
    //Generating random number of pizzas based on limit.
    srand(seedOfThread);
    int numOfPizzas = rand_r(&seedOfThread) % (PIZZASTOTALHIGH- PIZZASTOTALLOW +1) + PIZZASTOTALLOW;


    //Sum of pizza types.
    int tempSpecialPizzaSum = 0;
    int tempPlainPizzaSum = 0;

    //Mutex rc
    int rc;

    //Time stats
    struct timespec ready_start_time, ready_end_time;
    struct timespec delivered_start_time, delivered_end_time;
    struct timespec cooling_start_time, cooling_end_time;

    //Time stats vars
    int elapsedCompletionTimeUpToPacketing;
    int totalTimeToDeliver;
    float elapsedCoolingTime;

    for(int i = 0; i < numOfPizzas; i++){ //TO-DO: down mutex for payment and pizzas cat stat.
        
        //if pizzaCat > 60 = special else plain
        int pizzaCat = rand_r(&seedOfThread) % 101;
        
        if(pizzaCat > 60) tempSpecialPizzaSum += 1;

        else tempPlainPizzaSum += 1;           
        
    }
    //Sleep - Time taken to perform payment
    int paymentProccedTime = rand_r(&seedOfThread) % (PAYMENTPROCCEDTIMEHIGH - PAYMENTPROCCEDTIMELOW +1) + PAYMENTPROCCEDTIMELOW;
    sleep(paymentProccedTime);

    //Payment Success
    int paymentSuccess = rand_r(&seedOfThread) % 101;
    
    if(paymentSuccess <= 10) {
        rc = pthread_mutex_lock(&stats);
        rcCheckerMutex(rc,1);
        failedOrderSum += 1;
        pthread_mutex_unlock(&stats);
        rcCheckerMutex(rc,0);

        pthread_mutex_lock(&screenlock);
        rcCheckerMutex(rc,1);
        printf("The order with number: %d, was cancelled, payment failed.\n", threadId);
        pthread_mutex_unlock(&screenlock);
        rcCheckerMutex(rc,0);

        pthread_exit(NULL);
    }
    
    clock_gettime(CLOCK_REALTIME ,&ready_start_time); //Packeting starting time
    clock_gettime(CLOCK_REALTIME ,&delivered_start_time); //Delivery total (starting) time

    //Total price
    int totalPrice = PRICEPLAIN * tempPlainPizzaSum + PRICESPECIAL * tempSpecialPizzaSum;
 
    rc = pthread_mutex_lock(&screenlock);
    rcCheckerMutex(rc,1);
    printf("The order with number: %d, was succesful.\n", threadId);
    rc = pthread_mutex_unlock(&screenlock);
    rcCheckerMutex(rc,0);

    //mutex down for all statistics and add the temps to globals.
    rc = pthread_mutex_lock(&stats);
    rcCheckerMutex(rc,1);
    
    succesfulOrderSum += 1;
    totalRevenue += totalPrice;
    specialPizzaSum += tempSpecialPizzaSum;
    plainPizzaSum += tempPlainPizzaSum;

    rc = pthread_mutex_unlock(&stats);
    rcCheckerMutex(rc,0);

    //Pizzas Preparation

    //waiting for free prep(cook)
    rc = pthread_mutex_lock(&pizzaPrep);
    rcCheckerMutex(rc,1);

    while(availableCooks < 1) pthread_cond_wait(&freeCooks,&pizzaPrep);

    availableCooks += -1;
    rc = pthread_mutex_unlock(&pizzaPrep);
    rcCheckerMutex(rc,0);
    
    sleep(PREP * numOfPizzas); //sleep for total prep time

    //Waiting for total of number of pizzas ovens
    rc = pthread_mutex_lock(&pizzaPrep);
    rcCheckerMutex(rc,1);
    while (availableOvens < numOfPizzas) pthread_cond_wait(&freeOvens,&pizzaPrep);

    availableOvens -= numOfPizzas;
    availableCooks += 1; //cooks are now freed
    
    pthread_cond_signal(&freeCooks);
    rc = pthread_mutex_unlock(&pizzaPrep);
    rcCheckerMutex(rc,0);

    sleep(BAKE); //sleep for total bake time

    //Waiting for packeting staff
    rc = pthread_mutex_lock(&pizzaPrep);
    while (availablePackers < 1) pthread_cond_wait(&freePackers,&pizzaPrep);
    availablePackers -= 1;
    rc = pthread_mutex_unlock(&pizzaPrep);

    clock_gettime(CLOCK_REALTIME ,&cooling_start_time); 
    
    sleep(PACK * numOfPizzas);//sleep for total packaging

    //ovens and packeger are now freed

    rc = pthread_mutex_lock(&pizzaPrep);
    rcCheckerMutex(rc,1);
    availableOvens += numOfPizzas; 
    availablePackers += 1;
    pthread_cond_signal(&freeOvens);
    pthread_cond_signal(&freePackers);
    rc = pthread_mutex_unlock(&pizzaPrep);
    rcCheckerMutex(rc,0);

    clock_gettime(CLOCK_REALTIME ,&ready_end_time); //Packeting end time
    elapsedCompletionTimeUpToPacketing = (ready_end_time.tv_sec - ready_start_time.tv_sec) + (ready_end_time.tv_nsec - ready_start_time.tv_nsec) / 1e9;

    rc = pthread_mutex_lock(&screenlock);
    rcCheckerMutex(rc,1);
    printf("The order with number: %d, was ready for delivery in %d minutes.\n", threadId, elapsedCompletionTimeUpToPacketing );
    rc = pthread_mutex_unlock(&screenlock);
    rcCheckerMutex(rc,0);

    //Delivery
    rc = pthread_mutex_lock(&pizzaPrep);
    rcCheckerMutex(rc,1);
    while (availableDelivery < 1) pthread_cond_wait(&freeDeliverer,&pizzaPrep);
    
    availableDelivery -= 1;
    rc = pthread_mutex_unlock(&pizzaPrep);
    rcCheckerMutex(rc,0);

    sleep(rand_r(&seedOfThread) % (DELHIGH - DELLOW +1) + DELLOW);//Time to arrive

    clock_gettime(CLOCK_REALTIME ,&delivered_end_time); //Delivered  end time
    totalTimeToDeliver = (delivered_end_time.tv_sec - delivered_start_time.tv_sec) + (delivered_end_time.tv_nsec - delivered_start_time.tv_nsec) / 1e9;
    

    clock_gettime(CLOCK_REALTIME ,&cooling_end_time); //Cooling end time
    elapsedCoolingTime = (cooling_end_time.tv_sec - cooling_start_time.tv_sec) + (cooling_end_time.tv_nsec - cooling_start_time.tv_nsec) / 1e9;
    

    rc = pthread_mutex_lock(&screenlock);
    rcCheckerMutex(rc,1);
    printf("The order with number: %d, was delivered in %d minutes.\n", threadId, totalTimeToDeliver);
    rc = pthread_mutex_unlock(&screenlock);
    rcCheckerMutex(rc,0);

    rc = pthread_mutex_lock(&timeStats);
    rcCheckerMutex(rc,1);
    //Service Time
    averageServiceTime += totalTimeToDeliver;
    if(maxServiceTime < totalTimeToDeliver) maxServiceTime = totalTimeToDeliver;
    
    //Cooling Time
    averageCoolingTime += elapsedCoolingTime;
    if(maxCoolingTime < totalTimeToDeliver) maxCoolingTime = elapsedCoolingTime;

    rc = pthread_mutex_unlock(&timeStats);
    rcCheckerMutex(rc,0);

    sleep(rand_r(&seedOfThread) % (DELHIGH - DELLOW +1) + DELLOW);//Time to return
    

    //free delivery man
    rc = pthread_mutex_lock(&pizzaPrep);
    rcCheckerMutex(rc,1);
    availableDelivery += 1;
    pthread_cond_signal(&freeDeliverer);
    rc = pthread_mutex_unlock(&pizzaPrep);
    rcCheckerMutex(rc,0);

    return 0;
}


int main(int args, char* argv[]){
 
    if(args != 3 ){
        //error exit with proper message.. //TO-DO
        printf("ERROR: the program should take two arguments, the number of threads to create, and the seed!\n");
        exit(-1);
    }    
    int rc;
    int maxNumberOfThreads = atoi(argv[1]);
    int seed = atoi(argv[2]);
    
    int nextCustomerIn;
    unsigned int mainSeed = seed - 1;

    //Init Mutexes
    pthread_mutex_init(&stats,NULL);
    pthread_mutex_init(&pizzaPrep,NULL);

    //Init CondVars
    pthread_cond_init(&freeCooks, NULL);
    pthread_cond_init(&freeOvens, NULL);
    pthread_cond_init(&freePackers, NULL);
    pthread_cond_init(&freeDeliverer, NULL);

    //Init structs
    ORDER_ARGUMENTS *orderArgs;
    orderArgs = malloc(maxNumberOfThreads * sizeof(ORDER_ARGUMENTS));
    if (orderArgs == NULL) {
        printf("Not enough memory for orderArgs Struct creation!\n");
        return -1;
    }

    //Init Threads
    pthread_t *threads;
    threads = malloc(maxNumberOfThreads * sizeof(pthread_t));
    if (threads == NULL) {
        printf("Not enough memory for thread creation!\n");
        return -1;
    }
    
    //order threads creation
    for(int i = 0; i < maxNumberOfThreads; i++){
        orderArgs[i].threadId = i+1;
        orderArgs[i].seed = seed;
        
        if(i > 1) {
            nextCustomerIn = rand_r(&mainSeed) % (NEXTORDERHIGH - NEXTORDERLOW +1) + NEXTORDERLOW; //next customer in random time between limits nextOrderLow-NEXTORDERHIGH
            sleep(nextCustomerIn);
        }

        rc = pthread_create(&threads[i], NULL, order, &orderArgs[i]);
        if (rc != 0) {
            printf("ERROR: return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
   
    for(int i = 0; i < maxNumberOfThreads; i++){
        pthread_join(threads[i],NULL);
    }
    

    printf ("Total Revenue: %d\n", totalRevenue);
    printf ("Sold Pizzas:\n");
    printf ("\tSpecial: %d\n", specialPizzaSum);
    printf ("\tPlain: %d\n", plainPizzaSum);
    printf ("Successful Orders: %d\n", succesfulOrderSum);
    printf ("Failed Orders: %d\n", failedOrderSum);

    printf ("Average Service Time: %.2f\n", averageServiceTime / succesfulOrderSum);
    printf ("Maximum Service Time: %.2f\n", maxServiceTime);
    printf ("Average Cooling Time: %.2f\n", averageCoolingTime / succesfulOrderSum);
    printf ("Maximum Cooling Time: %.2f\n", maxCoolingTime);

    free(threads);
    free(orderArgs);

    return 0;
}
