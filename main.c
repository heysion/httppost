/*
  #include <stdio.h>
  #include "../libuv/include/uv.h"

  int main() {
  return 0;
  }
*/
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <curl/curl.h>
#include <string.h>
#include <time.h>
#include <iconv.h>

uv_loop_t *loop;
CURLM *curl_handle;
uv_timer_t timeout;

typedef struct curl_context_s {
    uv_poll_t poll_handle;
    curl_socket_t sockfd;
} curl_context_t;

curl_context_t *create_curl_context(curl_socket_t sockfd) {
    int r;
    curl_context_t *context;
    context = (curl_context_t*) malloc(sizeof *context);
    context->sockfd = sockfd;
    r = uv_poll_init_socket(loop, &context->poll_handle, sockfd);
    if(r)
    {
        fprintf(stdout,"[SYSTEM] ERROR %d",r);
    }
    context->poll_handle.data = context;

    return context;
}

size_t function_return( void *ptr, size_t size, size_t nmemb, void *stream)
{
    fprintf(stdout,"%s",ptr);
    return 0;
}

void curl_close_cb(uv_handle_t *handle) {
    curl_context_t *context = (curl_context_t*) handle->data;
    free(context);
}

void destroy_curl_context(curl_context_t *context) {
    uv_close((uv_handle_t*) &context->poll_handle, curl_close_cb);
}

typedef struct tag_vs_alarm_info_submit_t
{
    char id[21];
    char alarm_msg[81];
    char lv1_phone_no[21];
    char lv2_phone_no[21];
    char lv3_phone_no[21];
    char alarm_datetime[21];
}vs_alarm_info_submit_t;

int code_convert(char *from_charset, char *to_charset, char *inbuf, int inlen, char *outbuf, int outlen)
{
    iconv_t cd;
    int rc;
    char **pin = &inbuf;
    char **pout = &outbuf;

    cd = iconv_open(to_charset, from_charset);

    if (cd == 0) 
        return 1;

    memset(outbuf, 0, outlen);

    if (iconv(cd, pin, &inlen, pout, &outlen) == -1) 
        return 1;

    iconv_close(cd);
    return 0;
}

void add_download(const char *url, int num) {

    CURL *curl;
    //CURLcode res;
    curl = curl_easy_init();

    vs_alarm_info_submit_t *body = (vs_alarm_info_submit_t *)malloc(sizeof(vs_alarm_info_submit_t));
    time_t now;
    time(&now);
    strftime(body->id, 20 , "%Y%m%d%H%M%S001", localtime(&now));
    strftime(body->alarm_datetime,20,"%Y-%m-%d %H:%M:%S",localtime(&now));

    //strncpy(body->alarm_msg,"中文",80);
    printf("len:%d %d\n",strlen(body->alarm_msg),strlen("abc"));

    int inlen = 2048;
    iconv_t cd = iconv_open( "gb18030" , "utf-8");
    if(cd == (iconv_t)-1){ return -1;}
    char *outbuf = (char *)malloc(inlen * 4 );
    bzero( outbuf, inlen * 4);
    char *in = "中文";
    char *out = outbuf;
    size_t outlen = inlen *4;
    iconv(cd, &in, (size_t *)&inlen, &out,&outlen);
    outlen = strlen(outbuf);
    strncpy(body->alarm_msg,"中文",80);
    printf("in[%d]:%s\nkk[%d]:%s\n",inlen,body->alarm_msg,outlen,outbuf);
    bzero( outbuf, inlen * 4);
    
    strncpy(body->lv1_phone_no,"013062699080",20);
    strncpy(body->lv2_phone_no,"15618666035",20);
    strncpy(body->lv3_phone_no,"15618666035",20);
    char xml_cmd_buf[2048]={0,};
    snprintf(xml_cmd_buf,2048,"<RequestInfo><Id>flyingwings</Id><sign>58a4b20db19c1315c30f6cd259bf83b5</sign>"
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

    in = xml_cmd_buf;
    out = outbuf;
    outlen = inlen *4;
    iconv(cd, &in, (size_t *)&inlen, &out,&outlen);
    outlen = strlen(outbuf);
    //strncpy(body->alarm_msg,outbuf,80);
    printf("send-buf:\n%s\n",outbuf);
    iconv_close(cd);

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://116.226.70.205:6666/");
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,strlen(outbuf));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, outbuf);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, function_return);
        curl_multi_add_handle(curl_handle, curl);
    }

    fprintf(stderr, "Added download %s \n", url);
}

