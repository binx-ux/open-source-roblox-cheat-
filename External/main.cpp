#include <cstdint>
#include <windows.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include "src/memory/memory.h"
#include "src/sdk/offsets.h"
#include "src/sdk/offset_auto.h"
#include "src/sdk/sdk.h"
#include "src/sdk/scanner.h"
#include "src/core/cache/cache.h"
#include "src/core/globals/globals.h"
#include "src/core/tp_handler/tp_handler.h"
#include "src/core/features/visuals/visuals.h"
#include "src/core/features/aimbot/aimbot.h"
#include "src/core/features/exploits/exploits.h"
#include "src/core/features/exploits/gun_mods.h"
#include "src/core/audio/custom_music.h"
#include "src/core/games/arsenal.h"
#include "src/core/variables/variables.h"
#include "src/core/telemetry/telemetry.h"
#include "src/core/eject.h"
#include "src/core/features/combat_fx.h"
#include "src/render/render.h"

namespace
{
	constexpr const char* app = "RobloxPlayerBeta.exe";
	constexpr const wchar_t* apptitle = L"Roblox";

	// Feature keybinds only while Roblox is the foreground window.
	inline bool GameKeyDown(int vk)
	{
		return WindowManager::IsRobloxFocused() && (GetAsyncKeyState(vk) & 0x8000) != 0;
	}

	inline void EnforceMeshFpsProfile()
	{
		if (!Games::IsMeshFps()) return;

		variables::Rage::enabled = false;
		variables::Trigger::enabled = false;
		variables::Hitbox::enabled = false;
		variables::Local::hitboxEnabled = false;
		variables::Local::autoTp = false;
		variables::Local::speedEnabled = false;
		variables::Local::flyEnabled = false;
		variables::Local::flyActive = false;
		variables::Local::jumpEnabled = false;
		variables::Local::infJump = false;
		variables::Local::noclip = false;
		variables::Local::clickTp = false;
		variables::Local::walkFling = false;
		variables::Local::godMode = false;
		variables::Local::antiFling = false;
		variables::Local::bhopEnabled = false;
		variables::Local::floatEnabled = false;
		variables::Local::tpWalk = false;
		variables::Local::orbitPlayer = false;
		variables::Local::freeze = false;
		variables::Local::spin = false;
		variables::Local::gravityEnabled = false;
		variables::Local::autoClicker = false;
		variables::Local::vehicleBoost = false;
		variables::Local::antiVoid = false;
		variables::Local::sitSpam = false;

		variables::GunMods::fastReload = false;
		variables::GunMods::fastFire = false;
		variables::GunMods::rapidPlus = false;
		variables::GunMods::alwaysAuto = false;
		variables::GunMods::noSpread = false;
		variables::GunMods::noRecoil = false;
		variables::GunMods::noSway = false;
		variables::GunMods::infiniteAmmo = false;
		variables::GunMods::autoReload = false;
		variables::GunMods::damageBoost = false;
		variables::GunMods::longRange = false;
		variables::GunMods::wallbangHint = false;
		variables::GunMods::instantEquip = false;

		// Camera-locked FPS: silent mouse spoof is primary
		variables::Aimbot::aimType = 1;
		variables::Aimbot::silentAim = true;
		variables::Aimbot::targetMethod = 1;
		// Wall check left to user ? Mesh FPS uses a collide-only / shrink OBB path now
	}

	void SetLoad(float p, const char* status)
	{
		variables::Loading::progress = p;
		strncpy_s(variables::Loading::status, status, _TRUNCATE);
	}

	bool isgamerunning(const wchar_t*)
	{
		return WindowManager::IsRobloxOpen() || memory->find_process_id(app) != 0;
	}

	void PanicDisableAll()
	{
		FadeEject::DisableCheats();
		CustomMusic::StopLocal();
	}

	void WriteLocalVelocity(RBX::RbxInstance rootPart, const RBX::Vec3& vel)
	{
		auto prim = rootPart.GetPrimitivePtr();
		if (prim == 0) return;
		memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
	}

