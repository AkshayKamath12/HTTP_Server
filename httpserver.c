#include "helper_funcs.h"
#include "connection.h"
#include "response.h"
#include "request.h"
#include "queue.h"
#include "rwlock.h"
#include "map.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>


typedef struct server_t {
	map_t* ht;
    pthread_t* pt;
    queue_t* qt;
    pthread_mutex_t* mutex;
	
}server_t;

void handle_connection(int, server_t *);
void handle_get(conn_t *, server_t *);
void handle_put(conn_t *, server_t *);
void handle_unsupported(conn_t *);



void *worker(void* serverVoid){
	server_t* server= (server_t *)serverVoid;

	void *rv;
	while(true){
		queue_pop(server->qt, &rv);
		int fd = (int64_t)rv;
		handle_connection(fd, server);
		close(fd);
	}

}

int main(int argc, char **argv) {

    if (argc < 2) {
        warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
	int threads = 4;
	int opt = getopt(argc, argv, "t:");
	if(opt == 't'){
		threads = atoi(optarg);
	}
	if(threads < 1){
		threads = 4;
	}

    size_t port = (size_t) atoi(argv[optind]);



    if (port < 1 || port > 65535) {
        fprintf(stderr, "Invalid Port\n");
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);
    Listener_Socket sock;
    if (listener_init(&sock, port) < 0) {
        fprintf(stderr, "Invalid Port\n");
        return EXIT_FAILURE;
    }
	
	server_t *server = (server_t *) malloc(sizeof(server_t));
    server->ht = map_new(100);
    server->pt = (pthread_t *)malloc(threads * sizeof(pthread_t));
	server->qt = queue_new(threads);
	server->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	int rc = pthread_mutex_init(server->mutex, NULL);
	
    if (rc) {
    	
        return -1;
    }

	
    for (int i = 0; i < threads; i++) {
    	pthread_create(&(server->pt[i]), NULL, worker, server);   
    }

    
    while (1) {
        int connfd = listener_accept(&sock);
        queue_push(server->qt, (void *) (intptr_t)connfd);
    }

    return EXIT_SUCCESS;
}

void handle_connection(int connfd, server_t* server) {
    conn_t *conn = conn_new(connfd);
    const Response_t *res = conn_parse(conn);
    if (res != NULL) {
        conn_send_response(conn, res);
    } else {       
        const Request_t *req = conn_get_request(conn);       
        if (req == &REQUEST_GET) {
            handle_get(conn, server);
        } else if (req == &REQUEST_PUT) {
            handle_put(conn, server);
        } else {
            handle_unsupported(conn);
        }
    }

    conn_delete(&conn);
}

void handle_get(conn_t *conn, server_t * server){
	char *uri = conn_get_uri(conn);
    char *requestID = conn_get_header(conn, "Request-Id");	
	if(requestID == NULL){
		requestID = "0";
	}
    pthread_mutex_t* mutex = server->mutex;
    //hashtable_t* ht = server->ht;
    
    pthread_mutex_lock(mutex);
    void* res = map_get(server->ht, uri);
    rwlock_t* rwlock;
    if(res == NULL){
    	rwlock = rwlock_new(N_WAY, 1);
    	map_set(server->ht, uri, rwlock);
    }else{
    	rwlock = (rwlock_t *)res;
    }
    pthread_mutex_unlock(mutex);
    
    reader_lock(rwlock);
    int fd = open(uri, O_RDONLY);
    if(fd < 0){
    	if (errno == EACCES){
    		conn_send_response(conn, &RESPONSE_FORBIDDEN); 
    		fprintf(stderr, "GET,%s,%d,%s\n", uri, response_get_code(&RESPONSE_FORBIDDEN), requestID);
        }else if (errno == ENOENT){
    		conn_send_response(conn, &RESPONSE_NOT_FOUND);
    		fprintf(stderr, "GET,%s,%d,%s\n", uri, response_get_code(&RESPONSE_NOT_FOUND), requestID);
        }else{
        	conn_send_response(conn, &RESPONSE_INTERNAL_SERVER_ERROR);
    		fprintf(stderr, "GET,%s,%d,%s\n", uri, response_get_code(&RESPONSE_INTERNAL_SERVER_ERROR), requestID);
        }
        reader_unlock(rwlock);
        return;
	}

	DIR *dir = opendir(uri);
    if (dir) {
    	closedir(dir);
    	close(fd);
        conn_send_response(conn, &RESPONSE_FORBIDDEN);
		fprintf(stderr, "GET,%s,%d,%s\n", uri, response_get_code(&RESPONSE_FORBIDDEN), requestID);
        return;
    }
    
	int contentLength = -1;
	contentLength = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	conn_send_file(conn, fd, contentLength);
	fprintf(stderr, "GET,%s,%d,%s\n", uri, response_get_code(&RESPONSE_OK), requestID);
	reader_unlock(rwlock);
	
    close(fd);

}


void handle_put(conn_t *conn,  server_t * server) {
    char *uri = conn_get_uri(conn);
	char template[] = "tmpnameXXXXXX";
	int tmp = 0;
	tmp = mkstemp(template);
	conn_recv_file(conn, tmp);
	close(tmp);
	
	//if(tmp == -1){
	//	fprintf(stderr, "error making temp file");
	//}
	
    char *requestID = conn_get_header(conn, "Request-Id");	
	if(requestID == NULL){
		requestID = "0";
	}
	
    pthread_mutex_t* mutex = server->mutex;
    
	pthread_mutex_lock(mutex);
    void* res = map_get(server->ht, uri);
    rwlock_t* rwlock;
    if(res == NULL){
    	//fprintf(stderr, "found-lock;");
    	rwlock = rwlock_new(N_WAY, 1);
    	map_set(server->ht, uri, rwlock);
    }else{
    	//fprintf(stderr, "created-lock;");
    	rwlock = (rwlock_t *)res;
    }
    pthread_mutex_unlock(mutex);
    
    bool fileExists = false;
    
    writer_lock(rwlock);
    //fprintf(stderr, "in-lock;");
    int fd2 = open(uri, O_RDONLY);
	if(fd2 >= 0){
		fileExists = true;
		remove(uri);
		//if(val != 0){
		//	fprintf(stderr, "error removing og");
		//}
	
	}
	close(fd2);
	
	
	//int fd = open(uri, O_WRONLY | O_TRUNC | O_CREAT, 0666);//
	//conn_recv_file(conn, fd);//
	rename(template, uri);

	if(fileExists){
		conn_send_response(conn, &RESPONSE_OK);
		fprintf(stderr, "PUT,%s,%d,%s\n", uri, response_get_code(&RESPONSE_OK), requestID);
		
	}else{
		conn_send_response(conn, &RESPONSE_CREATED);
		fprintf(stderr, "PUT,%s,%d,%s\n", uri, response_get_code(&RESPONSE_CREATED), requestID);
		
	}
	writer_unlock(rwlock);
    //close(fd);//
}

void handle_unsupported(conn_t *conn) {
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
}