void curl_perform(uv_poll_t *req, int status, int events) {
    uv_timer_stop(&timeout);
    //printf("curl_perform# timeout %d status %d events %d \n",timeout,status,events);
    int running_handles;
    int flags = 0;
    if (events & UV_READABLE) flags |= CURL_CSELECT_IN;
    if (events & UV_WRITABLE) flags |= CURL_CSELECT_OUT;

    curl_context_t *context;

    context = (curl_context_t*)req;
    //printf("curl_perform# flag %d  sockfd %d \n",flags,context->sockfd);
    curl_multi_socket_action(curl_handle, context->sockfd, flags, &running_handles);

    char *done_url;

    CURLMsg *message;
    int pending;
    while ((message = curl_multi_info_read(curl_handle, &pending))) {
        switch (message->msg) {
            case CURLMSG_DONE:
                curl_easy_getinfo(message->easy_handle, CURLINFO_EFFECTIVE_URL, &done_url);
                printf("\n%s DONE\n", done_url);

                curl_multi_remove_handle(curl_handle, message->easy_handle);
                curl_easy_cleanup(message->easy_handle);

                break;
            default:
                fprintf(stderr, "CURLMSG default\n");
                abort();
        }
    }
}

void on_timeout(uv_timer_t *req, int status) {
    int running_handles;
    curl_multi_socket_action(curl_handle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
}

void start_timeout(CURLM *multi, long timeout_ms, void *userp) {
    if (timeout_ms <= 0)
        timeout_ms = 1; /* 0 means directly call socket_action, but we'll do it in a bit */
    uv_timer_start(&timeout, on_timeout, timeout_ms, 0);
}

int handle_socket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp) {
    curl_context_t *curl_context;
    //printf("handle_scoket# action:%d \n",action);
    if (action == CURL_POLL_IN || action == CURL_POLL_OUT) {
        if (socketp) {
            curl_context = (curl_context_t*) socketp;
        }
        else {
            curl_context = create_curl_context(s);
        }
        curl_multi_assign(curl_handle, s, (void *) curl_context);
    }

    switch (action) {
        case CURL_POLL_IN:
            uv_poll_start(&curl_context->poll_handle, UV_READABLE, curl_perform);
            break;
        case CURL_POLL_OUT:
            uv_poll_start(&curl_context->poll_handle, UV_WRITABLE, curl_perform);
            break;
        case CURL_POLL_REMOVE:
            if (socketp) {
                uv_poll_stop(&((curl_context_t*)socketp)->poll_handle);
                destroy_curl_context((curl_context_t*) socketp);               
                curl_multi_assign(curl_handle, s, NULL);
            }
            break;
        default:
            abort();
    }

    return 0;
}

int main(int argc, char **argv) {
    int i;
    loop = uv_default_loop();

    if (curl_global_init(CURL_GLOBAL_ALL)) {
        fprintf(stderr, "Could not init cURL\n");
        return 1;
    }

    uv_timer_init(loop, &timeout);

    curl_handle = curl_multi_init();
    curl_multi_setopt(curl_handle, CURLMOPT_SOCKETFUNCTION, handle_socket);
    curl_multi_setopt(curl_handle, CURLMOPT_TIMERFUNCTION, start_timeout);
   
    char upstream[128];
    memset(upstream,0,128);
    snprintf(upstream,128,"%d:%s:%d",i,"upstream",i+1);
    add_download(upstream,strlen(upstream));
   

    uv_run(loop, UV_RUN_DEFAULT);
    curl_multi_cleanup(curl_handle);
    return 0;
}

int set_make()
{
    vs_alarm_info_submit_t *body = (vs_alarm_info_submit_t *)malloc(sizeof(vs_alarm_info_submit_t));
    time_t now;
    time(&now);
    strftime(body->id, 20 , "%Y%m%d%H%M%S001", localtime(&now));
    strftime(body->alarm_datetime,20,"%Y-%m-%d %H:%M:%S",localtime(&now));
    strncpy(body->alarm_msg,"testing",80);
    strncpy(body->lv1_phone_no,"013062699080",20);
    strncpy(body->lv2_phone_no,"015618666035",20);
    strncpy(body->lv3_phone_no,"015618666035",20);
    char xml_cmd_buf[2048]={0,};
    snprintf(xml_cmd_buf,2048,"<RequestInfo><Id>flyingwings</Id><sign>58a4b20db19c1315c30f6cd259bf83b5</sign>"
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

    
    return 0;

}