	void localThread()
	{
		bool flyKeyLatched = false;
		auto lastTick = std::chrono::steady_clock::now();
		while (Globals::running) {
			auto now = std::chrono::steady_clock::now();
			float dt = std::chrono::duration<float>(now - lastTick).count();
			lastTick = now;
			if (dt < 0.001f) dt = 0.001f;
			if (dt > 0.05f) dt = 0.05f;

			if (variables::Loading::active) {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}

			// World / lighting ? capture originals so disable restores cleanly
			if (Globals::dataModel.Addr != 0) {
				struct LightSnap {
					bool valid = false;
					float brightness = 1.f, fogStart = 0.f, fogEnd = 100000.f, clock = 14.f, exposure = 0.f;
					float ambient[3]{1,1,1}, outdoor[3]{1,1,1}, fogCol[3]{1,1,1}, lightCol[3]{1,1,1};
					float shiftTop[3]{}, shiftBot[3]{};
					float envDiff = 1.f, envSpec = 1.f;
					bool shadows = true;
					bool hasAtmo = false;
					uintptr_t atmoAddr = 0;
					float atmoDensity = 0.f, atmoHaze = 0.f, atmoGlare = 0.f, atmoOffset = 0.f;
				};
				static LightSnap lightSnap;
				static bool lightWasOn = false;

				auto lighting = Globals::dataModel.FindChildByClass("Lighting");
				const bool wantLight =
					variables::World::fullbright || variables::World::customBrightness ||
					variables::World::noFog || variables::World::noShadows ||
					variables::World::customClock || variables::World::nightMode ||
					variables::World::customAmbient || variables::World::removeAtmosphere;

				auto readColor = [&](uintptr_t off, float* out) {
					out[0] = memory->read<float>(lighting.Addr + off + 0);
					out[1] = memory->read<float>(lighting.Addr + off + 4);
					out[2] = memory->read<float>(lighting.Addr + off + 8);
				};
				auto writeColor = [&](uintptr_t off, float r, float g, float b) {
					memory->write<float>(lighting.Addr + off + 0, r);
					memory->write<float>(lighting.Addr + off + 4, g);
					memory->write<float>(lighting.Addr + off + 8, b);
				};

				if (lighting.Addr != 0) {
					if (wantLight) {
						static auto lastLight = std::chrono::steady_clock::now();
						auto nowL = std::chrono::steady_clock::now();
						bool doLight = variables::World::fullbright ||
							std::chrono::duration<float>(nowL - lastLight).count() > 0.12f;
						if (doLight) lastLight = nowL;

						if (!lightSnap.valid) {
							lightSnap.brightness = memory->read<float>(lighting.Addr + Offsets::Lighting::Brightness);
							lightSnap.fogStart = memory->read<float>(lighting.Addr + Offsets::Lighting::FogStart);
							lightSnap.fogEnd = memory->read<float>(lighting.Addr + Offsets::Lighting::FogEnd);
							lightSnap.clock = memory->read<float>(lighting.Addr + Offsets::Lighting::ClockTime);
							lightSnap.exposure = memory->read<float>(lighting.Addr + Offsets::Lighting::ExposureCompensation);
							lightSnap.envDiff = memory->read<float>(lighting.Addr + Offsets::Lighting::EnvironmentDiffuseScale);
							lightSnap.envSpec = memory->read<float>(lighting.Addr + Offsets::Lighting::EnvironmentSpecularScale);
							lightSnap.shadows = memory->read<bool>(lighting.Addr + Offsets::Lighting::GlobalShadows);
							readColor(Offsets::Lighting::Ambient, lightSnap.ambient);
							readColor(Offsets::Lighting::OutdoorAmbient, lightSnap.outdoor);
							readColor(Offsets::Lighting::FogColor, lightSnap.fogCol);
							readColor(Offsets::Lighting::LightColor, lightSnap.lightCol);
							readColor(Offsets::Lighting::ColorShift_Top, lightSnap.shiftTop);
							readColor(Offsets::Lighting::ColorShift_Bottom, lightSnap.shiftBot);
							for (auto& ch : lighting.GetChildList()) {
								if (ch.GetClass() == "Atmosphere") {
									lightSnap.hasAtmo = true;
									lightSnap.atmoAddr = ch.Addr;
									lightSnap.atmoDensity = memory->read<float>(ch.Addr + Offsets::Atmosphere::Density);
									lightSnap.atmoHaze = memory->read<float>(ch.Addr + Offsets::Atmosphere::Haze);
									lightSnap.atmoGlare = memory->read<float>(ch.Addr + Offsets::Atmosphere::Glare);
									lightSnap.atmoOffset = memory->read<float>(ch.Addr + Offsets::Atmosphere::Offset);
									break;
								}
							}
							lightSnap.valid = true;
						}
						lightWasOn = true;

						if (doLight) {
							if (variables::World::fullbright) {
								writeColor(Offsets::Lighting::Ambient, 1.f, 1.f, 1.f);
								writeColor(Offsets::Lighting::OutdoorAmbient, 1.f, 1.f, 1.f);
								writeColor(Offsets::Lighting::ColorShift_Top, 0.f, 0.f, 0.f);
								writeColor(Offsets::Lighting::ColorShift_Bottom, 0.f, 0.f, 0.f);
								writeColor(Offsets::Lighting::FogColor, 1.f, 1.f, 1.f);
								writeColor(Offsets::Lighting::LightColor, 1.f, 1.f, 1.f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::Brightness, 4.0f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::ClockTime, 14.0f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::ExposureCompensation, 0.35f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::FogStart, 0.0f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::FogEnd, 1.0e6f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::EnvironmentDiffuseScale, 1.0f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::EnvironmentSpecularScale, 0.0f);
								memory->write<bool>(lighting.Addr + Offsets::Lighting::GlobalShadows, false);
								for (auto& ch : lighting.GetChildList()) {
									if (ch.GetClass() == "Atmosphere") {
										memory->write<float>(ch.Addr + Offsets::Atmosphere::Density, 0.0f);
										memory->write<float>(ch.Addr + Offsets::Atmosphere::Haze, 0.0f);
										memory->write<float>(ch.Addr + Offsets::Atmosphere::Glare, 0.0f);
										memory->write<float>(ch.Addr + Offsets::Atmosphere::Offset, 0.0f);
									}
								}
							}
							if (variables::World::customBrightness) {
								memory->write<float>(lighting.Addr + Offsets::Lighting::Brightness, variables::World::brightness);
							}
							if (variables::World::noFog) {
								memory->write<float>(lighting.Addr + Offsets::Lighting::FogEnd, 1.0e6f);
								memory->write<float>(lighting.Addr + Offsets::Lighting::FogStart, 0.0f);
							}
							if (variables::World::noShadows) {
								memory->write<bool>(lighting.Addr + Offsets::Lighting::GlobalShadows, false);
							}
							if (variables::World::nightMode) {
								memory->write<float>(lighting.Addr + Offsets::Lighting::ClockTime, 0.0f);
								if (!variables::World::customBrightness)
									memory->write<float>(lighting.Addr + Offsets::Lighting::Brightness, 0.35f);
							}
							else if (variables::World::customClock) {
								memory->write<float>(lighting.Addr + Offsets::Lighting::ClockTime, variables::World::clockTime);
							}
							if (variables::World::customAmbient) {
								float ar = variables::World::ambientColor[0];
								float ag = variables::World::ambientColor[1];
								float ab = variables::World::ambientColor[2];
								variables::World::ambientR = ar;
								variables::World::ambientG = ag;
								variables::World::ambientB = ab;
								writeColor(Offsets::Lighting::Ambient, ar, ag, ab);
								writeColor(Offsets::Lighting::OutdoorAmbient, ar, ag, ab);
								writeColor(Offsets::Lighting::ColorShift_Top, ar * 0.35f, ag * 0.35f, ab * 0.35f);
								writeColor(Offsets::Lighting::LightColor, ar, ag, ab);
							}
							if (variables::World::removeAtmosphere) {
								for (auto& ch : lighting.GetChildList()) {
									if (ch.GetClass() == "Atmosphere") {
										memory->write<float>(ch.Addr + Offsets::Atmosphere::Density, 0.0f);
										memory->write<float>(ch.Addr + Offsets::Atmosphere::Haze, 0.0f);
										memory->write<float>(ch.Addr + Offsets::Atmosphere::Glare, 0.0f);
									}
								}
							}
						}
					}
					else if (lightWasOn && lightSnap.valid) {
						memory->write<float>(lighting.Addr + Offsets::Lighting::Brightness, lightSnap.brightness);
						memory->write<float>(lighting.Addr + Offsets::Lighting::FogStart, lightSnap.fogStart);
						memory->write<float>(lighting.Addr + Offsets::Lighting::FogEnd, lightSnap.fogEnd);
						memory->write<float>(lighting.Addr + Offsets::Lighting::ClockTime, lightSnap.clock);
						memory->write<float>(lighting.Addr + Offsets::Lighting::ExposureCompensation, lightSnap.exposure);
						memory->write<float>(lighting.Addr + Offsets::Lighting::EnvironmentDiffuseScale, lightSnap.envDiff);
						memory->write<float>(lighting.Addr + Offsets::Lighting::EnvironmentSpecularScale, lightSnap.envSpec);
						memory->write<bool>(lighting.Addr + Offsets::Lighting::GlobalShadows, lightSnap.shadows);
						writeColor(Offsets::Lighting::Ambient, lightSnap.ambient[0], lightSnap.ambient[1], lightSnap.ambient[2]);
						writeColor(Offsets::Lighting::OutdoorAmbient, lightSnap.outdoor[0], lightSnap.outdoor[1], lightSnap.outdoor[2]);
						writeColor(Offsets::Lighting::FogColor, lightSnap.fogCol[0], lightSnap.fogCol[1], lightSnap.fogCol[2]);
						writeColor(Offsets::Lighting::LightColor, lightSnap.lightCol[0], lightSnap.lightCol[1], lightSnap.lightCol[2]);
						writeColor(Offsets::Lighting::ColorShift_Top, lightSnap.shiftTop[0], lightSnap.shiftTop[1], lightSnap.shiftTop[2]);
						writeColor(Offsets::Lighting::ColorShift_Bottom, lightSnap.shiftBot[0], lightSnap.shiftBot[1], lightSnap.shiftBot[2]);
						if (lightSnap.hasAtmo && lightSnap.atmoAddr) {
							memory->write<float>(lightSnap.atmoAddr + Offsets::Atmosphere::Density, lightSnap.atmoDensity);
							memory->write<float>(lightSnap.atmoAddr + Offsets::Atmosphere::Haze, lightSnap.atmoHaze);
							memory->write<float>(lightSnap.atmoAddr + Offsets::Atmosphere::Glare, lightSnap.atmoGlare);
							memory->write<float>(lightSnap.atmoAddr + Offsets::Atmosphere::Offset, lightSnap.atmoOffset);
						}
						lightSnap = LightSnap{};
						lightWasOn = false;
					}
				}
			}

			if (Globals::camera.Addr != 0) {
				static float fovOrig = 0.f;
				static bool fovWasOn = false;
				const bool wantFov = variables::World::customFov || variables::World::viewmodelFov;
				if (wantFov) {
					if (!fovWasOn) {
						fovOrig = memory->read<float>(Globals::camera.Addr + Offsets::Camera::FieldOfView);
						fovWasOn = true;
					}
					float deg = variables::World::customFov ? variables::World::fovAmount : variables::World::viewmodelFovAmt;
					float cur = memory->read<float>(Globals::camera.Addr + Offsets::Camera::FieldOfView);
					float writeVal = deg;
					if (cur > 0.05f && cur < 3.5f)
						writeVal = deg * 0.01745329251f;
					memory->write<float>(Globals::camera.Addr + Offsets::Camera::FieldOfView, writeVal);
				}
				else if (fovWasOn) {
					memory->write<float>(Globals::camera.Addr + Offsets::Camera::FieldOfView, fovOrig);
					fovWasOn = false;
				}
			}

			GunMods::Apply();

			auto character = Globals::localPlayer.GetModelRef();
			if (character.Addr == 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				continue;
			}

			auto humanoid = character.FindChildByClass("Humanoid");
			auto rootPart = character.FindChild("HumanoidRootPart");
			if (humanoid.Addr == 0 || rootPart.Addr == 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				continue;
			}

			// Sync Matcha Hitbox/Desync namespaces
			variables::Local::hitboxEnabled = variables::Hitbox::enabled;
			variables::Local::hitboxSize = variables::Hitbox::size;
			variables::Local::visualizeHitbox = variables::Hitbox::visualize;

			// MiscGunTest:X ??? hitbox expand is a ban risk; force off + keep toast visible once
			static bool mgtHitboxWarned = false;
			if (Games::Detect() == Games::Kind::MiscGunTest) {
				if (variables::Hitbox::enabled || variables::Local::hitboxEnabled) {
					variables::Hitbox::enabled = false;
					variables::Local::hitboxEnabled = false;
					if (!mgtHitboxWarned) {
						mgtHitboxWarned = true;
						variables::Toast::show = true;
						variables::Toast::warning = true;
						variables::Toast::timer = 8.0f;
						strcpy_s(variables::Toast::title, "Hitbox blocked");
						strcpy_s(variables::Toast::body, "MiscGunTest:X ??? do not expand");
						strcpy_s(variables::Toast::footer, "Ban risk ??? keep Hitbox OFF");
					}
				}
			} else {
				mgtHitboxWarned = false;
			}

			EnforceMeshFpsProfile();

			variables::Local::desyncEnabled = variables::Desync::enabled;

			static bool hitboxKeyLatch = false;
			if (!Games::IsMeshFps() && variables::Hitbox::key > 0) {
				bool hk = GameKeyDown(variables::Hitbox::key);
				if (hk && !hitboxKeyLatch) {
					variables::Hitbox::enabled = !variables::Hitbox::enabled;
					variables::Local::hitboxEnabled = variables::Hitbox::enabled;
					hitboxKeyLatch = true;
				}
				else if (!hk) hitboxKeyLatch = false;
			}

			static bool rageKeyLatch = false;
			if (!Games::IsMeshFps() && variables::Rage::key > 0) {
				bool rk = GameKeyDown(variables::Rage::key);
				if (rk && !rageKeyLatch) {
					variables::Rage::enabled = !variables::Rage::enabled;
					rageKeyLatch = true;
				}
				else if (!rk) rageKeyLatch = false;
			}

			static float walkOrig = 16.f;
			static bool speedWasOn = false;
			if (variables::Local::speedEnabled) {
				if (!speedWasOn) {
					walkOrig = memory->read<float>(humanoid.Addr + Offsets::Humanoid::Walkspeed);
					if (walkOrig < 1.f) walkOrig = 16.f;
					speedWasOn = true;
				}

				if (variables::Local::speedMethod == 0 || variables::Local::speedMethod == 2) {
					float spd = variables::Local::walkSpeed;
					if (variables::Local::speedMethod == 2) spd *= 1.35f;
					RBX::ModifyWalkSpeed(humanoid, spd);
					auto prim = rootPart.GetPrimitivePtr();
					if (prim && Globals::camera.Addr) {
						auto cf = Globals::camera.GetCameraCFrame();
						RBX::Vec3 look = cf.GetLookVector();
						look.X = -look.X; look.Z = -look.Z;
						float flat = sqrtf(look.X * look.X + look.Z * look.Z);
						RBX::Vec3 move{};
						if (GameKeyDown('W')) { move.X += look.X; move.Z += look.Z; }
						if (GameKeyDown('S')) { move.X -= look.X; move.Z -= look.Z; }
						RBX::Vec3 right = cf.GetRightVector();
						if (GameKeyDown('D')) { move.X += right.X; move.Z += right.Z; }
						if (GameKeyDown('A')) { move.X -= right.X; move.Z -= right.Z; }
						float ml = sqrtf(move.X * move.X + move.Z * move.Z);
						if (ml > 0.001f && flat > 0.001f) {
							RBX::Vec3 cur = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
							WriteLocalVelocity(rootPart, { (move.X / ml) * spd, cur.Y, (move.Z / ml) * spd });
						}
					}
				}
				else { // Position
					RBX::ModifyWalkSpeed(humanoid, 16.f);
					if (Globals::camera.Addr) {
						auto cf = Globals::camera.GetCameraCFrame();
						RBX::Vec3 look = cf.GetLookVector();
						look.X = -look.X; look.Z = -look.Z;
						RBX::Vec3 right = cf.GetRightVector();
						RBX::Vec3 move{};
						if (GameKeyDown('W')) { move.X += look.X; move.Z += look.Z; }
						if (GameKeyDown('S')) { move.X -= look.X; move.Z -= look.Z; }
						if (GameKeyDown('D')) { move.X += right.X; move.Z += right.Z; }
						if (GameKeyDown('A')) { move.X -= right.X; move.Z -= right.Z; }
						float ml = sqrtf(move.X * move.X + move.Z * move.Z);
						if (ml > 0.001f) {
							RBX::Vec3 pos = rootPart.GetPos();
							float spd = variables::Local::walkSpeed;
							pos.X += (move.X / ml) * spd * dt;
							pos.Z += (move.Z / ml) * spd * dt;
							rootPart.SetPos(pos);
						}
					}
				}
			}
			else if (speedWasOn) {
				RBX::ModifyWalkSpeed(humanoid, walkOrig);
				speedWasOn = false;
			}

			static float jumpOrig = 50.f;
			static bool jumpWasOn = false;
			if (variables::Local::jumpEnabled) {
				if (!jumpWasOn) {
					jumpOrig = memory->read<float>(humanoid.Addr + Offsets::Humanoid::JumpPower);
					if (jumpOrig < 1.f) jumpOrig = 50.f;
					jumpWasOn = true;
				}
				RBX::ModifyJumpPower(humanoid, variables::Local::jumpPower);
			}
			else if (jumpWasOn) {
				RBX::ModifyJumpPower(humanoid, jumpOrig);
				jumpWasOn = false;
			}

			// Inf jump
			if (variables::Local::infJump && GameKeyDown(VK_SPACE)) {
				RBX::ModifyJumpPower(humanoid, variables::Local::jumpPower > 1.f ? variables::Local::jumpPower : 50.f);
				RBX::ForceJump(humanoid);
				auto prim = rootPart.GetPrimitivePtr();
				if (prim) {
					RBX::Vec3 vel = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
					float boost = variables::Local::jumpPower * 0.55f;
					if (boost < 45.0f) boost = 45.0f;
					if (vel.Y < boost * 0.85f) {
						vel.Y = boost;
						WriteLocalVelocity(rootPart, vel);
					}
				}
			}

			// Float ??? hold height
			static float floatBaseY = 0.f;
			static bool floatArmed = false;
			if (variables::Local::floatEnabled) {
				if (!floatArmed) {
					floatBaseY = rootPart.GetPos().Y;
					floatArmed = true;
				}
				RBX::Vec3 pos = rootPart.GetPos();
				pos.Y = floatBaseY + variables::Local::floatHeight;
				rootPart.SetPos(pos);
				WriteLocalVelocity(rootPart, { 0, 0, 0 });
			}
			else {
				floatArmed = false;
			}

			// Touch fling ? only shove the OTHER player. Never spike your own velocity
			// (that was launching you). Keep normal walk; kill your angular so you stay upright.
			if (variables::Local::walkFling) {
				auto prim = rootPart.GetPrimitivePtr();
				if (prim)
					memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, { 0, 0, 0 });

				bool keyOk = variables::Local::walkFlingKey <= 0 || GameKeyDown(variables::Local::walkFlingKey);
				if (keyOk) {
					RBX::Vec3 myPos = rootPart.GetPos();
					float touchR = variables::Local::walkFlingRange;
					if (touchR < 2.f) touchR = 2.f;
					if (touchR > 12.f) touchR = 12.f;

					float power = variables::Local::walkFlingPower;
					if (power < 40.f) power = 40.f;
					if (power > 500.f) power = 500.f;

					// Your look / move direction helps aim the shove
					RBX::Vec3 pushHint{};
					if (Globals::camera.Addr) {
						auto cf = Globals::camera.GetCameraCFrame();
						RBX::Vec3 look = cf.GetLookVector();
						look.X = -look.X; look.Z = -look.Z; look.Y = 0.f;
						RBX::Vec3 right = cf.GetRightVector(); right.Y = 0.f;
						if (GameKeyDown('W')) { pushHint.X += look.X; pushHint.Z += look.Z; }
						if (GameKeyDown('S')) { pushHint.X -= look.X; pushHint.Z -= look.Z; }
						if (GameKeyDown('D')) { pushHint.X += right.X; pushHint.Z += right.Z; }
						if (GameKeyDown('A')) { pushHint.X -= right.X; pushHint.Z -= right.Z; }
						float hl = sqrtf(pushHint.X * pushHint.X + pushHint.Z * pushHint.Z);
						if (hl > 0.05f) { pushHint.X /= hl; pushHint.Z /= hl; }
					}

					for (auto& plr : PlayerCache::snapshotPlayers()) {
						if (!plr.isValid || plr.health <= 0.f || !plr.rootPartAddr) continue;
						float dx = plr.position.X - myPos.X;
						float dy = plr.position.Y - myPos.Y;
						float dz = plr.position.Z - myPos.Z;
						float d = sqrtf(dx * dx + dy * dy + dz * dz);
						if (d > touchR || d < 0.05f) continue;

						float inv = 1.f / d;
						RBX::Vec3 away{ dx * inv, dy * inv, dz * inv };

						// Mostly away from you; blend walk direction so it feels like a bump
						RBX::Vec3 dir = away;
						if (pushHint.X != 0.f || pushHint.Z != 0.f) {
							dir.X = away.X * 0.55f + pushHint.X * 0.45f;
							dir.Z = away.Z * 0.55f + pushHint.Z * 0.45f;
							dir.Y = 0.55f;
							float dl = sqrtf(dir.X * dir.X + dir.Y * dir.Y + dir.Z * dir.Z);
							if (dl > 0.001f) { dir.X /= dl; dir.Y /= dl; dir.Z /= dl; }
						}
						else {
							dir.Y = 0.55f;
							float dl = sqrtf(dir.X * dir.X + dir.Y * dir.Y + dir.Z * dir.Z);
							if (dl > 0.001f) { dir.X /= dl; dir.Y /= dl; dir.Z /= dl; }
						}

						float up = power * 0.85f;
						if (up < 70.f) up = 70.f;
						RBX::Vec3 flingVel{ dir.X * power, up, dir.Z * power };

						auto flingPart = [&](uintptr_t partAddr) {
							if (!partAddr) return;
							RBX::RbxInstance part(partAddr);
							auto pp = part.GetPrimitivePtr();
							if (!pp) return;
							memory->write<RBX::Vec3>(pp + Offsets::Primitive::AssemblyLinearVelocity, flingVel);
							// Spin them (not you) so the fling "takes"
							memory->write<RBX::Vec3>(pp + Offsets::Primitive::AssemblyAngularVelocity,
								{ 0.f, power * 0.08f, 0.f });
						};

						flingPart(plr.rootPartAddr);
						flingPart(plr.headAddr);
						if (plr.characterAddr) {
							RBX::RbxInstance ch(plr.characterAddr);
							auto torso = ch.FindChild("UpperTorso");
							if (!torso.Addr) torso = ch.FindChild("Torso");
							if (torso.Addr) flingPart(torso.Addr);
						}
					}

					// If a collision still kicks you, damp yourself back to walk speeds
					if (prim) {
						RBX::Vec3 me = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
						float mySpd = sqrtf(me.X * me.X + me.Y * me.Y + me.Z * me.Z);
						if (mySpd > 48.f) {
							float s = 22.f / mySpd;
							WriteLocalVelocity(rootPart, { me.X * s, me.Y * 0.35f, me.Z * s });
						}
						memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, { 0, 0, 0 });
					}
				}
			}

