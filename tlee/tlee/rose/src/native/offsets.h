#pragma once

/* =============================================================

/*                       RbxDumperV2                            

/*               https://imtheo.lol/Offsets                     

/* -------------------------------------------------------------

/*  Dumped By       : theo (https://imtheo.lol)                 

/*  Roblox Version  : version-ad5d3e2906444472

/*  Dumper Version  : 2.1.7

/*  Dumped At       : 21:59 03/06/2026 (GMT)

/*  Total Offsets   : 381

/* -------------------------------------------------------------

/*  Join the discord!                                           

/*  https://discord.gg/rbxoffsets                               

/* =============================================================

*/



#include <cstdint>

#include <string>

namespace Offsets {

    inline std::string ClientVersion = "version-ad5d3e2906444472";



    namespace AirProperties {

         inline uintptr_t AirDensity = 0x18;

         inline uintptr_t GlobalWind = 0x3c;

    }



    namespace AnimationTrack {

         inline uintptr_t Animation = 0xd0;

         inline uintptr_t Animator = 0x118;

         inline uintptr_t IsPlaying = 0xa10;

         inline uintptr_t Looped = 0xf5;

         inline uintptr_t Speed = 0xe4;

         inline uintptr_t TimePosition = 0xe8;

    }



    namespace Animator {

         inline uintptr_t ActiveAnimations = 0x868;

    }



    namespace Atmosphere {

         inline uintptr_t Color = 0xd0;

         inline uintptr_t Decay = 0xdc;

         inline uintptr_t Density = 0xe8;

         inline uintptr_t Glare = 0xec;

         inline uintptr_t Haze = 0xf0;

         inline uintptr_t Offset = 0xf4;

    }



    namespace Attachment {

         inline uintptr_t Position = 0xdc;

    }



    namespace BasePart {

         inline uintptr_t CastShadow = 0xf5;

         inline uintptr_t Color3 = 0x194;

         inline uintptr_t Locked = 0xf6;

         inline uintptr_t Massless = 0xf7;

         inline uintptr_t Primitive = 0x148;

         inline uintptr_t Reflectance = 0xec;

         inline uintptr_t Shape = 0x1b1;

         inline uintptr_t Transparency = 0xf0;

    }



    namespace Beam {

         inline uintptr_t Attachment0 = 0x170;

         inline uintptr_t Attachment1 = 0x180;

         inline uintptr_t Brightness = 0x190;

         inline uintptr_t CurveSize0 = 0x194;

         inline uintptr_t CurveSize1 = 0x198;

         inline uintptr_t LightEmission = 0x19c;

         inline uintptr_t LightInfluence = 0x1a0;

         inline uintptr_t Texture = 0x150;

         inline uintptr_t TextureLength = 0x1ac;

         inline uintptr_t TextureSpeed = 0x1b4;

         inline uintptr_t Width0 = 0x1b8;

         inline uintptr_t Width1 = 0x1bc;

         inline uintptr_t ZOffset = 0x1c0;

    }



    namespace BloomEffect {

         inline uintptr_t Enabled = 0xc8;

         inline uintptr_t Intensity = 0xd0;

         inline uintptr_t Size = 0xd4;

         inline uintptr_t Threshold = 0xd8;

    }



    namespace BlurEffect {

         inline uintptr_t Enabled = 0xc8;

         inline uintptr_t Size = 0xd0;

    }



    namespace ByteCode {

         inline uintptr_t Pointer = 0x10;

         inline uintptr_t Size = 0x20;

    }



    namespace Camera {

         inline uintptr_t CameraSubject = 0xe8;

         inline uintptr_t CameraType = 0x158;

         inline uintptr_t FieldOfView = 0x160;

         inline uintptr_t ImagePlaneDepth = 0x2f0;

         inline uintptr_t Position = 0x11c;

         inline uintptr_t Rotation = 0xf8;

         inline uintptr_t Viewport = 0x2ac;

         inline uintptr_t ViewportSize = 0x2e8;

    }



    namespace CharacterMesh {

         inline uintptr_t BaseTextureId = 0xe0;

         inline uintptr_t BodyPart = 0x160;

