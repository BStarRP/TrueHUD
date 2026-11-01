#pragma once
#include "Widgets/ShoutIndicator.h"

namespace Scaleform
{
	class PlayerWidget : public ShoutIndicator
	{
	public:
		PlayerWidget(RE::GPtr<RE::GFxMovieView> a_view) :
			ShoutIndicator(a_view)
		{}

		virtual void Update(float a_deltaTime) override;
		virtual void Initialize() override;
		virtual void Dispose() override;

		virtual void FlashActorValue(RE::ActorValue a_actorValue, bool a_bLong);
		virtual void FlashSpecial(bool a_bLong);
		virtual void RefreshColors();
		virtual void UpdatePenaltyPercentages();
		virtual void UpdatePlayerWidgetChargeMeters(float a_percent, bool a_bForce, bool a_bLeftHand, bool a_bShow);
		virtual void SetPlayerNeeds(float a_hungerPct, float a_thirstPct, float a_energyPct, bool a_vampire, bool a_recovering);

	protected:
		virtual void UpdatePosition() override;
		virtual void LoadConfig() override;

		virtual void UpdateInfo();

		virtual void UpdatePenalty();

		virtual void ToggleVanillaBars(bool a_bEnable);
		virtual void ToggleVanillaEnchantmentChargeMeters(bool a_bEnable);

		void EnsureNeedBars();
		void CleanupNeedBars();
		bool AttachNeedBar(RE::GFxValue& a_out, const char* a_name);
		void InitNeedBar(RE::GFxValue& a_bar);
		void ColorNeedBar(RE::GFxValue& a_bar, uint32_t a_barColor, uint32_t a_phantomColor, uint32_t a_flashColor);
		void PlaceNeedBar(RE::GFxValue& a_bar, float a_x, float a_y);
		void WriteNeedBar(RE::GFxValue& a_bar, float a_pct, float& a_lastPct, float& a_showLeft);
		void LayoutNeedBars();
		void TickNeedBar(RE::GFxValue& a_bar, float a_pct, float& a_showLeft, float a_deltaTime, bool a_allow);
		void SetNeedAlpha(RE::GFxValue& a_bar, float a_alpha);
		void ApplyNeedData();
		void PeekBars();

		bool _bInitialized = false;
		float _healthPenaltyPct = 0.f;
		float _magickaPenaltyPct = 0.f;
		float _staminaPenaltyPct = 0.f;

		bool _bVanillaBarsHidden = false;
		bool _bVanillaEnchantmentChargeMetersHidden = false;

		RE::GFxValue _needHunger;
		RE::GFxValue _needThirst;
		RE::GFxValue _needEnergy;
		bool _needBarsReady = false;
		bool _needHaveData = false;
		bool _needAttachWarned = false;
		bool _needLayoutDirty = false;
		bool _needColorDirty = false;
		bool _needVampire = false;
		bool _needRecovering = false;
		float _needHungerPct = 0.f;
		float _needThirstPct = 0.f;
		float _needEnergyPct = 0.f;
		float _lastHungerPct = -1.f;
		float _lastThirstPct = -1.f;
		float _lastEnergyPct = -1.f;
		float _hungerShow = 0.f;
		float _thirstShow = 0.f;
		float _energyShow = 0.f;
	};
}
