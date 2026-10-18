#include "Combat.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"

bool FVeilWeaponSpec::Validate() const {
 return !StableId.IsNone() && MagazineSize>0 && MagazineSize<=100 && ReserveLimit>=0 && ReserveLimit<=500
 && FMath::IsFinite(Damage) && Damage>0 && Damage<=200 && FMath::IsFinite(Penetration) && Penetration>=0
 && FMath::IsFinite(ShotInterval) && ShotInterval>=0.08f && FMath::IsFinite(ReloadSeconds) && ReloadSeconds>=0.2f
 && FMath::IsFinite(RangeCm) && RangeCm>0 && RangeCm<=50000 && Pellets>=1 && Pellets<=16
 && FMath::IsFinite(SpreadDegrees) && SpreadDegrees>=0 && SpreadDegrees<=10 && FMath::IsFinite(RecoilDegrees);
}
FVeilWeaponSpec UCombatDataLibrary::DefaultWeaponSpec(FName Id) {
 FVeilWeaponSpec S; S.StableId=Id;
 if(Id=="weapon.shotgun") {S.DisplayName="Shotgun";S.MagazineSize=6;S.ReserveLimit=24;S.Damage=11;S.Penetration=0;S.ShotInterval=.8f;S.ReloadSeconds=2.8f;S.RangeCm=2500;S.Pellets=8;S.SpreadDegrees=3;S.RecoilDegrees=2;S.bAutomatic=false;}
 else if(Id=="weapon.marksman") {S.DisplayName="Sniper rifle";S.MagazineSize=10;S.ReserveLimit=30;S.Damage=90;S.Penetration=20;S.ShotInterval=.8f;S.ReloadSeconds=2.6f;S.RangeCm=25000;S.RecoilDegrees=1.8f;S.bAutomatic=false;}
 else if(Id=="weapon.mg") {S.DisplayName="Machine gun";S.MagazineSize=100;S.ReserveLimit=300;S.Damage=30;S.Penetration=12;S.ShotInterval=.09f;S.ReloadSeconds=5;S.RangeCm=15000;S.SpreadDegrees=1.2f;S.RecoilDegrees=.45f;S.bAutomatic=true;}
 else if(Id=="weapon.pistol") {S.DisplayName="Pistol";S.MagazineSize=15;S.ReserveLimit=45;S.Damage=38;S.Penetration=3;S.ShotInterval=.22f;S.ReloadSeconds=1.4f;S.RangeCm=5000;S.RecoilDegrees=.6f;S.bAutomatic=false;}
 else if(Id!="weapon.carbine") {S.MagazineSize=0;}
 return S;
}
UVeilHealthComponent::UVeilHealthComponent(){SetIsReplicatedByDefault(true);PrimaryComponentTick.bCanEverTick=true;}
void UVeilHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(UVeilHealthComponent,State);}
float UVeilHealthComponent::ArmorProtection(EVeilArmor A){return A==EVeilArmor::Heavy?16:A==EVeilArmor::Medium?10:4;}
float UVeilHealthComponent::MovementSpeed(EVeilArmor A){return A==EVeilArmor::Heavy?420:A==EVeilArmor::Medium?510:600;}
float UVeilHealthComponent::DamageAfterArmor(const FVeilWeaponSpec& S,EVeilRegion R,EVeilArmor A,float& Stopped){
 Stopped=R==EVeilRegion::Torso?FMath::Min(S.Damage,FMath::Max(0.f,ArmorProtection(A)-S.Penetration)):0;
 return FMath::Max(0.f,S.Damage-Stopped)*(R==EVeilRegion::Head?3.f:R==EVeilRegion::Limb?.65f:1.f);
}
void UVeilHealthComponent::Publish(){OnRep_State();GetOwner()->ForceNetUpdate();}
void UVeilHealthComponent::OnRep_State(){if(auto* C=Cast<ACombatCharacter>(GetOwner()))C->RefreshPresentation();}
bool UVeilHealthComponent::ApplyShot(const FVeilWeaponSpec& S,EVeilRegion R,AActor* Source){
 if(!GetOwner()->HasAuthority() || !S.Validate() || !IsValid(Source) || Source==GetOwner() || State.Life==EVeilLife::Dead || GetWorld()->GetTimeSeconds()<ProtectedUntil)return false;
 // This slice is cooperative. Player weapons cannot damage teammates.
 if(auto* SourceHealth=Source->FindComponentByClass<UVeilHealthComponent>(); SourceHealth && Team!=EVeilTeam::Neutral && SourceHealth->Team==Team)return false;
 State.LastDamage=DamageAfterArmor(S,R,State.Armor,State.LastArmorStopped);State.LastRegion=R;++State.DamageRevision;
 State.Health=FMath::Max(0.f,State.Health-State.LastDamage);if(bDiagnosticSingleHitShield&&State.Health>0)ProtectedUntil=GetWorld()->GetTimeSeconds()+60;
 if(State.Health<=0){State.Life=bAllowDowned && State.Life==EVeilLife::Alive?EVeilLife::Downed:EVeilLife::Dead;State.BleedEndTime=State.Life==EVeilLife::Downed?GetWorld()->GetTimeSeconds()+40:0;if(State.Life==EVeilLife::Dead)DeadAt=GetWorld()->GetTimeSeconds();}
 if(auto* C=GetOwner()->FindComponentByClass<UVeilCombatComponent>())C->CancelActions();Publish();return true;
}
bool UVeilHealthComponent::TrainingDown(){if(!GetOwner()->HasAuthority()||State.Life!=EVeilLife::Alive)return false;State.Health=0;State.Life=EVeilLife::Downed;State.BleedEndTime=GetWorld()->GetTimeSeconds()+40;++State.DamageRevision;if(auto* C=GetOwner()->FindComponentByClass<UVeilCombatComponent>())C->CancelActions();Publish();return true;}
bool UVeilHealthComponent::Recover(float Amount){if(!GetOwner()->HasAuthority()||State.Life==EVeilLife::Dead||!FMath::IsFinite(Amount)||Amount<=0)return false;State.Health=FMath::Clamp(Amount,1.f,100.f);State.Life=EVeilLife::Alive;State.BleedEndTime=0;ProtectedUntil=GetWorld()->GetTimeSeconds()+1;Publish();return true;}
bool UVeilHealthComponent::SetArmor(EVeilArmor A){if(!GetOwner()->HasAuthority()||State.Life!=EVeilLife::Alive||uint8(A)>uint8(EVeilArmor::Heavy))return false;State.Armor=A;Publish();return true;}
void UVeilHealthComponent::ResetHealth(){if(GetOwner()->HasAuthority()){State.Health=100;State.Life=EVeilLife::Alive;State.BleedEndTime=0;ProtectedUntil=GetWorld()->GetTimeSeconds()+2;Publish();}}
void UVeilHealthComponent::TickComponent(float D,ELevelTick T,FActorComponentTickFunction* F){Super::TickComponent(D,T,F);if(GetOwner()->HasAuthority()&&State.Life==EVeilLife::Downed&&GetWorld()->GetTimeSeconds()>=State.BleedEndTime){State.Life=EVeilLife::Dead;DeadAt=GetWorld()->GetTimeSeconds();if(auto* C=GetOwner()->FindComponentByClass<UVeilCombatComponent>())C->CancelActions();Publish();}}

