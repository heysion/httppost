#ifndef HTTP_POST_H
#define HTTP_POST_H

#include <uv.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

typedef struct tag_task_s task_t;
typedef struct tag_async_s submit_async_t;
typedef struct tag_async_s update_async_t;
typedef struct tag_async_s main_async_t;
typedef struct tag_async_s tag_async_t;
typedef struct vs_alarm_info_submit_s vs_alarm_info_submit_t;
typedef struct vs_buf_s vs_buf_t;

struct vs_alarm_info_submit_s
{
    char id[21];
    char alarm_msg[81];
    char lv1_phone_no[21];
    char lv2_phone_no[21];
    char lv3_phone_no[21];
    char alarm_datetime[21];
};

struct tag_task_s
{
    unsigned int sn;
    vs_alarm_info_submit_t data;
    task_t *next;
};

struct tag_async_s
{
    uv_async_t parent;
    task_t *tasks;
    uv_mutex_t lock;
};

struct tag_update_async_s
{
    uv_async_t parent;
    task_t *tasks;
    uv_mutex_t lock;
};

task_t * task_new(unsigned int sn)
{
    task_t *task = (task_t *)malloc(sizeof(task_t));
    task->sn = sn;
    task->next = NULL;
    return task;
}


struct CBC
{
    char *buf;
    size_t pos;
    size_t size;
};

struct vs_buf_s
{
    char *buf;
    size_t sz;
    size_t pos;
};


task_t * task_make(vs_alarm_info_submit_t *new_data)
{
    task_t *task = (task_t *)malloc(sizeof(task_t));
    //  task->data sn;
    strncpy(task->data.id,new_data->id,21);
    strncpy(task->data.alarm_msg,new_data->alarm_msg,81);
    strncpy(task->data.lv1_phone_no,new_data->lv1_phone_no,21);
    strncpy(task->data.lv2_phone_no,new_data->lv2_phone_no,21);
    strncpy(task->data.lv3_phone_no,new_data->lv3_phone_no,21);
    task->next = NULL;

    return task;
}

void task_set_next(task_t *self, task_t *next)
{
    self->next = next;
}

void task_free(task_t *self)
{
    free(self);
}
void init_buffer(vs_buf_t *buf,char *data,size_t sz);

void submit_async_cb(uv_async_t *handle);

void update_async_cb(uv_async_t *handle);

void worker_entry_submit(void *arg);

void worker_entry_update(void *arg);

void submit_timer_cb(uv_timer_t *timer);

void update_timer_cb(uv_timer_t *timer);

task_t * submit_async_take_tasks(submit_async_t *self);

task_t * update_async_take_tasks(update_async_t *self);

size_t copyBuffer (void *ptr, size_t size, size_t nmemb, void *ctx);

int submit_item(vs_buf_t *body,char *url);

int fake_submit_item(vs_alarm_info_submit_t *body,vs_buf_t *buf);

int fake_get_item_info(vs_alarm_info_submit_t *body,vs_buf_t *buf);


#endif
