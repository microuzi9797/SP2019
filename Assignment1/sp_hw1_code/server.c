#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>

#define ERR_EXIT(a) { perror(a); exit(1); }

typedef struct {
	char hostname[512];  // server's hostname
	unsigned short port;  // port to listen
	int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
	char host[512];  // client's host
	int conn_fd;  // fd to talk with client
	char buf[512];  // data sent by/to client
	size_t buf_len;  // bytes used by buf
	// you don't need to change this.
	int item;
	int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

typedef struct {
	int id;
	int balance;
} Account;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";

// Forwards

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

static int handle_read(request* reqP);
// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error

int main(int argc, char** argv) {
	int i, ret;

	struct sockaddr_in cliaddr;  // used by accept()
	int clilen;

	int conn_fd;  // fd for a new connection with client
	int file_fd;  // fd for file that we open for reading
	char buf[512];
	int buf_len;

	// Parse args.
	if (argc != 2) {
		fprintf(stderr, "usage: %s [port]\n", argv[0]);
		exit(1);
	}

	// Initialize server
	init_server((unsigned short) atoi(argv[1]));

	// Get file descripter table size and initize request table
	maxfd = getdtablesize();
	requestP = (request*) malloc(sizeof(request) * maxfd);
	if (requestP == NULL) {
		ERR_EXIT("out of memory allocating all requests");
	}
	for (i = 0; i < maxfd; i++) {
		init_request(&requestP[i]);
	}
	requestP[svr.listen_fd].conn_fd = svr.listen_fd;
	strcpy(requestP[svr.listen_fd].host, svr.hostname);

	// Loop for handling connections
	fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

	// Initialize
	file_fd = open("account_list", O_RDWR);
	int account_index;
	fd_set readset;
	fd_set workset;
	FD_ZERO(&readset);
	FD_ZERO(&workset);
	FD_SET(svr.listen_fd, &readset);
	FD_SET(svr.listen_fd, &workset);
	if(maxfd > FD_SETSIZE)
	{
		maxfd = FD_SETSIZE;
	}
	struct flock lock[maxfd];
	int alreadylock[20] = {0};

	while (1) {
		// TODO: Add IO multiplexing
		readset = workset;
		int totalFds = select(maxfd, &readset, NULL, NULL, NULL);
		if(totalFds > 0)
		{
			for(int i = 0;i < maxfd;i++)
			{
				// Check new connection
				if(FD_ISSET(i, &readset) && i == svr.listen_fd)
				{
					clilen = sizeof(cliaddr);
					conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
					if (conn_fd < 0) {
						if (errno == EINTR || errno == EAGAIN) continue;  // try again
						if (errno == ENFILE) {
							(void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
							continue;
						}
						ERR_EXIT("accept")
					}
					requestP[conn_fd].conn_fd = conn_fd;
					strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
					fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);

					FD_SET(conn_fd, &workset);
				}
				else if(FD_ISSET(i, &readset) && i != svr.listen_fd)
				{
					ret = handle_read(&requestP[i]); // parse data from client to requestP[conn_fd].buf
					if (ret < 0) {
						fprintf(stderr, "bad request from %s\n", requestP[i].host);
						continue;
					}
#ifdef READ_SERVER
					// check if there is anyone writing this account
					account_index = atoi(requestP[i].buf) - 1;
					lock[i].l_type = F_RDLCK;
					lock[i].l_start = sizeof(Account) * account_index;
					lock[i].l_whence = SEEK_SET;
					lock[i].l_len = sizeof(Account);
					if(fcntl(file_fd, F_SETLK, &lock[i]) == -1 || alreadylock[account_index] == 1)
					{
						sprintf(buf, "This account is locked.\n");
					}
					else
					{
						Account read_account;
						lseek(file_fd, sizeof(Account) * account_index, SEEK_SET);
						read(file_fd, &read_account, sizeof(Account));
						sprintf(buf, "%d %d\n", read_account.id, read_account.balance);
					}
					write(requestP[i].conn_fd, buf, strlen(buf));
					// close the connection
					lock[i].l_type = F_UNLCK;
					fcntl(file_fd, F_SETLK, &lock[i]);
					FD_CLR(i, &workset);
					close(requestP[i].conn_fd);
					free_request(&requestP[i]);
#else
					// check if the client is ready to do some action
					if(requestP[i].wait_for_write != 1)
					{
						account_index = atoi(requestP[i].buf) - 1;
						// two steps, so we need to store account_index in another place
						requestP[i].item = account_index;
						lock[i].l_type = F_WRLCK;
						lock[i].l_start = sizeof(Account) * requestP[i].item;
						lock[i].l_whence = SEEK_SET;
						lock[i].l_len = sizeof(Account);
						if(fcntl(file_fd, F_SETLK, &lock[i]) == -1 || alreadylock[account_index] == 1)
						{
							sprintf(buf, "This account is locked.\n");
							write(requestP[i].conn_fd, buf, strlen(buf));
							// close the connection
							FD_CLR(i, &workset);
							close(requestP[i].conn_fd);
							free_request(&requestP[i]);
						}
						else
						{
							sprintf(buf, "This account is modifiable.\n");
							write(requestP[i].conn_fd, buf, strlen(buf));
							requestP[i].wait_for_write = 1;
							alreadylock[account_index] = 1;
						}
					}
					else if(requestP[i].wait_for_write == 1)
					{
						Account write_account;
						lseek(file_fd, sizeof(Account) * requestP[i].item, SEEK_SET);
						read(file_fd, &write_account, sizeof(Account));
						// which action to do
						char *action = strtok(requestP[i].buf, " ");
						if(strcmp(action, "save") == 0)
						{
							char *num = strtok(NULL, " ");
							int number = atoi(num);
							if(number < 0)
							{
								sprintf(buf, "Operation failed.\n");
								write(requestP[i].conn_fd, buf, strlen(buf));
							}
							else
							{
								write_account.balance += number;
								lseek(file_fd, sizeof(Account) * requestP[i].item, SEEK_SET);
								write(file_fd, &write_account, sizeof(Account));
							}
						}
						else if(strcmp(action, "withdraw") == 0)
						{
							char *num = strtok(NULL, " ");
							int number = atoi(num);
							if(number < 0 || number > write_account.balance)
							{
								sprintf(buf, "Operation failed.\n");
								write(requestP[i].conn_fd, buf, strlen(buf));
							}
							else
							{
								write_account.balance -= number;
								lseek(file_fd, sizeof(Account) * requestP[i].item, SEEK_SET);
								write(file_fd, &write_account, sizeof(Account));
							}
						}
						else if(strcmp(action, "transfer") == 0)
						{
							char *goal_account = strtok(NULL, " ");
							char *num = strtok(NULL, " ");
							int goal_account_id = atoi(goal_account);
							int number = atoi(num);
							if(number < 0)
							{
								sprintf(buf, "Operation failed.\n");
								write(requestP[i].conn_fd, buf, strlen(buf));
							}
							else
							{
								write_account.balance -= number;
								lseek(file_fd, sizeof(Account) * requestP[i].item, SEEK_SET);
								write(file_fd, &write_account, sizeof(Account));
								// save money to another account
								lseek(file_fd, sizeof(Account) * (goal_account_id - 1), SEEK_SET);
								read(file_fd, &write_account, sizeof(Account));
								write_account.balance += number;
								lseek(file_fd, sizeof(Account) * (goal_account_id - 1), SEEK_SET);
								write(file_fd, &write_account, sizeof(Account));
							}
						}
						else if(strcmp(action, "balance") == 0)
						{
							char *num = strtok(NULL, " ");
							int number = atoi(num);
							if(number < 0)
							{
								sprintf(buf, "Operation failed.\n");
								write(requestP[i].conn_fd, buf, strlen(buf));
							}
							else
							{
								write_account.balance = number;
								lseek(file_fd, sizeof(Account) * requestP[i].item, SEEK_SET);
								write(file_fd, &write_account, sizeof(Account));
							}
						}
						// close the connection
						requestP[i].wait_for_write = 0;
						lock[i].l_type = F_UNLCK;
						fcntl(file_fd, F_SETLK, &lock[i]);
						alreadylock[requestP[i].item] = 0;
						FD_CLR(i, &workset);
						close(requestP[i].conn_fd);
						free_request(&requestP[i]);
					}
#endif
				}
			}
		}
	}
	close(file_fd);
	free(requestP);
	return 0;
}


// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void* e_malloc(size_t size);


static void init_request(request* reqP) {
	reqP->conn_fd = -1;
	reqP->buf_len = 0;
	reqP->item = 0;
	reqP->wait_for_write = 0;
}

static void free_request(request* reqP) {
	/*if (reqP->filename != NULL) {
		free(reqP->filename);
		reqP->filename = NULL;
	}*/
	init_request(reqP);
}

// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error
static int handle_read(request* reqP) {
	int r;
	char buf[512];

	// Read in request from client
	r = read(reqP->conn_fd, buf, sizeof(buf));
	if (r < 0) return -1;
	if (r == 0) return 0;
	char* p1 = strstr(buf, "\015\012");
	int newline_len = 2;
	// be careful that in Windows, line ends with \015\012
	if (p1 == NULL) {
		p1 = strstr(buf, "\012");
		newline_len = 1;
		if (p1 == NULL) {
			ERR_EXIT("this really should not happen...");
		}
	}
	size_t len = p1 - buf + 1;
	memmove(reqP->buf, buf, len);
	reqP->buf[len - 1] = '\0';
	reqP->buf_len = len-1;
	return 1;
}

static void init_server(unsigned short port) {
	struct sockaddr_in servaddr;
	int tmp;

	gethostname(svr.hostname, sizeof(svr.hostname));
	svr.port = port;

	svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (svr.listen_fd < 0) ERR_EXIT("socket");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);
	tmp = 1;
	if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
		ERR_EXIT("setsockopt");
	}
	if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		ERR_EXIT("bind");
	}
	if (listen(svr.listen_fd, 1024) < 0) {
		ERR_EXIT("listen");
	}
}

static void* e_malloc(size_t size) {
	void* ptr;

	ptr = malloc(size);
	if (ptr == NULL) ERR_EXIT("out of memory");
	return ptr;
}

