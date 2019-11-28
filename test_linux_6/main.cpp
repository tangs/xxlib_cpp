﻿#include <xx_epoll.h>
#include <xx_timeouter.h>

struct Epoll;
struct FDHandler : std::enable_shared_from_this<FDHandler> {
	virtual ~FDHandler() {/* 最外层派生类析构要写 this->Dispose(-1); */ }

	Epoll* ep = nullptr;
	int fd = -1;

	inline bool Disposed() const noexcept {
		return !ep;
	}

	// flag == -1: call by destructor.  0 : do not callback.  1: callback
	// return true: dispose success. false: already disposed.
	virtual bool Dispose(int const& flag = 1) noexcept;

	// return 非 0 表示自杀
	virtual int OnEpollEvent(uint32_t const& e) = 0;
};

struct TcpListener;
struct TcpPeer : FDHandler {
	std::weak_ptr<TcpListener> listener;

	std::string ip;

	// 收数据用堆积容器
	xx::List<uint8_t> recv;

	// 是否正在发送( 是：不管 sendQueue 空不空，都不能 write, 只能塞 sendQueue )
	bool writing = false;

	// 待发送队列
	std::optional<xx::BufQueue> sendQueue;

	// 每 fd 每一次可写, 写入的长度限制( 希望能实现当大量数据下发时各个 socket 公平的占用带宽 )
	std::size_t sendLenPerFrame = 65536;

	// 读缓冲区内存扩容增量
	std::size_t readBufLen = 65536;

	// writev 函数 (buf + len) 数组 参数允许的最大数组长度
	static const std::size_t maxNumIovecs = 1024;

	~TcpPeer() {
		this->Dispose(-1);
	}

	int Send(xx::Buf&& eb);
	int Flush();

protected:
	int Write();
	int Read(int const& fd);
};

struct UdpPeer : FDHandler {
	~UdpPeer() {
		this->Dispose(-1);
	}
};

struct TcpListener : FDHandler {
	~TcpListener() {
		this->Dispose(-1);
	}

	// 覆盖并提供创建 peer 对象的实现
	virtual std::shared_ptr<TcpPeer> OnCreatePeer() = 0;

	// 调用 accept
	virtual int OnEpollEvent(uint32_t const& e) override;

protected:
	// return fd. <0: error. 0: empty (EAGAIN / EWOULDBLOCK), > 0: fd
	int Accept(int const& listenFD);
};

struct TcpDialer : FDHandler {
	~TcpDialer() {
		this->Dispose(-1);
	}
};

struct Epoll {
	int efd = -1;
	int lastErrorNumber = 0;
	std::array<epoll_event, 4096> events;
	std::vector<std::shared_ptr<FDHandler>> fdHandlers;
	xx::TimeouterManager timeouterManager;

	Epoll(int const& maxNumFD = 65536) {
		efd = epoll_create1(0);
		if (-1 == efd) throw - 1;
		fdHandlers.resize(maxNumFD);
	}

	~Epoll() {
		if (efd != -1) {
			close(efd);
			efd = -1;
		}
	}

	// return fd. <0: error
	inline static int MakeListenFD(int const& port) {
		char portStr[20];
		snprintf(portStr, sizeof(portStr), "%d", port);

		addrinfo hints;														// todo: ipv6 support
		memset(&hints, 0, sizeof(addrinfo));
		hints.ai_family = AF_UNSPEC;										// ipv4 / 6
		hints.ai_socktype = SOCK_STREAM;									// SOCK_STREAM / SOCK_DGRAM
		hints.ai_flags = AI_PASSIVE;										// all interfaces

		addrinfo* ai_, * ai;
		if (getaddrinfo(nullptr, portStr, &hints, &ai_)) return -1;

		int fd;
		for (ai = ai_; ai != nullptr; ai = ai->ai_next) {
			fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
			if (fd == -1) continue;

			int enable = 1;
			if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
				close(fd);
				continue;
			}
			if (!bind(fd, ai->ai_addr, ai->ai_addrlen)) break;				// success

			close(fd);
		}
		freeaddrinfo(ai_);

