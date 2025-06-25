#include <iostream>
#include <list>
#include <pthread.h>
#include <functional>
#include <stdlib.h>
#include <time.h>
#include <cstring>

using namespace std;

#define ERROR_MSG(msg) do { printf("%s\n", msg); } while(0)

int user_main(int argc, char **argv);
int in_c = 1;
double tot_ex_t = 0;

void demonstration(std::function<void()> &&lambda_f) {
    lambda_f();
}

typedef struct {
    int strt;
    int end;
    function<void(int)> lambda;
} threads_args_s;

typedef struct {
    int o_strt;
    int o_end;
    int i_strt;
    int i_end;
    function<void(int, int)> lambda;
} threads_args_nest;

void loop(int strt, int end, function<void(int)>&& lambda_f) {
    int i = strt;
    while (i < end) {
        lambda_f(i);
        i++;
    }
}

void n_loop(int o_strt, int o_end, int i_strt, int i_end, function<void(int, int)>&& lambda_f) {
    int i = o_strt;
    while (i < o_end) {
        int j = i_strt;
        while (j < i_end) {
            lambda_f(i, j);
            j++;
        }
        i++;
    }
}

void* thread_f_s(void* args) {
    threads_args_s* arg_ptr = (threads_args_s*)args;
    loop(arg_ptr->strt, arg_ptr->end, move(arg_ptr->lambda));
    pthread_exit(nullptr);
}

void* thread_f_nest(void* args) {
    threads_args_nest* arg_ptr = (threads_args_nest*)args;
    n_loop(arg_ptr->o_strt, arg_ptr->o_end, arg_ptr->i_strt, arg_ptr->i_end, move(arg_ptr->lambda));
    pthread_exit(nullptr);
}

int calc_chunk(int base, int idx, int chunk_sz) { 
  return base + idx * chunk_sz; 
}

int calc_chunk_ofl(int base, int idx, int chunk_sz, int ofl) {
    return base + idx * chunk_sz + ofl;
}

void parallel_for(int strt, int end, function<void(int)>&& lambda, int num_threads) {
    pthread_t threads[num_threads];
    threads_args_s thread_args[num_threads];

    int rng = (end - strt);
    int chunk_sz = rng / num_threads;
    int ofl = rng % num_threads;
    clock_t strt_t = clock();

    int i = 0;
    while (i < num_threads) {
        thread_args[i].strt = calc_chunk(strt, i, chunk_sz);
        thread_args[i].end = (i == num_threads - 1) ? calc_chunk_ofl(strt, i + 1, chunk_sz, ofl) : calc_chunk(strt, i + 1, chunk_sz);
        thread_args[i].lambda = lambda;
        if (pthread_create(&threads[i], nullptr, thread_f_s, (void*)&thread_args[i]) != 0) {
          ERROR_MSG("pthread_create failed"); 
        }
        i++;
    }

    i = 0;
    while (i < num_threads) {
        if (pthread_join(threads[i], nullptr) != 0) { 
          ERROR_MSG("pthread_join failed"); 
        }
        i++;
    }

    double elapsed_t = ((double)(clock() - strt_t)) * 1000.0 / CLOCKS_PER_SEC;
    printf("\nExecution Time for parallel_for Call %d: %f ms\n", in_c++, elapsed_t);
    tot_ex_t += elapsed_t;
}

void parallel_for(int o_strt, int o_end, int i_strt, int i_end, function<void(int, int)>&& lambda, int num_threads) {
    pthread_t threads[num_threads];
    threads_args_nest thread_args[num_threads];

    int rng = o_end - o_strt;
    int chunk_sz = rng / num_threads;
    int ofl = rng % num_threads;
    clock_t strt_t = clock();

    int i = 0;
    while (i < num_threads) {
        thread_args[i].o_strt = calc_chunk(o_strt, i, chunk_sz);
        thread_args[i].o_end = (i == num_threads - 1) ? calc_chunk_ofl(o_strt, i + 1, chunk_sz, ofl) : calc_chunk(o_strt, i + 1, chunk_sz);
        thread_args[i].i_strt = i_strt;
        thread_args[i].i_end = i_end;
        thread_args[i].lambda = lambda;
        if (pthread_create(&threads[i], nullptr, thread_f_nest, (void*)&thread_args[i]) != 0) { 
          ERROR_MSG("pthread_create failed"); 
        }
        i++;
    }

    i = 0;
    while (i < num_threads) {
        if (pthread_join(threads[i], nullptr) != 0) { ERROR_MSG("pthread_join failed"); }
        i++;
    }

    double elapsed_t = ((double)(clock() - strt_t)) * 1000.0 / CLOCKS_PER_SEC;
    printf("\nExecution Time for parallel_for Call %d: %f ms\n", in_c++, elapsed_t);
    tot_ex_t += elapsed_t;
}

int main(int argc, char **argv) {
  // defineStructures();
  /* 
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be 
   * explicity captured if they are used inside lambda.
   */
  int x=5,y=1;
  // Declaring a lambda expression that accepts void type parameter
  auto /*name*/ lambda1 = /*capture list*/[/*by value*/ x, /*by reference*/ &y](void) {
    /* Any changes to 'x' will throw compilation error as x is captured by value */
    y = 5;
    cout<<"====== Welcome to Assignment-"<<y<<" of the CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);

  auto /*name*/ lambda2 = [/*nothing captured*/]() {
    std::cout<<"\nTotal Execution Time for all parallel_for calls: "<<tot_ex_t<<" milliseconds\n";
    std::cout<<"\n====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

#define main user_main