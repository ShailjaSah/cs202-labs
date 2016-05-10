#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "EStore.h"
#include "TaskQueue.h"
#include "RequestGenerator.h"

class Simulation
{
    public:
    TaskQueue supplierTasks;
    TaskQueue customerTasks;
    EStore store;

    int maxTasks;
    int numSuppliers;
    int numCustomers;

    explicit Simulation(bool useFineMode) : store(useFineMode) { }
};

/*
 * ------------------------------------------------------------------
 * supplierGenerator --
 *
 *      The supplier generator thread. The argument is a pointer to
 *      the shared Simulation object.
 *
 *      Enqueue arg->maxTasks requests to the supplier queue, then
 *      stop all supplier threads by enqueuing arg->numSuppliers
 *      stop requests.
 *
 *      Use a SupplierRequestGenerator to generate and enqueue
 *      requests.
 *
 *      This thread should exit when done.
 *
 * Results:
 *      Does not return. Exit instead.
 *
 * ------------------------------------------------------------------
 */
static void*
supplierGenerator(void* arg)
{
  Simulation *simu = (Simulation *) arg;
  SupplierRequestGenerator srg(&simu->supplierTasks) ;
  srg.enqueueTasks(simu->maxTasks, &simu->store);
  srg.enqueueStops(simu->numSuppliers);
  sthread_exit();
    return NULL; // Keep compiler happy.
}

/*
 * ------------------------------------------------------------------
 * customerGenerator --
 *
 *      The customer generator thread. The argument is a pointer to
 *      the shared Simulation object.
 *
 *      Enqueue arg->maxTasks requests to the customer queue, then
 *      stop all customer threads by enqueuing arg->numCustomers
 *      stop requests.
 *
 *      Use a CustomerRequestGenerator to generate and enqueue
 *      requests.  For the fineMode argument to the constructor
 *      of CustomerRequestGenerator, use the output of
 *      store.fineModeEnabled() method, where store is a field
 *      in the Simulation class.
 *
 *      This thread should exit when done.
 *
 * Results:
 *      Does not return. Exit instead.
 *
 * ------------------------------------------------------------------
 */
static void*
customerGenerator(void* arg)
{
  Simulation* simu = (Simulation *) arg;
  CustomerRequestGenerator crg(&simu->customerTasks, simu->store.fineModeEnabled());
  crg.enqueueTasks(simu->maxTasks, &simu->store);
  crg.enqueueStops(simu->numCustomers);
  sthread_exit();
  return NULL; // Keep compiler happy.
}

/*
 * ------------------------------------------------------------------
 * supplier --
 *
 *      The main supplier thread. The argument is a pointer to the
 *      shared Simulation object.
 *
 *      Dequeue Tasks from the supplier queue and execute them.
 *
 * Results:
 *      Does not return.
 *
 * ------------------------------------------------------------------
 */
static void*
supplier(void* arg)
{
  while(true){
    Simulation *simu = (Simulation *) arg;
    Task task = simu->supplierTasks.dequeue();
    task.handler(task.arg);
  }
     return NULL; // Keep compiler happy.
}

/*
 * ------------------------------------------------------------------
 * customer --
 *
 *      The main customer thread. The argument is a pointer to the
 *      shared Simulation object.
 *
 *      Dequeue Tasks from the customer queue and execute them.
 *
 * Results:
 *      Does not return.
 *
 * ------------------------------------------------------------------
 */
static void*
customer(void* arg)
{
  while(true){
    Simulation *simu = (Simulation *) arg;
    Task task = simu->customerTasks.dequeue();
    task.handler(task.arg);
  }
  return NULL; // Keep compiler happy.
}

/*
 * ------------------------------------------------------------------
 * startSimulation --
 *      Create a new Simulation object. This object will serve as
 *      the shared state for the simulation. 
 *
 *      Create the following threads:
 *          - 1 supplier generator thread.
 *          - 1 customer generator thread.
 *          - numSuppliers supplier threads.
 *          - numCustomers customer threads.
 *
 *      After creating the worker threads, the main thread
 *      should wait until all of them exit, at which point it
 *      should return.
 *
 *      Hint: Use sthread_join.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
static void
startSimulation(int numSuppliers, int numCustomers, int maxTasks, bool useFineMode)
{
  
  Simulation *simu = new Simulation(useFineMode);
  simu->maxTasks = maxTasks;
  simu->numSuppliers = numSuppliers;
  simu->numCustomers = numCustomers;

  sthread_t supplierT; 
  sthread_t customerT; 

  sthread_create(&supplierT, supplierGenerator, simu);
  sthread_create(&customerT, customerGenerator, simu);

  sthread_t *stid = new sthread_t[numSuppliers];
  sthread_t *ctid = new sthread_t[numCustomers];
  
  for (int i = 0; i < numSuppliers; i++){
    sthread_create(&stid[i], supplier, simu);
  }
  for (int i = 0; i < numCustomers; i++){
    sthread_create(&ctid[i], customer, simu);
  }


  sthread_join(supplierT);
  Printf("Supplier generator Recycled");
  sthread_join(customerT);
  Printf("Customer Generator Recycled");
  for (int i = 0; i < numSuppliers; i++){
    sthread_join(stid[i]);
  }
  Printf("NS RECYCLED");
  fflush(stdout);
  for (int i = 0; i < numCustomers; i++){
    sthread_join(ctid[i]);
  }
  Printf("NC_RECYCLED");
  fflush(stdout);

  delete[] stid;
  delete[] ctid;
  delete simu;
}

int main(int argc, char **argv)
{
    bool useFineMode = false;

    // Seed the random number generator.
    // You can remove this line or set it to some constant to get deterministic
    // results, but make sure you put it back before turning in.
    srand(time(NULL));

    if (argc > 1)
        useFineMode = strcmp(argv[1], "--fine") == 0;
    startSimulation(10, 10, 100, useFineMode);
    return 0;
}

