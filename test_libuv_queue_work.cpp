#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <assert.h>
#include <unistd.h>

uv_loop_t *loop;

void worker_callback(uv_work_t *req ,int status)
{
    printf("work callback %d\n",(unsigned int)pthread_self());
}
void worker_entry_invoke(uv_work_t * req)
{
    //sleep(1);
    printf("work entry invoke %d\n",(unsigned int)pthread_self());
    sleep(1);
}
int main()
{
    int r;
    
    int p = 10;
    //worker_handle.data = (void *) &p  ;
    uv_work_t worker_handle[10];
    loop = uv_default_loop();
    for(int i =0 ;i<10 ;i++)
    {
        
        r = uv_queue_work(loop,&worker_handle[i],worker_entry_invoke,worker_callback);
        assert(r == 0);
    }

    return uv_run(loop,UV_RUN_DEFAULT);
    
}