UVeilCombatComponent::UVeilCombatComponent(){SetIsReplicatedByDefault(true);PrimaryComponentTick.bCanEverTick=true;}
void UVeilCombatComponent::BeginPlay(){Super::BeginPlay();if(GetOwner()->HasAuthority()){ServerSeed=FMath::Rand();State.Carried={"weapon.carbine","weapon.shotgun","weapon.pistol"};Restock();}}
void UVeilCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME_CONDITION(UVeilCombatComponent,State,COND_OwnerOnly);DOREPLIFETIME(UVeilCombatComponent,EquippedId);DOREPLIFETIME_CONDITION(UVeilCombatComponent,Definitions,COND_OwnerOnly);}
FVeilWeaponSpec UVeilCombatComponent::Spec(FName Id)const{for(const auto& Definition:Definitions)if(IsValid(Definition)&&Definition->Spec.StableId==Id&&Definition->Spec.Validate())return Definition->Spec;return UCombatDataLibrary::DefaultWeaponSpec(Id);}
FVeilAmmo* UVeilCombatComponent::CurrentAmmo(){return State.Ammo.FindByPredicate([this](const FVeilAmmo& A){return A.WeaponId==EquippedId;});}
const FVeilAmmo* UVeilCombatComponent::CurrentAmmo()const{return State.Ammo.FindByPredicate([this](const FVeilAmmo& A){return A.WeaponId==EquippedId;});}
bool UVeilCombatComponent::ValidateAmmo()const{TSet<FName> Ids;for(const auto& A:State.Ammo){const auto S=Spec(A.WeaponId);if(!S.Validate()||Ids.Contains(A.WeaponId)||A.Magazine<0||A.Magazine>S.MagazineSize||A.Reserve<0||A.Reserve>S.ReserveLimit)return false;Ids.Add(A.WeaponId);}return State.Carried.Num()==3&&State.Carried.IsValidIndex(State.EquippedSlot)&&State.Carried[State.EquippedSlot]==EquippedId&&CurrentAmmo();}
bool UVeilCombatComponent::ValidOwner()const{const auto* P=Cast<APawn>(GetOwner());return P&&P->HasAuthority()&&P->GetController()&&P->GetController()->GetPawn()==P;}
bool UVeilCombatComponent::LocalOwner()const{const auto* P=Cast<APawn>(GetOwner());return P&&P->IsLocallyControlled();}
void UVeilCombatComponent::Publish(){OnRep_State();GetOwner()->ForceNetUpdate();}
void UVeilCombatComponent::OnRep_State(){if(auto* C=Cast<ACombatCharacter>(GetOwner()))C->RefreshPresentation();}
void UVeilCombatComponent::OnRep_Equipped(){OnRep_State();}
void UVeilCombatComponent::StartFire(){bHeld=true;RequestShot();}
void UVeilCombatComponent::StopFire(){bHeld=false;}
void UVeilCombatComponent::RequestShot(){if(!LocalOwner())return;auto* H=GetOwner()->FindComponentByClass<UVeilHealthComponent>();const auto* A=CurrentAmmo();const auto S=Spec(EquippedId);const double Now=GetWorld()->GetTimeSeconds();if(!H||H->State.Life!=EVeilLife::Alive||State.bReloading||State.Use!=EVeilUse::None||!A||A->Magazine<=0||Now-LastLocalShot<S.ShotInterval)return;LastLocalShot=Now;LocalShotPresentation();FVector Eye;FRotator R;CastChecked<APawn>(GetOwner())->GetActorEyesViewPoint(Eye,R);ServerFire(R.Vector(),++LocalSequence);DrawDebugPoint(GetWorld(),Eye+R.Vector()*55+FRotationMatrix(R).GetUnitAxis(EAxis::Y)*12-FVector(0,0,16),12,FColor::Yellow,false,.06f);CastChecked<APawn>(GetOwner())->AddControllerPitchInput(-S.RecoilDegrees*.2f);}
void UVeilCombatComponent::RequestReload(){if(LocalOwner())ServerReload();}
void UVeilCombatComponent::RequestSlot(int32 S){if(LocalOwner())ServerEquip(S);}
void UVeilCombatComponent::RequestMedical(){if(LocalOwner())ServerUse(EVeilUse::Medical);}
void UVeilCombatComponent::RequestAmmoKit(){if(LocalOwner())ServerUse(EVeilUse::Ammunition);}
void UVeilCombatComponent::RequestRespawn(){if(LocalOwner())ServerRespawn();}
void UVeilCombatComponent::ServerFire_Implementation(FVector_NetQuantizeNormal Aim,uint32 Sequence){
 bool Accepted=false;auto* H=GetOwner()->FindComponentByClass<UVeilHealthComponent>();auto* A=CurrentAmmo();const auto S=Spec(EquippedId);const FVector Direction(Aim);const double Now=GetWorld()->GetTimeSeconds();
 if(ValidOwner()&&H&&H->State.Life==EVeilLife::Alive&&S.Validate()&&ValidateAmmo()&&!State.bReloading&&State.Use==EVeilUse::None&&A&&A->Magazine>0&&Sequence>LastSequence&&!Direction.ContainsNaN()&&FMath::Abs(Direction.SizeSquared()-1)<.02f&&Now-LastServerShot+0.001>=S.ShotInterval){
  FVector Eye;FRotator R;CastChecked<APawn>(GetOwner())->GetActorEyesViewPoint(Eye,R);
  if(FVector::DotProduct(Direction,R.Vector())>=.96f){LastSequence=Sequence;LastServerShot=Now;--A->Magazine;++State.AcceptedShots;Accepted=true;
   const FVector Breech=Eye+FRotationMatrix(R).GetUnitAxis(EAxis::Y)*12-FVector(0,0,16),Muzzle=Breech+Direction*55;
   FRandomStream Random(ServerSeed+State.AcceptedShots);float Total=0;FString Region="miss";
   for(int32 P=0;P<S.Pellets;++P)ResolvePellet(S,Eye,Random.VRandCone(Direction,FMath::DegreesToRadians(S.SpreadDegrees)),Breech,Muzzle,Total,Region);
   State.Feedback=FString::Printf(TEXT("%s | %.0f damage"),*Region,Total);Publish();
  }
 }
 ClientShotResult(Sequence,Accepted);
}
void UVeilCombatComponent::ResolvePellet(const FVeilWeaponSpec& S,const FVector& Eye,const FVector& Direction,const FVector& Breech,const FVector& Muzzle,float& Total,FString& Region){
 FCollisionQueryParams Q(SCENE_QUERY_STAT(VeilFire),false,GetOwner());FHitResult EyeHit,Hit;const FVector Far=Eye+Direction*S.RangeCm;
 GetWorld()->LineTraceSingleByChannel(EyeHit,Eye,Far,ECC_Visibility,Q);
 bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,Eye,Breech,ECC_Visibility,Q)||GetWorld()->LineTraceSingleByChannel(Hit,Breech,Muzzle,ECC_Visibility,Q);
 if(!Blocked)Blocked=GetWorld()->LineTraceSingleByChannel(Hit,Muzzle,EyeHit.bBlockingHit?EyeHit.ImpactPoint+(EyeHit.ImpactPoint-Muzzle).GetSafeNormal()*2:Far,ECC_Visibility,Q);
 if(Blocked){EVeilRegion R=EVeilRegion::Torso;if(Hit.GetComponent()&&Hit.GetComponent()->ComponentHasTag("Head"))R=EVeilRegion::Head;else if(Hit.GetComponent()&&Hit.GetComponent()->ComponentHasTag("Limb"))R=EVeilRegion::Limb;
  if(auto* H=Hit.GetActor()?Hit.GetActor()->FindComponentByClass<UVeilHealthComponent>():nullptr){if(H->ApplyShot(S,R,GetOwner())){Total+=H->State.LastDamage;Region=R==EVeilRegion::Head?"head":R==EVeilRegion::Limb?"limb":"torso";}}else Region="blocked";
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("G3Smoke"))&&GetOwner()->GetName().Contains(TEXT("SquadEnemy")))UE_LOG(LogTemp,Display,TEXT("G3_SHOT_TRACE eye=%s dir=%s eyeActor=%s muzzleActor=%s component=%s"),*Eye.ToString(),*Direction.ToString(),*GetNameSafe(EyeHit.GetActor()),*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()));
 MulticastTrace(Muzzle,Blocked?Hit.ImpactPoint:Far,Blocked);
}
void UVeilCombatComponent::ClientShotResult_Implementation(uint32 Sequence,bool Accepted){ConfirmedShotPresentation(Accepted);}
void UVeilCombatComponent::MulticastTrace_Implementation(FVector_NetQuantize Start,FVector_NetQuantize End,bool Hit){if(GetWorld()->GetNetMode()!=NM_DedicatedServer)DrawDebugLine(GetWorld(),Start,End,Hit?FColor::Orange:FColor::Yellow,false,.08f,0,.6f);}
void UVeilCombatComponent::ServerReload_Implementation(){auto* H=GetOwner()->FindComponentByClass<UVeilHealthComponent>();auto* A=CurrentAmmo();const auto S=Spec(EquippedId);if(!ValidOwner()||!H||H->State.Life!=EVeilLife::Alive||!A||!S.Validate()||State.bReloading||State.Use!=EVeilUse::None||A->Magazine>=S.MagazineSize||A->Reserve<=0)return;State.bReloading=true;ReloadWeapon=EquippedId;State.ReloadEndTime=GetWorld()->GetTimeSeconds()+S.ReloadSeconds;State.Feedback="Reloading";Publish();}
void UVeilCombatComponent::CompleteReload(){auto* A=CurrentAmmo();if(State.bReloading&&ReloadWeapon==EquippedId&&A){const int32 Transfer=FMath::Min(Spec(EquippedId).MagazineSize-A->Magazine,A->Reserve);A->Magazine+=Transfer;A->Reserve-=Transfer;}State.bReloading=false;State.ReloadEndTime=0;State.Feedback="Reload complete";Publish();}
void UVeilCombatComponent::CancelActions(){if(!GetOwner()->HasAuthority())return;State.bReloading=false;State.ReloadEndTime=0;State.Use=EVeilUse::None;State.UseEndTime=0;UseTarget=nullptr;Publish();}
void UVeilCombatComponent::ServerEquip_Implementation(int32 Index){auto* H=GetOwner()->FindComponentByClass<UVeilHealthComponent>();if(!ValidOwner()||!H||H->State.Life!=EVeilLife::Alive||!State.Carried.IsValidIndex(Index)||!Spec(State.Carried[Index]).Validate())return;CancelActions();State.EquippedSlot=Index;EquippedId=State.Carried[Index];State.Feedback=Spec(EquippedId).DisplayName;Publish();}
bool UVeilCombatComponent::Restock(){if(!GetOwner()->HasAuthority())return false;auto* H=GetOwner()->FindComponentByClass<UVeilHealthComponent>();if(H&&H->State.Life!=EVeilLife::Alive)return false;CancelActions();for(FName Id:State.Carried){auto* A=State.Ammo.FindByPredicate([Id](const FVeilAmmo& Entry){return Entry.WeaponId==Id;});if(!A){FVeilAmmo New;New.WeaponId=Id;State.Ammo.Add(New);A=&State.Ammo.Last();}const auto S=Spec(Id);A->Magazine=S.MagazineSize;A->Reserve=S.ReserveLimit;}State.MedicalCharges=2;State.AmmoCharges=1;State.Feedback="Restocked";Publish();return true;}
bool UVeilCombatComponent::SwapPrimary(){if(!ValidOwner())return false;auto* H=GetOwner()->FindComponentByClass<UVeilHealthComponent>();if(!H||H->State.Life!=EVeilLife::Alive||State.Carried.Num()!=3)return false;CancelActions();const FName Id=State.Carried[0]=="weapon.carbine"?FName("weapon.marksman"):FName("weapon.carbine");State.Carried[0]=Id;if(!State.Ammo.ContainsByPredicate([Id](const FVeilAmmo& A){return A.WeaponId==Id;})){FVeilAmmo A;A.WeaponId=Id;A.Magazine=Spec(Id).MagazineSize;A.Reserve=Spec(Id).ReserveLimit;State.Ammo.Add(A);}if(State.EquippedSlot==0)EquippedId=Id;Publish();return true;}
bool UVeilCombatComponent::CycleArmor(){auto* H=GetOwner()->FindComponentByClass<UVeilHealthComponent>();return ValidOwner()&&H&&H->SetArmor(EVeilArmor((uint8(H->State.Armor)+1)%3));}
bool UVeilCombatComponent::BeginUse(EVeilUse Kind,UVeilHealthComponent* Target){auto* H=GetOwner()->FindComponentByClass<UVeilHealthComponent>();if(!ValidOwner()||!H||!Target||State.Use!=EVeilUse::None||H->State.Life==EVeilLife::Dead)return false;
 if(Kind==EVeilUse::Ammunition){if(H->State.Life!=EVeilLife::Alive||State.AmmoCharges<=0)return false;bool Needed=false;for(const auto& A:State.Ammo)if(State.Carried.Contains(A.WeaponId)&&A.Reserve<Spec(A.WeaponId).ReserveLimit)Needed=true;if(!Needed)return false;}
 else {if(State.MedicalCharges<=0||Target->State.Life==EVeilLife::Dead)return false;if(Kind==EVeilUse::Revive&&(H->State.Life!=EVeilLife::Alive||Target==H||Target->State.Life!=EVeilLife::Downed))return false;if(Kind==EVeilUse::Medical&&H->State.Life==EVeilLife::Alive&&H->State.Health>=100)return false;}
 CancelActions();UseTarget=Target;UseStartLocation=GetOwner()->GetActorLocation();UseDamageRevision=H->State.DamageRevision;State.Use=Kind;State.UseEndTime=GetWorld()->GetTimeSeconds()+(Kind==EVeilUse::Ammunition?1.5f:H->State.Life==EVeilLife::Downed?5.f:3.f);
 if(!UseStillValid()){CancelActions();return false;}State.Feedback="Hold position to complete";Publish();return true;
}
bool UVeilCombatComponent::UseStillValid()const{auto* H=GetOwner()->FindComponentByClass<UVeilHealthComponent>();if(!ValidOwner()||!H||!IsValid(UseTarget)||H->State.Life==EVeilLife::Dead||UseTarget->State.Life==EVeilLife::Dead||H->State.DamageRevision!=UseDamageRevision||FVector::DistSquared(UseStartLocation,GetOwner()->GetActorLocation())>FMath::Square(35.f))return false;if(State.Use==EVeilUse::Revive){if(H->State.Life!=EVeilLife::Alive||UseTarget->State.Life!=EVeilLife::Downed||FVector::DistSquared(GetOwner()->GetActorLocation(),UseTarget->GetOwner()->GetActorLocation())>FMath::Square(300.f))return false;FVector Eye;FRotator R;CastChecked<APawn>(GetOwner())->GetActorEyesViewPoint(Eye,R);FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(VeilRevive),false,GetOwner());if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,UseTarget->GetOwner()->GetActorLocation(),ECC_Visibility,Q)&&Hit.GetActor()!=UseTarget->GetOwner())return false;}return true;}
void UVeilCombatComponent::CompleteUse(){if(!UseStillValid()){CancelActions();return;}if(State.Use==EVeilUse::Ammunition&&State.AmmoCharges>0){for(auto& A:State.Ammo)if(State.Carried.Contains(A.WeaponId))A.Reserve=Spec(A.WeaponId).ReserveLimit;--State.AmmoCharges;}else if(State.MedicalCharges>0&&UseTarget->Recover(State.Use==EVeilUse::Revive||UseTarget->State.Life==EVeilLife::Downed?50:100))--State.MedicalCharges;CancelActions();State.Feedback="Use complete";Publish();}
bool UVeilCombatComponent::BeginRevive(APawn* Target){return IsValid(Target)&&Target!=GetOwner()&&BeginUse(EVeilUse::Revive,Target->FindComponentByClass<UVeilHealthComponent>());}
void UVeilCombatComponent::ServerUse_Implementation(EVeilUse Kind){if(Kind==EVeilUse::Medical||Kind==EVeilUse::Ammunition)BeginUse(Kind,GetOwner()->FindComponentByClass<UVeilHealthComponent>());}
void UVeilCombatComponent::ServerRespawn_Implementation(){auto* H=GetOwner()->FindComponentByClass<UVeilHealthComponent>();auto* P=Cast<APawn>(GetOwner());if(!ValidOwner()||!H||H->State.Life!=EVeilLife::Dead||GetWorld()->GetTimeSeconds()-H->DeadAt<3)return;if(auto* Mode=GetWorld()->GetAuthGameMode()){if(auto* Start=Mode->FindPlayerStart(P->GetController())){if(P->TeleportTo(Start->GetActorLocation(),Start->GetActorRotation())){H->ResetHealth();CancelActions();}}}}
void UVeilCombatComponent::TickComponent(float D,ELevelTick T,FActorComponentTickFunction* F){Super::TickComponent(D,T,F);if(bHeld&&LocalOwner()&&Spec(EquippedId).bAutomatic)RequestShot();if(GetOwner()->HasAuthority()){auto* H=GetOwner()->FindComponentByClass<UVeilHealthComponent>();if(State.bReloading){if(!ValidOwner()||!H||H->State.Life!=EVeilLife::Alive)CancelActions();else if(GetWorld()->GetTimeSeconds()>=State.ReloadEndTime)CompleteReload();}if(State.Use!=EVeilUse::None){if(!UseStillValid())CancelActions();else if(GetWorld()->GetTimeSeconds()>=State.UseEndTime)CompleteUse();}}}

