#include <uv.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <iostream>

#include "httppost.h"


uv_loop_t *main_loop;
uv_loop_t *submit_loop,*update_loop;
uv_thread_t worker_submit;
uv_thread_t worker_update;
unsigned int sn = 0;
submit_async_t submit_async;
update_async_t update_async;
main_async_t   main_async;
uv_timer_t timer_submit,timer_update;

using namespace std;


void work_cb(uv_work_t *req)
{
    int i =0;
    printf("%d : %d  : %u\n",(int *)req->data,getpid(),(unsigned int)pthread_self());
    i = *((int *)req->data);
    i = i+3;
    sleep(i%5);
}

void after_work_cb(uv_work_t *req,int status)
{

}


void submit_async_callback(main_async_t *self, task_t *head,task_t *tail)
{

    uv_mutex_lock(&self->lock);
    tail->next = self->tasks;
    self->tasks = head;
    uv_mutex_unlock(&self->lock);

    uv_async_send((uv_async_t *) self);
}


void update_async_callback(main_async_t *self, task_t *head,task_t *tail)
{
    uv_mutex_lock(&self->lock);
    tail->next = self->tasks;
    self->tasks = head;
    uv_mutex_unlock(&self->lock);

    uv_async_send((uv_async_t *) self);
}


void submit_async_add_task(submit_async_t *self, task_t *task)
{
    uv_mutex_lock(&self->lock);
    task->next = self->tasks;
    self->tasks = task;
    uv_mutex_unlock(&self->lock);

    uv_async_send((uv_async_t *) self);
}

void update_async_add_task(update_async_t *self, task_t *task)
{
    uv_mutex_lock(&self->lock);
    task->next = self->tasks;
    self->tasks = task;
    uv_mutex_unlock(&self->lock);

    uv_async_send((uv_async_t *) self);
}

task_t * submit_async_take_tasks(submit_async_t *self)
{
    if(NULL == self->tasks) {
        return NULL;
    }

    uv_mutex_lock(&self->lock);
    task_t *tasks = self->tasks;
    self->tasks = NULL;
    uv_mutex_unlock(&self->lock);

    return tasks;
}

task_t * update_async_take_tasks(update_async_t *self)
{
    if(NULL == self->tasks) {
        return NULL;
    }

    uv_mutex_lock(&self->lock);
    task_t *tasks = self->tasks;
    self->tasks = NULL;
    uv_mutex_unlock(&self->lock);

    return tasks;
}

task_t * main_async_take_tasks(main_async_t *self)
{
    if(NULL == self->tasks) {
        return NULL;
    }

    uv_mutex_lock(&self->lock);
    task_t *tasks = self->tasks;
    self->tasks = NULL;
    uv_mutex_unlock(&self->lock);

    return tasks;
}


void submit_async_cb(uv_async_t *handle)
{
    submit_async_t *async = (submit_async_t *) handle;
    task_t *head = submit_async_take_tasks(async), *tail;
    task_t *task = head;

    // simulate database latency
    do {
        //sleep(1);
        //putc('.', stdout);
        vs_alarm_info_submit_t *body=&(task->data) ;
        printf("%u:%d=%s\n",(unsigned int)pthread_self(),getpid(),body->id);
        //fflush(stdout);

        if(! task->next) {
            tail = task;
        }

    } while((task = task->next));

    submit_async_callback(&main_async, head, tail);
}

void update_async_cb(uv_async_t *handle)
{
    update_async_t *async = (update_async_t *) handle;
    task_t *head = update_async_take_tasks(async), *tail;
    task_t *task = head;

    // simulate database latency
    do {
        //        sleep(1);
        //putc('.', stdout);
        printf("%u:%d=%d\n",(unsigned int)pthread_self(),getpid(),task->sn);
        //fflush(stdout);

        if(! task->next) {
            tail = task;
        }
    } while((task = task->next));

    update_async_callback(&main_async, head, tail);
}

void worker_entry_submit(void *arg)
{
    submit_loop = uv_loop_new();

    uv_async_init(submit_loop, (uv_async_t *) &submit_async, submit_async_cb);
    submit_async.tasks = NULL ;
    uv_mutex_init(&submit_async.lock) ;

    uv_run(submit_loop, UV_RUN_DEFAULT);
}

void worker_entry_update(void *arg)
{
    update_loop = uv_loop_new();

    uv_async_init(update_loop, (uv_async_t *) &update_async, update_async_cb);

    uv_run(update_loop, UV_RUN_DEFAULT);
}

void submit_timer_cb(uv_timer_t *timer)
{

    std::vector<vs_alarm_info_submit_t> vs_alarm_list;
    vs_alarm_info_submit_t body;
    //DBOperator *db_handler = new DBOperator();

    //db_handler->get_alarm_info_list(&vs_alarm_list);

    memset(&body,0,sizeof(vs_alarm_list));

    int list_len = vs_alarm_list.size();
    if(list_len)
    {
        for(int i=0;i<list_len;i++)
        {
            body=vs_alarm_list.at(i) ;
            submit_async_add_task(&submit_async,task_make(&body));
            memset(&body,0,sizeof(vs_alarm_list));
        }
    }

    uv_timer_stop(timer);
    //delete db_handler;
}

void alarm_command_make(vs_alarm_info_submit_t *body,char *xml_cmd_buf,size_t buf_sz)
{
    snprintf(xml_cmd_buf,buf_sz,"",);
    
}

