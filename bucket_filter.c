#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include "cs402.h"
#include "my402list.h"
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>



pthread_t Packet;
pthread_t Token;
pthread_t Server1;
pthread_t Server2;

pthread_mutex_t mutex;
pthread_cond_t cond;


My402List *Q1;
My402List *Q2;
My402List *list; //to store contents of a file

My402List *completed;  //packets
My402List *dropped;
My402List *removed;



int numlist; //to caculate some numbers in list
int key_wait; //if Q1_list = 0, Q1 and list are empty. if Q1_list = 1 not all of them are empty
int isTracedriven;
int ntoken;
int nthtoken;
int nthpack;
int nthpack_copy;
int nthpack_leave;
int npack_arrive;
int packet_end;
int wait_S1;
int wait_S2;
int server1_end;
int server2_end;
int token_drop;
struct timeval point0;
struct timeval point1;
struct timeval point2;
double time_period1; //now - begin
double time_period2; // now - last point
double tt_arive_time;
double tt_em_time;
double time_in_Q1;
double time_in_Q2;
double time_in_S1;
double time_in_S2;
double tt_time_sqr;
char *filename;
void CalculateTimePeriod();
struct timespec outtime;
struct timespec relative_timeout, absolute_timeout;
struct timeval now;


typedef struct Content_of_FIle
{
    /**
    store[1] represents inter_arrival_time
    store[2] represents need_token;
    store[3] represents service_time;
    */
    char *store[3];
    double arrive;
    double enter_Q1;
    double leave_Q1;
    double enter_Q2;
    double leave_Q2;
    double serve_in;
    double serve_out;
    int nthpac;

} contentF;

typedef struct thread_global
{
    double r;
    double lambda; // packet arrive rate
    double mu;  //service time
    int numtoken;  //number of tokens in total
    int numpacket; // numner of packets
    int p; //number of token for each packet



} GlobalVal;




void Readfile(FILE *file , void* argument)
{
    GlobalVal *arg = (GlobalVal*) argument;
    char buf[1024];
    char *n ;
    n = (char *)malloc(sizeof(char));



    fgets(buf,sizeof(buf),file);
    strcpy(n,buf);
    n[strlen(n)-1] = '\0';
    arg-> numpacket = atoi(n);
    while(1)
    {

        contentF *content;
        content = (contentF*)malloc(sizeof(contentF));
        if(fgets(buf,sizeof(buf),file) == NULL)
        {
            break;
        }
        else
        {
            int i;
            char *start_ptr = buf;

            for(i = 0 ; i < strlen(buf); i++)
            {

                if(isspace(buf[i]))
                {
                    buf[i] = '\t';        //chang all space to TAB
                }
            }

            char *tab_ptr = strchr(start_ptr, '\t');
            if (tab_ptr != NULL)
            {
                while (tab_ptr[0] == '\t')
                {
                    *tab_ptr++ = '\0';
                }
                //  *tab_ptr++ = '\0';
                content->store[0] = (char*)malloc(1026*sizeof(char));

                strcpy(content->store[0], start_ptr);
            }

            for( i=1; i <=2; i++ )
            {
                start_ptr = tab_ptr;
                tab_ptr = strchr(start_ptr, '\t');
                while (tab_ptr[0] == '\t')
                {
                    *tab_ptr++ = '\0';
                }
                content->store[i] = (char*)malloc(1026*sizeof(char));

                strcpy(content->store[i], start_ptr);
            }
            My402ListAppend(list, (void* )content);
        }
    }
}

void pthreadmutexunlock(void* arg)
{
    pthread_mutex_unlock(arg);
}