         inline uintptr_t MeshId = 0x110;

         inline uintptr_t OverlayTextureId = 0x140;

    }



    namespace ClickDetector {

         inline uintptr_t MaxActivationDistance = 0x100;

         inline uintptr_t MouseIcon = 0xe0;

    }



    namespace Clothing {

         inline uintptr_t Color3 = 0x128;

         inline uintptr_t Template = 0x108;

    }



    namespace ColorCorrectionEffect {

         inline uintptr_t Brightness = 0xdc;

         inline uintptr_t Contrast = 0xe0;

         inline uintptr_t Enabled = 0xc8;

         inline uintptr_t TintColor = 0xd0;

    }



    namespace ColorGradingEffect {

         inline uintptr_t Enabled = 0xc8;

         inline uintptr_t TonemapperPreset = 0xd0;

    }



    namespace DataModel {

         inline uintptr_t CreatorId = 0x190;

         inline uintptr_t GameId = 0x198;

         inline uintptr_t GameLoaded = 0x638;

         inline uintptr_t JobId = 0x138;

         inline uintptr_t PlaceId = 0x1a0;

         inline uintptr_t PlaceVersion = 0x1bc;

         inline uintptr_t PrimitiveCount = 0x480;

         inline uintptr_t ScriptContext = 0x440;

         inline uintptr_t ServerIP = 0x620;

         inline uintptr_t ToRenderView1 = 0x1d8;

         inline uintptr_t ToRenderView2 = 0x8;

         inline uintptr_t ToRenderView3 = 0x28;

         inline uintptr_t Workspace = 0x178;

    }



    namespace DepthOfFieldEffect {

         inline uintptr_t Enabled = 0xc8;

         inline uintptr_t FarIntensity = 0xd0;

         inline uintptr_t FocusDistance = 0xd4;

         inline uintptr_t InFocusRadius = 0xd8;

         inline uintptr_t NearIntensity = 0xdc;

    }



    namespace DragDetector {

         inline uintptr_t ActivatedCursorIcon = 0x1d8;

         inline uintptr_t CursorIcon = 0xe0;

         inline uintptr_t MaxActivationDistance = 0x100;

         inline uintptr_t MaxDragAngle = 0x2c0;

         inline uintptr_t MaxDragTranslation = 0x284;

         inline uintptr_t MaxForce = 0x2c4;

         inline uintptr_t MaxTorque = 0x2c8;

         inline uintptr_t MinDragAngle = 0x2cc;

         inline uintptr_t MinDragTranslation = 0x290;

         inline uintptr_t ReferenceInstance = 0x208;

         inline uintptr_t Responsiveness = 0x2d8;

    }



    namespace FakeDataModel {

         inline uintptr_t Pointer = 0x78ff228;

         inline uintptr_t RealDataModel = 0x1d0;

    }



    namespace GuiBase2D {

         inline uintptr_t AbsolutePosition = 0x110;

         inline uintptr_t AbsoluteRotation = 0x188;

         inline uintptr_t AbsoluteSize = 0x118;

    }



    namespace GuiObject {

         inline uintptr_t BackgroundColor3 = 0x540;

         inline uintptr_t BackgroundTransparency = 0x54c;

         inline uintptr_t BorderColor3 = 0x54c;

         inline uintptr_t Image = 0x988;

         inline uintptr_t LayoutOrder = 0x580;

         inline uintptr_t Position = 0x510;

         inline uintptr_t RichText = 0xb58;

         inline uintptr_t Rotation = 0x188;

         inline uintptr_t ScreenGui_Enabled = 0x4c4;

         inline uintptr_t Size = 0x530;

         inline uintptr_t Text = 0xda0;

         inline uintptr_t TextColor3 = 0xe50;

         inline uintptr_t Visible = 0x5ad;

         inline uintptr_t ZIndex = 0x19b;

    }



    namespace Humanoid {

         inline uintptr_t AutoJumpEnabled = 0x1e0;

         inline uintptr_t AutoRotate = 0x1e1;

         inline uintptr_t AutomaticScalingEnabled = 0x1e2;

