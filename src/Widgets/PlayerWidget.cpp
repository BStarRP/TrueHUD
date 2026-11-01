#include "Widgets/PlayerWidget.h"
#include "Offsets.h"
#include "Settings.h"
#include "Utils.h"
#include "HUDHandler.h"

#include <algorithm>
#include <cmath>

namespace Scaleform
{
	void PlayerWidget::Update(float a_deltaTime)
	{
		if (!_bInitialized) {
			RE::GFxValue result;
			_object.Invoke("isInitialized", &result);
			if (result.GetBool()) {
				_bInitialized = true;
				UpdatePenalty();
				ApplyNeedData();
				PeekBars();
			}
		} else if (_needHaveData && !_needBarsReady) {
			ApplyNeedData();
		}

		UpdateInfo();
		if (_needBarsReady) {
			TickNeedBar(_needHunger, _needHungerPct, _hungerShow, a_deltaTime, true);
			TickNeedBar(_needThirst, _needThirstPct, _thirstShow, a_deltaTime, !_needVampire);
			TickNeedBar(_needEnergy, _needEnergyPct, _energyShow, a_deltaTime, true);
		}

		if (Settings::bPlayerWidgetHideVanillaBars && Settings::bPlayerWidgetForceHideVanillaBars) {
			ToggleVanillaBars(false);
		}

		if (Settings::bPlayerWidgetDisplayEnchantmentChargeMeter && Settings::bPlayerWidgetForceHideVanillaBars) {
			ToggleVanillaEnchantmentChargeMeters(false);
		}
	}

	void PlayerWidget::Initialize()
	{
		LoadConfig();

		if (Settings::bPlayerWidgetHideVanillaBars) {
			ToggleVanillaBars(false);
		} else if (_bVanillaBarsHidden) {
			ToggleVanillaBars(true);
		}

		if (Settings::bPlayerWidgetDisplayEnchantmentChargeMeter) {
			ToggleVanillaEnchantmentChargeMeters(false);
		} else if (_bVanillaEnchantmentChargeMetersHidden) {
			ToggleVanillaEnchantmentChargeMeters(true);
		}

		UpdatePosition();
	}

	void PlayerWidget::Dispose()
	{
		if (_bVanillaBarsHidden) {
			ToggleVanillaBars(true);
		}

		if (_bVanillaEnchantmentChargeMetersHidden) {
			ToggleVanillaEnchantmentChargeMeters(true);
		}

		CleanupNeedBars();
		_object.Invoke("cleanUp", nullptr, nullptr, 0);
	}

	void PlayerWidget::FlashActorValue(RE::ActorValue a_actorValue, bool a_bLong)
	{
		RE::GFxValue args[2];
		args[0].SetNumber(static_cast<int32_t>(a_actorValue));
		args[1].SetBoolean(a_bLong);

		_object.Invoke("flashAV", nullptr, args, 2);
	}

	void PlayerWidget::FlashSpecial(bool a_bLong)
	{
		RE::GFxValue arg;
		arg.SetBoolean(a_bLong);

		_object.Invoke("flashSpecial", nullptr, &arg, 1);
	}