void *PachetThread(void *arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,0);
    GlobalVal *argument = (GlobalVal*) arg;
    My402ListElem *elem=NULL;

    int key;
    double save_time;
    key=1;
    int first_time;
    first_time = 1;
    double time_1;
    double time_2;

    if(isTracedriven == 1)
    {
      //  argument -> numpacket = list -> num_members;
  //   elem=My402ListFirst(list);
   //   while(elem != NULL){
        for(elem=My402ListFirst(list); elem != NULL; elem=My402ListNext(list, elem))
       {
            nthpack++;
            contentF *cont = elem->obj;
            cont->nthpac = nthpack;
            CalculateTimePeriod();

            if(first_time > 0)
            {

                usleep(atoi(cont->store[0])*1000);
              //  nthtoken++;
                first_time --;

            }
            else
            {
                usleep((atoi(cont->store[0])*1000) - (time_2*1000 - time_1*1000));
            //    nthtoken++;

            }
            CalculateTimePeriod();
            time_1 = time_period1;


            if(key > 0)
            {
                key--;
                CalculateTimePeriod();
                save_time = time_period1;
                cont -> arrive = time_period1;
                if(atoi(cont->store[1]) > argument -> numtoken)
                {
                    My402ListAppend(dropped, cont);

       //     time_1 = time_period1;
                    fprintf(stdout, "%012.3fms: p%i arrives, needs %i tokens, inter-arrival time = %.3fms, dropped\n",time_period1 ,cont->nthpac , atoi(cont->store[1]) , time_period1 -  save_time );
                }else{


                fprintf(stdout, "%012.3fms: p%i arrives, needs %i tokens, inter-arrival time = %.3fms\n",time_period1 ,cont->nthpac,atoi(cont->store[1]),(double)atoi(cont->store[0]) );

                }
                tt_arive_time += atoi(cont->store[0]);
            }
            else
            {
                if(atoi(cont->store[1]) > argument -> numtoken)
                {
                    My402ListAppend(dropped, cont);
                    CalculateTimePeriod();
       //     time_1 = time_period1;
                    fprintf(stdout, "%012.3fms: p%i arrives, needs %i tokens, inter-arrival time = %.3fms, dropped\n",time_period1 ,cont->nthpac , atoi(cont->store[1]) , time_period1 -  save_time );
                }else{
                cont -> arrive = time_period1;
                fprintf(stdout, "%012.3fms: p%i arrives, needs %i tokens, inter-arrival time = %.3fms\n",time_period1 ,cont->nthpac,atoi(cont->store[1]),time_period1 -  save_time );
                tt_arive_time += time_period1 -  save_time;
                save_time = time_period1;


                }
            }

            if(atoi(cont->store[1])<= argument -> numtoken)
            {

                pthread_mutex_lock(&mutex);
                pthread_cleanup_push(pthreadmutexunlock, &mutex);
                My402ListAppend(Q1, cont);

                pthread_cleanup_pop(0);
                pthread_mutex_unlock(&mutex);
                CalculateTimePeriod();
                cont -> enter_Q1 = time_period1;
                fprintf(stdout, "%012.3fms: p%i enters Q1\n", time_period1 , cont->nthpac );

                My402ListElem *elemq1=NULL;
                elemq1=My402ListFirst(Q1);

                contentF *cont1 = elemq1->obj;

                if(atoi(cont->store[1]) <= ntoken)
                {

                    pthread_mutex_lock(&mutex);
                    pthread_cleanup_push(pthreadmutexunlock, &mutex);
                    My402ListUnlink(Q1,elemq1);

                    pthread_cleanup_pop(0);
                    pthread_mutex_unlock(&mutex);


                    CalculateTimePeriod();
                    cont -> leave_Q1 = time_period1;
                    ntoken -= atoi(cont->store[1]);
                    fprintf(stdout, "%012.3fms: p%i leaves Q1, time in Q1 = %.3fms, token bucket now has %i token\n", time_period1 , cont->nthpac , cont ->leave_Q1 - cont ->enter_Q1, ntoken );
                    pthread_mutex_lock(&mutex);

                    pthread_cleanup_push(pthreadmutexunlock, &mutex);

                    time_in_Q1 += cont ->leave_Q1 - cont ->enter_Q1;

                    My402ListAppend(Q2, cont1);

                    pthread_cond_signal(&cond);
                    pthread_cleanup_pop(0);
                    pthread_mutex_unlock(&mutex);
                    CalculateTimePeriod();
                    cont -> enter_Q2 = time_period1;
                    fprintf(stdout, "%012.3fms: p%i enters Q2\n", time_period1 , cont->nthpac );

                }
            }
   //         elem=My402ListNext(list, elem);
            CalculateTimePeriod();
            time_2 = time_period1;

            //    nthpack_copy = nthpack;

        }
    }
    else if(isTracedriven == 0)
    {

        while(nthpack < argument -> numpacket)
        {
            nthpack++;
            contentF *cont;
            cont = (contentF *)malloc(sizeof(contentF));

            cont->nthpac = nthpack;

            CalculateTimePeriod();


            if(first_time > 0)
            {

                usleep((1/(argument -> lambda))*1000000 );
             //   nthtoken++;
                first_time --;
      //        tt_arive_time += (1/(argument -> lambda))*1000000;

            }
            else
            {
                usleep((1/(argument -> lambda))*1000000 - (time_2*1000 - time_1*1000));
            //    nthtoken++;
     //           tt_arive_time += (time_2*1000 - time_1*1000);
            }

            CalculateTimePeriod();
            time_1 = time_period1;

            if(key > 0)
            {
                key--;
                save_time = time_period1;
                cont -> arrive = time_period1;
                if(argument->p > argument -> numtoken)
                {
                    My402ListAppend(dropped, cont);

       //     time_1 = time_period1;
                    fprintf(stdout, "%012.3fms: p%i arrives, needs %i tokens, inter-arrival time = %.3fms, dropped\n",time_period1 ,cont->nthpac , argument->p , time_period1 -  save_time );
                }else{


            //    fprintf(stdout, "%012.3fms: p%i arrives, needs %i tokens, inter-arrival time = %.3fms\n",time_period1 ,cont->nthpac,atoi(cont->store[1]),(double)atoi(cont->store[0]) );
                fprintf(stdout, "%012.3fms: p%i arrives, needs %i tokens, inter-arrival time = %.3fms\n",time_period1 ,cont->nthpac,argument->p,(double)(1/(argument -> lambda))*1000 );
                }
              //  fprintf(stdout, "%012.3fms: p%i arrives, needs %i tokens, inter-arrival time = %.3fms\n",time_period1 ,cont->nthpac,argument->p,(double)(1/(argument -> lambda))*1000 );
                tt_arive_time += (1/(argument -> lambda))*1000;
            }
            else
            {
                if(argument->p > argument -> numtoken)
                {
                    My402ListAppend(dropped, cont);
                    CalculateTimePeriod();

                    fprintf(stdout, "%012.3fms: p%i arrives, needs %i tokens, inter-arrival time = %.3fms, dropped\n", time_period1,cont->nthpac , argument->p , time_period1 -  save_time );

                }else {
                cont -> arrive = time_period1;
                fprintf(stdout, "%012.3fms: p%i arrives, needs %i tokens, inter-arrival time = %.3fms\n",time_period1 ,cont->nthpac, argument->p ,time_period1 -  save_time );
                tt_arive_time += time_period1 -  save_time;
                save_time = time_period1;

                }
            }

            if(argument->p <= argument -> numtoken)
            {
                pthread_mutex_lock(&mutex);
                pthread_cleanup_push(pthreadmutexunlock, &mutex);
                My402ListAppend(Q1, cont);

                pthread_cleanup_pop(0);
                pthread_mutex_unlock(&mutex);

                CalculateTimePeriod();
                cont -> enter_Q1 = time_period1;
                fprintf(stdout, "%012.3fms: p%i enters Q1\n", time_period1 , cont->nthpac );

                My402ListElem *elemq1=NULL;
                elemq1=My402ListFirst(Q1);

                contentF *cont1 = elemq1->obj;

                if(argument->p <= ntoken)
                {
                    pthread_mutex_lock(&mutex);
                    pthread_cleanup_push(pthreadmutexunlock, &mutex);
                    My402ListUnlink(Q1,elemq1);


                    pthread_cleanup_pop(0);
                    pthread_mutex_unlock(&mutex);


                    CalculateTimePeriod();
                    cont -> leave_Q1 = time_period1;
                    ntoken -= argument->p;
                    fprintf(stdout, "%012.3fms: p%i leaves Q1, time in Q1 = %.3fms, token bucket now has %i token\n", time_period1 , cont->nthpac , cont ->leave_Q1 - cont ->enter_Q1, ntoken );
                    pthread_mutex_lock(&mutex);

                    pthread_cleanup_push(pthreadmutexunlock, &mutex);

                    time_in_Q1 += cont ->leave_Q1 - cont ->enter_Q1;

                    My402ListAppend(Q2, cont1);


                    pthread_cond_signal(&cond);
                    pthread_cleanup_pop(0);
                    pthread_mutex_unlock(&mutex);
                    CalculateTimePeriod();
                    cont -> enter_Q2 = time_period1;

                }
            }
            CalculateTimePeriod();
            time_2 = time_period1;
            //   nthpack_copy = nthpack;

        }

    }

    packet_end = 1;
    pthread_exit((void *)2);

}