ACombatCharacter::ACombatCharacter(const FObjectInitializer& Initializer):Super(Initializer){Health=CreateDefaultSubobject<UVeilHealthComponent>("Health");Combat=CreateDefaultSubobject<UVeilCombatComponent>("Combat");static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto Make=[this](const TCHAR* Name,USceneComponent* Parent,FVector Location,FVector Scale){auto* M=CreateDefaultSubobject<UStaticMeshComponent>(Name);M->SetupAttachment(Parent);M->SetStaticMesh(Cube.Object);M->SetRelativeLocation(Location);M->SetRelativeScale3D(Scale);M->SetCollisionEnabled(ECollisionEnabled::NoCollision);return M;};
 // Stock UE first-person template model: full-body Manny for everyone else, first-person copy for the owner.
 static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
 static ConstructorHelpers::FClassFinder<UAnimInstance> BodyAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
 static ConstructorHelpers::FClassFinder<UAnimInstance> FirstPersonAnim(TEXT("/Game/FirstPerson/Anims/ABP_FP_Copy"));
 GetMesh()->SetSkeletalMesh(Manny.Object);GetMesh()->SetAnimInstanceClass(BodyAnim.Class);GetMesh()->SetRelativeLocationAndRotation(FVector(0,0,-GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()),FRotator(0,-90,0));GetMesh()->SetOwnerNoSee(true);GetMesh()->FirstPersonPrimitiveType=EFirstPersonPrimitiveType::WorldSpaceRepresentation;GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 FirstPersonMesh=CreateDefaultSubobject<USkeletalMeshComponent>("FirstPersonMesh");FirstPersonMesh->SetupAttachment(GetMesh());FirstPersonMesh->SetSkeletalMesh(Manny.Object);FirstPersonMesh->SetAnimInstanceClass(FirstPersonAnim.Class);FirstPersonMesh->SetOnlyOwnerSee(true);FirstPersonMesh->FirstPersonPrimitiveType=EFirstPersonPrimitiveType::FirstPerson;FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);FirstPersonMesh->CastShadow=false;
 Camera->bEnableFirstPersonFieldOfView=true;Camera->bEnableFirstPersonScale=true;Camera->FirstPersonFieldOfView=70;Camera->FirstPersonScale=.6f;
 Weapon=Make(TEXT("Weapon"),Camera,FVector(45,18,-20),FVector(.55,.08,.10));Weapon->SetOnlyOwnerSee(true);WorldWeapon=Make(TEXT("WorldWeapon"),GetCapsuleComponent(),FVector(35,15,30),FVector(.55,.08,.10));WorldWeapon->SetOwnerNoSee(true);
 auto HitBox=[this](const TCHAR* Name,FVector Location,FVector Extent,FName Tag){auto* B=CreateDefaultSubobject<UBoxComponent>(Name);B->SetupAttachment(GetCapsuleComponent());B->SetRelativeLocation(Location);B->SetBoxExtent(Extent);B->SetCollisionEnabled(ECollisionEnabled::QueryOnly);B->SetCollisionResponseToAllChannels(ECR_Ignore);B->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);B->ComponentTags.Add(Tag);};
 GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);HitBox(TEXT("TorsoHit"),FVector(0,0,10),FVector(22,25,38),"Torso");HitBox(TEXT("HeadHit"),FVector(0,0,65),FVector(16,16,16),"Head");HitBox(TEXT("LimbHit"),FVector(0,0,-55),FVector(20,22,25),"Limb");
}
void ACombatCharacter::RefreshPresentation(){if(!Health||!Combat)return;const bool Alive=Health->State.Life==EVeilLife::Alive;const bool Dead=Health->State.Life==EVeilLife::Dead;GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,Dead?ECR_Ignore:ECR_Block);TArray<UBoxComponent*> HitBoxes;GetComponents(HitBoxes);for(auto* Box:HitBoxes)Box->SetCollisionEnabled(Dead?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryOnly);GetCharacterMovement()->MaxWalkSpeed=Alive?UVeilHealthComponent::MovementSpeed(Health->State.Armor):80;GetCharacterMovement()->JumpZVelocity=Alive?420:0;if(Health->State.Life==EVeilLife::Dead)GetCharacterMovement()->DisableMovement();else if(GetCharacterMovement()->MovementMode==MOVE_None)GetCharacterMovement()->SetMovementMode(MOVE_Walking);GetMesh()->SetRelativeRotation(Alive?FRotator(0,-90,0):FRotator(0,-90,75));FirstPersonMesh->SetVisibility(Alive);Weapon->SetVisibility(Alive);WorldWeapon->SetVisibility(Alive);}
void ACombatCharacter::Tick(float D){Super::Tick(D);SmokeTick();if(IsLocallyControlled()){const float FOV=Combat->IsADS()?65:90;Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView,FOV,D,12));}}
bool ACombatCharacter::Activate_Implementation(APawn* User){if(!HasAuthority()||!IsValid(User))return false;auto* C=User->FindComponentByClass<UVeilCombatComponent>();return C&&C->BeginRevive(this);}
void ACombatCharacter::SetupPlayerInputComponent(UInputComponent* Input){Super::SetupPlayerInputComponent(Input);auto* E=CastChecked<UEnhancedInputComponent>(Input);CombatContext=NewObject<UInputMappingContext>(this);
 auto Bind=[this,E](FKey Key,auto Method){auto* A=NewObject<UInputAction>(this);CombatActions.Add(A);CombatContext->MapKey(A,Key);E->BindAction(A,ETriggerEvent::Started,this,Method);return A;};
 auto* Fire=Bind(EKeys::LeftMouseButton,&ACombatCharacter::FireStart);E->BindAction(Fire,ETriggerEvent::Completed,this,&ACombatCharacter::FireStop);E->BindAction(Fire,ETriggerEvent::Canceled,this,&ACombatCharacter::FireStop);
 auto* Aim=Bind(EKeys::RightMouseButton,&ACombatCharacter::ADSStart);E->BindAction(Aim,ETriggerEvent::Completed,this,&ACombatCharacter::ADSStop);E->BindAction(Aim,ETriggerEvent::Canceled,this,&ACombatCharacter::ADSStop);
 Bind(EKeys::R,&ACombatCharacter::Reload);Bind(EKeys::One,&ACombatCharacter::SlotPrimary);Bind(EKeys::Two,&ACombatCharacter::SlotSecondary);Bind(EKeys::Three,&ACombatCharacter::SlotSidearm);Bind(EKeys::G,&ACombatCharacter::Medical);Bind(EKeys::H,&ACombatCharacter::AmmoKit);Bind(EKeys::Enter,&ACombatCharacter::Respawn);
 if(auto* PC=Cast<APlayerController>(GetController()))if(auto* LP=PC->GetLocalPlayer())ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP)->AddMappingContext(CombatContext,1);
}
void ACombatCharacter::FireStart(){Combat->StartFire();}void ACombatCharacter::FireStop(){Combat->StopFire();}void ACombatCharacter::Reload(){Combat->RequestReload();}void ACombatCharacter::SlotPrimary(){Combat->RequestSlot(0);}void ACombatCharacter::SlotSecondary(){Combat->RequestSlot(1);}void ACombatCharacter::SlotSidearm(){Combat->RequestSlot(2);}void ACombatCharacter::ADSStart(){Combat->SetADS(true);}void ACombatCharacter::ADSStop(){Combat->SetADS(false);}void ACombatCharacter::Medical(){Combat->RequestMedical();}void ACombatCharacter::AmmoKit(){Combat->RequestAmmoKit();}void ACombatCharacter::Respawn(){Combat->RequestRespawn();}

