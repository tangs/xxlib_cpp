﻿#pragma execution_character_set("utf-8")
#define _CRT_SECURE_NO_WARNINGS
#include "xx_uv.h"
#include "xx_pos.h"
#include "xx_random.h"
#include "PKG_class.h"
#include "PKG_class.hpp"
#include "xx_random.hpp"

namespace std {
	template<>
	struct hash<std::pair<int, int>> {
		std::size_t operator()(std::pair<int, int> const& in) const noexcept {
			static_assert(sizeof(std::size_t) >= 8);
			return (std::size_t)in.first | ((std::size_t)in.second << 32);
		}
	};
}

struct Service {
	xx::Uv uv;
	xx::UvListener_s listener;

	// 计算服务至少具备 总输入 & 输出 两项持久化累加值, 以便根据胜率曲线配型, 决策输赢. 
	// 控制的目的是令实际输赢尽量贴近设计的曲线, 达到可控范围内的大起大落效果.
	// 就曲线图来讲, x 值为总输入, y 值为胜率. 需要配置这样一张配置表( 机器人有另外一套胜率表 )
	// 当总输入 > 总输出 / 胜率 时, 差值将对胜率起到放大作用. 反之则缩小.
	// 长时间后最终盈利 = (输入 - 输出) + (机器人总得 - 机器人总押)
	int64_t totalInput = 0;
	int64_t totalOutput = 0;
	xx::Random rnd;

	// 这里采用流式计算法, 省去两端组织 & 还原收发的数据. 每发生一次碰撞, 就产生一条 Hit 请求. 
	// 当一帧的收包 & 处理阶段结束后, 将产生的 Hit 队列打包发送给 Calc 服务计算.
	// Calc 依次处理, 以 fishId & playerId + bulletId 做 key, 逐个判断碰撞击打的成败. 
	// key: fishId, value: Hit* 如果鱼已死, 则放入该字典. 存当前 Hit 数据以标识打死该鱼的玩家 & 子弹
	// key: pair<playerId, bulletId>, value: bulletCount 如果子弹数量有富余( 鱼死了, 没用完 ), 就会创建一行记录来累加
	// 最后将这两个字典的结果序列化返回
	xx::Dict<int, PKG::Calc::CatchFish::Hit const*> fishs;
	xx::Dict<std::pair<int, int>, int> bullets;
	PKG::Calc_CatchFish::HitCheckResult_s hitCheckResult;

	// ...
	// ...

	inline int HandleRequest(xx::UvPeer_s const& peer, int const& serial, xx::Object_s&& msg) {
		switch (msg->GetTypeId()) {
		case xx::TypeId_v<PKG::CatchFish_Calc::HitCheck>:
			return HandleRequest_HitCheck(peer, serial, xx::As<PKG::CatchFish_Calc::HitCheck>(msg));
			// ...
			// ...
		default:
			return -1;	// unknown package type received.
		}
		return 0;
	}

	inline int HandleRequest_HitCheck(xx::UvPeer_s const& peer, int const& serial, PKG::CatchFish_Calc::HitCheck_s&& msg) {
		// 依次处理所有 hit
		for (auto&& hit : *msg->hits) {
			Handle_Hit(hit);
		}

		// 扫描 & 填充输出结果
		for (auto&& o : fishs) {
			auto&& t = hitCheckResult->fishs->Emplace();
			auto&& f = *o.value;
			t.fishId = f.fishId;
			t.playerId = f.playerId;
			t.bulletId = f.bulletId;
		}
		for (auto&& o : bullets) {
			//hitCheckResult->bullets
		}

		// 发送
		peer->SendResponse(serial, hitCheckResult);

		// 各式 cleanup
		hitCheckResult->fishs->Clear();
		hitCheckResult->bullets->Clear();
		fishs.Clear();
		bullets.Clear();
		return 0;
	}

	inline void Handle_Hit(PKG::Calc::CatchFish::Hit const& hit) {
		// 如果 fishId 已存在: 该 fish 已死, 子弹退回剩余次数
		if (fishs.Find(hit.fishId) != -1) {
			bullets[std::make_pair(hit.playerId, hit.bulletId)] += hit.bulletCount;
			return;
		}
		// 根据子弹数量来多次判定, 直到耗光或鱼死中断
		for (int i = 0; i < hit.bulletCount; ++i) {
			// 进行鱼死判定. 如果死掉：记录下来, 剩余子弹退回
			if (FishDieCheck(hit)) {
				fishs[hit.fishId] = &hit;
				// 如果没有剩余数量就不记录了
				if (auto && left = hit.bulletCount - i - 1) {	// - 1: 本次的扣除
					bullets[std::make_pair(hit.playerId, hit.bulletId)] += left;
				}
				return;
			}
			// else 子弹消耗掉了，鱼没死，啥都不记录
		}
	}

	// 忽略数量，进行一次鱼死判定
	inline bool FishDieCheck(PKG::Calc::CatchFish::Hit const& hit) {
		// todo: 根据胜率曲线配置加权随机
		assert(hit.fishCoin <= 0x7FFFFFFF);
		return rnd.Next((int)hit.fishCoin) == 0;
	}

	Service() {
		// todo: fill totalInput & totalOutput
		// todo: win ratio config

		xx::MakeTo(hitCheckResult);
		xx::MakeTo(hitCheckResult->fishs);
		xx::MakeTo(hitCheckResult->bullets);

		xx::MakeTo(listener, uv, "0.0.0.0", 12333);
		listener->onAccept = [this](auto p) {
			p->onReceiveRequest = [this, p](auto s, auto m) { return HandleRequest(p, s, std::move(m)); };
		};
		xx::CoutTN("service running...");
		uv.Run();
	}
};

int main() {
	try {
		Service service;
	}
	catch (int const& r) {
		xx::CoutTN("run service throw exception: r = ", r);
	}
	return 0;
}