void *TokenDepositThread(void * arg)
{
    GlobalVal *argument = (GlobalVal*) arg;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,0);

    double time_1;
    double time_2;
    int first_time;
    first_time = 1;


    while(1)
    {
        CalculateTimePeriod();
        time_1 = time_period1;

        if(first_time > 0)
        {
            usleep(1/(argument->r)*1000000);
            nthtoken++;
            first_time --;
        }
        else
        {
            usleep(1/(argument->r)*1000000 - (time_2*1000 - time_1*1000));
            nthtoken++;
        }

        CalculateTimePeriod();


        if(ntoken < argument -> numtoken)
        {
            ntoken ++;


            if(ntoken == 1)
            {
                fprintf(stdout, "%012.3fms: token t%i arrives, token bucket now has %i token\n", time_period1 ,nthtoken ,ntoken);

            }
            else
            {
                fprintf(stdout, "%012.3fms: token t%i arrives, token bucket now has %i tokens\n", time_period1 ,nthtoken ,ntoken);
            }

        }
        else
        {
            CalculateTimePeriod();
            fprintf(stdout,"%012.3fms: token t%i arrives, dropped\n", time_period1,nthtoken);
            token_drop ++;
        }

        My402ListElem *elem=NULL;

        if(Q1->num_members != 0)
        {
            elem = My402ListFirst(Q1);
            contentF *cont = elem->obj;

            int p_copy;
            if(isTracedriven == 0)
                p_copy = argument -> p;
            else
                p_copy = atoi(cont-> store[1]);

            if(p_copy <= ntoken)
            {
                pthread_mutex_lock(&mutex);
                pthread_cleanup_push(pthreadmutexunlock,&mutex);

                My402ListUnlink(Q1,elem);  //Delete the element in Q1

                ntoken -= p_copy;
                pthread_cleanup_pop(0);
                pthread_mutex_unlock(&mutex);
                CalculateTimePeriod();
                cont -> leave_Q1 = time_period1;

                fprintf(stdout, "%012.3fms: p%i leaves Q1, time in Q1 = %.3fms, token bucket now has %i token\n", time_period1 , cont->nthpac , cont ->leave_Q1 - cont ->enter_Q1, ntoken );

                pthread_mutex_lock(&mutex);
                pthread_cleanup_push(pthreadmutexunlock, &mutex);
                time_in_Q1 += cont ->leave_Q1 - cont ->enter_Q1;

                My402ListAppend(Q2, cont);
                CalculateTimePeriod();
                cont -> enter_Q2 = time_period1;
                fprintf(stdout, "%012.3fms: p%i enters Q2\n", time_period1 , cont->nthpac );

                pthread_cond_signal(&cond);
                pthread_cleanup_pop(0);
                pthread_mutex_unlock(&mutex);





            }
        }
        if(packet_end == 1  && Q1->num_members == 0)
        {


            key_wait = 1;
            pthread_cancel(Packet);
            pthread_exit((void *)2);

        }



        CalculateTimePeriod();
        time_2 = time_period1;
    }

}