void update_timer_cb(uv_timer_t *timer)
{
    int i =0 ;
    for(;i < 10; i++)
    {
        update_async_add_task(&update_async, task_new(i));
    }
}

void main_callback(uv_async_t *handle)
{
    main_async_t *async = (main_async_t *) handle;
    task_t *task = main_async_take_tasks(async), *next;
    int count = 0;
    do {
        count ++;

        next = task->next;
        task_free(task);
        task = next;
    } while(task);

    printf("%d tasks done :%u\n", count,getpid());
    uv_timer_again(&timer_submit);
}

int main()
{

    int r,i;

    main_loop = uv_default_loop();
    uv_thread_create(&worker_submit, worker_entry_submit, NULL);
    uv_thread_create(&worker_update, worker_entry_update, NULL);

    uv_timer_init(main_loop,&timer_submit);
    uv_timer_init(main_loop,&timer_update);

    uv_timer_start(&timer_submit, submit_timer_cb, 0, 50);
    uv_timer_start(&timer_update, update_timer_cb, 50000, 50000);

    uv_async_init(main_loop,(uv_async_t *)&main_async,main_callback);
    main_async.tasks = NULL ;
    uv_mutex_init(&main_async.lock);

    return uv_run(main_loop, UV_RUN_DEFAULT);

}

size_t copyBuffer (void *ptr, size_t size, size_t nmemb, void *ctx)
{
    vs_buf_t *cbc = ctx;
    fprintf(stdout,"cpy-len0:%d[%d][%d][cbc->pos:%d:%d]\n",strlen(ptr),size,nmemb,cbc->pos,cbc->sz);
    if (cbc->pos + size * nmemb > cbc->sz)
    {
        return 0; /* overflow */
    }
    memcpy (&cbc->buf[cbc->pos], ptr, size * nmemb);
    cbc->pos += size * nmemb;
    fprintf(stdout,"cpy-len1:%d[%d][%d][cbc->pos:%d:%d]\n",strlen(ptr),size,nmemb,cbc->pos,cbc->sz);
    return size * nmemb;
}

int fake_submit_item(vs_alarm_info_submit_t *body,vs_buf_t *buf)
{
    time_t now;
    time(&now);
    strftime(body->id, 20 , "%Y%m%d%H%M%S001", localtime(&now));
    strftime(body->alarm_datetime,20,"%Y-%m-%d %H:%M:%S",localtime(&now));
    strncpy(body->alarm_msg,"中文",80);

    strncpy(body->lv1_phone_no,"013062699080",20);
    strncpy(body->lv2_phone_no,"15618666035",20);
    strncpy(body->lv3_phone_no,"15618666035",20);
    snprintf(buf->buf,buf->sz,"<RequestInfo><Id>flyingwings</Id><sign>58a4b20db19c1315c30f6cd259bf83b5</sign>"
        "<Data><RequestDailerInfo>"
        "<Item><DestNo>%s</DestNo><DailerDsc>%s</DailerDsc><DailerReqTime>%s</DailerReqTime>"
        "<ErrorDailerCount>1</ErrorDailerCount><DailerTrunkNo>%s</DailerTrunkNo><ExtId>%s</ExtId>"
        "<DailerLevel>%d</DailerLevel><DailerRecordCount>%d</DailerRecordCount></Item>"
        "<Item><DestNo>%s</DestNo><DailerDsc>%s</DailerDsc><DailerReqTime>%s</DailerReqTime>"
        "<ErrorDailerCount>1</ErrorDailerCount><DailerTrunkNo>%s</DailerTrunkNo><ExtId>%s</ExtId>"
        "<DailerLevel>%d</DailerLevel><DailerRecordCount>%d</DailerRecordCount></Item>"
        "<Item><DestNo>%s</DestNo><DailerDsc>%s</DailerDsc><DailerReqTime>%s</DailerReqTime>"
        "<ErrorDailerCount>1</ErrorDailerCount><DailerTrunkNo>%s</DailerTrunkNo><ExtId>%s</ExtId>"
        "<DailerLevel>%d</DailerLevel><DailerRecordCount>%d</DailerRecordCount></Item>"
        "</RequestDailerInfo></Data></RequestInfo>",
        body->lv1_phone_no,body->alarm_msg,body->alarm_datetime,"02158383041",body->id,1,3,
        body->lv2_phone_no,body->alarm_msg,body->alarm_datetime,"02158383041",body->id,2,3,
        body->lv3_phone_no,body->alarm_msg,body->alarm_datetime,"02158383041",body->id,3,3);

    buf->pos = strlen(buf->buf);
    buf->buf[pos]= 0 ;
    printf("xxsend:%s\n",buf->buf);

}

int submit_item(vs_buf_t *body,char *url)
{
    CURLMcode res;
    CURL *curl;
    char buf[2048];
    vs_buf_t cbc;
    cbc.buf = buf;
    cbc.sz = 2048;
    cbc.pos = 0;
    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,body->sz);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body->buf);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, copyBuffer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cbc);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        }
    }

    curl_easy_cleanup(curl);

    cbc.buf[cbc.pos]=0;
    printf("cbc-buf:%d %d\n%s\n",strlen(cbc.buf),cbc.pos,cbc.buf);
    return res;
}

