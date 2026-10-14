#include "Combat.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

void ACombatCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ACombatCharacter,DiagnosticTarget);}
void ACombatCharacter::SmokeTick(){
 // Opt-in automated fixture. Never active in ordinary editor or packaged play.
 if(!FParse::Param(FCommandLine::Get(),TEXT("G2Smoke"))||!GetController())return;
 const double Now=GetWorld()->GetTimeSeconds();
 if(HasAuthority()&&!IsLocallyControlled()&&!DiagnosticTarget){
  const FVector Location(100000+float(GetUniqueID()%100)*3000,100000,10000);
  SetActorLocation(Location);GetCharacterMovement()->SetMovementMode(MOVE_Flying);GetCharacterMovement()->MaxFlySpeed=0;
  GetController()->SetControlRotation(FRotator::ZeroRotator);CastChecked<APlayerController>(GetController())->ClientSetRotation(FRotator::ZeroRotator);
  DiagnosticTarget=GetWorld()->SpawnActor<ACombatTarget>(Location+FVector(500,0,64),FRotator::ZeroRotator);DiagnosticTarget->SetReplicates(true);ForceNetUpdate();
 }
 if(HasAuthority()&&!IsLocallyControlled()&&Combat->State.AcceptedShots>=3&&Combat->State.AmmoCharges==0&&SmokePhase==0){
  SmokePhase=99;Health->TrainingDown();
 }
 if(HasAuthority()&&!IsLocallyControlled()&&SmokePhase==99&&Health->State.Life==EVeilLife::Alive&&Health->State.Health==50&&Combat->State.MedicalCharges==1){SmokePhase=100;UE_LOG(LogTemp,Log,TEXT("G2_SMOKE_SERVER_PASS pawn=%s shots=%d validAmmo=%d targetLife=%d recoveredHP=50 med=1"),*GetName(),Combat->State.AcceptedShots,Combat->ValidateAmmo(),DiagnosticTarget?int32(DiagnosticTarget->Health->State.Life):-1);}
 if(!IsLocallyControlled()||HasAuthority()||!DiagnosticTarget||Now<SmokeNext)return;
 GetController()->SetControlRotation(FRotator::ZeroRotator);
 auto* A=Combat->CurrentAmmo();const auto& S=Combat->State;if(!A)return;
 if(SmokePhase==0){SmokePhase=1;SmokeNext=Now+1;return;}
 if(SmokePhase==1){Combat->RequestShot();SmokePhase=2;SmokeNext=Now+.5;}
 else if(SmokePhase==2&&S.AcceptedShots==1&&A->Magazine==29&&DiagnosticTarget->Health->State.Health==50){Combat->RequestReload();SmokePhase=3;}
 else if(SmokePhase==3&&!S.bReloading&&A->Magazine==30&&A->Reserve==89){Combat->RequestSlot(1);SmokePhase=4;}
 else if(SmokePhase==4&&S.EquippedSlot==1){Combat->RequestShot();SmokePhase=5;SmokeNext=Now+.5;}
 else if(SmokePhase==5&&S.AcceptedShots==2&&A->Magazine==5){Combat->RequestSlot(2);SmokePhase=6;}
 else if(SmokePhase==6&&S.EquippedSlot==2){Combat->RequestShot();SmokePhase=7;SmokeNext=Now+.5;}
 else if(SmokePhase==7&&S.AcceptedShots==3&&A->Magazine==14){Combat->RequestAmmoKit();SmokePhase=8;}
 else if(SmokePhase==8&&S.AmmoCharges==0&&Combat->ValidateAmmo()&&DiagnosticTarget->Health->State.Health<=19&&Health->State.Life==EVeilLife::Downed){Combat->RequestMedical();SmokePhase=9;}
 else if(SmokePhase==9&&Health->State.Life==EVeilLife::Alive&&Health->State.Health==50&&S.MedicalCharges==1){SmokePhase=10;UE_LOG(LogTemp,Log,TEXT("G2_SMOKE_CLIENT_PASS shots=3 carbineReload=30/89 shotgun=5 pistol=14 ammoKit=0 targetHP<=19 recoveredHP=50 med=1"));}
}

