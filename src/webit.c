/* 
 * File:   webit.c
 * Author: Fcten
 *
 * Created on 2014年8月20日, 下午2:10
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>  
#include <grp.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "webit.h"
#include "common/wbt_string.h"
#include "common/wbt_memory.h"
#include "common/wbt_log.h"
#include "common/wbt_module.h"

extern wbt_module_t * wbt_modules[];

void wbt_null() {}

extern char **environ;
char *last;

void initProcTitle(int argc, char **argv)
{
    size_t size = 0;
    int i;
    for (i = 0; environ[i]; ++i) {
        size += strlen(environ[i])+1; 
    }   
 
    char *raw = malloc(size*sizeof(char));
    for (i = 0; environ[i]; ++i) {
        memcpy(raw, environ[i], strlen(environ[i]) + 1); 
        environ[i] = raw;
        raw += strlen(environ[i]) + 1;
    }   
 
    last = argv[0];
    for (i = 0; i < argc; ++i) {
        last += strlen(argv[i]) + 1;   
    }   
    for (i = 0; environ[i]; ++i) {
        last += strlen(environ[i]) + 1;
    }   
}
 
void setProcTitle(int argc, char **argv, const char *title)
{
    argv[1] = 0;
    char *p = argv[0];
    memset(p, 0x00, last - p); 
    strncpy(p, title, last - p); 
}
 
/*
 * 
 */
int main(int argc, char** argv) {    
    wbt_log_debug("Webit startup (pid: %d)", getpid());

    /* 设置程序允许打开的最大文件句柄数 */
    struct rlimit rlim;
    rlim.rlim_cur = 65535;
    rlim.rlim_max = 65535;
    setrlimit(RLIMIT_NOFILE, &rlim);
    
    /* 设置需要监听的信号 */
    /*sigaction(SIGCHLD, NULL, NULL);
    sigaction(SIGALRM, NULL, NULL);
    sigaction(SIGIO, NULL, NULL);
    sigaction(SIGINT, NULL, NULL);*/
    signal(SIGINT, wbt_null);

#ifndef WBT_DEBUG 
    /* 转入后台运行 */
    if( daemon(1,0) < 0 ) {
        perror("error daemon");  
        return 1;
    }
#endif

    initProcTitle(argc, argv);
    
    /* 初始化模块 */
    int i;
    for( i = 0 ; wbt_modules[i] ; i++ ) {
        if( wbt_modules[i]->init && wbt_modules[i]->init(/*cycle*/) != WBT_OK ) {
            /* fatal */
            wbt_log_debug( "module %.*s occured errors", wbt_modules[i]->name.len, wbt_modules[i]->name.str );
            return 1;
        } else {
            wbt_log_debug( "module %.*s loaded", wbt_modules[i]->name.len, wbt_modules[i]->name.str );
        }
    }

#ifndef WBT_DEBUG 
    /* 创建运行实例 */
    pid_t childpid;
    int status;
    while(1) {
        childpid = fork();

        if ( -1 == childpid ) {
            perror( "fork()" );
            exit( EXIT_FAILURE );
        } else if ( 0 == childpid ) {
            /* In child process */
            setProcTitle(argc, argv, "Webit: worker process");
            break;
        } else {
            /* In parent */
            setProcTitle(argc, argv, "Webit: master process (default)");
            wbt_log_add("Webit startup (pid: %d)\n", getpid());
            waitpid( childpid, &status, 0 );
        }
    }
#else
    wbt_log_add("Webit startup (pid: %d)\n", getpid());
#endif

    /* 限制可以访问的目录 */
    if(chroot("/home/wwwroot/")) {
        perror("chroot");
        return 1;
    }

    /* 降低 worker 进程的权限 */
    if (geteuid() == 0) {
        if (setgid(33) == -1) {
            wbt_log_debug("setgid(%d) failed", 33)
            return 1;
        }

        if (initgroups("www-data", 33) == -1) {
            wbt_log_debug("initgroups(www-data, %d) failed", 33);
            return 1;
        }

        if (setuid(33) == -1) {
            wbt_log_debug("setuid(%d) failed", 33);
            return 1;
        }
    }

    wbt_event_dispatch();

    wbt_conn_cleanup();

    wbt_log_add("Webit exit (pid: %d)\n", getpid());

    return 0;
}
