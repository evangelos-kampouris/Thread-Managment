#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#define COOKS 2
#define OVENS 15
#define PACKER 2
#define DELIVERER 10
#define NEXTORDERLOW 1
#define NEXTORDERHIGH 3
#define PIZZASTOTALLOW 1
#define PIZZASTOTALHIGH 5
#define PLAINPERCENT 60
#define PAYMENTPROCCEDTIMELOW 1
#define PAYMENTPROCCEDTIMEHIGH 3
#define PAYMENTFAIL 10
#define PRICEPLAIN 10
#define PRICESPECIAL 12
#define PREP 1
#define BAKE 10
#define PACK 1
#define DELLOW 5
#define DELHIGH 15

void* order(void *args);

/*
@args type if == 1 then lock else unlock
*/
void rcCheckerMutex(int rc, int type);

int main(int args, char* argv[]);

typedef struct order_arguments {
	int threadId;
	int seed;
}  ORDER_ARGUMENTS;

//Mutexes
pthread_mutex_t screenlock;
pthread_mutex_t stats;
pthread_mutex_t pizzaPrep;
pthread_mutex_t timeStats;

//Conds
pthread_cond_t freeCooks;
pthread_cond_t freeOvens;
pthread_cond_t freePackers;
pthread_cond_t freeDeliverer;


int availableCooks = COOKS;
int availableOvens = OVENS;
int availablePackers = PACKER;
int availableDelivery = DELIVERER;

//General stats
int specialPizzaSum = 0;
int plainPizzaSum = 0;
int succesfulOrderSum = 0;
int failedOrderSum = 0;
int totalRevenue = 0;

//Time stats
float averageServiceTime = 0.f; 
float maxServiceTime = 0.f;

float averageCoolingTime = 0.f;
float maxCoolingTime = 0.f;

    
