#ifndef poll_socket_epoll_h
#define poll_socket_epoll_h

#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

//判断是否无效
static bool 
sp_invalid(int efd) {
	return efd == -1;
}

//创建epoll文件描述符
static int
sp_create() {
	return epoll_create(1024);
}

//释放epoll文件描述符
static void
sp_release(int efd) {
	close(efd);
}

//把sock句柄放入efd监听
static int 
sp_add(int efd, int sock, void *ud) {
	struct epoll_event ev;
	//默认开启读，没有开启写
	ev.events = EPOLLIN;
	ev.data.ptr = ud;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &ev) == -1) { // 第四个参数是告诉内核需要监听什么事件
		return 1;
	}
	return 0;
}

//efd监听中删除传入的sock
static void 
sp_del(int efd, int sock) {
	epoll_ctl(efd, EPOLL_CTL_DEL, sock , NULL);
}

//efd监听中删除传入的sock
static void 
sp_write(int efd, int sock, void *ud, bool enable) {
	struct epoll_event ev;
	ev.events = EPOLLIN | (enable ? EPOLLOUT : 0);
	ev.data.ptr = ud;
	epoll_ctl(efd, EPOLL_CTL_MOD, sock, &ev);
}

//等待事件产生
static int 
sp_wait(int efd, struct event *e, int max) {
	// ev用于回传代处理事件的数组；
	struct epoll_event ev[max];
	// 等待事件触发，当超过timeout还没有事件触发时，就超时。该函数返回需要处理的事件数目，如返回0表示已超时。
	// timeout为等待I/O事件发生的超时值，-1相当于阻塞
	int n = epoll_wait(efd , ev, max, -1); 
	int i;
	//EPOLLIN：表示对应的文件描述符上有可读数据
	//EPOLLOUT：表示对应的文件描述符上可以写数据,譬如收到RST包
	//EPOLLHUP：表示对应的文件描述符被挂断
	//EPOLLERR：表示对应的文件描述符发生错误
	for (i=0;i<n;i++) {
		e[i].s = ev[i].data.ptr;
		unsigned flag = ev[i].events;
		e[i].write = (flag & EPOLLOUT) != 0;
		e[i].read = (flag & (EPOLLIN | EPOLLHUP)) != 0;
		e[i].error = (flag & EPOLLERR) != 0;
	}

	return n;
}

//IO为非阻塞
static void
sp_nonblocking(int fd) {
	int flag = fcntl(fd, F_GETFL, 0);
	if ( -1 == flag ) {
		return;
	}

	fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

#endif