void *ServerThread1(void *arg)
{
    GlobalVal *argument = (GlobalVal*) arg;

    while(1)
    {


        pthread_mutex_lock(&mutex);
        pthread_cleanup_push(pthreadmutexunlock, &mutex);
        while(My402ListEmpty(Q2) && wait_S1 == 0 )
        {
            gettimeofday(&now,0);
            absolute_timeout.tv_sec = now.tv_sec + relative_timeout.tv_sec;
            absolute_timeout.tv_nsec = 1000*now.tv_usec + relative_timeout.tv_nsec;
            if (absolute_timeout.tv_nsec >= 1000000000) {

// deal with the carry
                absolute_timeout.tv_nsec -= 1000000000;
                absolute_timeout.tv_sec++;
            }


            if(Q2->num_members == 0 && Q1->num_members == 0 && packet_end == 1 && completed->num_members == argument -> numpacket - dropped -> num_members)
            {
                wait_S2 = 1;
                pthread_cond_signal(&cond);
                pthread_exit((void*) 3);
            }

            pthread_cond_timedwait(&cond, &mutex, &absolute_timeout);



            //   usleep(100000);
        }
        pthread_cleanup_pop(0);
        pthread_mutex_unlock(&mutex);

        if(Q2->num_members != 0)
        {
            My402ListElem *elem=NULL;
            elem=My402ListFirst(Q2);

            contentF *content = elem->obj;
            pthread_mutex_lock(&mutex);
            pthread_cleanup_push(pthreadmutexunlock, &mutex);

            My402ListUnlink(Q2,elem);

            pthread_cleanup_pop(0);
            pthread_mutex_unlock(&mutex);

            server1_end = 1;

            CalculateTimePeriod();
            content->leave_Q2 = time_period1;
            fprintf(stdout, "%012.3fms: p%i leaves Q2, time in Q2 = %.3f\n", content->leave_Q2 , content->nthpac , content->leave_Q2 - content->enter_Q2 );

            time_in_Q2 += content->leave_Q2 - content->enter_Q2;

            double number;
            if(isTracedriven == 0)
                number = (1/argument->mu )* 1000;
            else
                number = (double)atoi(content->store[2]) ;

            CalculateTimePeriod();
            content->serve_in = time_period1;


            fprintf(stdout, "%012.3fms: p%i begins service at S1, requesting %.3fms of service\n", content->serve_in , content->nthpac , number );





            usleep((int)number*1000);

            CalculateTimePeriod();
            content->serve_out = time_period1;
            fprintf(stdout, "%012.3fms: p%i departs from S1, service time = %.3fms, time in system = %.3fms\n", content->serve_out , content->nthpac , content->serve_out - content->serve_in ,content->serve_out - content->arrive);

            pthread_mutex_lock(&mutex);
            pthread_cleanup_push(pthreadmutexunlock, &mutex);
            time_in_S1 += content->serve_out - content->serve_in;
            tt_time_sqr += time_in_S1*time_in_S1;

            My402ListAppend(completed, content);

            pthread_cleanup_pop(0);
            pthread_mutex_unlock(&mutex);

            server1_end = 0;

        }

        if(Q2->num_members == 0 && Q1->num_members == 0 && packet_end == 1 && completed->num_members == argument -> numpacket - dropped -> num_members)
        {
            wait_S2 = 1;
            pthread_cond_signal(&cond);
            pthread_exit((void*) 3);
        }


    }

}

