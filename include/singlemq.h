#ifndef __SINGLE_MQ_H
#define __SINGLE_MQ_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include "zmq.h"
using namespace std;

/*#define ZMQ_PAIR 0
#define ZMQ_PUB 1
#define ZMQ_SUB 2
#define ZMQ_REQ 3
#define ZMQ_REP 4
#define ZMQ_DEALER 5
#define ZMQ_ROUTER 6
#define ZMQ_PULL 7
#define ZMQ_PUSH 8
#define ZMQ_XPUB 9
#define ZMQ_XSUB 10*/


typedef struct
{
    string strhost;
    int iport;
    unsigned char mode;//
    unsigned char type;// 1-REP,2-REQ,3-PUB,4-sub,5-PULL,6-PUSH
    unsigned char server_type;//1-server,2-client
    unsigned char protocol_type;//1-tcp
}mq_base_info;

typedef struct
{
    char strValue[1024];
    int strlen;
    unsigned char mode;
}mq_value_info;

class CSingleMq
{
    private:
        void *m_context;
        void * m_requester;
        char m_errMsg[1024];

    public:
        CSingleMq(){
            m_context = NULL;
            m_requester = NULL;
            memset(m_errMsg,0,sizeof(m_errMsg));
        }

        ~CSingleMq()
        {
            DelInit();
        }
    private:
        int reconnect(mq_base_info & stinfo);

        int connect(mq_base_info & stinfo);
        
    public:
        const char* getErrMsg(){return m_errMsg;}

        int init(mq_base_info & stinfo);
        void DelInit();

        int sub(mq_value_info & stinfo);
        int pub(mq_value_info & stinfo);

        int req(mq_value_info & stinfo);
        int resp(mq_value_info & stinfo);

        int push(mq_value_info & stinfo);
        int pull(mq_value_info & stinfo);
};

