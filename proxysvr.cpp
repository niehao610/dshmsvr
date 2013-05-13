#include "singlemq.h"
#include "proxyserver.h"


class CConfig: public MyLibConfig
{
    MyLog   myLog;
    string  sLogFilePath;
    int     iMaxLogSize;
    int     iMaxLogNum;
    int     iMaxSocketNum;
    string  sPullIp;
    int     nPullPort;
    string  sPubIp;    
    int     nPubPort;    
    string  sRespIp;    
    int     nRespPort;

    string  sMasterIp;    
    int     nMasterPort;    

    int     iLogLvShmKey;

    string  sDbIp;
    string  sDbUserName;
    string  sDbPass;
    string  sDbName;
    int     iDbPort;
    int     iDbMaxRecord;
    int     iLoopInterMsec;
    
};


CConfig g_config;

static int Init(int argc, char *argv[])
{
    CConfig *pConfig = &g_config;

    char sConfFilePath[255]={0};

    if (argc != 2) 
    {
        printf("Usage: %s Config_file\n", argv[0]); return -1;
    }

    if (strlen(argv[1]) >= sizeof(sConfFilePath)) 
    {
        printf("Config file path too long!\n"); return -1;
    }
    
    strncpy(sConfFilePath, argv[1], strlen(argv[1]));
    pConfig->GetConfigFile(sConfFilePath, 
        "LOGFILEPATH", CONFIG_STRING, &(pConfig->sLogFilePath), "/tmp/dshmproxy",
        "LOG_SIZE", CONFIG_INT, &(pConfig->iMaxLogSize), 0,
        "LOG_NUM", CONFIG_INT, &(pConfig->iMaxLogNum), 0,
        "LOG_LV_SHM_KEY", CONFIG_INT, &(pConfig->iLogLvShmKey), 0,        
        "PULL_IP", CONFIG_STRING, &(pConfig->sPullIp), "",
        "PULL_PORT", CONFIG_INT, &(pConfig->nPullPort), 0,
        "PUB_IP",  CONFIG_STRING, &(pConfig->sPubIp), "",
        "PUB_PORT", CONFIG_INT, &(pConfig->nPubPort), 0,
        "RESP_IP", CONFIG_STRING, &(pConfig->sRespIp), "",
        "RESP_PORT", CONFIG_INT, &(pConfig->nRespPort), 0,
        "MASTER_IP", CONFIG_STRING, &(pConfig->sMasterIp), "",
        "MASTER_PORT", CONFIG_INT, &(pConfig->nMasterPort), 0,
        "DB_IP", CONFIG_STRING, &(pConfig->sDbIp), "",
        "DB_USER_NAME", CONFIG_STRING, &(pConfig->sDbUserName), "",
        "DB_PASS", CONFIG_STRING, &(pConfig->sDbPass), "",
        "DB_NAME", CONFIG_STRING, &(pConfig->sDbName), "",
        "DB_PORT", CONFIG_INT, &(pConfig->iDbPort), 0,
        "DB_MAX_RECORD", CONFIG_INT, &(pConfig->iDbMaxRecord), 10000,
        "LOOP_SLEEP", CONFIG_INT, &(pConfig->iLoopInterMsec), 20,
    NULL);

    
    return 0;
}

int proc_init(int pid)
{
    char logfile[512] = {0};
    
    int procIndex = 1;
    sprintf(logfile, "%s_%d_", g_config.sLogFilePath.c_str(), procIndex);
    g_config.myLog.Log_Init(logfile, g_config.iMaxLogSize, g_config.iMaxLogNum, 0);

    return 0;
}