			// Anti-fling ? kill spin + touch, only damp true fling spikes (never your walk/fly speed)
			if (variables::Local::antiFling) {
				for (auto& ch : character.GetChildList()) {
					std::string cls = ch.GetClass();
					if (cls == "Part" || cls == "MeshPart" || cls == "UnionOperation") {
						auto p = ch.GetPrimitivePtr();
						if (!p) continue;
						uint8_t flags = memory->read<uint8_t>(p + Offsets::Primitive::Flags);
						flags = (uint8_t)(flags & ~Offsets::PrimitiveFlags::CanTouch);
						memory->write<uint8_t>(p + Offsets::Primitive::Flags, flags);
					}
				}

				auto prim = rootPart.GetPrimitivePtr();
				if (prim) {
					memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, { 0, 0, 0 });

					if (!variables::Local::walkFling) {
						RBX::Vec3 v = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
						float horiz = sqrtf(v.X * v.X + v.Z * v.Z);
						float expected = 22.f;
						if (variables::Local::speedEnabled)
							expected = (std::max)(expected, variables::Local::walkSpeed * 1.15f);
						if (variables::Local::flyEnabled && variables::Local::flyActive)
							expected = (std::max)(expected, variables::Local::flySpeed * 1.15f);
						const float flingFloor = expected + 90.f;
						const bool crazyHoriz = horiz > flingFloor;
						const bool crazyVert = fabsf(v.Y) > 160.f;
						if (crazyHoriz || crazyVert) {
							float hx = v.X, hz = v.Z;
							if (crazyHoriz && horiz > 1.f) {
								float keep = expected / horiz;
								hx *= keep;
								hz *= keep;
							}
							float hy = v.Y;
							if (crazyVert)
								hy = (v.Y > 0.f ? 1.f : -1.f) * (std::min)(fabsf(v.Y), 70.f);
							WriteLocalVelocity(rootPart, { hx, hy, hz });
						}
					}
				}
			}

			// No-clip best-effort
			if (variables::Local::noclip) {
				for (auto& ch : character.GetChildList()) {
					std::string cls = ch.GetClass();
					if (cls == "Part" || cls == "MeshPart" || cls == "UnionOperation") {
						auto prim = ch.GetPrimitivePtr();
						if (prim) {
							uint8_t flags = memory->read<uint8_t>(prim + Offsets::Primitive::Flags);
							flags = (uint8_t)(flags & ~Offsets::PrimitiveFlags::CanCollide);
							memory->write<uint8_t>(prim + Offsets::Primitive::Flags, flags);
						}
					}
				}
			}

			// Click TP
			static bool clickTpLatch = false;
			if (variables::Local::clickTp && variables::Local::clickTpKey > 0) {
				bool held = GameKeyDown(variables::Local::clickTpKey);
				if (held && !clickTpLatch && Globals::camera.Addr) {
					auto cf = Globals::camera.GetCameraCFrame();
					RBX::Vec3 look = cf.GetLookVector();
					look.X = -look.X; look.Y = -look.Y; look.Z = -look.Z;
					RBX::Vec3 pos = rootPart.GetPos();
					pos.X += look.X * 25.f;
					pos.Y += look.Y * 25.f;
					pos.Z += look.Z * 25.f;
					rootPart.SetPos(pos);
					clickTpLatch = true;
				}
				else if (!held) clickTpLatch = false;
			}

			// Auto TP Loop ??? sticky teleport to nearest player (0 delay = every frame)
			{
				static auto lastAutoTp = std::chrono::steady_clock::now();
				static bool autoTpKeyLatch = false;
				bool want = variables::Local::autoTp;
				if (variables::Local::autoTpKey > 0) {
					bool held = GameKeyDown(variables::Local::autoTpKey);
					if (held && !autoTpKeyLatch) {
						variables::Local::autoTp = !variables::Local::autoTp;
						want = variables::Local::autoTp;
						autoTpKeyLatch = true;
					}
					else if (!held) autoTpKeyLatch = false;
				}
				if (want) {
					auto now = std::chrono::steady_clock::now();
					float delay = (std::max)(0.f, variables::Local::autoTpDelay);
					if (delay <= 0.f || std::chrono::duration<float>(now - lastAutoTp).count() >= delay) {
						lastAutoTp = now;
						float bestDist = 1.0e9f;
						uintptr_t bestRoot = 0;
						RBX::Vec3 bestVel{};
						RBX::Vec3 bestPos{};
						bool found = false;
						for (auto& plr : PlayerCache::snapshotPlayers()) {
							if (!plr.isValid || plr.health <= 0.f) continue;
							if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr))
								continue;
							if (plr.distance < bestDist && plr.distance > 0.15f) {
								bestDist = plr.distance;
								bestRoot = plr.rootPartAddr;
								bestVel = plr.velocity;
								bestPos = plr.bones.hasHrp ? plr.bones.hrp : plr.position;
								found = true;
							}
						}
						if (found) {
							// Live HRP + short prediction so moving targets don't pull away
							if (bestRoot) {
								RBX::RbxInstance tgt(bestRoot);
								RBX::Vec3 live = tgt.GetPos();
								if (live.X != 0.f || live.Y != 0.f || live.Z != 0.f)
									bestPos = live;
								auto tprim = tgt.GetPrimitivePtr();
								if (tprim)
									bestVel = memory->read<RBX::Vec3>(tprim + Offsets::Primitive::AssemblyLinearVelocity);
							}
							const float pred = 0.08f;
							bestPos.X += bestVel.X * pred;
							bestPos.Y += bestVel.Y * pred * 0.35f;
							bestPos.Z += bestVel.Z * pred;

							RBX::Vec3 me = rootPart.GetPos();
							RBX::Vec3 dir{ bestPos.X - me.X, 0.f, bestPos.Z - me.Z };
							float len = sqrtf(dir.X * dir.X + dir.Z * dir.Z);
							RBX::Vec3 dest = bestPos;
							// Sit close in front of their facing from us (tight for gun fights)
							const float standOff = 1.6f;
							if (len > 0.05f) {
								dest.X -= (dir.X / len) * standOff;
								dest.Z -= (dir.Z / len) * standOff;
							}
							dest.Y += 1.25f;

							// Double-write sticks better against server correction
							rootPart.SetPos(dest);
							WriteLocalVelocity(rootPart, { 0, 0, 0 });
							rootPart.SetPos(dest);
							auto prim = rootPart.GetPrimitivePtr();
							if (prim)
								memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, { 0, 0, 0 });
						}
					}
				}
			}

			// Rage TP ??? sit behind target (hard to shoot you) + optional health stick
			if (variables::Rage::enabled && variables::Rage::teleport) {
				RBX::Vec3 targetPos{};
				RBX::Vec3 targetLook{};
				uintptr_t targetRoot = 0;
				bool found = false;
				bool haveLook = false;
				if (Aimbot::lockedPlayerAddr != 0) {
					for (auto& plr : PlayerCache::snapshotPlayers()) {
						if (plr.playerAddr == Aimbot::lockedPlayerAddr && plr.isValid && plr.health > 0.f) {
							targetPos = plr.bones.hasHrp ? plr.bones.hrp : plr.position;
							targetRoot = plr.rootPartAddr;
							found = true;
							break;
						}
					}
				}
				if (!found) {
					float bestDist = 1.0e9f;
					for (auto& plr : PlayerCache::snapshotPlayers()) {
						if (!plr.isValid || plr.health <= 0.f) continue;
						if (variables::teamCheck && !PlayerCache::PassesTeamFilter(plr))
							continue;
						if (plr.distance < bestDist) {
							bestDist = plr.distance;
							targetPos = plr.bones.hasHrp ? plr.bones.hrp : plr.position;
							targetRoot = plr.rootPartAddr;
							found = true;
						}
					}
				}
				if (found && targetRoot) {
					RBX::RbxInstance tgt(targetRoot);
					RBX::Vec3 live = tgt.GetPos();
					if (live.X != 0.f || live.Y != 0.f || live.Z != 0.f)
						targetPos = live;
					auto tcf = tgt.GetCFrame();
					RBX::Vec3 look = tcf.GetLookVector();
					// Match camera convention: negate Z-column ??? facing
					look.X = -look.X; look.Y = 0.f; look.Z = -look.Z;
					float flat = sqrtf(look.X * look.X + look.Z * look.Z);
					if (flat > 0.001f) {
						targetLook = { look.X / flat, 0.f, look.Z / flat };
						haveLook = true;
					}
				}
				if (found) {
					float dist = (std::max)(0.5f, variables::Rage::tpDistance);
					RBX::Vec3 dest = targetPos;
					if (variables::Rage::unkillable && haveLook) {
						// Behind their facing so they can't track you
						dest.X -= targetLook.X * dist;
						dest.Z -= targetLook.Z * dist;
						dest.Y += 1.1f;
					}
					else if (Globals::camera.Addr) {
						auto cf = Globals::camera.GetCameraCFrame();
						RBX::Vec3 look = cf.GetLookVector();
						look.X = -look.X; look.Z = -look.Z;
						float flat = sqrtf(look.X * look.X + look.Z * look.Z);
						if (flat > 0.001f) {
							dest.X -= (look.X / flat) * dist;
							dest.Z -= (look.Z / flat) * dist;
						}
						dest.Y += 1.5f;
					}
					else {
						dest.Y += 1.5f;
					}

					rootPart.SetPos(dest);
					WriteLocalVelocity(rootPart, { 0, 0, 0 });
					rootPart.SetPos(dest);

					// Soft noclip while glued to them
					for (auto& ch : character.GetChildList()) {
						std::string cls = ch.GetClass();
						if (cls == "Part" || cls == "MeshPart" || cls == "UnionOperation") {
							auto p = ch.GetPrimitivePtr();
							if (!p) continue;
							uint8_t flags = memory->read<uint8_t>(p + Offsets::Primitive::Flags);
							flags = (uint8_t)(flags & ~Offsets::PrimitiveFlags::CanCollide);
							memory->write<uint8_t>(p + Offsets::Primitive::Flags, flags);
						}
					}

					if (variables::Rage::unkillable && humanoid.Addr) {
						float maxHp = memory->read<float>(humanoid.Addr + Offsets::Humanoid::MaxHealth);
						if (maxHp < 1.f) maxHp = 100.f;
						memory->write<float>(humanoid.Addr + Offsets::Humanoid::Health, maxHp);
					}
				}
			}

			// Fly keybind
			{
				int fk = variables::Local::flyKey;
				if (fk <= 0) fk = 'F';
				bool flyHeld = GameKeyDown(fk);
				if (flyHeld && !flyKeyLatched) {
					if (!variables::Local::flyEnabled)
						variables::Local::flyEnabled = true;
					variables::Local::flyActive = !variables::Local::flyActive;
					flyKeyLatched = true;
				}
				else if (!flyHeld) flyKeyLatched = false;

				if (!variables::Local::flyEnabled)
					variables::Local::flyActive = false;
			}

			if (variables::Local::flyActive && Globals::camera.Addr != 0) {
				auto cf = Globals::camera.GetCameraCFrame();
				RBX::Vec3 look = cf.GetLookVector();
				look.X = -look.X; look.Y = -look.Y; look.Z = -look.Z;
				RBX::Vec3 right = cf.GetRightVector();
				RBX::Vec3 move{ 0, 0, 0 };
				if (GameKeyDown('W')) { move.X += look.X; move.Y += look.Y; move.Z += look.Z; }
				if (GameKeyDown('S')) { move.X -= look.X; move.Y -= look.Y; move.Z -= look.Z; }
				if (GameKeyDown('D')) { move.X += right.X; move.Y += right.Y; move.Z += right.Z; }
				if (GameKeyDown('A')) { move.X -= right.X; move.Y -= right.Y; move.Z -= right.Z; }
				if (GameKeyDown(VK_SPACE)) move.Y += 1.0f;
				if (GameKeyDown(VK_LCONTROL) || GameKeyDown(VK_LSHIFT)) move.Y -= 1.0f;

				float len = sqrtf(move.X * move.X + move.Y * move.Y + move.Z * move.Z);
				float spd = variables::Local::flySpeed;
				if (len > 0.001f) {
					if (variables::Local::flyMethod == 0) {
						WriteLocalVelocity(rootPart, { (move.X / len) * spd, (move.Y / len) * spd, (move.Z / len) * spd });
					}
					else {
						RBX::Vec3 pos = rootPart.GetPos();
						pos.X += (move.X / len) * spd * dt;
						pos.Y += (move.Y / len) * spd * dt;
						pos.Z += (move.Z / len) * spd * dt;
						rootPart.SetPos(pos);
						WriteLocalVelocity(rootPart, { 0, 0, 0 });
					}
				}
				else if (variables::Local::flyMethod == 0) {
					WriteLocalVelocity(rootPart, { 0, 0, 0 });
				}
				auto prim = rootPart.GetPrimitivePtr();
				if (prim)
					memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, { 0, 0, 0 });
			}
			else if (variables::Local::bhopEnabled && GameKeyDown(VK_SPACE) && Globals::camera.Addr != 0) {
				auto cf = Globals::camera.GetCameraCFrame();
				RBX::Vec3 look = cf.GetLookVector();
				look.X = -look.X; look.Z = -look.Z;
				float flat = sqrtf(look.X * look.X + look.Z * look.Z);
				if (flat > 0.001f) {
					auto prim = rootPart.GetPrimitivePtr();
					if (prim) {
						RBX::Vec3 cur = memory->read<RBX::Vec3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
						float spd = variables::Local::bhopSpeed;
						WriteLocalVelocity(rootPart, { (look.X / flat) * spd, cur.Y, (look.Z / flat) * spd });
					}
				}
			}

			// Hitbox extender ? expand while on; restore originals for several frames when off
			// (single-frame restore often lost the race and left giant parts stuck).
			struct HitboxSave {
				rbx::vector3_t size{};
				uint8_t flags = 0;
				bool haveFlags = false;
			};
			static std::unordered_map<uintptr_t, HitboxSave> hitboxOrig;
			static bool hitboxWasOn = false;
			static int hitboxRestoreFrames = 0;

			// Keep both toggles in sync (UI vs keybind / legacy)
			variables::Local::hitboxEnabled = variables::Hitbox::enabled;
			variables::Local::hitboxSize = variables::Hitbox::size;
			variables::Local::visualizeHitbox = variables::Hitbox::visualize;

			const bool hitboxOn = variables::Hitbox::enabled;

			auto restoreHitboxes = [&]() {
				for (auto& kv : hitboxOrig) {
					RBX::RbxInstance part(kv.first);
					auto prim = part.GetPrimitivePtr();
					if (!prim) continue;
					memory->write<rbx::vector3_t>(prim + Offsets::Primitive::Size, kv.second.size);
					// Second write ? engine sometimes ignores the first
					memory->write<rbx::vector3_t>(prim + Offsets::Primitive::Size, kv.second.size);
					if (kv.second.haveFlags)
						memory->write<uint8_t>(prim + Offsets::Primitive::Flags, kv.second.flags);
				}
			};

			if (hitboxOn) {
				hitboxWasOn = true;
				hitboxRestoreFrames = 0;
				float sz = variables::Hitbox::size;
				if (sz < 2.f) sz = 2.f;
				if (sz > 50.f) sz = 50.f;
				rbx::vector3_t ns{ sz, sz * 0.85f, sz };
				rbx::vector3_t headSz{ sz * 0.55f, sz * 0.55f, sz * 0.55f };

				auto expandPart = [&](uintptr_t partAddr, const rbx::vector3_t& want) {
					if (!partAddr) return;
					RBX::RbxInstance part(partAddr);
					auto prim = part.GetPrimitivePtr();
					if (!prim) return;

					if (hitboxOrig.find(partAddr) == hitboxOrig.end()) {
						rbx::vector3_t cur = memory->read<rbx::vector3_t>(prim + Offsets::Primitive::Size);
						// Don't treat an already-expanded part as the "original"
						float mx = cur.x; if (cur.y > mx) mx = cur.y; if (cur.z > mx) mx = cur.z;
						HitboxSave sav{};
						if (mx > 6.5f) {
							// Fallback humanoid defaults
							sav.size = { 2.f, 2.f, 1.f };
						}
						else {
							sav.size = cur;
						}
						sav.flags = memory->read<uint8_t>(prim + Offsets::Primitive::Flags);
						sav.haveFlags = true;
						hitboxOrig[partAddr] = sav;
					}

					part.SetSize(want);
					memory->write<rbx::vector3_t>(prim + Offsets::Primitive::Size, want);
				};

				for (auto& plr : PlayerCache::snapshotPlayers()) {
					if (!plr.isValid || plr.rootPartAddr == 0) continue;
					if (variables::Hitbox::teamCheck && !PlayerCache::PassesTeamFilter(plr)) continue;
					if (variables::Hitbox::healthCheck && plr.health <= 0) continue;

					expandPart(plr.rootPartAddr, ns);
					if (plr.headAddr) expandPart(plr.headAddr, headSz);

					if (variables::Hitbox::type == 1 && plr.characterAddr) {
						RBX::RbxInstance ch(plr.characterAddr);
						const char* parts[] = {
							"Head", "UpperTorso", "LowerTorso", "Torso",
							"HumanoidRootPart", "LeftUpperArm", "RightUpperArm",
							"LeftUpperLeg", "RightUpperLeg"
						};
						for (auto* n : parts) {
							auto p = ch.FindChild(n);
							if (!p.Addr) continue;
							bool isHead = (n[0] == 'H' && n[1] == 'e');
							expandPart(p.Addr, isHead ? headSz : ns);
						}
					}
				}
			}
			else if (hitboxWasOn || hitboxRestoreFrames > 0) {
				if (hitboxWasOn) {
					hitboxWasOn = false;
					hitboxRestoreFrames = 120; // retry restore ~2s so sizes actually stick
				}
				restoreHitboxes();
				hitboxRestoreFrames--;
				if (hitboxRestoreFrames <= 0) {
					hitboxOrig.clear();
					hitboxRestoreFrames = 0;
				}
			}

			static bool lastDesync = false;
			if (Desync::PhysicsSenderMaxBandwidthBps != 0 && variables::Local::desyncEnabled != lastDesync) {
				auto base = memory->get_module_address();
				if (base) {
					auto addr = base + Desync::PhysicsSenderMaxBandwidthBps;
					memory->write<float>(addr, variables::Local::desyncEnabled ? 0.0f : 5.431432847722991e-41f);
					lastDesync = variables::Local::desyncEnabled;
				}
			}

			// Speed key toggle
			static bool speedKeyLatch = false;
			if (variables::Local::speedKey > 0) {
				bool sk = GameKeyDown(variables::Local::speedKey);
				if (sk && !speedKeyLatch) {
					variables::Local::speedEnabled = !variables::Local::speedEnabled;
					speedKeyLatch = true;
				}
				else if (!sk) speedKeyLatch = false;
			}

			// Freeze ??? zero linear/angular velocity
			static bool freezeKeyLatch = false;
			if (variables::Local::freezeKey > 0) {
				bool fk = GameKeyDown(variables::Local::freezeKey);
				if (fk && !freezeKeyLatch) {
					variables::Local::freeze = !variables::Local::freeze;
					freezeKeyLatch = true;
				}
				else if (!fk) freezeKeyLatch = false;
			}
			if (variables::Local::freeze) {
				WriteLocalVelocity(rootPart, { 0, 0, 0 });
				auto prim = rootPart.GetPrimitivePtr();
				if (prim)
					memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity, { 0, 0, 0 });
			}

			// Spin ??? yaw angular velocity
			if (variables::Local::spin && !variables::Local::walkFling) {
				auto prim = rootPart.GetPrimitivePtr();
				if (prim)
					memory->write<RBX::Vec3>(prim + Offsets::Primitive::AssemblyAngularVelocity,
						{ 0, variables::Local::spinSpeed, 0 });
			}

			// Hip height
			if (variables::Local::hipHeightEnabled && humanoid.Addr)
				memory->write<float>(humanoid.Addr + Offsets::Humanoid::HipHeight, variables::Local::hipHeight);

			// Custom gravity (workspace)
			if (variables::Local::gravityEnabled && Globals::dataModel.Addr) {
				auto ws = Globals::dataModel.FindChildByClass("Workspace");
				if (ws.Addr)
					memory->write<float>(ws.Addr + Offsets::World::Gravity, variables::Local::gravity);
			}

			// God mode ??? stick health near max
			if (variables::Local::godMode && humanoid.Addr) {
				float maxHp = memory->read<float>(humanoid.Addr + Offsets::Humanoid::MaxHealth);
				if (maxHp < 1.f) maxHp = 100.f;
				memory->write<float>(humanoid.Addr + Offsets::Humanoid::Health, maxHp);
			}

			// Anti-void ??? teleport up if falling too far
			if (variables::Local::antiVoid) {
				auto pos = rootPart.GetPos();
				if (pos.Y < -50.f) {
					pos.Y = 80.f;
					rootPart.SetPos(pos);
				}
			}

			// TP walk ??? nudge HRP while holding WASD
			if (variables::Local::tpWalk && Globals::camera.Addr) {
				auto cf = Globals::camera.GetCameraCFrame();
				RBX::Vec3 look = cf.GetLookVector();
				look.Y = 0;
				float flat = sqrtf(look.X * look.X + look.Z * look.Z);
				if (flat > 0.001f) { look.X /= flat; look.Z /= flat; }
				RBX::Vec3 right = cf.GetRightVector();
				RBX::Vec3 step{};
				float amt = variables::Local::tpWalkStep;
				if (GameKeyDown('W')) { step.X += look.X * amt; step.Z += look.Z * amt; }
				if (GameKeyDown('S')) { step.X -= look.X * amt; step.Z -= look.Z * amt; }
				if (GameKeyDown('D')) { step.X += right.X * amt; step.Z += right.Z * amt; }
				if (GameKeyDown('A')) { step.X -= right.X * amt; step.Z -= right.Z * amt; }
				if (step.X != 0.f || step.Z != 0.f) {
					auto pos = rootPart.GetPos();
					pos.X += step.X; pos.Z += step.Z;
					rootPart.SetPos(pos);
				}
			}

			// Auto clicker
			if (variables::Local::autoClicker) {
				bool keyOk = variables::Local::autoClickerKey == 0 ||
					GameKeyDown(variables::Local::autoClickerKey);
				static auto lastClick = std::chrono::steady_clock::now();
				float interval = 1.f / (std::max)(1.f, variables::Local::autoClickerCps);
				if (keyOk && std::chrono::duration<float>(now - lastClick).count() >= interval) {
					lastClick = now;
					INPUT in[2] = {};
					in[0].type = INPUT_MOUSE; in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					in[1].type = INPUT_MOUSE; in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
					SendInput(2, in, sizeof(INPUT));
				}
			}

			// Orbit locked / nearest enemy
			if (variables::Local::orbitPlayer && rootPart.Addr) {
				uintptr_t target = Aimbot::lockedPlayerAddr;
				RBX::Vec3 tpos{};
				bool have = false;
				auto snap = PlayerCache::snapshotPlayers();
				for (auto& p : snap) {
					if (!p.isValid || p.health <= 0.f) continue;
					if (target && p.playerAddr == target) {
						tpos = p.bones.hasHrp ? p.bones.hrp : p.position;
						have = true;
						break;
					}
				}
				if (!have) {
					float best = 1e9f;
					for (auto& p : snap) {
						if (!p.isValid || p.health <= 0.f) continue;
						if (variables::teamCheck && !PlayerCache::PassesTeamFilter(p)) continue;
						if (p.distance < best) {
							best = p.distance;
							tpos = p.bones.hasHrp ? p.bones.hrp : p.position;
							have = true;
						}
					}
				}
				if (have) {
					static float orbitAng = 0.f;
					orbitAng += variables::Local::orbitSpeed * 0.016f;
					float r = variables::Local::orbitRadius;
					RBX::Vec3 pos = rootPart.GetPos();
					pos.X = tpos.X + cosf(orbitAng) * r;
					pos.Z = tpos.Z + sinf(orbitAng) * r;
					pos.Y = tpos.Y + 1.5f;
					rootPart.SetPos(pos);
				}
			}

			// Sit spam ??? toggle Humanoid.Sit
			if (variables::Local::sitSpam && humanoid.Addr) {
				static auto lastSit = std::chrono::steady_clock::now();
				if (std::chrono::duration<float, std::milli>(now - lastSit).count() >= 120.f) {
					lastSit = now;
					static bool sitFlip = false;
					sitFlip = !sitFlip;
					memory->write<uint8_t>(humanoid.Addr + Offsets::Humanoid::Sit, sitFlip ? 1 : 0);
				}
			}

			// Vehicle boost ??? raise MaxSpeed on occupied VehicleSeat
			if (variables::Local::vehicleBoost && humanoid.Addr) {
				uintptr_t seat = memory->read<uintptr_t>(humanoid.Addr + Offsets::Humanoid::SeatPart);
				if (seat) {
					memory->write<float>(seat + Offsets::VehicleSeat::MaxSpeed, variables::Local::vehicleBoostAmt);
					memory->write<float>(seat + Offsets::VehicleSeat::Torque, variables::Local::vehicleBoostAmt * 2.f);
				}
			}

			// Unlock / third-person zoom ? restore when both off
			{
				static float maxZoomOrig = 128.f, minZoomOrig = 0.5f;
				static bool zoomWasOn = false;
				const bool wantZoom = (variables::World::unlockZoom || variables::World::thirdPerson) && Globals::localPlayer.Addr;
				if (wantZoom) {
					if (!zoomWasOn) {
						maxZoomOrig = memory->read<float>(Globals::localPlayer.Addr + Offsets::Player::MaxZoomDistance);
						minZoomOrig = memory->read<float>(Globals::localPlayer.Addr + Offsets::Player::MinZoomDistance);
						zoomWasOn = true;
					}
					if (variables::World::thirdPerson) {
						memory->write<float>(Globals::localPlayer.Addr + Offsets::Player::MaxZoomDistance,
							(std::max)(variables::World::thirdPersonDistance, 8.f));
						memory->write<float>(Globals::localPlayer.Addr + Offsets::Player::MinZoomDistance, 0.5f);
					}
					else if (variables::World::unlockZoom) {
						memory->write<float>(Globals::localPlayer.Addr + Offsets::Player::MaxZoomDistance, variables::World::maxZoom);
					}
				}
				else if (zoomWasOn && Globals::localPlayer.Addr) {
					memory->write<float>(Globals::localPlayer.Addr + Offsets::Player::MaxZoomDistance, maxZoomOrig);
					memory->write<float>(Globals::localPlayer.Addr + Offsets::Player::MinZoomDistance, minZoomOrig);
					zoomWasOn = false;
				}
			}

			// Anti-AFK / AFK Assist ??? keep Roblox from idling when you walk away
			if (variables::Misc::antiAfk || variables::Misc::afkAssist) {
				static auto lastAfk = std::chrono::steady_clock::now();
				static POINT lastPt{ 0, 0 };
				static int pulse = 0;
				POINT pt{};
				GetCursorPos(&pt);
				bool moved = (pt.x != lastPt.x || pt.y != lastPt.y);
				lastPt = pt;
				auto nowAfk = std::chrono::steady_clock::now();
				if (moved)
					lastAfk = nowAfk;
				float interval = variables::Misc::antiAfkSeconds;
				if (interval < 5.f) interval = 5.f;
				if (variables::Misc::afkAssist && interval > 14.f) interval = 14.f;
				if (std::chrono::duration<float>(nowAfk - lastAfk).count() > interval) {
					INPUT in = {};
					in.type = INPUT_MOUSE;
					in.mi.dwFlags = MOUSEEVENTF_MOVE;
					in.mi.dx = (pulse % 2 == 0) ? 1 : -1;
					in.mi.dy = (pulse % 3 == 0) ? 1 : 0;
					SendInput(1, &in, sizeof(INPUT));
					in.mi.dx = -in.mi.dx;
					in.mi.dy = -in.mi.dy;
					SendInput(1, &in, sizeof(INPUT));

					INPUT keys[2] = {};
					keys[0].type = INPUT_KEYBOARD;
					keys[0].ki.wVk = VK_SHIFT;
					keys[1].type = INPUT_KEYBOARD;
					keys[1].ki.wVk = VK_SHIFT;
					keys[1].ki.dwFlags = KEYEVENTF_KEYUP;
					SendInput(2, keys, sizeof(INPUT));

					pulse++;
					lastAfk = nowAfk;
				}
			}

			// Keep exploit loop tight while Auto TP is sticking to a target
			if (variables::Local::autoTp)
				std::this_thread::sleep_for(std::chrono::milliseconds(0));
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(4));
		}
	}

	bool IsInActiveGame()
	{
		if (!Globals::dataModel.Addr || !Globals::players.Addr)
			return false;

		const int64_t placeId = memory->read<int64_t>(Globals::dataModel.Addr + Offsets::DataModel::PlaceId);
		if (placeId <= 0)
			return false;

		const uint8_t loaded = memory->read<uint8_t>(Globals::dataModel.Addr + Offsets::DataModel::GameLoaded);
		if (loaded == 0)
			return false;

		const uintptr_t localPlayerAddr = memory->read<uintptr_t>(Globals::players.Addr + Offsets::Player::LocalPlayer);
		if (!localPlayerAddr)
			return false;

		Globals::localPlayer = RBX::RbxInstance(localPlayerAddr);
		const uintptr_t character = memory->read<uintptr_t>(localPlayerAddr + Offsets::Player::ModelInstance);
		if (!character)
			return false;

		// Supported FPS places only (Arsenal, BloxStrike, MiscGunTest, Baseplate)
		if (!Arsenal::IsSupportedPlace())
			return false;

		return true;
	}

	bool FailAttach(const char* msg)
	{
		variables::Loading::failed = true;
		variables::Loading::active = false;
		strncpy_s(variables::Loading::error, msg, _TRUNCATE);
		Telemetry::ReportError("Attach failed", msg);
		return false;
	}

	bool AttachGame()
	{
		SetLoad(0.10f, "Connecting to Roblox");
		int tries = 0;
		while (!memory->find_process_id(app)) {
			if (++tries > 40)
				return FailAttach("Roblox not found. Open Roblox, join a supported game, then restart.");
			SetLoad(0.10f + (tries % 20) * 0.01f, "Waiting for Roblox");
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		SetLoad(0.25f, "Attaching process");
		if (!memory->attach_to_process(app) || !memory->find_module_address(app))
			return FailAttach("Failed to attach to Roblox.");

		SetLoad(0.32f, "Updating offsets (Theo)");
		OffsetAuto::EnsureLatest();
		SetLoad(0.40f, OffsetAuto::status[0] ? OffsetAuto::status : "Resolving offsets");

		const auto anchors = Scanner::ResolveAnchors();
		if (!anchors.success) {
			SetLoad(0.42f, "Refreshing Theo offsets?");
			OffsetAuto::ForceRefresh();
			const auto retry = Scanner::ResolveAnchors();
			if (!retry.success)
				return FailAttach("Offset resolve failed. Wait for Theo dump / update Roblox.");
			Globals::dataModel = RBX::RbxInstance(retry.dataModel);
			Globals::renderEngine = RBX::RenderEngine(retry.visualEngine);
		} else {
			Globals::dataModel = RBX::RbxInstance(anchors.dataModel);
			Globals::renderEngine = RBX::RenderEngine(anchors.visualEngine);
		}
		Globals::workspace = Globals::dataModel.FindChildByClass("Workspace");
		Globals::players = Globals::dataModel.FindChildByClass("Players");
		Globals::camera = Globals::workspace.FindChildByClass("Camera");

		if (Globals::players.Addr == 0)
			return FailAttach("Players service missing. Join a supported game first.");

		SetLoad(0.55f, "Waiting for supported game");
		tries = 0;
		while (!IsInActiveGame()) {
			if (!memory->find_process_id(app))
				return FailAttach("Roblox closed while waiting for a supported game.");

			const int64_t placeId = Globals::dataModel.Addr
				? memory->read<int64_t>(Globals::dataModel.Addr + Offsets::DataModel::PlaceId) : 0;
			const uint8_t loaded = Globals::dataModel.Addr
				? memory->read<uint8_t>(Globals::dataModel.Addr + Offsets::DataModel::GameLoaded) : 0;
			if (placeId > 0 && loaded != 0 && !Arsenal::IsSupportedPlace())
				return FailAttach("Unsupported game. Join Arsenal, BloxStrike, Counter Blox, MiscGunTest:X, or Baseplate, then restart.");

			if (++tries > 600)
				return FailAttach("Timed out. Join a supported game, spawn in, then restart.");
			if ((tries % 10) == 0) {
				Globals::workspace = Globals::dataModel.FindChildByClass("Workspace");
				Globals::players = Globals::dataModel.FindChildByClass("Players");
				if (Globals::workspace.Addr)
					Globals::camera = Globals::workspace.FindChildByClass("Camera");
				if (!Globals::dataModel.Addr || !Globals::players.Addr) {
					const auto again = Scanner::ResolveAnchors();
					if (again.success) {
						Globals::dataModel = RBX::RbxInstance(again.dataModel);
						Globals::renderEngine = RBX::RenderEngine(again.visualEngine);
						Globals::workspace = Globals::dataModel.FindChildByClass("Workspace");
						Globals::players = Globals::dataModel.FindChildByClass("Players");
						if (Globals::workspace.Addr)
							Globals::camera = Globals::workspace.FindChildByClass("Camera");
					}
				}
			}
			float pulse = 0.55f + 0.25f * ((tries % 20) / 20.0f);
			char waitMsg[96];
			sprintf_s(waitMsg, "Join %s", Games::SupportedListShort());
			SetLoad(pulse, waitMsg);
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		char readyMsg[96];
		sprintf_s(readyMsg, "Loaded %s", Games::Name());

		const float minHold = (std::max)(0.45f, variables::Loading::minSeconds);
		auto holdStart = std::chrono::steady_clock::now();
		float elapsed = 0.f;
		while (elapsed < minHold) {
			elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - holdStart).count();
			float t = elapsed / minHold;
			if (t > 1.f) t = 1.f;
			float p = 0.88f + 0.12f * t;
			SetLoad(p, readyMsg);
			std::this_thread::sleep_for(std::chrono::milliseconds(40));
		}

		Globals::workspace = Globals::dataModel.FindChildByClass("Workspace");
		Globals::players = Globals::dataModel.FindChildByClass("Players");
		if (Globals::workspace.Addr)
			Globals::camera = Globals::workspace.FindChildByClass("Camera");

		SetLoad(1.0f, readyMsg);
		std::this_thread::sleep_for(std::chrono::milliseconds(280));
		return true;
	}
}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	FreeConsole();

	SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* ep) -> LONG {
		char detail[160]{};
		DWORD code = ep && ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionCode : 0;
		sprintf_s(detail, "Unhandled exception 0x%08lX", code);
		Telemetry::ReportError("Crash", detail);
		Sleep(400);
		TerminateProcess(GetCurrentProcess(), 1);
		return EXCEPTION_EXECUTE_HANDLER;
	});

	OverlayWindow overlay;
	if (!overlay.Initialize())
		return 1;

	std::atomic<bool> attachSucceeded{ false };
	std::thread attachThread;
	bool attachStarted = false;

	std::thread tpThread;
	std::thread locThread;
	std::thread animThread;
	bool workersStarted = false;
	int frameCounter = 0;

	while (Globals::running) {
		// Throttle expensive process scans ??? window check is cheap
		static auto lastAliveCheck = std::chrono::steady_clock::now() - std::chrono::seconds(2);
		auto aliveNow = std::chrono::steady_clock::now();
		if (std::chrono::duration<float>(aliveNow - lastAliveCheck).count() >= 0.75f) {
			lastAliveCheck = aliveNow;
			if (!WindowManager::FindRobloxHwnd() && !memory->find_process_id(app))
				break;
		} else if (!WindowManager::IsRobloxOpen() && memory->get_process_id() == 0) {
			if (!WindowManager::FindRobloxHwnd() && !memory->find_process_id(app))
				break;
		}

		if (variables::Loading::failed) {
			overlay.BeginFrame();
			overlay.RenderLoading();
			overlay.EndFrame();
			if (GetAsyncKeyState(VK_ESCAPE) & 1) break;
			continue;
		}

		// Key gate removed ??? open source build starts session load immediately

		if (!attachStarted) {
			attachStarted = true;
			variables::Loading::active = true;
			SetLoad(0.0f, "Loading session");
			attachThread = std::thread([&]() {
		if (AttachGame()) {
					attachSucceeded.store(true);
					variables::Loading::active = false;
					variables::Theme::useFloatingHeader = false;
					variables::Misc::floatingPanelOpen = false;
					variables::menuOpen = true;
					if (Games::Detect() == Games::Kind::MiscGunTest) {
						variables::Hitbox::enabled = false;
						variables::Local::hitboxEnabled = false;
						variables::Toast::show = true;
						variables::Toast::warning = true;
						variables::Toast::timer = 10.0f;
						strcpy_s(variables::Toast::title, "MiscGunTest:X warning");
						strcpy_s(variables::Toast::body, "Do NOT use Hitbox Extender");
						strcpy_s(variables::Toast::footer, "You will be banned");
					}
					if (Games::IsMeshFps()) {
						EnforceMeshFpsProfile();
						GunMods::DisableAll();
					}
				}
			});
		}

		if (attachSucceeded.load() && !variables::Loading::active && !workersStarted) {
			tpThread = std::thread(Core::tp_handler::thread);
			locThread = std::thread(localThread);
			animThread = std::thread(animation_changer);
			workersStarted = true;
		}

		int menuVk = variables::Misc::menuVk;
		const bool gameFocused = WindowManager::IsRobloxFocused();
		if (gameFocused && ((GetAsyncKeyState(menuVk) & 1) || (GetAsyncKeyState(VK_INSERT) & 1) || (GetAsyncKeyState(VK_RCONTROL) & 1))) {
			if (!variables::Loading::active && !Telemetry::consentPending.load()) {
				variables::Misc::floatingPanelOpen = false;
				variables::menuOpen = !variables::menuOpen;
			}
		}
		if (gameFocused && variables::Misc::panicKey && (GetAsyncKeyState(VK_DELETE) & 1))
			PanicDisableAll();
		if (gameFocused && variables::Misc::ejectBind && variables::Misc::ejectVk &&
			(GetAsyncKeyState(variables::Misc::ejectVk) & 1))
			FadeEject::Request();

		overlay.BeginFrame();

		if (variables::Loading::active) {
			overlay.RenderLoading();
			overlay.EndFrame();
			continue;
		}

		static bool telemetryReady = false;
		if (!telemetryReady) {
			Telemetry::OnReady();
			telemetryReady = true;
		}

		const bool robloxFocused = WindowManager::IsRobloxFocused();
		const bool forceUi = Telemetry::consentPending.load();

		// When Roblox isn't focused, hide ESP + all GUIs (Matcha behavior)
		if (!robloxFocused && !forceUi) {
			overlay.EndFrame();
			continue;
		}

		overlay.RenderMenu();
		overlay.RenderSpotifyMini();
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();
		overlay.render(drawList);

		if (Globals::renderEngine.Addr != 0) {
			// Soft-gate: if they leave a supported game mid-session, stop cheats cleanly
			static bool warnedLeave = false;
			if (!Arsenal::IsSupportedPlace()) {
				if (!warnedLeave) {
					GunMods::DisableAll();
					Aimbot::lockedPlayerAddr = 0;
					variables::Aimbot::enabled = false;
					variables::ESP::enabled = false;
					variables::Toast::show = true;
					variables::Toast::warning = true;
					variables::Toast::timer = 6.0f;
					strcpy_s(variables::Toast::title, "Unsupported place");
					strcpy_s(variables::Toast::body, "Aim / ESP paused - rejoin a supported game");
					strcpy_s(variables::Toast::footer, "Fade");
					warnedLeave = true;
				}
			} else {
				warnedLeave = false;
				PlayerCache::EnsureCacheWorker();
				PlayerCache::refreshLivePositions();

				auto viewMatrix = Globals::renderEngine.GetViewMat();
				auto players = PlayerCache::snapshotPlayers();
				Aimbot::RunAimbot(viewMatrix, players);
				Aimbot::RunTriggerbot(viewMatrix, players);
				Aimbot::RunMeleeAura(players);
				Visuals::RenderESP(drawList, viewMatrix, players);
				CombatFx::Tick(players, viewMatrix);
				CombatFx::Draw(drawList, ImGui::GetIO().DeltaTime);

				// Overlay extras
				if (variables::Misc::enemyCounter) {
					int enemies = 0;
					for (auto& p : players) {
						if (p.health > 0) enemies++;
					}
					char buf[48];
					sprintf_s(buf, "Enemies %d", enemies);
					drawList->AddText(ImVec2(18, 52), IM_COL32(240, 240, 245, 230), buf);
				}
				if (variables::Misc::targetHud && Aimbot::lockedPlayerAddr) {
					for (auto& p : players) {
						if (p.playerAddr != Aimbot::lockedPlayerAddr) continue;
						char buf[96];
						sprintf_s(buf, "Target  %s  %.0f HP", p.name.c_str(), p.health);
						drawList->AddRectFilled(ImVec2(14, 72), ImVec2(14 + 220, 98), IM_COL32(10, 10, 12, 180), 6.f);
						drawList->AddText(ImVec2(22, 78), IM_COL32(240, 240, 245, 255), buf);
						break;
					}
				}
			}
		}

		overlay.EndFrame();
	}

	Globals::running = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(150));
	if (attachThread.joinable()) attachThread.join();
	if (tpThread.joinable()) tpThread.join();
	if (locThread.joinable()) locThread.join();
	if (animThread.joinable()) animThread.join();
	overlay.Cleanup();
	return 0;
}
