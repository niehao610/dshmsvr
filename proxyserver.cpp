#include "proxyserver.h"
#include <errno.h>




void * threadfun(void * p)
{
    dshm_thread_info * p = (dshm_thread_info *)p;
    
    while(1)
    {
        // hello to all svr

        
    }
    return 0;
}



ProxySvr::ProxySvr()
{
    m_so_load = 0;
    m_lastchangetime = 0;
    return 0;
}

ProxySvr::~ProxySvr()
{
     return 0;
}


ProxySvr::load_so()
{
    if(m_so_load)
    {
        return 0;
    }

    m_so_load = 1;
    
    return 0;
}

int ProxySvr::init(mq_init_info pA[], int cnt, int loopinteval_milliseconds)
{
    
    if(pA == NULL || cnt <=0 )
    {
        return -1;
    }
    
    m_loopinteval_milliseconds = loopinteval_milliseconds;
    m_items.items = new mq_item_info[cnt];
    m_items.cnt = cnt;
    m_items.name =  new int[cnt];

    char uri[256] ={0};
    int ret = 0;
    
    for(int i = 0; i < cnt ; i++)
    {
        pA[i].context = zmq_ctx_new ();
        
        switch(case pA[i].type)
        {
            case ZMQ_PULL:
                pA[i].socket = zmq_socket (pA[i].context, ZMQ_PULL);
                break;
            case ZMQ_PUSH:
                pA[i].socket = zmq_socket (pA[i].context, ZMQ_PUSH);
                break;
            case ZMQ_PUB:
                pA[i].socket = zmq_socket (pA[i].context, ZMQ_PUB);
                break;                
            case ZMQ_SUB:
                pA[i].socket = zmq_socket (pA[i].context, ZMQ_SUB);
                break;
            case ZMQ_REQ:
                pA[i].socket = zmq_socket (pA[i].context, ZMQ_REQ);
                break;                
            case ZMQ_REP:
                pA[i].socket = zmq_socket (pA[i].context, ZMQ_REP);
                break;
            default:
                sprintf(m_errmsg, "Name %d  type %d is error", pA[i].name, pA[i].type);
                return -2;
        }
        m_items.items[i].socket =  pA[i].socket;
        m_items.items[i].events = ZMQ_POLLIN;
                m_items.name[i] = pA[i].name;
        if(pA[i].role == ROLE_Client)
        {
            continue;
        }
        
        sprintf(uri, "tcp://%s:%d",  pA[i].ip , pA[i].port);
        
        if(pA[i].role == ROLE_Server) // role is server
        {
            ret = zmq_bind (pA[i].socket, uri);
        }
        //else
        //{
        //    ret = zmq_connect(pA[i].socket, uri);
        //}

        if(ret < 0)
        {
            sprintf(m_errmsg, "Name %d  type %d , role %d is error", pA[i].name, pA[i].type, pA[i].role);
            return -2;            
        }
        

        

    }
    
    m_item_num = cnt;
    
    //create a new thread 
    dshm_thread_info * pt = new dshm_thread_info();
    pt->info = pA[pt->info];
    pt->svrlist = &m_svrlist;

    
    return 0;
}



int  ProxySvr::handle_pkg( zmq_pollitem_t * pitem, int name,  char * buf, int len)
{
    int ret = 0;
    switch(name)
    {
        case PULL_FROM_STATCLIENT:
            // recv from stat client. pub out
            ret = zmq_send(m_items.items[PUSH_TO_STATCENTER].socket, buf, len, 0);

            if(ret )
            {
                //exception
                
            }
            
            break;
            
        case RESP_TO_OTHER:
            handle_resp( pitem, buf,len);
            break;
            
        case REQ_AS_CLIENT:
            //resp pkg 
            
            break;
        default:
            return 0;        
    }
    return 0;
}

int  ProxySvr::loop()
{
    int ret = -1;
    char msg[1024] ={0};
    int errnum = 0;
    while(1)
    {   
        loop_proc();
        
        ret = zmq_poll (m_items.items, m_items.cnt, m_loopinteval_milliseconds);

        if(ret < 0)
        {
            errnum = errno ;
            switch(errnum)
            {
                case ETERM:
                    // may be one  MQ context was terminated 
                    LOG("may be one  MQ context was terminated");
                    break;
                case EFAULT:
                    //The provided items was not valid
                    LOG("The provided items was not valid");
                    break

                case EINTR:
                    //The operation was interrupted by delivery of a signal before any events were available.
                    LOG("The operation was interrupted by delivery of a signal before any events were available");
                    break;
            }
        }
        else if(ret == 0)
        {
            continue;
        }
        
        for(int i = 0 ; i < m_item_num ,  ret > 0 ; i++ )
        {
            if (m_items[i].items.revents & ZMQ_POLLIN) 
            {
                ret--;
                
                int size = zmq_recv(m_items.items[i].socket , msg, sizeof(msg), 0);
                
                if ( size != -1 )
                {
                    
                    handle_pkg(&(m_items.items[i]), m_items.name[i], msg, size);
                }
            }
        }
    }
    return 0;
}




