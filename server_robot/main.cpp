#pragma execution_character_set("utf-8")
#define _CRT_SECURE_NO_WARNINGS
#include "xx_uv.h"
#include "xx_pos.h"
#include "xx_random.h"
#include "PKG_class.h"
#include "PKG_class.hpp"
#include "xx_random.hpp"
#include <random>
#include <tuple>

namespace std {
	template<>
	struct hash<std::tuple<int, int, int>> {
		std::size_t operator()(std::tuple<int, int, int> const& in) const noexcept {
			static_assert(sizeof(std::size_t) >= 8);
			return (std::size_t)std::get<0>(in) | ((std::size_t)std::get<2>(in) << 32);	// 1 是 cannonId 极少冲突
		}
	};
}

// todo: 增强断线重连或消息重发的兼容性。
// game_server 在连接上来之后，需要先汇报自己的 id，无脑信任。 id 与 peer 对应起来，如果 peer 对应已存在，则踢掉旧的, 换新的 peer
// 每个计算请求附带一个编号，该编号与 id peer 对应起来，每次收到请求后更新，如果收到重复的或更小的编号，则补发 cache 数据, 不再重复计算

struct Service {
	xx::Uv uv;
	xx::UvListener_s listener;
    xx::UvDialer_s dialer;
	xx::UvPeer_s peer;
    std::deque<xx::Object_s> recvs;
    
    // xx::UvResolver_s resolver;

	// 随机数发生器
	std::mt19937_64 rnd;

	// 最后将这两个字典的结果序列化返回
	xx::Dict<int, PKG::Calc::CatchFish::Hit const*> fishs;
	xx::Dict<std::tuple<int, int, int>, std::pair<int, int64_t>> bullets;
	PKG::Calc_CatchFish::HitCheckResult_s hitCheckResult;

	inline int OnReceiveRequest(xx::UvPeer_s const& peer, int const& serial, xx::Object_s&& msg) {
		// switch (msg->GetTypeId()) {
		// case xx::TypeId_v<PKG::CatchFish_Calc::Push>:
		// 	return HandleRequest(peer, serial, xx::As<PKG::CatchFish_Calc::Push>(msg));
		// case xx::TypeId_v<PKG::CatchFish_Calc::Pop>:
		// 	return HandleRequest(peer, serial, xx::As<PKG::CatchFish_Calc::Pop>(msg));
		// case xx::TypeId_v<PKG::CatchFish_Calc::HitCheck>:
		// 	return HandleRequest(peer, serial, xx::As<PKG::CatchFish_Calc::HitCheck>(msg));
		// 	// ...
		// 	// ...
		// default:
		// 	return -1;	// unknown package type received.
		// }
		return 0;
	}

	Service() {
        dialer = xx::Make<xx::UvDialer>(uv);
        dialer->onAccept = [this](xx::UvPeer_s peer) {
            this->peer = peer;
            if (peer) {
                peer->onReceivePush = [this](xx::Object_s && msg) {
                    this->recvs.push_back(std::move(msg));
                    return 0;
                };
            }
            OnDialerAccept();
            // finished = true;
        };
		// todo: fill totalInput & totalOutput
		// todo: win ratio config

		rnd.seed(std::random_device()());

		// xx::MakeTo(hitCheckResult);
		// xx::MakeTo(hitCheckResult->fishs);
		// xx::MakeTo(hitCheckResult->bullets);

		// xx::MakeTo(listener, uv, "127.1", 45621, 0);
		// listener->onAccept = [this](xx::UvPeer_s p) {
		// 	xx::CoutTN("accept: ", p->GetIP());
		// 	p->onDisconnect = [p] {
		// 		xx::CoutTN("disconnect: ", p->GetIP());
		// 	};
		// 	p->onReceiveRequest = [this, p](auto s, auto m) { return OnReceiveRequest(p, s, std::move(m)); };
		// };

	}

    inline static PKG::Client_CatchFish::Enter_s pkgEnter = xx::Make<PKG::Client_CatchFish::Enter>();
    inline void OnDialerAccept() {
        xx::Cout("OnDialerAccept");
        if (peer->SendPush(pkgEnter)) {
		
        }
        
    }

	inline void Run() {
		xx::CoutTN("Calc service running...");
		uv.Run();
	}
};

int main() {

	std::shared_ptr<Service> service;
	try {
		xx::MakeTo(service);
	}
	catch (int const& r) {
		xx::CoutTN("create Calc service throw exception: r = ", r);
	}
	service->Run();
	return 0;
}
