#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

class $modify(BufferedPlayer, PlayerObject) {
	struct Fields {
		float sincePress = 9999.f;   // ms since last jump press
		bool pressPending = false;   // a press happened and hasn't been "used" yet
		bool wasOnGround = false;
		bool releaseNext = false;    // we injected a press, release it next tick
		bool injecting = false;      // guard so our own press isn't re-buffered
		bool fellAtPress = false;    // was the player falling when the press happened?
	};

	bool isFalling() {
		// m_yVelocity is positive upward; flip when gravity is inverted.
		return m_isUpsideDown ? (m_yVelocity > 0.0) : (m_yVelocity < 0.0);
	}

	bool pushButton(PlayerButton btn) {
		auto f = m_fields.self();
		if (btn == PlayerButton::Jump && !f->injecting && PlayLayer::get()) {
			f->sincePress = 0.f;
			f->pressPending = true;
			f->fellAtPress = this->isFalling();
		}
		return PlayerObject::pushButton(btn);
	}

	void update(float dt) {
		auto f = m_fields.self();
		auto pl = PlayLayer::get();

		// Only act inside a level, on a real (non-dead) player.
		if (!pl || !Mod::get()->getSettingValue<bool>("enabled")) {
			PlayerObject::update(dt);
			return;
		}

		// Finish a press we injected last tick.
		if (f->releaseNext) {
			f->releaseNext = false;
			PlayerObject::releaseButton(PlayerButton::Jump);
		}

		f->sincePress += dt * 1000.f;

		PlayerObject::update(dt);

		bool landedNow = m_isOnGround && !f->wasOnGround;
		f->wasOnGround = m_isOnGround;

		if (!f->pressPending) return;

		auto window = static_cast<float>(Mod::get()->getSettingValue<int64_t>("window-ms"));
		if (f->sincePress > window) {
			f->pressPending = false; // expired
			return;
		}

		if (landedNow) {
			bool needFalling = Mod::get()->getSettingValue<bool>("only-falling");
			f->pressPending = false;
			if (needFalling && !f->fellAtPress) return;

			// If the player is still holding, GD already handles the re-jump.
			if (m_isHolding) return;

			f->injecting = true;
			PlayerObject::pushButton(PlayerButton::Jump);
			f->injecting = false;
			f->releaseNext = true;
		}
	}
};