         inline uintptr_t BreakJointsOnDeath = 0x1e3;

         inline uintptr_t CameraOffset = 0x140;

         inline uintptr_t DisplayDistanceType = 0x18c;

         inline uintptr_t DisplayName = 0xd0;

         inline uintptr_t EvaluateStateMachine = 0x1e4;

         inline uintptr_t FloorMaterial = 0x190;

         inline uintptr_t Health = 0x194;

         inline uintptr_t HealthDisplayDistance = 0x198;

         inline uintptr_t HealthDisplayType = 0x19c;

         inline uintptr_t HipHeight = 0x1a0;

         inline uintptr_t HumanoidRootPart = 0x480;

         inline uintptr_t HumanoidState = 0x8a0;

         inline uintptr_t HumanoidStateID = 0x20;

         inline uintptr_t IsWalking = 0x91f;

         inline uintptr_t Jump = 0x1e6;

         inline uintptr_t JumpHeight = 0x1ac;

         inline uintptr_t JumpPower = 0x1b0;

         inline uintptr_t MaxHealth = 0x1b4;

         inline uintptr_t MaxSlopeAngle = 0x1b8;

         inline uintptr_t MoveDirection = 0x158;

         inline uintptr_t MoveToPart = 0x130;

         inline uintptr_t MoveToPoint = 0x17c;

         inline uintptr_t NameDisplayDistance = 0x1bc;

         inline uintptr_t NameOcclusion = 0x1c0;

         inline uintptr_t PlatformStand = 0x1e8;

         inline uintptr_t PlatformStatePointer = 0x262CF636;

         inline uintptr_t RequiresNeck = 0x1e9;

         inline uintptr_t RigType = 0x1cc;

         inline uintptr_t SeatPart = 0x120;

         inline uintptr_t Sit = 0x1e9;

         inline uintptr_t TargetPoint = 0x164;

         inline uintptr_t UseJumpPower = 0x1ec;

         inline uintptr_t WalkTimer = 0x410;

         inline uintptr_t Walkspeed = 0x1dc;

         inline uintptr_t WalkspeedCheck = 0x3c4;

    }



    namespace Instance {

         inline uintptr_t AttributeContainer = 0x48;

         inline uintptr_t AttributeList = 0x18;

         inline uintptr_t AttributeToNext = 0x58;

         inline uintptr_t AttributeToValue = 0x18;

         inline uintptr_t ChildrenEnd = 0x8;

         inline uintptr_t ChildrenStart = 0x78;

         inline uintptr_t ClassBase = 0x4920;

         inline uintptr_t ClassDescriptor = 0x18;

         inline uintptr_t ClassName = 0x8;

         inline uintptr_t Name = 0xb0;

         inline uintptr_t Parent = 0x70;

         inline uintptr_t This = 0x8;

    }



    namespace Lighting {

         inline uintptr_t Ambient = 0xe0;

         inline uintptr_t Brightness = 0x128;

         inline uintptr_t ClockTime = 0x1c0;

         inline uintptr_t ColorShift_Bottom = 0xf8;

         inline uintptr_t ColorShift_Top = 0xec;

         inline uintptr_t EnvironmentDiffuseScale = 0x12c;

         inline uintptr_t EnvironmentSpecularScale = 0x130;

         inline uintptr_t ExposureCompensation = 0x134;

         inline uintptr_t FogColor = 0x104;

         inline uintptr_t FogEnd = 0x13c;

         inline uintptr_t FogStart = 0x140;

         inline uintptr_t GeographicLatitude = 0x198;

         inline uintptr_t GlobalShadows = 0x150;

         inline uintptr_t GradientBottom = 0x19c;

         inline uintptr_t GradientTop = 0x158;

         inline uintptr_t LightColor = 0x164;

         inline uintptr_t LightDirection = 0x170;

         inline uintptr_t MoonPosition = 0x18c;

         inline uintptr_t OutdoorAmbient = 0x110;

         inline uintptr_t Sky = 0x1e0;

         inline uintptr_t Source = 0x17c;

         inline uintptr_t SunPosition = 0x180;

    }



    namespace LocalScript {

