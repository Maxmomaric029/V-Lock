#include "offset_updater.h"
#include "offsets.h"
#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <thread>
#include <chrono>

#pragma comment(lib, "wininet.lib")

namespace offset_updater
{
    std::string get_local_version_hash()
    {
        HANDLE hFile = CreateFileA("offsets_version.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return "";

        char buffer[1024];
        DWORD bytesRead;
        if (!ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
        {
            CloseHandle(hFile);
            return "";
        }
        CloseHandle(hFile);
        buffer[bytesRead] = '\0';
        return std::string(buffer);
    }

    void save_local_version_hash(const std::string& hash)
    {
        HANDLE hFile = CreateFileA("offsets_version.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return;

        DWORD bytesWritten;
        WriteFile(hFile, hash.c_str(), hash.length(), &bytesWritten, NULL);
        CloseHandle(hFile);
    }

    // Simple FNV-1a hash for string comparison
    std::string calculate_string_hash(const std::string& str)
    {
        uint64_t hash = 14695981039346656037ULL;
        for (char c : str)
        {
            hash ^= (uint64_t)c;
            hash *= 1099511628211ULL;
        }
        return std::to_string(hash);
    }

    std::string download_offsets_json()
    {
        HINTERNET hInternet = InternetOpenA("Vertex Updater", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) return "";

        HINTERNET hConnect = InternetOpenUrlA(hInternet, "https://imtheo.lol/Offsets/Offsets.json", NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hConnect)
        {
            InternetCloseHandle(hInternet);
            return "";
        }

        std::string result;
        char buffer[4096];
        DWORD bytesRead;

        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
        {
            result.append(buffer, bytesRead);
        }

        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);

        return result;
    }

    uintptr_t parse_offset(const std::string& json, const std::string& parent, const std::string& key, uintptr_t fallback)
    {
        std::string parent_key = "\"" + parent + "\"";
        size_t parent_pos = json.find(parent_key);
        if (parent_pos == std::string::npos) return fallback;

        size_t block_start = json.find('{', parent_pos);
        size_t block_end = json.find('}', block_start);
        if (block_start == std::string::npos || block_end == std::string::npos) return fallback;

        std::string block = json.substr(block_start, block_end - block_start);
        
        std::string child_key = "\"" + key + "\"";
        size_t key_pos = block.find(child_key);
        if (key_pos == std::string::npos) return fallback;

        size_t colon_pos = block.find(':', key_pos);
        if (colon_pos == std::string::npos) return fallback;

        size_t val_start = colon_pos + 1;
        while (val_start < block.size() && (block[val_start] == ' ' || block[val_start] == '\t' || block[val_start] == '\n' || block[val_start] == '\r'))
            val_start++;

        size_t val_end = val_start;
        while (val_end < block.size() && isdigit(block[val_end]))
            val_end++;

        if (val_start == val_end) return fallback;

        try {
            return std::stoull(block.substr(val_start, val_end - val_start));
        } catch (...) {
            return fallback;
        }
    }

#define UPDATE_OFFSET(ns, key) Offsets::ns::key = parse_offset(json, #ns, #key, Offsets::ns::key)

    bool update_offsets()
    {
        std::string json = download_offsets_json();
        if (json.empty())
        {
            // If download fails, we return false so main.cpp knows we're using cached/fallback values
            return false;
        }

        std::string remote_hash = calculate_string_hash(json);
        std::string local_hash = get_local_version_hash();

        if (remote_hash == local_hash)
        {
            // Already up to date, skip parsing
            return true;
        }

        printf("[Updater] New offsets detected. Syncing...\n");

        UPDATE_OFFSET(AirProperties, AirDensity);
        UPDATE_OFFSET(AirProperties, GlobalWind);
        UPDATE_OFFSET(AnimationTrack, Animation);
        UPDATE_OFFSET(AnimationTrack, Animator);
        UPDATE_OFFSET(AnimationTrack, IsPlaying);
        UPDATE_OFFSET(AnimationTrack, Looped);
        UPDATE_OFFSET(AnimationTrack, Speed);
        UPDATE_OFFSET(AnimationTrack, TimePosition);
        UPDATE_OFFSET(Animator, ActiveAnimations);
        UPDATE_OFFSET(Atmosphere, Color);
        UPDATE_OFFSET(Atmosphere, Decay);
        UPDATE_OFFSET(Atmosphere, Density);
        UPDATE_OFFSET(Atmosphere, Glare);
        UPDATE_OFFSET(Atmosphere, Haze);
        UPDATE_OFFSET(Atmosphere, Offset);
        UPDATE_OFFSET(Attachment, Position);
        UPDATE_OFFSET(BasePart, CastShadow);
        UPDATE_OFFSET(BasePart, Color3);
        UPDATE_OFFSET(BasePart, Locked);
        UPDATE_OFFSET(BasePart, Massless);
        UPDATE_OFFSET(BasePart, Primitive);
        UPDATE_OFFSET(BasePart, Reflectance);
        UPDATE_OFFSET(BasePart, Shape);
        UPDATE_OFFSET(BasePart, Transparency);
        UPDATE_OFFSET(Beam, Attachment0);
        UPDATE_OFFSET(Beam, Attachment1);
        UPDATE_OFFSET(Beam, Brightness);
        UPDATE_OFFSET(Beam, CurveSize0);
        UPDATE_OFFSET(Beam, CurveSize1);
        UPDATE_OFFSET(Beam, LightEmission);
        UPDATE_OFFSET(Beam, LightInfluence);
        UPDATE_OFFSET(Beam, Texture);
        UPDATE_OFFSET(Beam, TextureLength);
        UPDATE_OFFSET(Beam, TextureSpeed);
        UPDATE_OFFSET(Beam, Width0);
        UPDATE_OFFSET(Beam, Width1);
        UPDATE_OFFSET(Beam, ZOffset);
        UPDATE_OFFSET(BloomEffect, Enabled);
        UPDATE_OFFSET(BloomEffect, Intensity);
        UPDATE_OFFSET(BloomEffect, Size);
        UPDATE_OFFSET(BloomEffect, Threshold);
        UPDATE_OFFSET(BlurEffect, Enabled);
        UPDATE_OFFSET(BlurEffect, Size);
        UPDATE_OFFSET(ByteCode, Pointer);
        UPDATE_OFFSET(ByteCode, Size);
        UPDATE_OFFSET(Camera, CameraSubject);
        UPDATE_OFFSET(Camera, CameraType);
        UPDATE_OFFSET(Camera, FieldOfView);
        UPDATE_OFFSET(Camera, ImagePlaneDepth);
        UPDATE_OFFSET(Camera, Position);
        UPDATE_OFFSET(Camera, Rotation);
        UPDATE_OFFSET(Camera, Viewport);
        UPDATE_OFFSET(Camera, ViewportSize);
        UPDATE_OFFSET(CharacterMesh, BaseTextureId);
        UPDATE_OFFSET(CharacterMesh, BodyPart);
        UPDATE_OFFSET(CharacterMesh, MeshId);
        UPDATE_OFFSET(CharacterMesh, OverlayTextureId);
        UPDATE_OFFSET(ClickDetector, MaxActivationDistance);
        UPDATE_OFFSET(ClickDetector, MouseIcon);
        UPDATE_OFFSET(Clothing, Color3);
        UPDATE_OFFSET(Clothing, Template);
        UPDATE_OFFSET(ColorCorrectionEffect, Brightness);
        UPDATE_OFFSET(ColorCorrectionEffect, Contrast);
        UPDATE_OFFSET(ColorCorrectionEffect, Enabled);
        UPDATE_OFFSET(ColorCorrectionEffect, TintColor);
        UPDATE_OFFSET(ColorGradingEffect, Enabled);
        UPDATE_OFFSET(ColorGradingEffect, TonemapperPreset);
        UPDATE_OFFSET(DataModel, CreatorId);
        UPDATE_OFFSET(DataModel, GameId);
        UPDATE_OFFSET(DataModel, GameLoaded);
        UPDATE_OFFSET(DataModel, JobId);
        UPDATE_OFFSET(DataModel, PlaceId);
        UPDATE_OFFSET(DataModel, PlaceVersion);
        UPDATE_OFFSET(DataModel, PrimitiveCount);
        UPDATE_OFFSET(DataModel, ScriptContext);
        UPDATE_OFFSET(DataModel, ServerIP);
        UPDATE_OFFSET(DataModel, ToRenderView1);
        UPDATE_OFFSET(DataModel, ToRenderView2);
        UPDATE_OFFSET(DataModel, ToRenderView3);
        UPDATE_OFFSET(DataModel, Workspace);
        UPDATE_OFFSET(DepthOfFieldEffect, Enabled);
        UPDATE_OFFSET(DepthOfFieldEffect, FarIntensity);
        UPDATE_OFFSET(DepthOfFieldEffect, FocusDistance);
        UPDATE_OFFSET(DepthOfFieldEffect, InFocusRadius);
        UPDATE_OFFSET(DepthOfFieldEffect, NearIntensity);
        UPDATE_OFFSET(DragDetector, ActivatedCursorIcon);
        UPDATE_OFFSET(DragDetector, CursorIcon);
        UPDATE_OFFSET(DragDetector, MaxActivationDistance);
        UPDATE_OFFSET(DragDetector, MaxDragAngle);
        UPDATE_OFFSET(DragDetector, MaxDragTranslation);
        UPDATE_OFFSET(DragDetector, MaxForce);
        UPDATE_OFFSET(DragDetector, MaxTorque);
        UPDATE_OFFSET(DragDetector, MinDragAngle);
        UPDATE_OFFSET(DragDetector, MinDragTranslation);
        UPDATE_OFFSET(DragDetector, ReferenceInstance);
        UPDATE_OFFSET(DragDetector, Responsiveness);
        UPDATE_OFFSET(FakeDataModel, Pointer);
        UPDATE_OFFSET(FakeDataModel, RealDataModel);
        UPDATE_OFFSET(GuiBase2D, AbsolutePosition);
        UPDATE_OFFSET(GuiBase2D, AbsoluteRotation);
        UPDATE_OFFSET(GuiBase2D, AbsoluteSize);
        UPDATE_OFFSET(GuiObject, BackgroundColor3);
        UPDATE_OFFSET(GuiObject, BackgroundTransparency);
        UPDATE_OFFSET(GuiObject, BorderColor3);
        UPDATE_OFFSET(GuiObject, Image);
        UPDATE_OFFSET(GuiObject, LayoutOrder);
        UPDATE_OFFSET(GuiObject, Position);
        UPDATE_OFFSET(GuiObject, RichText);
        UPDATE_OFFSET(GuiObject, Rotation);
        UPDATE_OFFSET(GuiObject, ScreenGui_Enabled);
        UPDATE_OFFSET(GuiObject, Size);
        UPDATE_OFFSET(GuiObject, Text);
        UPDATE_OFFSET(GuiObject, TextColor3);
        UPDATE_OFFSET(GuiObject, Visible);
        UPDATE_OFFSET(GuiObject, ZIndex);
        UPDATE_OFFSET(Humanoid, AutoJumpEnabled);
        UPDATE_OFFSET(Humanoid, AutoRotate);
        UPDATE_OFFSET(Humanoid, AutomaticScalingEnabled);
        UPDATE_OFFSET(Humanoid, BreakJointsOnDeath);
        UPDATE_OFFSET(Humanoid, CameraOffset);
        UPDATE_OFFSET(Humanoid, DisplayDistanceType);
        UPDATE_OFFSET(Humanoid, DisplayName);
        UPDATE_OFFSET(Humanoid, EvaluateStateMachine);
        UPDATE_OFFSET(Humanoid, FloorMaterial);
        UPDATE_OFFSET(Humanoid, Health);
        UPDATE_OFFSET(Humanoid, HealthDisplayDistance);
        UPDATE_OFFSET(Humanoid, HealthDisplayType);
        UPDATE_OFFSET(Humanoid, HipHeight);
        UPDATE_OFFSET(Humanoid, HumanoidRootPart);
        UPDATE_OFFSET(Humanoid, HumanoidState);
        UPDATE_OFFSET(Humanoid, HumanoidStateID);
        UPDATE_OFFSET(Humanoid, IsWalking);
        UPDATE_OFFSET(Humanoid, Jump);
        UPDATE_OFFSET(Humanoid, JumpHeight);
        UPDATE_OFFSET(Humanoid, JumpPower);
        UPDATE_OFFSET(Humanoid, MaxHealth);
        UPDATE_OFFSET(Humanoid, MaxSlopeAngle);
        UPDATE_OFFSET(Humanoid, MoveDirection);
        UPDATE_OFFSET(Humanoid, MoveToPart);
        UPDATE_OFFSET(Humanoid, MoveToPoint);
        UPDATE_OFFSET(Humanoid, NameDisplayDistance);
        UPDATE_OFFSET(Humanoid, NameOcclusion);
        UPDATE_OFFSET(Humanoid, PlatformStand);
        UPDATE_OFFSET(Humanoid, RequiresNeck);
        UPDATE_OFFSET(Humanoid, RigType);
        UPDATE_OFFSET(Humanoid, SeatPart);
        UPDATE_OFFSET(Humanoid, Sit);
        UPDATE_OFFSET(Humanoid, TargetPoint);
        UPDATE_OFFSET(Humanoid, UseJumpPower);
        UPDATE_OFFSET(Humanoid, WalkTimer);
        UPDATE_OFFSET(Humanoid, Walkspeed);
        UPDATE_OFFSET(Humanoid, WalkspeedCheck);
        UPDATE_OFFSET(Instance, AttributeContainer);
        UPDATE_OFFSET(Instance, AttributeList);
        UPDATE_OFFSET(Instance, AttributeToNext);
        UPDATE_OFFSET(Instance, AttributeToValue);
        UPDATE_OFFSET(Instance, ChildrenEnd);
        UPDATE_OFFSET(Instance, ChildrenStart);
        UPDATE_OFFSET(Instance, ClassBase);
        UPDATE_OFFSET(Instance, ClassDescriptor);
        UPDATE_OFFSET(Instance, ClassName);
        UPDATE_OFFSET(Instance, Name);
        UPDATE_OFFSET(Instance, Parent);
        UPDATE_OFFSET(Instance, This);
        UPDATE_OFFSET(Lighting, Ambient);
        UPDATE_OFFSET(Lighting, Brightness);
        UPDATE_OFFSET(Lighting, ClockTime);
        UPDATE_OFFSET(Lighting, ColorShift_Bottom);
        UPDATE_OFFSET(Lighting, ColorShift_Top);
        UPDATE_OFFSET(Lighting, EnvironmentDiffuseScale);
        UPDATE_OFFSET(Lighting, EnvironmentSpecularScale);
        UPDATE_OFFSET(Lighting, ExposureCompensation);
        UPDATE_OFFSET(Lighting, FogColor);
        UPDATE_OFFSET(Lighting, FogEnd);
        UPDATE_OFFSET(Lighting, FogStart);
        UPDATE_OFFSET(Lighting, GeographicLatitude);
        UPDATE_OFFSET(Lighting, GlobalShadows);
        UPDATE_OFFSET(Lighting, GradientBottom);
        UPDATE_OFFSET(Lighting, GradientTop);
        UPDATE_OFFSET(Lighting, LightColor);
        UPDATE_OFFSET(Lighting, LightDirection);
        UPDATE_OFFSET(Lighting, MoonPosition);
        UPDATE_OFFSET(Lighting, OutdoorAmbient);
        UPDATE_OFFSET(Lighting, Sky);
        UPDATE_OFFSET(Lighting, Source);
        UPDATE_OFFSET(Lighting, SunPosition);
        UPDATE_OFFSET(LocalScript, ByteCode);
        UPDATE_OFFSET(LocalScript, GUID);
        UPDATE_OFFSET(LocalScript, Hash);
        UPDATE_OFFSET(MaterialColors, Asphalt);
        UPDATE_OFFSET(MaterialColors, Basalt);
        UPDATE_OFFSET(MaterialColors, Brick);
        UPDATE_OFFSET(MaterialColors, Cobblestone);
        UPDATE_OFFSET(MaterialColors, Concrete);
        UPDATE_OFFSET(MaterialColors, CrackedLava);
        UPDATE_OFFSET(MaterialColors, Glacier);
        UPDATE_OFFSET(MaterialColors, Grass);
        UPDATE_OFFSET(MaterialColors, Ground);
        UPDATE_OFFSET(MaterialColors, Ice);
        UPDATE_OFFSET(MaterialColors, LeafyGrass);
        UPDATE_OFFSET(MaterialColors, Limestone);
        UPDATE_OFFSET(MaterialColors, Mud);
        UPDATE_OFFSET(MaterialColors, Pavement);
        UPDATE_OFFSET(MaterialColors, Rock);
        UPDATE_OFFSET(MaterialColors, Salt);
        UPDATE_OFFSET(MaterialColors, Sand);
        UPDATE_OFFSET(MaterialColors, Sandstone);
        UPDATE_OFFSET(MaterialColors, Slate);
        UPDATE_OFFSET(MaterialColors, Snow);
        UPDATE_OFFSET(MaterialColors, WoodPlanks);

        UPDATE_OFFSET(MeshPart, MeshId);
        UPDATE_OFFSET(MeshPart, Texture);
        UPDATE_OFFSET(Misc, Adornee);
        UPDATE_OFFSET(Misc, AnimationId);
        UPDATE_OFFSET(Misc, StringLength);
        UPDATE_OFFSET(Misc, Value);
        UPDATE_OFFSET(Model, PrimaryPart);
        UPDATE_OFFSET(Model, Scale);
        UPDATE_OFFSET(ModuleScript, ByteCode);
        UPDATE_OFFSET(ModuleScript, GUID);
        UPDATE_OFFSET(ModuleScript, Hash);
        UPDATE_OFFSET(ModuleScript, IsCoreScript);
        UPDATE_OFFSET(MouseService, InputObject);
        UPDATE_OFFSET(MouseService, InputObject2);
        UPDATE_OFFSET(MouseService, MousePosition);
        UPDATE_OFFSET(MouseService, SensitivityPointer);
        UPDATE_OFFSET(ParticleEmitter, Acceleration);
        UPDATE_OFFSET(ParticleEmitter, Brightness);
        UPDATE_OFFSET(ParticleEmitter, Drag);
        UPDATE_OFFSET(ParticleEmitter, Lifetime);
        UPDATE_OFFSET(ParticleEmitter, LightEmission);
        UPDATE_OFFSET(ParticleEmitter, LightInfluence);
        UPDATE_OFFSET(ParticleEmitter, Rate);
        UPDATE_OFFSET(ParticleEmitter, RotSpeed);
        UPDATE_OFFSET(ParticleEmitter, Rotation);
        UPDATE_OFFSET(ParticleEmitter, Speed);
        UPDATE_OFFSET(ParticleEmitter, SpreadAngle);
        UPDATE_OFFSET(ParticleEmitter, Texture);
        UPDATE_OFFSET(ParticleEmitter, TimeScale);
        UPDATE_OFFSET(ParticleEmitter, VelocityInheritance);
        UPDATE_OFFSET(ParticleEmitter, ZOffset);
        UPDATE_OFFSET(Player, AccountAge);
        UPDATE_OFFSET(Player, CameraMode);
        UPDATE_OFFSET(Player, DisplayName);
        UPDATE_OFFSET(Player, HealthDisplayDistance);
        UPDATE_OFFSET(Player, LocalPlayer);
        UPDATE_OFFSET(Player, LocaleId);
        UPDATE_OFFSET(Player, MaxZoomDistance);
        UPDATE_OFFSET(Player, MinZoomDistance);
        UPDATE_OFFSET(Player, ModelInstance);
        UPDATE_OFFSET(Player, Mouse);
        UPDATE_OFFSET(Player, NameDisplayDistance);
        UPDATE_OFFSET(Player, Team);
        UPDATE_OFFSET(Player, TeamColor);
        UPDATE_OFFSET(Player, UserId);
        UPDATE_OFFSET(PlayerConfigurer, Pointer);
        UPDATE_OFFSET(PlayerMouse, Icon);
        UPDATE_OFFSET(PlayerMouse, Workspace);
        UPDATE_OFFSET(Primitive, AssemblyAngularVelocity);
        UPDATE_OFFSET(Primitive, AssemblyLinearVelocity);
        UPDATE_OFFSET(Primitive, Flags);
        UPDATE_OFFSET(Primitive, Material);
        UPDATE_OFFSET(Primitive, Owner);
        UPDATE_OFFSET(Primitive, Position);
        UPDATE_OFFSET(Primitive, Rotation);
        UPDATE_OFFSET(Primitive, Size);
        UPDATE_OFFSET(Primitive, Validate);
        UPDATE_OFFSET(PrimitiveFlags, Anchored);
        UPDATE_OFFSET(PrimitiveFlags, CanCollide);
        UPDATE_OFFSET(PrimitiveFlags, CanQuery);
        UPDATE_OFFSET(PrimitiveFlags, CanTouch);
        UPDATE_OFFSET(ProximityPrompt, ActionText);
        UPDATE_OFFSET(ProximityPrompt, Enabled);
        UPDATE_OFFSET(ProximityPrompt, GamepadKeyCode);
        UPDATE_OFFSET(ProximityPrompt, HoldDuration);
        UPDATE_OFFSET(ProximityPrompt, KeyCode);
        UPDATE_OFFSET(ProximityPrompt, MaxActivationDistance);
        UPDATE_OFFSET(ProximityPrompt, ObjectText);
        UPDATE_OFFSET(ProximityPrompt, RequiresLineOfSight);
        UPDATE_OFFSET(RenderJob, FakeDataModel);
        UPDATE_OFFSET(RenderJob, RealDataModel);
        UPDATE_OFFSET(RenderJob, RenderView);
        UPDATE_OFFSET(RenderView, DeviceD3D11);
        UPDATE_OFFSET(RenderView, LightingValid);
        UPDATE_OFFSET(RenderView, SkyValid);
        UPDATE_OFFSET(RenderView, VisualEngine);
        UPDATE_OFFSET(RunService, HeartbeatFPS);
        UPDATE_OFFSET(RunService, HeartbeatTask);
        UPDATE_OFFSET(Script, ByteCode);
        UPDATE_OFFSET(Script, GUID);
        UPDATE_OFFSET(Script, Hash);
        UPDATE_OFFSET(ScriptContext, RequireBypass);
        UPDATE_OFFSET(Seat, Occupant);
        UPDATE_OFFSET(Sky, MoonAngularSize);
        UPDATE_OFFSET(Sky, MoonTextureId);
        UPDATE_OFFSET(Sky, SkyboxBk);
        UPDATE_OFFSET(Sky, SkyboxDn);
        UPDATE_OFFSET(Sky, SkyboxFt);
        UPDATE_OFFSET(Sky, SkyboxLf);
        UPDATE_OFFSET(Sky, SkyboxOrientation);
        UPDATE_OFFSET(Sky, SkyboxRt);
        UPDATE_OFFSET(Sky, SkyboxUp);
        UPDATE_OFFSET(Sky, StarCount);
        UPDATE_OFFSET(Sky, SunAngularSize);
        UPDATE_OFFSET(Sky, SunTextureId);
        UPDATE_OFFSET(Sound, Looped);
        UPDATE_OFFSET(Sound, PlaybackSpeed);

        UPDATE_OFFSET(Sound, RollOffMaxDistance);
        UPDATE_OFFSET(Sound, RollOffMinDistance);
        UPDATE_OFFSET(Sound, SoundGroup);
        UPDATE_OFFSET(Sound, SoundId);
        UPDATE_OFFSET(Sound, Volume);
        UPDATE_OFFSET(SpawnLocation, AllowTeamChangeOnTouch);
        UPDATE_OFFSET(SpawnLocation, Enabled);
        UPDATE_OFFSET(SpawnLocation, ForcefieldDuration);
        UPDATE_OFFSET(SpawnLocation, Neutral);
        UPDATE_OFFSET(SpawnLocation, TeamColor);
        UPDATE_OFFSET(SpecialMesh, MeshId);
        UPDATE_OFFSET(SpecialMesh, Scale);
        UPDATE_OFFSET(StatsItem, Value);
        UPDATE_OFFSET(SunRaysEffect, Enabled);
        UPDATE_OFFSET(SunRaysEffect, Intensity);
        UPDATE_OFFSET(SunRaysEffect, Spread);
        UPDATE_OFFSET(SurfaceAppearance, AlphaMode);
        UPDATE_OFFSET(SurfaceAppearance, Color);
        UPDATE_OFFSET(SurfaceAppearance, ColorMap);
        UPDATE_OFFSET(SurfaceAppearance, EmissiveMaskContent);
        UPDATE_OFFSET(SurfaceAppearance, EmissiveStrength);
        UPDATE_OFFSET(SurfaceAppearance, EmissiveTint);
        UPDATE_OFFSET(SurfaceAppearance, MetalnessMap);
        UPDATE_OFFSET(SurfaceAppearance, NormalMap);
        UPDATE_OFFSET(SurfaceAppearance, RoughnessMap);
        UPDATE_OFFSET(TaskScheduler, JobEnd);
        UPDATE_OFFSET(TaskScheduler, JobName);
        UPDATE_OFFSET(TaskScheduler, JobStart);
        UPDATE_OFFSET(TaskScheduler, MaxFPS);
        UPDATE_OFFSET(TaskScheduler, Pointer);
        UPDATE_OFFSET(Team, BrickColor);
        UPDATE_OFFSET(Terrain, GrassLength);
        UPDATE_OFFSET(Terrain, MaterialColors);
        UPDATE_OFFSET(Terrain, WaterColor);
        UPDATE_OFFSET(Terrain, WaterReflectance);
        UPDATE_OFFSET(Terrain, WaterTransparency);
        UPDATE_OFFSET(Terrain, WaterWaveSize);
        UPDATE_OFFSET(Terrain, WaterWaveSpeed);
        UPDATE_OFFSET(Textures, Decal_Texture);
        UPDATE_OFFSET(Textures, Texture_Texture);
        UPDATE_OFFSET(Tool, CanBeDropped);
        UPDATE_OFFSET(Tool, Enabled);
        UPDATE_OFFSET(Tool, Grip);
        UPDATE_OFFSET(Tool, ManualActivationOnly);
        UPDATE_OFFSET(Tool, RequiresHandle);
        UPDATE_OFFSET(Tool, TextureId);
        UPDATE_OFFSET(Tool, Tooltip);
        UPDATE_OFFSET(UnionOperation, AssetId);
        UPDATE_OFFSET(UserInputService, WindowInputState);
        UPDATE_OFFSET(VehicleSeat, MaxSpeed);
        UPDATE_OFFSET(VehicleSeat, SteerFloat);
        UPDATE_OFFSET(VehicleSeat, ThrottleFloat);
        UPDATE_OFFSET(VehicleSeat, Torque);
        UPDATE_OFFSET(VehicleSeat, TurnSpeed);
        UPDATE_OFFSET(VisualEngine, Dimensions);
        UPDATE_OFFSET(VisualEngine, FakeDataModel);
        UPDATE_OFFSET(VisualEngine, Pointer);
        UPDATE_OFFSET(VisualEngine, RenderView);
        UPDATE_OFFSET(VisualEngine, ViewMatrix);
        UPDATE_OFFSET(Weld, Part0);
        UPDATE_OFFSET(Weld, Part1);
        UPDATE_OFFSET(WeldConstraint, Part0);
        UPDATE_OFFSET(WeldConstraint, Part1);
        UPDATE_OFFSET(WindowInputState, CapsLock);
        UPDATE_OFFSET(WindowInputState, CurrentTextBox);
        UPDATE_OFFSET(Workspace, CurrentCamera);
        UPDATE_OFFSET(Workspace, DistributedGameTime);
        UPDATE_OFFSET(Workspace, ReadOnlyGravity);
        UPDATE_OFFSET(Workspace, World);
        UPDATE_OFFSET(World, AirProperties);
        UPDATE_OFFSET(World, FallenPartsDestroyHeight);
        UPDATE_OFFSET(World, Gravity);
        UPDATE_OFFSET(World, Primitives);
        UPDATE_OFFSET(World, worldStepsPerSec);
        
        save_local_version_hash(remote_hash);
        printf("[Updater] Successfully loaded %d total offsets from imtheo.lol.\n", 391);
        return true;
    }
}