		return ai ? fd : -2;
	}

	// return !0: error
	inline int Ctl(int const& fd, uint32_t const& flags, int const& op = EPOLL_CTL_ADD) {
		epoll_event event;
		event.data.fd = fd;
		event.events = flags;
		return epoll_ctl(efd, op, fd, &event);
	};

	// 添加监听器
	template<typename H, typename ...Args>
	inline std::shared_ptr<H> CreateTcpListener(int const& port, Args&&... args) {
		auto&& fd = MakeListenFD(port);
		if (fd < 0) return -2;

		xx::ScopeGuard sg([&] { close(fd); });
		if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK)) return -3;
		if (-1 == listen(fd, SOMAXCONN)) return -4;

		if (-1 == Ctl(fd, EPOLLIN)) return -5;

		auto h = xx::Make<H>(std::forward<Args>(args)...);
		h->ep = this;
		h->fd = fd;
		fdHandlers[fd] = std::move(h);

		sg.Cancel();
		return 0;
	}

	// 进入一次 epoll wait. 可传入超时时间. 
	inline int Wait(int const& timeoutMS) {
		int n = epoll_wait(efd, events.data(), events.size(), timeoutMS);
		if (n == -1) return errno;

		for (int i = 0; i < n; ++i) {
			auto fd = events[i].data.fd;
			auto e = events[i].events;
			auto&& h = fdHandlers[fd];
			//xx::CoutN(fd, " ", ev);

			if (e & EPOLLERR || e & EPOLLHUP) {
				if (h) h->Dispose();
				continue;
			}
			else {
				assert(h);
				if (h->OnEpollEvent(e)) {
					h->Dispose();
				}
			}
		}
		return 0;
	}

	// 开始运行并尽量维持在指定帧率. 临时拖慢将补帧
	inline int Run(double const& frameRate = 60.3) {
		assert(frameRate > 0);

		// 稳定帧回调用的时间池
		double ticksPool = 0;

		// 本次要 Wait 的超时时长
		int waitMS = 0;

		// 计算帧时间间隔
		auto ticksPerFrame = 10000000.0 / frameRate;

		// 取当前时间
		auto lastTicks = xx::NowEpoch10m();

		// 开始循环
		while (true) {

			// 计算上个循环到现在经历的时长, 并累加到 pool
			auto currTicks = xx::NowEpoch10m();
			auto elapsedTicks = (double)(currTicks - lastTicks);
			ticksPool += elapsedTicks;
			lastTicks = currTicks;

			// 如果累计时长跨度大于一帧的时长 则 Update
			if (ticksPool > ticksPerFrame) {

				// 消耗累计时长
				ticksPool -= ticksPerFrame;

				// 本次 Wait 不等待.
				waitMS = 0;

				// 驱动 timers
				timeouterManager.Update();

				// 帧逻辑调用一次
				//if (int r = Update()) return r;
			}
			else {
				// 计算等待时长
				waitMS = (int)((ticksPerFrame - elapsedTicks) / 10000.0);
			}

			// 调用一次 epoll wait. 
			if (int r = Wait(waitMS)) return r;
		}

		return 0;
	}

	// 帧逻辑可以放在这. 返回非 0 将令 Run 退出
	inline virtual int Update() {
		return 0;
	}
};

inline bool FDHandler::Dispose(int const& flag) noexcept {
	if (Disposed()) return false;
	if (fd != -1) {
		epoll_ctl(ep->efd, EPOLL_CTL_DEL, fd, nullptr);
		close(fd);
		ep->fdHandlers[fd].reset();
		fd = -1;
	}
	return true;
}


inline int TcpListener::OnEpollEvent(uint32_t const& e) {
	// accept 到 没有 或 出错 为止
	while (Accept(fd) >= 0) {};
	return 0;
}