	void PlayerWidget::RefreshColors()
	{
		using ColorType = TRUEHUD_API::BarColorType;
		
		auto hudMenu = HUDHandler::GetTrueHUDMenu();
		auto playerHandle = RE::PlayerCharacter::GetSingleton()->GetHandle();

		RE::GFxValue args[20];
		args[0].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::BarColor));
		args[1].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::PhantomColor));
		args[2].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::BackgroundColor));
		args[3].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::PenaltyColor));
		args[4].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::FlashColor));
		args[5].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::BarColor));
		args[6].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::PhantomColor));
		args[7].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::BackgroundColor));
		args[8].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::PenaltyColor));
		args[9].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::FlashColor));
		args[10].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::BarColor));
		args[11].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::PhantomColor));
		args[12].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::BackgroundColor));
		args[13].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::PenaltyColor));
		args[14].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::FlashColor));
		args[15].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::BarColor));
		args[16].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::PhantomColor));
		args[17].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::BackgroundColor));
		args[18].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::PenaltyColor));
		args[19].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::FlashColor));
		_object.Invoke("setColors", nullptr, args, 20);
	}

	void PlayerWidget::UpdatePenaltyPercentages()
	{
		if (Settings::glob_survivalModeEnabled &&
			Settings::glob_survivalHealthPenaltyPercent &&
			Settings::glob_survivalMagickaPenaltyPercent &&
			Settings::glob_survivalStaminaPenaltyPercent)
		{
			if (Settings::glob_survivalModeEnabled->value == 1) {
				_healthPenaltyPct = Settings::glob_survivalHealthPenaltyPercent->value / 100.f;
				_magickaPenaltyPct = Settings::glob_survivalMagickaPenaltyPercent->value / 100.f;
				_staminaPenaltyPct = Settings::glob_survivalStaminaPenaltyPercent->value / 100.f;
			} else {
				_healthPenaltyPct = 0.f;
				_magickaPenaltyPct = 0.f;
				_staminaPenaltyPct = 0.f;
			}
			UpdatePenalty();
		}
	}

	void PlayerWidget::UpdatePlayerWidgetChargeMeters(float a_percent, bool a_bForce, bool a_bLeftHand, bool a_bShow)
	{
		RE::GFxValue args[4];
		args[0].SetNumber(a_percent);
		args[1].SetBoolean(a_bForce);
		args[2].SetBoolean(a_bLeftHand);
		args[3].SetBoolean(a_bShow);
		_object.Invoke("updateEnchantmentChargeMeter", nullptr, args, 4);
	}

	void PlayerWidget::UpdatePosition()
	{
		RE::GFxValue::DisplayInfo displayInfo;
		float scale = Settings::fPlayerWidgetScale * 100.f;
		displayInfo.SetScale(scale, scale);

		if (Settings::bPlayerWidgetCombined) {
			RE::GRectF rect = _view->GetVisibleFrameRect();

			RE::NiPoint2 screenPos;

			auto def = _view->GetMovieDef();
			if (def) {
				screenPos.x = def->GetWidth();
				screenPos.y = def->GetHeight();
			}

			screenPos.x *= Settings::fPlayerWidgetX;
			screenPos.y *= Settings::fPlayerWidgetY;

			displayInfo.SetPosition(screenPos.x, screenPos.y);
		} else {
			displayInfo.SetPosition(0, 0);
		}
		
		_object.SetDisplayInfo(displayInfo);
		LayoutNeedBars();
	}

	void PlayerWidget::LoadConfig()
	{
		using ColorType = TRUEHUD_API::BarColorType;

		auto hudHandler = HUDHandler::GetSingleton();
		auto hudMenu = HUDHandler::GetTrueHUDMenu();

		auto playerHandle = RE::PlayerCharacter::GetSingleton()->GetHandle();

		RE::NiPoint2 stageSize;
		auto def = _view->GetMovieDef();
		if (def) {
			stageSize.x = def->GetWidth();
			stageSize.y = def->GetHeight();
		}

		RE::GFxValue args[54];
		args[0].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetHealthBarDirection));
		args[1].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetMagickaBarDirection));
		args[2].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetStaminaBarDirection));
		args[3].SetBoolean(Settings::bPlayerWidgetDisplayPhantomBars);
		args[4].SetBoolean(Settings::bPlayerWidgetDisplaySpecialPhantomBar);
		args[5].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetHealthMode));
		args[6].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetMagickaMode));
		args[7].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetStaminaMode));
		args[8].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetShoutIndicatorMode));
		args[9].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetSpecialMode));
		args[10].SetBoolean(hudHandler->bSpecialMode);
		args[11].SetBoolean(Settings::bPlayerWidgetDisplayEnchantmentChargeMeter);
		args[12].SetNumber(Settings::fPlayerWidgetPhantomDuration);
		args[13].SetNumber(Settings::fPlayerWidgetScale);
		args[14].SetNumber((Settings::bPlayerWidgetUseHUDOpacity ? *g_fHUDOpacity : Settings::fPlayerWidgetOpacity) * 100.f);
		args[15].SetNumber(Settings::fPlayerWidgetBarWidth);
		args[16].SetNumber(Settings::fPlayerWidgetSpecialBarWidth);
		args[17].SetBoolean(Settings::bPlayerWidgetScaleBars);
		args[18].SetNumber(Settings::fPlayerWidgetMinBarWidth);
		args[19].SetNumber(Settings::fPlayerWidgetMaxBarWidth);
		args[20].SetNumber(Settings::fPlayerWidgetHealthBarScaleMult);
		args[21].SetNumber(Settings::fPlayerWidgetMagickaBarScaleMult);
		args[22].SetNumber(Settings::fPlayerWidgetStaminaBarScaleMult);
		args[23].SetNumber(stageSize.x * Settings::fPlayerWidgetX);
		args[24].SetNumber(stageSize.y * Settings::fPlayerWidgetY);
		args[25].SetNumber(stageSize.x * Settings::fPlayerWidgetEnchantmentChargeMeterX);
		args[26].SetNumber(stageSize.y * Settings::fPlayerWidgetEnchantmentChargeMeterY);
		args[27].SetNumber(Settings::bPlayerWidgetCombined);
		args[28].SetNumber(stageSize.x * Settings::fPlayerWidgetHealthX);
		args[29].SetNumber(stageSize.y * Settings::fPlayerWidgetHealthY);
		args[30].SetNumber(stageSize.x * Settings::fPlayerWidgetMagickaX);
		args[31].SetNumber(stageSize.y * Settings::fPlayerWidgetMagickaY);
		args[32].SetNumber(stageSize.x * Settings::fPlayerWidgetStaminaX);
		args[33].SetNumber(stageSize.y * Settings::fPlayerWidgetStaminaY);
		args[34].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::BarColor));
		args[35].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::PhantomColor));
		args[36].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::BackgroundColor));
		args[37].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::PenaltyColor));
		args[38].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::FlashColor));
		args[39].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::BarColor));
		args[40].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::PhantomColor));
		args[41].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::BackgroundColor));
		args[42].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::PenaltyColor));
		args[43].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::FlashColor));
		args[44].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::BarColor));
		args[45].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::PhantomColor));
		args[46].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::BackgroundColor));
		args[47].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::PenaltyColor));
		args[48].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::FlashColor));
		args[49].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::BarColor));
		args[50].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::PhantomColor));
		args[51].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::BackgroundColor));
		args[52].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::PenaltyColor));
		args[53].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::FlashColor));
		_object.Invoke("loadConfig", nullptr, args, 54);
	}

	void PlayerWidget::UpdateInfo()
	{
		SetPercent(GetPercent());
		UpdatePenaltyPercentages();

		auto playerCharacter = RE::PlayerCharacter::GetSingleton();
		float permanentHealth, temporaryHealth, maxHealth, health, permanentMagicka, temporaryMagicka, maxMagicka, magicka, permanentStamina, temporaryStamina, maxStamina, stamina;

		auto actorValueOwner = playerCharacter->AsActorValueOwner();

		permanentHealth = actorValueOwner->GetPermanentActorValue(RE::ActorValue::kHealth);
		temporaryHealth = playerCharacter->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kHealth);
		maxHealth = permanentHealth + temporaryHealth;
		maxHealth /= (1.f - _healthPenaltyPct);
		if (playerCharacter->IsDead()) {
			health = 0;
		} else {
			health = actorValueOwner->GetActorValue(RE::ActorValue::kHealth);
		}

		permanentMagicka = actorValueOwner->GetPermanentActorValue(RE::ActorValue::kMagicka);
		temporaryMagicka = playerCharacter->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kMagicka);
		maxMagicka = permanentMagicka + temporaryMagicka;
		maxMagicka /= (1.f - _magickaPenaltyPct);
		magicka = actorValueOwner->GetActorValue(RE::ActorValue::kMagicka);

		RE::ActorPtr mount;
		bool bIsInMountMode = false;
		if (Settings::bPlayerWidgetDisplayMountStamina && playerCharacter->GetMount(mount)) {
			auto mountActorValueOwner = mount->AsActorValueOwner();

			permanentStamina = mountActorValueOwner->GetPermanentActorValue(RE::ActorValue::kStamina);
			temporaryStamina = mount->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kStamina);
			maxStamina = permanentStamina + temporaryStamina;
			stamina = mountActorValueOwner->GetActorValue(RE::ActorValue::kStamina);
			bIsInMountMode = true;
		} else {
			permanentStamina = actorValueOwner->GetPermanentActorValue(RE::ActorValue::kStamina);
			temporaryStamina = playerCharacter->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kStamina);
			maxStamina = permanentStamina + temporaryStamina;
			maxStamina /= (1.f - _staminaPenaltyPct);
			stamina = actorValueOwner->GetActorValue(RE::ActorValue::kStamina);
			bIsInMountMode = false;
		}

		bool bIsInCombat = playerCharacter->IsInCombat();

		RE::GFxValue args[12];
		args[0].SetNumber(ceil(maxHealth));
		args[1].SetNumber(ceil(health));
		args[2].SetNumber(Utils::GetPct(health, maxHealth));
		args[3].SetNumber(ceil(maxMagicka));
		args[4].SetNumber(ceil(magicka));
		args[5].SetNumber(Utils::GetPct(magicka, maxMagicka));
		args[6].SetNumber(ceil(maxStamina));
		args[7].SetNumber(ceil(stamina));
		args[8].SetNumber(Utils::GetPct(stamina, maxStamina));
		args[9].SetBoolean(bIsInCombat);
		args[10].SetBoolean(bIsInMountMode);
		args[11].SetBoolean(false);

		_object.Invoke("updateData", nullptr, args, 12);

		auto hudHandler = HUDHandler::GetSingleton();
		if (hudHandler && hudHandler->bSpecialFunctionsProvided && hudHandler->bDisplaySpecialForPlayer) {
			RE::GFxValue specialArgs[3];
			auto maxSpecial = fmax(hudHandler->GetMaxSpecial(playerCharacter), 0.f);
			auto currentSpecial = fmax(hudHandler->GetCurrentSpecial(playerCharacter), 0.f);

			specialArgs[0].SetNumber(maxSpecial);
			specialArgs[1].SetNumber(currentSpecial);
			specialArgs[2].SetNumber(Utils::GetPct(currentSpecial, maxSpecial));
			_object.Invoke("updateSpecial", nullptr, specialArgs, 3);

			const bool belowDefault = hudHandler->bSpecialMode
				? currentSpecial + 0.01f < maxSpecial
				: currentSpecial > 0.01f;
			if (belowDefault) {
				RE::GFxValue show;
				show.SetBoolean(true);
				_object.Invoke("showSpecial", nullptr, &show, 1);
			}
		}
	}

	void PlayerWidget::UpdatePenalty()
	{
		RE::GFxValue penaltyArgs[4];

		penaltyArgs[0].SetNumber(_healthPenaltyPct);
		penaltyArgs[1].SetNumber(_magickaPenaltyPct);
		penaltyArgs[2].SetNumber(_staminaPenaltyPct);
		penaltyArgs[3].SetBoolean(false);

		_object.Invoke("updatePenalty", nullptr, penaltyArgs, 4);
	}

	void PlayerWidget::ToggleVanillaBars(bool a_bEnable)
	{
		auto hud = RE::UI::GetSingleton()->GetMenu(RE::HUDMenu::MENU_NAME);
		if (hud) {
			auto& uiMovie = hud->uiMovie;
			if (uiMovie) {
				RE::GFxValue bEnable;
				bEnable.SetBoolean(a_bEnable);
				/*uiMovie->SetVariable("HUDMovieBaseInstance.Health._visible", bEnable);
			uiMovie->SetVariable("HUDMovieBaseInstance.Magica._visible", bEnable);
			uiMovie->SetVariable("HUDMovieBaseInstance.Stamina._visible", bEnable);*/
				uiMovie->SetVariable("HUDMovieBaseInstance.Health.HealthMeter_mc._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Health.HealthPenaltyMeter_mc._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Magica.MagickaMeter_mc._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Magica.MagickaFlashInstance._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Magica.MagickaPenaltyMeter_mc._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Stamina.StaminaMeter_mc._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Stamina.StaminaFlashInstance._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Stamina.StaminaPenaltyMeter_mc._visible", bEnable);

				_bVanillaBarsHidden = !a_bEnable;
			}
		}
	}

	void PlayerWidget::ToggleVanillaEnchantmentChargeMeters(bool a_bEnable)
	{
		auto hud = RE::UI::GetSingleton()->GetMenu(RE::HUDMenu::MENU_NAME);
		if (hud) {
			auto& uiMovie = hud->uiMovie;
			if (uiMovie) {
				RE::GFxValue bEnable;
				bEnable.SetBoolean(a_bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.BottomLeftLockInstance._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.BottomRightLockInstance._visible", bEnable);

				_bVanillaEnchantmentChargeMetersHidden = !a_bEnable;
			}
		}			
	}

	namespace
	{
		constexpr float kNeedBarWidth = 148.f;
		constexpr float kNeedBarGap = 100.f;
		constexpr float kNeedInward = 0.5f;
		constexpr float kNeedMinGap = kNeedBarWidth + 36.f;
		constexpr float kNeedAboveHealth = 40.f;
		constexpr float kNeedShowSec = 15.f;
		constexpr float kNeedFadeSec = 1.f;
		constexpr float kNeedPinAt = 15.f;
		constexpr float kNeedCritical = 15.f;
		constexpr float kNeedDrop = 0.5f;
		constexpr uint32_t kNeedBg = 0x000000;
		constexpr uint32_t kNeedPenalty = 0x000000;
		constexpr uint32_t kHungerBar = 0xE8A428;
		constexpr uint32_t kHungerPhantom = 0x8A5A10;
		constexpr uint32_t kHungerFlash = 0xFFC040;
		constexpr uint32_t kThirstBar = 0x62C0F4;
		constexpr uint32_t kThirstPhantom = 0x2A6A90;
		constexpr uint32_t kThirstFlash = 0x9ADBFF;
		constexpr uint32_t kBloodBar = 0xA81C26;
		constexpr uint32_t kBloodPhantom = 0x5C0F15;
		constexpr uint32_t kBloodFlash = 0xE04050;
		constexpr uint32_t kEnergyBar = 0x824BB9;
		constexpr uint32_t kEnergyPhantom = 0x462866;
		constexpr uint32_t kEnergyFlash = 0xB070E0;
		constexpr uint32_t kEnergyRecoverBar = 0x7A7066;
		constexpr uint32_t kEnergyRecoverPhantom = 0x3E3A36;
		constexpr uint32_t kEnergyRecoverFlash = 0xA09080;

		float NeedOpacity()
		{
			const float opacity = Settings::bPlayerWidgetUseHUDOpacity && g_fHUDOpacity
				? *g_fHUDOpacity
				: Settings::fPlayerWidgetOpacity;
			return opacity * 100.f;
		}
	}

	void PlayerWidget::SetPlayerNeeds(float a_hungerPct, float a_thirstPct, float a_energyPct, bool a_vampire, bool a_recovering)
	{
		_needLayoutDirty = _needLayoutDirty || !_needHaveData || a_vampire != _needVampire;
		_needColorDirty = _needColorDirty || _needLayoutDirty || a_recovering != _needRecovering;
		_needHungerPct = a_hungerPct;
		_needThirstPct = a_thirstPct;
		_needEnergyPct = a_energyPct;
		_needVampire = a_vampire;
		_needRecovering = a_recovering;
		_needHaveData = true;
		ApplyNeedData();
	}

	void PlayerWidget::ApplyNeedData()
	{
		if (!_needHaveData || !_bInitialized) {
			return;
		}

		EnsureNeedBars();
		if (!_needBarsReady) {
			return;
		}

		if (_needColorDirty) {
			ColorNeedBar(_needHunger,
				_needVampire ? kBloodBar : kHungerBar,
				_needVampire ? kBloodPhantom : kHungerPhantom,
				_needVampire ? kBloodFlash : kHungerFlash);
			ColorNeedBar(_needThirst, kThirstBar, kThirstPhantom, kThirstFlash);
			ColorNeedBar(_needEnergy,
				_needRecovering ? kEnergyRecoverBar : kEnergyBar,
				_needRecovering ? kEnergyRecoverPhantom : kEnergyPhantom,
				_needRecovering ? kEnergyRecoverFlash : kEnergyFlash);
			_needColorDirty = false;
		}
		if (_needLayoutDirty) {
			LayoutNeedBars();
			_needLayoutDirty = false;
		}

		WriteNeedBar(_needHunger, _needHungerPct, _lastHungerPct, _hungerShow);
		if (_needVampire) {
			_lastThirstPct = _needThirstPct;
			_thirstShow = 0.f;
		} else {
			WriteNeedBar(_needThirst, _needThirstPct, _lastThirstPct, _thirstShow);
		}
		WriteNeedBar(_needEnergy, _needEnergyPct, _lastEnergyPct, _energyShow);
	}

	void PlayerWidget::PeekBars()
	{
		RE::GFxValue refresh;
		refresh.SetBoolean(true);
		_object.Invoke("showHealth", nullptr, &refresh, 1);
		_object.Invoke("showMagicka", nullptr, &refresh, 1);
		_object.Invoke("showStamina", nullptr, &refresh, 1);
		_hungerShow = kNeedShowSec;
		_thirstShow = kNeedShowSec;
		_energyShow = kNeedShowSec;
	}

	void PlayerWidget::EnsureNeedBars()
	{
		if (_needBarsReady) {
			return;
		}

		RE::GFxValue health;
		if (!_object.GetMember("Health", &health) || !health.IsDisplayObject()) {
			if (!_needAttachWarned) {
				logger::warn("TrueHUD: need bars waiting for Health clip");
				_needAttachWarned = true;
			}
			return;
		}

		if (!AttachNeedBar(_needHunger, "NeedHunger") ||
			!AttachNeedBar(_needThirst, "NeedThirst") ||
			!AttachNeedBar(_needEnergy, "NeedEnergy")) {
			logger::warn("TrueHUD: failed to attach player need bars");
			CleanupNeedBars();
			return;
		}

		InitNeedBar(_needHunger);
		InitNeedBar(_needThirst);
		InitNeedBar(_needEnergy);
		ColorNeedBar(_needHunger, kHungerBar, kHungerPhantom, kHungerFlash);
		ColorNeedBar(_needThirst, kThirstBar, kThirstPhantom, kThirstFlash);
		ColorNeedBar(_needEnergy, kEnergyBar, kEnergyPhantom, kEnergyFlash);
		_needBarsReady = true;
		LayoutNeedBars();
		logger::info("TrueHUD: player need bars attached");
	}

	void PlayerWidget::CleanupNeedBars()
	{
		const auto drop = [](RE::GFxValue& a_clip) {
			if (a_clip.IsDisplayObject()) {
				a_clip.Invoke("cleanUp", nullptr, nullptr, 0);
				a_clip.Invoke("removeMovieClip", nullptr, nullptr, 0);
			}
			a_clip = RE::GFxValue();
		};
		drop(_needHunger);
		drop(_needThirst);
		drop(_needEnergy);
		_needBarsReady = false;
		_needAttachWarned = false;
		_lastHungerPct = -1.f;
		_lastThirstPct = -1.f;
		_lastEnergyPct = -1.f;
		_hungerShow = 0.f;
		_thirstShow = 0.f;
		_energyShow = 0.f;
	}

	bool PlayerWidget::AttachNeedBar(RE::GFxValue& a_out, const char* a_name)
	{
		if (!_object.IsDisplayObject()) {
			return false;
		}

		RE::GFxValue health;
		if (!_object.GetMember("Health", &health) || !health.IsDisplayObject()) {
			return false;
		}

		RE::GFxValue depth;
		_object.Invoke("getNextHighestDepth", &depth);
		RE::GFxValue args[2];
		args[0].SetString(a_name);
		args[1] = depth;
		health.Invoke("duplicateMovieClip", &a_out, args, 2);
		if (!a_out.IsDisplayObject()) {
			_object.GetMember(a_name, &a_out);
		}
		return a_out.IsDisplayObject();
	}

	void PlayerWidget::InitNeedBar(RE::GFxValue& a_bar)
	{
		RE::GFxValue initArgs[8];
		initArgs[0].SetBoolean(true);
		initArgs[1].SetNumber(Settings::fPlayerWidgetPhantomDuration);
		initArgs[2].SetNumber(0);
		initArgs[3].SetBoolean(true);
		initArgs[4].SetBoolean(true);
		initArgs[5].SetBoolean(true);
		initArgs[6].SetBoolean(true);
		initArgs[7].SetBoolean(false);
		a_bar.Invoke("init", nullptr, initArgs, 8);

		RE::GFxValue width;
		width.SetNumber(kNeedBarWidth);
		a_bar.Invoke("setWidth", nullptr, &width, 1);
		SetNeedAlpha(a_bar, 0.f);
	}

	void PlayerWidget::ColorNeedBar(RE::GFxValue& a_bar, uint32_t a_barColor, uint32_t a_phantomColor, uint32_t a_flashColor)
	{
		RE::GFxValue args[5];
		args[0].SetNumber(a_barColor);
		args[1].SetNumber(a_phantomColor);
		args[2].SetNumber(kNeedBg);
		args[3].SetNumber(kNeedPenalty);
		args[4].SetNumber(a_flashColor);
		a_bar.Invoke("setColor", nullptr, args, 5);
	}

	void PlayerWidget::PlaceNeedBar(RE::GFxValue& a_bar, float a_x, float a_y)
	{
		RE::GFxValue::DisplayInfo displayInfo;
		displayInfo.SetPosition(a_x, a_y);
		a_bar.SetDisplayInfo(displayInfo);
	}

	void PlayerWidget::WriteNeedBar(RE::GFxValue& a_bar, float a_pct, float& a_lastPct, float& a_showLeft)
	{
		a_pct = std::clamp(a_pct, 0.f, 100.f);
		const bool first = a_lastPct < 0.f;
		const bool changed = first || std::fabs(a_pct - a_lastPct) >= 0.25f;
		const bool drop = !first && a_pct + kNeedDrop < a_lastPct;
		const bool critical = !first && a_lastPct > kNeedCritical && a_pct <= kNeedCritical;

		RE::GFxValue args[2];
		args[0].SetNumber(a_pct / 100.f);
		args[1].SetBoolean(first);
		a_bar.Invoke("updatePercent", nullptr, args, 2);

		if (drop || critical) {
			RE::GFxValue flash;
			flash.SetBoolean(critical);
			a_bar.Invoke("playFlash", nullptr, &flash, 1);
		}
		if (changed) {
			a_showLeft = kNeedShowSec;
		}
		a_lastPct = a_pct;
	}

	void PlayerWidget::LayoutNeedBars()
	{
		if (!_needBarsReady) {
			return;
		}

		RE::NiPoint2 stageSize;
		auto def = _view->GetMovieDef();
		if (def) {
			stageSize.x = def->GetWidth();
			stageSize.y = def->GetHeight();
		}
		const float scale = Settings::fPlayerWidgetScale > 0.f ? Settings::fPlayerWidgetScale : 1.f;

		RE::GFxValue health;
		RE::GFxValue magicka;
		RE::GFxValue stamina;
		_object.GetMember("Health", &health);
		_object.GetMember("Magicka", &magicka);
		_object.GetMember("Stamina", &stamina);
		RE::GFxValue::DisplayInfo healthInfo;
		RE::GFxValue::DisplayInfo magickaInfo;
		RE::GFxValue::DisplayInfo staminaInfo;
		if (health.IsDisplayObject()) {
			health.GetDisplayInfo(&healthInfo);
		}
		if (magicka.IsDisplayObject()) {
			magicka.GetDisplayInfo(&magickaInfo);
		}
		if (stamina.IsDisplayObject()) {
			stamina.GetDisplayInfo(&staminaInfo);
		}

		if (Settings::bPlayerWidgetCombined) {
			const float step = kNeedBarWidth + kNeedBarGap;
			const int count = _needVampire ? 2 : 3;
			const float start = -0.5f * static_cast<float>(count - 1) * step;
			const float combinedY = static_cast<float>(healthInfo.GetY()) - kNeedAboveHealth;
			PlaceNeedBar(_needHunger, start, combinedY);
			if (_needVampire) {
				PlaceNeedBar(_needEnergy, start + step, combinedY);
			} else {
				PlaceNeedBar(_needThirst, start + step, combinedY);
				PlaceNeedBar(_needEnergy, start + step * 2.f, combinedY);
			}
			return;
		}

		const float y = health.IsDisplayObject()
			? static_cast<float>(healthInfo.GetY()) - kNeedAboveHealth
			: stageSize.y * Settings::fPlayerWidgetHealthY / scale - kNeedAboveHealth;
		const float healthX = health.IsDisplayObject()
			? static_cast<float>(healthInfo.GetX())
			: stageSize.x * Settings::fPlayerWidgetHealthX / scale;
		const float magickaX = magicka.IsDisplayObject()
			? static_cast<float>(magickaInfo.GetX())
			: stageSize.x * Settings::fPlayerWidgetMagickaX / scale;
		const float staminaX = stamina.IsDisplayObject()
			? static_cast<float>(staminaInfo.GetX())
			: stageSize.x * Settings::fPlayerWidgetStaminaX / scale;
		float hungerX = magickaX + (healthX - magickaX) * kNeedInward;
		float energyX = staminaX + (healthX - staminaX) * kNeedInward;
		if (_needVampire) {
			if (energyX - hungerX < kNeedMinGap) {
				const float mid = (hungerX + energyX) * 0.5f;
				hungerX = mid - kNeedMinGap * 0.5f;
				energyX = mid + kNeedMinGap * 0.5f;
			}
			PlaceNeedBar(_needHunger, hungerX, y);
			PlaceNeedBar(_needEnergy, energyX, y);
			return;
		}
		if (healthX - hungerX < kNeedMinGap) {
			hungerX = healthX - kNeedMinGap;
		}
		if (energyX - healthX < kNeedMinGap) {
			energyX = healthX + kNeedMinGap;
		}
		PlaceNeedBar(_needHunger, hungerX, y);
		PlaceNeedBar(_needThirst, healthX, y);
		PlaceNeedBar(_needEnergy, energyX, y);
	}

	void PlayerWidget::TickNeedBar(RE::GFxValue& a_bar, float a_pct, float& a_showLeft, float a_deltaTime, bool a_allow)
	{
		if (!a_bar.IsDisplayObject()) {
			return;
		}
		if (!a_allow) {
			SetNeedAlpha(a_bar, 0.f);
			return;
		}

		const float opacity = NeedOpacity();
		if (a_pct <= kNeedPinAt) {
			SetNeedAlpha(a_bar, opacity);
			return;
		}

		a_showLeft -= a_deltaTime;
		if (a_showLeft > 0.f) {
			SetNeedAlpha(a_bar, opacity);
		} else if (a_showLeft > -kNeedFadeSec) {
			SetNeedAlpha(a_bar, opacity * (1.f + a_showLeft / kNeedFadeSec));
		} else {
			SetNeedAlpha(a_bar, 0.f);
		}
	}

	void PlayerWidget::SetNeedAlpha(RE::GFxValue& a_bar, float a_alpha)
	{
		RE::GFxValue::DisplayInfo info;
		a_bar.GetDisplayInfo(&info);
		info.SetAlpha(a_alpha);
		a_bar.SetDisplayInfo(info);
	}
}
