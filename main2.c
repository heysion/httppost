
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

typedef struct tag_vs_alarm_info_submit_t
{
    char id[21];
    char alarm_msg[81];
    char lv1_phone_no[21];
    char lv2_phone_no[21];
    char lv3_phone_no[21];
    char alarm_datetime[21];
}vs_alarm_info_submit_t;

struct CBC
{
    char *buf;
    size_t pos;
    size_t size;
};

size_t
copyBuffer (void *ptr, size_t size, size_t nmemb, void *ctx)
{
    struct CBC *cbc = ctx;
    fprintf(stdout,"cpy-len0:%d[%d][%d][cbc->pos:%d:%d]\n",strlen(ptr),size,nmemb,cbc->pos,cbc->size);
    if (cbc->pos + size * nmemb > cbc->size)
    {

        return 0; /* overflow */
    }
    memcpy (&cbc->buf[cbc->pos], ptr, size * nmemb);
    cbc->pos += size * nmemb;
    fprintf(stdout,"cpy-len1:%d[%d][%d][cbc->pos:%d:%d]\n",strlen(ptr),size,nmemb,cbc->pos,cbc->size);
    return size * nmemb;
}

size_t function_return( void *ptr, size_t size, size_t nmemb, void *stream)
{
    fprintf(stdout,"len:%d\n%s\n",strlen(ptr),ptr);
    return 0;
}

int cmd_utf8_to_gb2312(char *in,char *outbuf,size_t outlen)
{
    int inlen = strlen(in);
    char *out = outbuf;
    iconv_t cd = iconv_open( "gb18030" , "utf-8");
    if(cd == (iconv_t)-1){ return -1;}
    bzero( outbuf, inlen * 4);

    iconv(cd, &in, (size_t *)&inlen, &out,&outlen);
    outlen = strlen(outbuf);
    int flag = iconv_close(cd);
    printf("kk[%d]:%s\n",outlen,outbuf);
    return 0;
}

typedef struct vs_buf_s
{
    char buf[2048];
    size_t len;
    size_t pos;
}buf2k_t;

void init_buf2k(buf2k_t *buf)
{
    bzero(buf->buf,2048);
    buf->len = 2048;
    buf->pos =0 ;
}

int test()
{
    CURLMcode res;
    CURL *curl;
    char buf[2048];
    
    struct CBC cbc;
    buf2k_t send_buf;
    init_buf2k(&send_buf);
    cbc.buf = buf;
    cbc.size = 2048;
    cbc.pos = 0;
    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    vs_alarm_info_submit_t *body = (vs_alarm_info_submit_t *)malloc(sizeof(vs_alarm_info_submit_t));
    time_t now;
    time(&now);
    strftime(body->id, 20 , "%Y%m%d%H%M%S001", localtime(&now));
    strftime(body->alarm_datetime,20,"%Y-%m-%d %H:%M:%S",localtime(&now));
    strncpy(body->alarm_msg,"中文",80);

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

    printf("xxsend:%s\n",xml_cmd_buf);
    //if(cmd_utf8_to_gb2312(xml_cmd_buf,send_buf.buf,send_buf.len) == 0)
    //{
    
        if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://116.226.70.205:6666/");
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,strlen(xml_cmd_buf));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xml_cmd_buf);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, copyBuffer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cbc);

        //res = curl_multi_add_handle(m, curl);
        res = curl_easy_perform(curl);
    /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        }
        //}

    curl_easy_cleanup(curl);

    cbc.buf[cbc.pos]=0;
    printf("cbc-buf:%d %d\n%s\n",strlen(cbc.buf),cbc.pos,cbc.buf);
    return res;
}
int main()
{
    loop = uv_default_loop();
    if(curl_global_init(CURL_GLOBAL_ALL))
    {
        fprintf(stderr, "Could not init cURL\n");
        return 1;
    }
    uv_timer_init(loop,&timeout);
    curl_handle = NULL ;
    return 0;
    test();
}

int main2(int argc , char **argv)
{
    loop = uv_default_loop();
    if(curl_global_init(CURL_GLOBAL_ALL))
    {
        fprintf(stderr, "Could not init cURL\n");
        return 1;
    }
    uv_timer_init(loop,&timeout);
    curl_handle = NULL ;
    return 0;
}

int test_get_result()
{

}