inline int TcpListener::Accept(int const& listenFD) {
	sockaddr addr;									// todo: ipv6 support
	socklen_t len = sizeof(addr);

	// 接收并得到目标 fd
	int fd = accept(listenFD, &addr, &len);
	if (-1 == fd) {
		ep->lastErrorNumber = errno;
		if (ep->lastErrorNumber == EAGAIN || ep->lastErrorNumber == EWOULDBLOCK) return 0;
		else return -1;
	}

	// 确保退出时自动关闭 fd
	xx::ScopeGuard sg([&] { close(fd); });

	// 如果创建 socket 容器类失败则直接退出
	auto peer = OnCreatePeer();
	if (!peer) return 0;
	peer->listener = xx::As<TcpListener>(shared_from_this());

	// 继续初始化该 fd. 如果有异常则退出
	if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK)) return -2;
	int on = 1;
	if (-1 == setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on))) return -3;

	// 填充 ip
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	if (!getnameinfo(&addr, len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV)) {
		xx::Append(peer->ip, hbuf, ":", sbuf);
	}

	// 纳入 ep 管理
	if (-1 == ep->Ctl(fd, EPOLLIN)) return -6;
	ep->fdHandlers[fd] = std::move(peer);

	// 取消自动 close fd 行为
	sg.Cancel();
	return fd;
}


inline int TcpPeer::Send(xx::Buf&& eb) {
	sendQueue.value().Push(std::move(eb));
	return !writing ? Write() : 0;
}

inline int TcpPeer::Flush() {
	return !writing ? Write() : 0;
}

inline int TcpPeer::Write() {
	auto&& q = sendQueue.value();

	// 如果没有待发送数据，停止监控 EPOLLOUT 并退出
	if (!q.bytes) return ep->Ctl(fd, EPOLLIN, EPOLL_CTL_MOD);

	// 前置准备
	std::array<iovec, maxNumIovecs> vs;					// buf + len 数组指针
	int vsLen = 0;										// 数组长度
	auto bufLen = sendLenPerFrame;						// 计划发送字节数

	// 填充 vs, vsLen, bufLen 并返回预期 offset. 每次只发送 bufLen 长度
	auto&& offset = q.Fill(vs, vsLen, bufLen);

	// 返回值为 实际发出的字节数
	auto&& sentLen = writev(fd, vs.data(), vsLen);

	// 已断开
	if (sentLen == 0) return -2;

	// 繁忙 或 出错
	else if (sentLen == -1) {
		if (errno == EAGAIN) goto LabEnd;
		else return -3;
	}

	// 完整发送
	else if ((std::size_t)sentLen == bufLen) {
		// 快速弹出已发送数据
		q.Pop(vsLen, offset, bufLen);

		// 这次就写这么多了. 直接返回. 下次继续 Write
		return 0;
	}

	// 发送了一部分
	else {
		// 弹出已发送数据
		q.Pop(sentLen);
	}

LabEnd:
	// 标记为不可写
	writing = true;

	// 开启对可写状态的监控, 直到队列变空再关闭监控
	return ep->Ctl(fd, EPOLLIN | EPOLLOUT, EPOLL_CTL_MOD);
}

inline int TcpPeer::Read(int const& fd) {
	if (!recv.cap) {
		recv.Reserve(readBufLen);
	}
	if (recv.len == recv.cap) return -1;
	auto&& len = read(fd, recv.buf + recv.len, recv.cap - recv.len);
	if (len <= 0) return -2;
	recv.len += len;
	return 0;
}




struct Foo : xx::TimeouterBase {
	virtual void OnTimeout() override {
		xx::Cout("x");
	}
};

int main() {
	xx::TimeouterManager tm(8);

	auto foo = xx::Make<Foo>();
	foo->timerManager = &tm;
	foo->SetTimeout(5);

	auto foo2 = xx::Make<Foo>();
	foo2->timerManager = &tm;
	foo2->SetTimeout(5);

	for (int i = 0; i < 32; ++i) {
		tm.Update();
		xx::Cout(".");
	}
	xx::CoutN("end");
	return 0;
}