void *ServerThread2(void *arg)
{
    GlobalVal *argument = (GlobalVal*) arg;
    while(1)
    {

        pthread_mutex_lock(&mutex);
        pthread_cleanup_push(pthreadmutexunlock, &mutex);
        while(My402ListEmpty(Q2) && wait_S2 == 0)
        {
            gettimeofday(&now,0);
            absolute_timeout.tv_sec = now.tv_sec + relative_timeout.tv_sec;
            absolute_timeout.tv_nsec = 1000*now.tv_usec + relative_timeout.tv_nsec;
            if (absolute_timeout.tv_nsec >= 1000000000) {

                absolute_timeout.tv_nsec -= 1000000000;
                absolute_timeout.tv_sec++;
            }

            if(Q2->num_members == 0 && Q1->num_members == 0 && packet_end == 1 && completed->num_members == argument -> numpacket - dropped -> num_members)
            {
                wait_S1 = 1;
                pthread_cond_signal(&cond);
                pthread_exit((void*) 3);
            }

            pthread_cond_timedwait(&cond, &mutex, &outtime);
        }
        pthread_cleanup_pop(0);
        pthread_mutex_unlock(&mutex);

        if(Q2->num_members != 0)
        {
            My402ListElem *elem=NULL;
            elem=My402ListFirst(Q2);

            contentF *content = elem->obj;
            pthread_mutex_lock(&mutex);
            pthread_cleanup_push(pthreadmutexunlock, &mutex);

            My402ListUnlink(Q2,elem);
            pthread_cleanup_pop(0);
            pthread_mutex_unlock(&mutex);

            server2_end = 1;
            //////////////////////////////
            CalculateTimePeriod();
            content->leave_Q2 = time_period1;
            fprintf(stdout, "%012.3fms: p%i leaves Q2, time in Q2 = %.3f\n", content -> leave_Q2 , content->nthpac , content-> leave_Q2 - content->enter_Q2 );

            time_in_Q2 += content->leave_Q2 - content->enter_Q2;

            CalculateTimePeriod();
            content->serve_in = time_period1;

            double number;
            if(isTracedriven == 0)
                number = (1/argument->mu )* 1000;
            else
                number = (double)atoi(content->store[2]) ;

            fprintf(stdout, "%012.3fms: p%i begins service at S2, requesting %.3fms of service\n", content->serve_in , content->nthpac , number );

            usleep(number*1000);

            CalculateTimePeriod();
            content->serve_out = time_period1;
            fprintf(stdout, "%012.3fms: p%i departs from S2, service time = %.3fms, time in system = %.3fms\n", content->serve_out , content->nthpac , content->serve_out - content->serve_in ,content->serve_out - content->arrive);



            pthread_mutex_lock(&mutex);
            pthread_cleanup_push(pthreadmutexunlock, &mutex);
            time_in_S2 += content->serve_out - content->serve_in;
            tt_time_sqr += time_in_S2 * time_in_S2;

            My402ListAppend(completed, content);
            pthread_cleanup_pop(0);
            pthread_mutex_unlock(&mutex);
            server2_end = 0;

        }



        if(Q2->num_members == 0 && Q1->num_members == 0 && packet_end == 1 && completed->num_members == argument -> numpacket - dropped -> num_members)
        {
            wait_S1 = 1;
            pthread_cond_signal(&cond);
            pthread_exit((void*) 3);
        }

    }

}