         inline uintptr_t ByteCode = 0x1a8;

         inline uintptr_t GUID = 0xe8;

         inline uintptr_t Hash = 0x1b8;

    }



    namespace MaterialColors {

         inline uintptr_t Asphalt = 0x30;

         inline uintptr_t Basalt = 0x27;

         inline uintptr_t Brick = 0xf;

         inline uintptr_t Cobblestone = 0x33;

         inline uintptr_t Concrete = 0xc;

         inline uintptr_t CrackedLava = 0x2d;

         inline uintptr_t Glacier = 0x1b;

         inline uintptr_t Grass = 0x6;

         inline uintptr_t Ground = 0x2a;

         inline uintptr_t Ice = 0x36;

         inline uintptr_t LeafyGrass = 0x39;

         inline uintptr_t Limestone = 0x3f;

         inline uintptr_t Mud = 0x24;

         inline uintptr_t Pavement = 0x42;

         inline uintptr_t Rock = 0x18;

         inline uintptr_t Salt = 0x3c;

         inline uintptr_t Sand = 0x12;

         inline uintptr_t Sandstone = 0x21;

         inline uintptr_t Slate = 0x9;

         inline uintptr_t Snow = 0x1e;

         inline uintptr_t WoodPlanks = 0x15;

    }



    namespace MeshPart {

         inline uintptr_t MeshId = 0x300;

         inline uintptr_t Texture = 0x330;

    }



    namespace Misc {

         inline uintptr_t Adornee = 0x108;

         inline uintptr_t AnimationId = 0xd0;

         inline uintptr_t StringLength = 0x10;

         inline uintptr_t Value = 0xd0;

    }



    namespace Model {

         inline uintptr_t PrimaryPart = 0x278;

         inline uintptr_t Scale = 0x164;

    }



    namespace ModuleScript {

         inline uintptr_t ByteCode = 0x150;

         inline uintptr_t GUID = 0xe8;

         inline uintptr_t Hash = 0x160;

         inline uintptr_t IsCoreScript = 0x0;

    }



    namespace MouseService {

         inline uintptr_t InputObject = 0x108;

         inline uintptr_t InputObject2 = 0x118;

         inline uintptr_t MousePosition = 0xec;

         inline uintptr_t SensitivityPointer = 0x307;

    }



    namespace ParticleEmitter {

         inline uintptr_t Acceleration = 0x1f0;

         inline uintptr_t Brightness = 0x22c;

         inline uintptr_t Drag = 0x230;

         inline uintptr_t Lifetime = 0x204;

         inline uintptr_t LightEmission = 0x248;

         inline uintptr_t LightInfluence = 0x24c;

         inline uintptr_t Rate = 0x258;

         inline uintptr_t RotSpeed = 0x20c;

         inline uintptr_t Rotation = 0x214;

         inline uintptr_t Speed = 0x21c;

         inline uintptr_t SpreadAngle = 0x224;

         inline uintptr_t Texture = 0x1d0;

         inline uintptr_t TimeScale = 0x26c;

         inline uintptr_t VelocityInheritance = 0x270;

         inline uintptr_t ZOffset = 0x274;

    }



    namespace Player {

         inline uintptr_t AccountAge = 0x32c;

         inline uintptr_t CameraMode = 0x338;

         inline uintptr_t DisplayName = 0x130;

         inline uintptr_t HealthDisplayDistance = 0x358;

         inline uintptr_t LocalPlayer = 0x138;

         inline uintptr_t LocaleId = 0x110;

         inline uintptr_t MaxZoomDistance = 0x330;

         inline uintptr_t MinZoomDistance = 0x334;

         inline uintptr_t ModelInstance = 0x280;

         inline uintptr_t Mouse = 0x1180;

         inline uintptr_t NameDisplayDistance = 0x368;

         inline uintptr_t Team = 0x2b0;

         inline uintptr_t TeamColor = 0x374;

         inline uintptr_t UserId = 0x2d8;

    }



    namespace PlayerConfigurer {

         inline uintptr_t Pointer = 0x0;

    }



    namespace PlayerMouse {

         inline uintptr_t HitPosition = 0x144;

