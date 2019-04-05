﻿inline int Bullet::InitCascade(void* const& o) noexcept {
	scene = (Scene*)o;
	assert(player);
	assert(cannon);
	assert(cfg);

	// 计算炮口坐标
	pos = cannon->pos + xx::Rotate(xx::Pos{ cfg->muzzleLen ,0 }, cannon->angle * (float(M_PI) / 180.0f));

	// 角度沿用炮台的( 在发射前炮台已经调整了角度 )
	angle = cannon->angle;

	// 计算出每帧移动增量
	moveInc = xx::Rotate(xx::Pos{ cfg->distance ,0 }, angle * (float(M_PI) / 180.0f));

	// 存储发射时炮台的倍率
	coin = cannon->coin;

	if (int r = this->BaseType::InitCascade(o)) return r;
#ifdef CC_TARGET_PLATFORM
	DrawInit();
#endif
	return 0;
}

inline int Bullet::Update(int const& frameNumber) noexcept {
#ifndef CC_TARGET_PLATFORM
	pos += moveInc;

	// 先简单实现飞出屏幕就消失	// todo: 反弹
	auto&& w = ::ScreenCenter.x + cfg->maxRadius;
	auto&& h = ::ScreenCenter.y + cfg->maxRadius;
	if (pos.x > w || pos.x < -w || pos.y > h || pos.y < -h) return -1;
#else

	// 遍历所有鱼
	auto&& fs = *scene->fishs;
	if (fs.len) {
		for (size_t i = fs.len - 1; i != -1; --i) {
			// 命中检查
			if (xx::As<Fish>(fs[i])->HitCheck(this)) {
				// todo: 发包

				//// 删鱼
				//fs[fs.len - 1]->indexAtContainer = (int)i;
				//fs.SwapRemoveAt(i);
				// todo: 播放子弹爆炸特效?
				return -1;
			}
		}
	}

	DrawUpdate();
#endif
	return 0;
};

inline Bullet::~Bullet() {
#ifdef CC_TARGET_PLATFORM
	DrawDispose();
#endif
}


#ifdef CC_TARGET_PLATFORM
inline void Bullet::DrawInit() noexcept {
	assert(!body);
	body = cocos2d::Sprite::create();
	body->retain();
	body->setGlobalZOrder(cfg->zOrder);
	auto&& sf = xx::As<SpriteFrame>(cfg->frames->At(1))->spriteFrame;
	body->setSpriteFrame(sf);
	body->setPosition(pos);
	body->setScale(cfg->scale);
	body->setRotation(-angle);
	cc_scene->addChild(body);
}

inline void Bullet::DrawUpdate() noexcept {
	assert(body);
	body->setRotation(-angle);
	body->setPosition(pos);
}

inline void Bullet::DrawDispose() noexcept {
	if (!body) return;

	if (body->getParent()) {
		body->removeFromParent();
	}
	body->release();
	body = nullptr;
}
#endif
