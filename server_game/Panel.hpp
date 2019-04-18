inline Panel::Panel(Dialer* dialer)
	: dialer(dialer) {
	btnAutoFire = cocos2d::Label::createWithSystemFont("", "", 32);
	btnAutoFire->setPosition(10 - ScreenCenter.x, 180 - ScreenCenter.y);
	btnAutoFire->setAnchorPoint({ 0, 0.5 });
	btnAutoFire->setGlobalZOrder(1000);
	cc_scene->addChild(btnAutoFire);
	SetText_AutoFire(dialer->autoFire);

	listenerAutoFire = cocos2d::EventListenerTouchOneByOne::create();
	listenerAutoFire->onTouchBegan = [this](cocos2d::Touch * t, cocos2d::Event * e) {
		auto&& tL = t->getLocation();
		auto&& p = btnAutoFire->convertToNodeSpace(tL);
		auto&& s = btnAutoFire->getContentSize();
		cocos2d::Rect r{ 0,0, s.width, s.height };
		auto&& b = r.containsPoint(p);
		if (b) {
			this->dialer->autoFire = !this->dialer->autoFire;
			SetText_AutoFire(this->dialer->autoFire);
		}
		return b;
	};
	cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listenerAutoFire, btnAutoFire);

	labelNumDialTimes = cocos2d::Label::createWithSystemFont("", "", 32);
	labelNumDialTimes->setPosition(10 - ScreenCenter.x, 150 - ScreenCenter.y);
	labelNumDialTimes->setAnchorPoint({ 0, 0.5 });
	labelNumDialTimes->setGlobalZOrder(1000);
	cc_scene->addChild(labelNumDialTimes);

	labelNumFishs = cocos2d::Label::createWithSystemFont("", "", 32);
	labelNumFishs->setPosition(10 - ScreenCenter.x, 120 - ScreenCenter.y);
	labelNumFishs->setAnchorPoint({ 0, 0.5 });
	labelNumFishs->setGlobalZOrder(1000);
	cc_scene->addChild(labelNumFishs);

	labelPing = cocos2d::Label::createWithSystemFont("", "", 32);
	labelPing->setPosition(10 - ScreenCenter.x, 90 - ScreenCenter.y);
	labelPing->setAnchorPoint({ 0, 0.5 });
	labelPing->setGlobalZOrder(1000);
	cc_scene->addChild(labelPing);
}

inline void Panel::SetText_AutoFire(bool const& value) noexcept {
	if (btnAutoFire) {
		std::string s = "auto fire: ";
		s += value ? "ON" : "OFF";
		btnAutoFire->setString(s);
	}
}
inline void Panel::SetText_NumDialTimes(int64_t const& value) noexcept {
	if (labelNumDialTimes) {
		labelNumDialTimes->setString("reconnect times: " + std::to_string(value));
	}
}
inline void Panel::SetText_Ping(int64_t const& value) noexcept {
	if (labelPing) {
		if (value < 0) {
			labelPing->setString("ping: timeout");
		}
		else {
			std::string s;
			xx::Append(s, "ping: ", value, "ms");
			labelPing->setString(s);
		}
	}
}
inline void Panel::SetText_NumFishs(size_t const& value) noexcept {
	if (labelNumFishs) {
		labelNumFishs->setString("num fishs: " + std::to_string(value));
	}
}