         inline uintptr_t Icon = 0xe0;

         inline uintptr_t Workspace = 0x168;

    }



    namespace Primitive {

         inline uintptr_t AssemblyAngularVelocity = 0x104;

         inline uintptr_t AssemblyLinearVelocity = 0xf8;

         inline uintptr_t Flags = 0x1b6;

         inline uintptr_t Material = 0x0;

         inline uintptr_t Owner = 0x200;

         inline uintptr_t Position = 0xec;

         inline uintptr_t Rotation = 0xc8;

         inline uintptr_t Size = 0x1b8;

         inline uintptr_t Validate = 0x6;

    }



    namespace PrimitiveFlags {

         inline uintptr_t Anchored = 0x2;

         inline uintptr_t CanCollide = 0x8;

         inline uintptr_t CanQuery = 0x20;

         inline uintptr_t CanTouch = 0x10;

    }



    namespace ProximityPrompt {

         inline uintptr_t ActionText = 0xc8;

         inline uintptr_t Enabled = 0x14e;

         inline uintptr_t GamepadKeyCode = 0x134;

         inline uintptr_t HoldDuration = 0x138;

         inline uintptr_t KeyCode = 0x13c;

         inline uintptr_t MaxActivationDistance = 0x140;

         inline uintptr_t ObjectText = 0xe8;

         inline uintptr_t RequiresLineOfSight = 0x14f;

    }



    namespace RenderJob {

         inline uintptr_t FakeDataModel = 0x38;

         inline uintptr_t RealDataModel = 0x1c0;

         inline uintptr_t RenderView = 0x1d0;

    }



    namespace RenderView {

         inline uintptr_t DeviceD3D11 = 0x8;

         inline uintptr_t LightingValid = 0x150;

         inline uintptr_t SkyValid = 0x28d;

         inline uintptr_t VisualEngine = 0x10;

    }



    namespace RunService {

         inline uintptr_t HeartbeatFPS = 0xb8;

         inline uintptr_t HeartbeatTask = 0xf8;

    }



    namespace Script {

         inline uintptr_t ByteCode = 0x1a8;

         inline uintptr_t GUID = 0xe8;

         inline uintptr_t Hash = 0x1b8;

    }



    namespace ScriptContext {

         inline uintptr_t RequireBypass = 0x0;

    }



    namespace Seat {

         inline uintptr_t Occupant = 0x220;

    }



    namespace Sky {

         inline uintptr_t MoonAngularSize = 0x25c;

         inline uintptr_t MoonTextureId = 0xe0;

         inline uintptr_t SkyboxBk = 0x110;

         inline uintptr_t SkyboxDn = 0x140;

         inline uintptr_t SkyboxFt = 0x170;

         inline uintptr_t SkyboxLf = 0x1a0;

         inline uintptr_t SkyboxOrientation = 0x250;

         inline uintptr_t SkyboxRt = 0x1d0;

         inline uintptr_t SkyboxUp = 0x200;

         inline uintptr_t StarCount = 0x260;

         inline uintptr_t SunAngularSize = 0x254;

         inline uintptr_t SunTextureId = 0x230;

    }



    namespace Sound {

         inline uintptr_t Looped = 0x155;

         inline uintptr_t PlaybackSpeed = 0x134;

         inline uintptr_t RollOffMaxDistance = 0x138;

         inline uintptr_t RollOffMinDistance = 0x13c;

         inline uintptr_t SoundGroup = 0x100;

         inline uintptr_t SoundId = 0xe0;

         inline uintptr_t Volume = 0x148;

    }



    namespace SpawnLocation {

         inline uintptr_t AllowTeamChangeOnTouch = 0x1f8;

         inline uintptr_t Enabled = 0x1f9;

         inline uintptr_t ForcefieldDuration = 0x1f0;

         inline uintptr_t Neutral = 0x1fa;

         inline uintptr_t TeamColor = 0x1f4;

    }



    namespace SpecialMesh {

         inline uintptr_t MeshId = 0x108;

         inline uintptr_t Scale = 0xdc;

    }



    namespace StatsItem {