void CalculateTimePeriod()
{

    gettimeofday(&point2, NULL);

    time_period1 = ((double)point2.tv_sec - (double)point0.tv_sec) * 1000 + ((double)point2.tv_usec - (double)point0.tv_usec) / 1000 ;

}

void CreatPthread(void* arg)
{
    GlobalVal *argument = (GlobalVal*) arg;
    fprintf(stdout, "\n00000000.000ms: emulation begins\n");
    gettimeofday(&point0, NULL);
    point1 = point0;

    pthread_create(&Packet , 0 , PachetThread , argument); //for arrive package
    pthread_create(&Token , 0 , TokenDepositThread , argument);
    pthread_create(&Server1 , 0 , ServerThread1 , argument);
    pthread_create(&Server2 , 0 , ServerThread2 , argument);
}

void JoinPthread()
{
    pthread_join(Packet , 0);
    pthread_join(Token , 0) ;
    pthread_join(Server1 , 0);
    pthread_join(Server2 , 0);
    CalculateTimePeriod();
    fprintf(stdout, "%012.3fms: emulation ends\n", time_period1);
    tt_em_time = time_period1;

}

void InitialVal(void * argument)
{
    GlobalVal *arg = (GlobalVal*) argument;
    arg ->r = 1.5;
    arg ->lambda = 1;
    arg -> mu = 0.35;
    arg -> numpacket = 20;
    arg -> numtoken = 10;
    arg -> p = 3;


    Q1 = (My402List*)malloc(sizeof(My402List));
    Q2 = (My402List*)malloc(sizeof(My402List));
    list = (My402List*)malloc(sizeof(My402List));
    completed = (My402List*)malloc(sizeof(My402List));
    dropped = (My402List*)malloc(sizeof(My402List));
    removed = (My402List*)malloc(sizeof(My402List));
    filename = (char*)malloc(sizeof(char));
    My402ListInit(list);
    My402ListInit(Q1);
    My402ListInit(Q2);
    My402ListInit(completed);
    My402ListInit(dropped);
    My402ListInit(removed);
    ntoken = 0;
    nthtoken = 0;
    key_wait = 0; //means open , when 1 means clsoe
    nthpack = 0;
    nthpack_leave = 0;
    server1_end = 0;
    server2_end = 0;
    tt_em_time = 0;
    time_in_Q1 = 0;
    time_in_Q2 = 0;
    time_in_S1 = 0;
    time_in_S2 = 0;
    tt_arive_time = 0;
    tt_time_sqr = 0;
    token_drop = 0;
    npack_arrive = 0;
    nthpack_copy = 0;
    packet_end = 0;
    wait_S2 = 0;
    wait_S1 = 0;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
  //  outtime.tv_sec = 0;
  //  outtime.tv_nsec = 100000000;
    relative_timeout.tv_sec = 0;
    relative_timeout.tv_nsec = 200000000;

}