ACombatTarget::ACombatTarget(){bReplicates=true;PrimaryActorTick.bCanEverTick=true;Health=CreateDefaultSubobject<UVeilHealthComponent>("Health");Health->bAllowDowned=false;Health->Team=EVeilTeam::Neutral;TorsoHit=CreateDefaultSubobject<UBoxComponent>("TorsoHit");SetRootComponent(TorsoHit);TorsoHit->SetBoxExtent(FVector(20,25,40));TorsoHit->SetCollisionProfileName("BlockAll");HeadHit=CreateDefaultSubobject<UBoxComponent>("HeadHit");HeadHit->SetupAttachment(TorsoHit);HeadHit->SetRelativeLocation(FVector(0,0,60));HeadHit->SetBoxExtent(FVector(16));HeadHit->SetCollisionProfileName("BlockAll");HeadHit->ComponentTags.Add("Head");LimbHit=CreateDefaultSubobject<UBoxComponent>("LimbHit");LimbHit->SetupAttachment(TorsoHit);LimbHit->SetRelativeLocation(FVector(0,0,-65));LimbHit->SetBoxExtent(FVector(20,22,25));LimbHit->SetCollisionProfileName("BlockAll");LimbHit->ComponentTags.Add("Limb");
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));auto Make=[this](const TCHAR* Name,USceneComponent* Parent,FVector Scale){auto* M=CreateDefaultSubobject<UStaticMeshComponent>(Name);M->SetupAttachment(Parent);M->SetStaticMesh(Cube.Object);M->SetRelativeScale3D(Scale);M->SetCollisionEnabled(ECollisionEnabled::NoCollision);};Make(TEXT("Torso"),TorsoHit,FVector(.4,.5,.8));Make(TEXT("Head"),HeadHit,FVector(.32));Make(TEXT("Legs"),LimbHit,FVector(.4,.44,.5));Label=CreateDefaultSubobject<UTextRenderComponent>("Label");Label->SetupAttachment(TorsoHit);Label->SetRelativeLocation(FVector(0,0,105));Label->SetWorldSize(16);}
void ACombatTarget::BeginPlay(){Super::BeginPlay();if(HasAuthority())Health->SetArmor(ArmorPreset);}
void ACombatTarget::Tick(float D){Super::Tick(D);Label->SetText(FText::FromString(FString::Printf(TEXT("%s | %s armor | HP %.0f\n%s | last %.0f (armor %.0f)"),*StableId.ToString(),Health->State.Armor==EVeilArmor::Light?TEXT("Light"):Health->State.Armor==EVeilArmor::Medium?TEXT("Medium"):TEXT("Heavy"),Health->State.Health,Health->State.Life==EVeilLife::Dead?TEXT("F: reset target"):TEXT("Training target"),Health->State.LastDamage,Health->State.LastArmorStopped)));}
bool ACombatTarget::Activate_Implementation(APawn* User){if(!HasAuthority()||!IsValid(User)||Health->State.Life!=EVeilLife::Dead)return false;Health->ResetHealth();return true;}
ACombatLocker::ACombatLocker(){bReplicates=true;Mesh=CreateDefaultSubobject<UStaticMeshComponent>("Mesh");SetRootComponent(Mesh);static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));Mesh->SetStaticMesh(Cube.Object);Mesh->SetCollisionProfileName("BlockAll");Mesh->SetRelativeScale3D(FVector(.5,.5,.8));Label=CreateDefaultSubobject<UTextRenderComponent>("Label");Label->SetupAttachment(Mesh);Label->SetRelativeLocation(FVector(0,0,90));Label->SetWorldSize(20);}
void ACombatLocker::BeginPlay(){Super::BeginPlay();const TCHAR* Text=Action==ECombatBenchAction::PrimarySwap?TEXT("F: carbine / marksman"):Action==ECombatBenchAction::ArmorCycle?TEXT("F: change armor"):Action==ECombatBenchAction::DownSelf?TEXT("F: down SELF (training)"):TEXT("F: restock");Label->SetText(FText::FromString(Text));}
bool ACombatLocker::Activate_Implementation(APawn* User){if(!HasAuthority()||!IsValid(User)||FVector::DistSquared(User->GetActorLocation(),GetActorLocation())>FMath::Square(350.f))return false;auto* C=User->FindComponentByClass<UVeilCombatComponent>();auto* H=User->FindComponentByClass<UVeilHealthComponent>();if(!C||!H||H->State.Life!=EVeilLife::Alive)return false;if(!Catalog.IsEmpty())C->Definitions=Catalog;return Action==ECombatBenchAction::PrimarySwap?C->SwapPrimary():Action==ECombatBenchAction::ArmorCycle?C->CycleArmor():Action==ECombatBenchAction::DownSelf?H->TrainingDown():C->Restock();}
ACombatGameMode::ACombatGameMode(){DefaultPawnClass=ACombatCharacter::StaticClass();}









