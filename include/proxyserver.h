#ifndef  _PROXY_SERVER_H_
#define _PROXY_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <list>
#include <map>

#include "AddAttrToShm.h"
#include "log.h"
#include <zmq.h>
#include "comm.h"
#include <pthread.h>


#define LOG(lv, fmt, args...)   {\
    if(lv <= m_iLogLv)     {   \
        if(m_pLog != NULL)    \
        m_pLog->Log_Msg("%s:%d(%s): " fmt, __FILE__, __LINE__, __FUNCTION__ , ## args);   \
    }   \
}



#define LOG2(lv, fmt, args...)   {\
    if(lv <= pt.log_level)     {   \
        if(pt.plog != NULL)    \
        pt.plog->Log_Msg("%s:%d(%s): " fmt, __FILE__, __LINE__, __FUNCTION__ , ## args);   \
    }   \
}




enum 
{
    ROLE_Client = 1,
    ROLE_Server ,
};


typedef struct _addr_info
{
    char ip[32];
    unsigned short pull_port2client;
    unsigned short push_port2server;
    unsigned short resp_port;

    int  svrid;
    long last_time;
    long max_ver;
    int  stat;
    int  err_num;
}addr_info;

#define server_info  addr_info
#define client_info  addr_info


typedef struct _mq_init_info
{
    void *context;
    void *socket;    
    int name;
    int type;
    int proto;    
    int role;  // 1-client 2-server
    char ip[32];
    short port;
}mq_init_info;

typedef struct _dshm_thread_info
{
    mq_init_info * info;
    std::map<std::string , server_info > * svrlist;
    MyLog * plog;
    int          log_level;
}dshm_thread_info;


typedef struct _mq_item_info
{
    zmq_pollitem_t * items;
    int * name;
    int cnt;
}mq_item_info;



class ProxySvr
{
    public:
        ProxySvr();
        ~ProxySvr();
        
        int init(mq_init_info pA[], int cnt, int loopinteval_milliseconds);
        int loop();

        int handle_resp(zmq_pollitem_t * t, char * buf, int len);
        
        int handle_pkg(mq_item_info * pitem, char * buf, int len);
        virtual int loop_proc() = 0;
        
        void setSysLogInfo(MyLog * pLog, int level){m_pLog = pLog;  m_iLogLv = level}
        
        enum{ PULL_FROM_STATCLIENT = 0, PUSH_TO_STATCENTER , RESP_TO_OTHER, REQ_AS_CLIENT};

        void  setsopath(std::string path){ m_so_path = path; }

        int  add_svrlist(server_info infos);
        int  get_svrlist(server_info infos[], int & cnt);

        int  add_clientlist(server_info infos);
        int  get_clientlist(server_info infos[], int & cnt);

        void  set_mainsvr(std::string ip, int port){m_mainsvrip = ip ; m_mainsvrport = port;}
        
        int  handle_pkg( zmq_pollitem_t * pitem, int name,  char * buf, int len);
    private:
        int load_so();

    private:
        mq_item_info  m_items;
        int m_item_num;
        char m_errmsg[1024];
        int  m_loopinteval_milliseconds;

        int   m_iLogLv;
        MyLog * m_pLog;

        int   m_so_load;
        std::string m_so_path;
        unsigned char m_respbuff[1024];

        int m_flag;
        int m_lastchangetime ;

        int m_mainsvrip;
        int m_mainsvrport;
        
    private:
        std::map<std::string , server_info> m_svrlist;
        std::map<std::string, client_info> m_clientlist;
       pthread_t m_threadid;

};

#endif 