void PrintStatistics()
{
    printf("\nStatistics:\n\n");

    if(nthpack == 0)
    {
        printf("\taverage packet inter-arrival time = 0.000000s\n");
    }
    else
    {
        printf("\taverage packet inter-arrival time = %.6gs\n",( (double)tt_arive_time/nthpack)/1000);
    }

    if(completed -> num_members == 0)
    {
        printf("\taverage packet service time = 0.000000s\n\n");
    }
    else
    {

        printf("\taverage packet service time = %.6gs\n\n",((double) (time_in_S2 + time_in_S1)/completed -> num_members)/1000);
    }

    printf("\taverage number of packets in Q1 = %.6g\n", (double)time_in_Q1/tt_em_time);
    printf("\taverage number of packets in Q2 = %.6g\n", (double)time_in_Q2/tt_em_time);
    printf("\taverage number of packets in S1 = %.6g\n", (double)time_in_S1/tt_em_time);
    printf("\taverage number of packets in S2 = %.6g\n\n", (double)time_in_S2/tt_em_time);
    My402ListElem *elem1=NULL;
    double spd_in_sys;
    spd_in_sys = 0;



    if(completed -> num_members == 0)
    {
        printf("\taverage time a packet spent in system = 0.000000s\n");

    }
    else
    {


        for(elem1=My402ListFirst(completed); elem1 != NULL; elem1=My402ListNext(completed, elem1))
        {
            contentF *cont = elem1->obj;
            spd_in_sys += cont-> serve_out -cont ->arrive;


        }
        printf("\taverage time a packet spent in system = %.6gs\n", ((double)spd_in_sys/completed->num_members)/1000);
    }
    double squr = ((time_in_S2 + time_in_S1)/completed -> num_members) * ((time_in_S2 + time_in_S1)/completed -> num_members);
    double newsqur = tt_time_sqr/completed->num_members - squr;
    printf("\tstandard deviation for time spent in system = %.6g\n\n",sqrt(newsqur)/1000);

    printf("\ttoken drop probability = %.6g\n",(double)token_drop/nthtoken);
    printf("\tpacket drop probability = %.6g\n\n\n",(double)dropped -> num_members/nthpack);
}

void handler()
{
    pthread_cancel(Packet);
    pthread_cancel(Token);

    My402ListElem * elem = NULL;
    for(elem=My402ListFirst(Q1); elem != NULL; elem=My402ListNext(Q1, elem))
    {
        contentF *cont = elem->obj;

        CalculateTimePeriod();

        My402ListAppend(removed, cont);


    }





    for(elem=My402ListFirst(Q2); elem != NULL; elem=My402ListNext(Q2, elem))
    {
        contentF *cont = elem->obj;
        cont= elem-> obj;
        CalculateTimePeriod();
        My402ListAppend(removed, cont);
        fprintf(stdout, "%012.3lfms: p%i removed from  Q2\n",time_period1,cont->nthpac);


    }
    while(1)
    {
        usleep(100000);
        if(server1_end == 0)
        {
            pthread_cancel(Server1);
        }
        if(server2_end == 0)
        {

            pthread_cancel(Server2);
        }
        if(server1_end == 0 && server2_end == 0)
        {
            break;
        }
    }
    CalculateTimePeriod();

    fprintf(stdout, "%012.3fms: emulation ends\n", time_period1);
    My402ListUnlinkAll(Q1);
    My402ListUnlinkAll(Q2);

    tt_em_time = time_period1;

    PrintStatistics();

    exit(1);

}