         inline uintptr_t Value = 0xc8;

    }



    namespace SunRaysEffect {

         inline uintptr_t Enabled = 0xc8;

         inline uintptr_t Intensity = 0xd0;

         inline uintptr_t Spread = 0xd4;

    }



    namespace SurfaceAppearance {

         inline uintptr_t AlphaMode = 0x2a0;

         inline uintptr_t Color = 0x288;

         inline uintptr_t ColorMap = 0xe0;

         inline uintptr_t EmissiveMaskContent = 0x110;

         inline uintptr_t EmissiveStrength = 0x2a4;

         inline uintptr_t EmissiveTint = 0x294;

         inline uintptr_t MetalnessMap = 0x140;

         inline uintptr_t NormalMap = 0x170;

         inline uintptr_t RoughnessMap = 0x1a0;

    }



    namespace TaskScheduler {

         inline uintptr_t JobEnd = 0xd0;

         inline uintptr_t JobName = 0x18;

         inline uintptr_t JobStart = 0xc8;

         inline uintptr_t MaxFPS = 0xb0;

         inline uintptr_t Pointer = 0x7e90548;

    }



    namespace Team {

         inline uintptr_t BrickColor = 0xd0;

    }



    namespace Terrain {

         inline uintptr_t GrassLength = 0x1f8;

         inline uintptr_t MaterialColors = 0x2a8;

         inline uintptr_t WaterColor = 0x1e8;

         inline uintptr_t WaterReflectance = 0x200;

         inline uintptr_t WaterTransparency = 0x204;

         inline uintptr_t WaterWaveSize = 0x208;

         inline uintptr_t WaterWaveSpeed = 0x20c;

    }



    namespace Textures {

         inline uintptr_t Decal_Texture = 0x198;

         inline uintptr_t Texture_Texture = 0x198;

    }



    namespace Tool {

         inline uintptr_t CanBeDropped = 0x4c8;

         inline uintptr_t Enabled = 0x4c9;

         inline uintptr_t Grip = 0x4bc;

         inline uintptr_t ManualActivationOnly = 0x4ca;

         inline uintptr_t RequiresHandle = 0x4cb;

         inline uintptr_t TextureId = 0x370;

         inline uintptr_t Tooltip = 0x478;

    }



    namespace UnionOperation {

         inline uintptr_t AssetId = 0x2f8;

    }



    namespace UserInputService {

         inline uintptr_t WindowInputState = 0x2d8;

    }



    namespace VehicleSeat {

         inline uintptr_t MaxSpeed = 0x238;

         inline uintptr_t SteerFloat = 0x240;

         inline uintptr_t ThrottleFloat = 0x248;

         inline uintptr_t Torque = 0x24c;

         inline uintptr_t TurnSpeed = 0x250;

    }



    namespace VisualEngine {

         inline uintptr_t Dimensions = 0xab0;

         inline uintptr_t FakeDataModel = 0xa90;

         inline uintptr_t Pointer = 0x801dfb0;

         inline uintptr_t RenderView = 0xbb0;

         inline uintptr_t ViewMatrix = 0x150;

    }



    namespace Weld {

         inline uintptr_t Part0 = 0x130;

         inline uintptr_t Part1 = 0x140;

    }



    namespace WeldConstraint {

         inline uintptr_t Part0 = 0xd0;

         inline uintptr_t Part1 = 0xe0;

    }



    namespace WindowInputState {

         inline uintptr_t CapsLock = 0x40;

         inline uintptr_t CurrentTextBox = 0x48;

    }



    namespace Workspace {

         inline uintptr_t CurrentCamera = 0x4b0;

         inline uintptr_t DistributedGameTime = 0x4d0;

         inline uintptr_t ReadOnlyGravity = 0x9f0;

         inline uintptr_t World = 0x408;

    }



    namespace World {

         inline uintptr_t AirProperties = 0x218;

         inline uintptr_t FallenPartsDestroyHeight = 0x208;

         inline uintptr_t Gravity = 0x210;

         inline uintptr_t Primitives = 0x280;

         inline uintptr_t worldStepsPerSec = 0x678;

    }

}
