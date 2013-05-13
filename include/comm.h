#ifndef _DSHM_COM_H_
#define _DSHM_COM_H_

#program pack(1)

typedef struct _DShmHead
{
    unsigned char cmd;
    unsigned char ver;    
    unsigned short seq;        
}DShmHead;


enum
{
   EDSHM_SYNC_FROM_CLIENT = 1,
   EDSHM_GET_SERVER_LIST_FROMCLIENT, 
   EDSHM_SYNC_MISSING_DATA ,

   EDSHM_HELLO_FROM_CLIENT = 6,       
   EDSHM_JOIN ,     
   EDSHM_GET_SERVER_LIST_FROMSERVER ,    
};

#program pack()

#endif