void printerr()
{
    fprintf(stderr, "(malformed command)\n");
    fprintf(stderr, "Correct format: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
    exit(1);
}

void Judge_which_mode(int argc , char* argv[], void *argument)
{
    GlobalVal *arg = (GlobalVal*) argument;

    isTracedriven = 0; //means read command
    struct stat a;


    int i;
    if(argc%2 == 0)
        printerr();
    for(i=1; i<argc; i+=2)
    {


        if(strcmp("-mu",argv[i])==0)
        {
            if(!isdigit(argv[i+1][0]))
                printerr();

            if(atof(argv[i+1]) < 0.1)
                arg -> mu = 0.1;
            else
                arg -> mu = atof(argv[i+1]);
        }
        else if(strcmp("-r",argv[i])==0)
        {
            if(!isdigit(argv[i+1][0]))
                printerr();
            if(atof(argv[i+1]) < 0.1)
                arg -> r = 0.1;
            else
                arg -> r = atof(argv[i+1]);
        }
        else if(strcmp("-B",argv[i])==0)
        {
            if(!isdigit(argv[i+1][0]))
                printerr();
            arg -> numtoken = atoi(argv[i+1]);

        }
        else if(strcmp("-P",argv[i])==0)
        {
            if(!isdigit(argv[i+1][0]))
                printerr();
            arg -> p = atoi(argv[i+1]);
        }
        else if(strcmp("-n",argv[i])==0)
        {
            if(!isdigit(argv[i+1][0]))
                printerr();
            arg -> numpacket = atoi(argv[i+1]);
        }
        else if(strcmp("-lambda",argv[i])==0)
        {
            if(!isdigit(argv[i+1][0]))
                printerr();
            if(atof(argv[i+1]) < 0.1)
                arg -> lambda = 0.1;
            else
                arg -> lambda = atof(argv[i+1]);
        }
        else if(strcmp("-t",argv[i])==0)
        {
            isTracedriven = 1; //means read file
            //    strcpy(filename, argv[i+1]);
            filename =  argv[i+1];
            stat( argv[i+1], &a);

            if (S_ISDIR(a.st_mode))
            {
                printf("(input file %s is a directory.)\n", argv[2]);
                exit(1);
            }


        }

        else
        {
            if(argv[i][0] != '-')
            {
                printerr();
            }
        }
    }
}

void printParameters(void* arg)
{
    GlobalVal *argu = (GlobalVal*) arg;


    printf("Emulation Parameters:\n\n");
    fprintf(stdout, "\tnumber to arrive = %i\n", argu-> numpacket);

    if(isTracedriven == 0)
    {
        printf("\tlambda = %f\n", argu-> lambda);
        printf("\tmu = %f\n", argu-> mu);

    }
    printf("\tr = %f\n", argu-> r);
    printf("\tB = %d\n", argu-> numtoken);
    printf("\tP = %d\n", argu-> p);

    if(isTracedriven == 1)
    {
        printf("\ttsfile = %s\n", filename);

    }


}

int main(int argc , char* argv[])
{
    signal(SIGINT, handler);
    FILE *file ;
    GlobalVal *argu = (GlobalVal*) malloc(sizeof(GlobalVal));

    InitialVal(argu);
    Judge_which_mode(argc , argv , argu );

    if(isTracedriven == 1)
    {
        file = fopen(filename,"r");
        if (file == NULL)
        {

            if (errno == ENOENT)
            {
                printf("(input file %s does not exist)\n", filename);
            }
            else if(errno == EACCES)
            {
                printf("(input file %s cannot be opened - access denies)\n", filename);
            }
            exit(1);
        }
        int len = strlen(filename);
            if(filename[len - 1] != 't' || filename[len - 2] != 'x'|| filename[len - 3] != 't' || filename[len - 4] != '.')
            {
                printf("(input file is not in the right format)\n");
                exit(1);
            }
        Readfile(file, argu);
        fclose(file);
    }

    printParameters(argu);

    CreatPthread(argu);
    JoinPthread();
    PrintStatistics();

    return 0;
}
