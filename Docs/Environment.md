# G0 environment — 2026-10-03
Current build ID: VF-G0-20261003-B; original A package retained.
UE path: C:/Program Files/Epic Games/UE_5.8. Build.version: 5.8.3, CL 58210709, compatible CL 55116800, ++UE5+Release-5.8.
Project association: 5.8. Existing parent project preserved, fresh project here.
Windows 11 Home 10.0.26200; Intel i7-13700H; RTX 4060 Laptop GPU driver 32.0.15.9186; Intel Iris Xe; 16869351424 bytes RAM.
MSVC selected by UBT: 14.51.36246; SDK 10.0.22621.0. UBT warns compiler newer than preferred 14.50.35717.
Git 2.54.0.windows.1; LFS 3.7.1. Blender 5.2.2 LTS d13f752e3b9c; Blender 4.4 also installed.
EnhancedInput, PythonScriptPlugin and EditorScriptingUtilities installed. No Unreal MCP connection discovered; command-line editor automation used.
Production package attachment not found; supplied user brief is authoritative.
Very low free RAM (~248 MiB in audit), 38.5/44.27 GB committed during build. Avoid terminating user applications automatically.
Hardware is below provisional 32 GB reference. GPU VRAM capacity and storage type NOT VERIFIED.
G1 retains the audited UE 5.8.3 engine, hardware and toolchain. Runtime module name stays VeilFallG0 to preserve compatible assets and existing G0 map references; G1 map selects its own native game mode.

G3 retains UE 5.8.3 CL 58210709 and the audited hardware/toolchain. AIModule, NavigationSystem and GameplayTasks are installed engine runtime modules, not third-party plugins. Actual engine runtime starts Behavior Trees and completes custom EQS queries using the pawn as the query owner in this version. The Editor compiled this implementation; package and engine evidence is listed separately in G3Evidence.md.

G5 preparation retains UE 5.8.3 CL 58210709 and the audited compiler/SDK/Blender. EditorStartupMap and GameDefaultMap now select /Game/DeadSignal/L_DeadSignal_G5_Workshop. Actual native editor build, real Blueprint/map creation, fresh asset validation and Windows cook/archive ran; see G5Evidence.md. OpenMissionEditor.cmd opens the interactive editor using the existing lower-memory preset and isolated G5Preview campaign state. No external editor connector or paid asset service was required.
