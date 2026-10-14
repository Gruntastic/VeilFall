#pragma once
// Small helpers shared by the intro greybox classes. Materials come from /Game/VeilFall/Materials (made by create_vf_intro.py);
// BasicShapeMaterial is the fallback so code never hard-fails on a missing placeholder asset.
#include "CoreMinimal.h"
#include "Combat.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace VF {
enum class EMat : uint8 { Lit, Emissive, Smoke };
inline UMaterialInterface* BaseMaterial(EMat Kind){
 const TCHAR* Path=Kind==EMat::Emissive?TEXT("/Game/VeilFall/Materials/M_VF_Emissive.M_VF_Emissive"):Kind==EMat::Smoke?TEXT("/Game/VeilFall/Materials/M_VF_Smoke.M_VF_Smoke"):TEXT("/Game/VeilFall/Materials/M_VF_Greybox.M_VF_Greybox");
 if(auto* M=LoadObject<UMaterialInterface>(nullptr,Path,nullptr,LOAD_NoWarn|LOAD_Quiet))return M;
 return LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}
inline UMaterialInstanceDynamic* Paint(UPrimitiveComponent* C,FLinearColor Color,EMat Kind=EMat::Lit){
 if(!C)return nullptr;auto* Base=BaseMaterial(Kind);if(!Base)return nullptr;
 auto* M=UMaterialInstanceDynamic::Create(Base,C);M->SetVectorParameterValue("Color",Color);C->SetMaterial(0,M);return M;
}
// Scripted kill for fictional blast zones; mirrors what a lethal hit does without needing a weapon spec.
inline void Kill(UVeilHealthComponent* H){
 if(!H||H->State.Life==EVeilLife::Dead)return;H->State.Health=0;H->State.Life=EVeilLife::Dead;++H->State.DamageRevision;H->OnRep_State();
}
inline FVeilWeaponSpec BlastSpec(float Damage){
 FVeilWeaponSpec S=UCombatDataLibrary::DefaultWeaponSpec("weapon.carbine");S.StableId="vf.blast";S.Damage=FMath::Clamp(Damage,1.f,200.f);S.Penetration=100;return S;
}
}