int ProxySvr::handle_resp(zmq_pollitem_t * pitem, char * buf, int len)
{   
    DShmHead * phead = (DShmHead *)buf;
    int ret = 0;

    server_info infos[50];
    int cnt = sizeof(infos/infos[0]);

    unsigned char * q = (unsigned char *)buf + sizeof(DShmHead);
    
    unsigned char * p = m_respbuff;
    
    memcpy(p, phead, sizeof(phead));
    
    p += sizeof(DShmHead);

    switch(phead->cmd)
    {
        case EDSHM_GET_SERVER_LIST_FROMCLIENT:
            
            ret = get_svrlist(infos, cnt);
            
            if(ret != 0)
            {
                *(short*)p = htons(1);p += sizeof(short);
            }
            else
            {
                *(short*)p = htons(0); p += sizeof(short);
                *(short*)p = htons(cnt); p += sizeof(short);

                for(int k = 0; k < cnt ; k++)
                {
                    *(int*)p = inet_addr(infos[k].ip); p += sizeof(int);
                    *(int*)p = htons(infos[k].pull_port2client); p += sizeof(short);
                }
            }
            
            break;

        case EDSHM_GET_SERVER_LIST_FROMSERVER:
            ret = get_svrlist(infos, cnt);
            
            if(ret != 0)
            {
                * (short*)p = htons(1);p += sizeof(short);
            }
            else
            {
                * (short*)p = htons(0); p += sizeof(short);
                * (short*)p = htons(cnt); p += sizeof(short);

                for(int k = 0; k < cnt ; k++)
                {
                    *(int*)p = inet_addr(infos[k].ip); p += sizeof(int);
                    *(int*)p = htons(infos[k].push_port2server); p += sizeof(short);
                }
            }
            
            break;
        case EDSHM_HELLO_FROM_CLIENT:
            {
                *(short*)p = htons(0); p += sizeof(short);
                break;
            }
        case EDSHM_JOIN:
            {
                struct in_addr addr;
                char keys[64] = {0};
                
                
                addr.s_addr = *(unsigned int*)q; q += sizeof(unsigned int);
                
                sprintf(infos[0].ip, "%s", inet_ntoa(addr));
                infos[0].pull_port = *(short*)q; q += sizeof(short);
                infos[0].push_port = *(short*)q; q += sizeof(short);
                infos[0].resp_port = *(short*)q; q += sizeof(short);
                infos[0].svrid = *(int*)q; q += sizeof(int);
                infos[0].max_ver = *(long*)q; q += sizeof(long);
                
                sprintf(keys, "%s:%d",  infos[0].ip, infos[0].pull_port);
                
                std::pair<std::map<std::string,server_info>::iterator,bool> ret;
                ret = m_svrlist.insert ( std::pair<char,int>(keys, infos[0]));
                
                if(ret.second==false){
                    //already exist
                   LOG("server %s exist already", keys);
                }
                else{
                   m_flag = 1;
                }
            }
 
            *(short*)p = htons(0); p += sizeof(short);
 
            break;
 
        case EDSHM_SYNC_MISSING_DATA:
            *(short*)p = htons(0); p += sizeof(short);            
            break;
            
        default:
            return 0;
    }

    int len = p - buf;
    int size = zmq_send(pitem->socket , m_respbuff, len, 0);
    
    return 0;        
}


int  ProxySvr::add_svrlist(server_info infos)
{
    std::string  key;
    key = infos.ip + std::to_string(infos.port);
    m_svrlist.insert(std::pair<key, infos>);
    return 0;
}

int  ProxySvr::get_svrlist(server_info infos[], int & cnt)
{
    int count =  0;
    std::map<char,int>::iterator it = m_svrlist.begin();
    for ( ; count!= m_svrlist.end(); ++it){

        if(count < cnt)
        {
            infos[count] = it.second;
        }

        count++;
    }

    if( count >= cnt ){
        return -1;
    }

    cnt = count;
    
    return 0;
}

int  ProxySvr::add_clientlist(server_info infos)
{
    std::string  key;
    key = infos.ip + std::to_string(infos.port);
    m_clientlist.insert(std::pair<key, infos>);
    
    return 0;
}

int  ProxySvr::get_clientlist(client_info infos[], int & cnt)
{
    int count =  0;
    std::map<char,int>::iterator it = m_clientlist.begin();
    
    for ( ; count != m_clientlist.end(); ++it){

        if(count < cnt)
        {
            infos[count] = it.second;
        }
        count++;
    }
    
    if( count >= cnt ){
        return -1;
    }
    
    cnt = count;

    return 0;
}