int monitor_proc()
{

    int status = 0;
    int ret = 0;
    while(1)        
    {
        // master proc , monitor child proc 
        ret = wait(&status);
        
        if(ret == 0)    
        {
            g_config.myLog.Log_Msg("no child process , exited now !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            return -1;
        }
        else if(ret < 0)        
        {
            g_config.myLog.Log_Msg("waitpid error. errno:%d %s , exited now !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", errno, strerror(errno));
            return -2;
        }
        else    
        {
            if(WIFEXITED(status))   
            {
                g_config.myLog.Log_Msg("child process(%d) exited sucessfully", ret);
                g_config.myLog.Log_Msg("And it's exited code is:%d\n", WEXITSTATUS(status));
            }
            else if(WIFSIGNALED(status))    
            {
                g_config.myLog.Log_Msg("child process (%d)  exited by signal," , ret );
                g_config.myLog.Log_Msg("And the signal code is:%d\n", WTERMSIG(status));
            }
            else if(WCOREDUMP(status))      
            {
                g_config.myLog.Log_Msg("child process(%d) exited with core dump\n", ret );
            }
            else    
            {
                g_config.myLog.Log_Msg("child process (%d) stopped or resumed\n", ret );
            }


            int pid = fork();

            if(pid == 0)
            {
                break;
            }
            else if(pid < 0 )
            {
                g_config.myLog.Log_Msg("fork fail !!!!!!!!!!!!!!! \n"); 
                return -3;
            }
            else
            {
                // master proc 
                g_config.myLog.Log_Msg("child proc pid %d restart", pid);

                proc_init(pid);
            }

        }

    }
    
    return 0;
}



int main(int argc, char * argv[])
{
    Init(argc, argv);

    char logfile[512] = {0};

    int procIndex = 0;

    int ret = 0;
    ret = fork();
    
    if(ret == 0 )
    {
        procIndex = 1;
    }
    else if( ret < 0 )
    {
        printf("fork fail \n");
        return 0;
    }
    else
    {
        procIndex = 0;
    }
    sprintf(logfile, "%s_%d_", g_config.sLogFilePath.c_str(), procIndex);
    g_config.myLog.Log_Init(logfile, g_config.iMaxLogSize, g_config.iMaxLogNum, 0);

    if(procIndex == 0)
    {
        // main proc . monitor child proc
        ret = monitor_proc();

        if(ret < 0)
        {
            printf("monitor_proc fail !!!!!!!!\n");
            return 0;
        }
    }

    ProxySvr  server;

    mq_init_info minfo[3];

    minfo[0].context = NULL;
    minfo[0].socket = NULL;
    snprintf(minfo[0].ip, sizeof(minfo[0].ip), "%s", g_config.sPullIp.c_str();
    minfo[0].port = g_config.nPullPort;
    minfo[0].role = ROLE_Server;
    minfo[0].type = ZMQ_PULL;
    minfo[0].name = ProxySvr::PULL_FROM_STATCLIENT;

    minfo[1].context = NULL;
    minfo[1].socket = NULL;
    snprintf(minfo[1].ip, sizeof(minfo[1].ip), "%s", g_config.sPubIp.c_str();
    minfo[1].port = g_config.nPubPort;
    minfo[1].role = ROLE_Server;
    minfo[1].type = ZMQ_PUB;
    minfo[1].name = ProxySvr::PUB_TO_STATCENTER;
    
    minfo[2].context = NULL;
    minfo[2].socket = NULL;
    snprintf(minfo[2].ip, sizeof(minfo[2].ip), "%s", g_config.sRespIp.c_str();
    minfo[2].port = g_config.nRespPort;
    minfo[2].role = ROLE_Server;
    minfo[2].type = ZMQ_REP;
    minfo[2].name = ProxySvr::RESP_TO_OTHER;

    minfo[3].context = NULL;
    minfo[3].socket = NULL;
    snprintf(minfo[3].ip, sizeof(minfo[3].ip), "%s", g_config.sMasterIp.c_str();
    minfo[3].port = g_config.nMasterPort;
    minfo[3].role = ROLE_Client;
    minfo[3].type = ZMQ_REP;
    minfo[3].name = ProxySvr::REQ_AS_CLIENT;
    
    server.init(minfo , sizeof(minfo)/sizeof(minfo[0]),  g_config.iLoopInterMsec);

    server.loop();
    return 0;
}





