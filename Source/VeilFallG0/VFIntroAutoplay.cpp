// Development-only autoplay: -VFAutoplay drives the intro from NORMAL_HOME to MISSION_COMPLETE without input,
// making the same interactions a player would, so the whole state chain can be smoke-tested headless.
#include "VFIntro.h"
#include "VFIntroShared.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"

void AVFMissionController::Autoplay(){
#if !UE_BUILD_SHIPPING
 using S=EVFIntroState;auto* P=Player();if(!P)return;
 if(P->Health->State.Life==EVeilLife::Alive&&P->Health->State.Health<70)P->Health->Recover(100); // autoplay pawn is not testing survival
 auto Use=[this,P](FName Id){if(auto* I=Item(Id))if(I->UsableNow())IFoundationInteractable::Execute_Activate(I,P);};
 auto Near=[this,P](FName M){bool b=false;const FTransform T=MarkerTransform(M,&b);if(b)P->SetActorLocation(T.GetLocation()+FVector(0,0,100),false,nullptr,ETeleportType::TeleportPhysics);};
 const double Now=GetWorld()->GetTimeSeconds();
 switch(State){
 case S::NORMAL_HOME:for(const FName R:{FName("wife"),FName("son_oldest"),FName("son_middle")})if(auto* N=NPC(R))if(Talkable(N)){OnInteract(N,FName(*(FString("talk.")+R.ToString())),P);break;}break;
 case S::MOVE_TO_SHOP:Near("player_shop");break;
 case S::OPEN_VAULT:if(Now-StateStart>6)OnKeypadResult(true);break;
 case S::EQUIP:for(const FName& G:{FName("equip.rifle"),FName("equip.sidearm"),FName("equip.armor"),FName("equip.ammo"),FName("equip.radio"),FName("equip.medical")})Use(G);break;
 case S::LOCALS_ARRIVE:if(bTruckStopped)Near("player_baydoor");Use("panel.road");break;
 case S::SECURE_CHILDREN:if(bKidsInVault&&!bRadioGiven){if(auto* N=NPC("son_oldest"))OnInteract(N,"talk.son_oldest",P);}else Use("vault.keypad");break;
 case S::NCD_ARRIVAL:Use("shop.monitors");break;
 case S::NCD_DEMAND:Use("panel.intercom");Use("panel.road");break;
 case S::DEFENSE_ARMED:Use("panel.road");break;
 case S::PROPERTY_ATTACK:Use("panel.bay");break;
 case S::HOLD_SHOP:case S::VANGUARD_ARRIVAL:
  Near("player_port");
  if(Now-AutoLastKill>2.5)for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It)if(It->Faction==EVFFaction::NCD&&It->IsAlive()&&!It->IsHidden()){VF::Kill(It->Health);OnPawnKilled(*It);++AutoKills;AutoLastKill=Now;break;}
  break;
 case S::EXTRACT_FAMILY:Use("vault.keypad");break;
 case S::EVACUATE:Use("evac.board");break;
 case S::SCUTTLE_PROPERTY:OnPlayerUse();break;
 case S::MISSION_COMPLETE:
  if(Now-StateStart>6){UE_LOG(LogTemp,Display,TEXT("VF_AUTOPLAY_COMPLETE seconds=%.0f autoKills=%d"),Now,AutoKills);GetWorldTimerManager().ClearTimer(AutoTimer);FPlatformMisc::RequestExit(false);}
  break;
 default:break;
 }
#endif
}
