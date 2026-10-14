#include "VFIntro.h"
#include "VFIntroShared.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace {
constexpr float StepSeconds=1.f/30.f;
UStaticMesh* EngineShape(const TCHAR* Name){return LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Name,Name));}
UStaticMeshComponent* Part(AActor* Owner,const TCHAR* Name,USceneComponent* Parent,UStaticMesh* Mesh,FVector Location,FVector Scale,bool bCollide=true){
 auto* C=Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);C->SetupAttachment(Parent);C->SetStaticMesh(Mesh);C->SetRelativeLocation(Location);C->SetRelativeScale3D(Scale);
 if(!bCollide)C->SetCollisionEnabled(ECollisionEnabled::NoCollision);return C;
}
float Ease(float A){return A*A*(3-2*A);}
}

// ---------------------------------------------------------------- markers
AVFMarker::AVFMarker(){
 SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));
 Arrow=CreateDefaultSubobject<UArrowComponent>("Arrow");Arrow->SetupAttachment(GetRootComponent());Arrow->ArrowColor=FColor(80,200,255);Arrow->bIsScreenSizeScaled=true;
 SetActorHiddenInGame(true);
}
AVFCoverPoint::AVFCoverPoint(){
 SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));
 Arrow=CreateDefaultSubobject<UArrowComponent>("Arrow");Arrow->SetupAttachment(GetRootComponent());Arrow->ArrowColor=FColor(255,120,40);Arrow->bIsScreenSizeScaled=true;
 SetActorHiddenInGame(true);
}

// ---------------------------------------------------------------- animated props
AVFAnimatedProp::AVFAnimatedProp(){
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));
 Hinge=CreateDefaultSubobject<USceneComponent>("Hinge");Hinge->SetupAttachment(GetRootComponent());
 Mesh=CreateDefaultSubobject<UStaticMeshComponent>("Mesh");Mesh->SetupAttachment(Hinge);Mesh->SetStaticMesh(Cube.Object);Mesh->SetMobility(EComponentMobility::Movable);
 Hinge->SetMobility(EComponentMobility::Movable);GetRootComponent()->SetMobility(EComponentMobility::Movable);
}
void AVFAnimatedProp::OnConstruction(const FTransform& T){Super::OnConstruction(T);VF::Paint(Mesh,Color);}
void AVFAnimatedProp::BeginPlay(){Super::BeginPlay();VF::Paint(Mesh,Color);}
void AVFAnimatedProp::SetOpen(bool bOpen,bool bInstant){
 bOpenTarget=bOpen;
 if(bInstant){Alpha=bOpen?1:0;Step();GetWorldTimerManager().ClearTimer(Timer);return;}
 GetWorldTimerManager().SetTimer(Timer,this,&AVFAnimatedProp::Step,StepSeconds,true);
}
void AVFAnimatedProp::Step(){
 const float Target=bOpenTarget?1.f:0.f;Alpha=FMath::FInterpConstantTo(Alpha,Target,StepSeconds,1.f/FMath::Max(.05f,Duration));
 const float E=Ease(Alpha);Hinge->SetRelativeLocationAndRotation(OpenOffset*E,FQuat::Slerp(FQuat::Identity,OpenRotation.Quaternion(),E));
 if(FMath::IsNearlyEqual(Alpha,Target))GetWorldTimerManager().ClearTimer(Timer);
}

// ---------------------------------------------------------------- interactables
AVFInteractable::AVFInteractable(){
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 bReplicates=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));
 Mesh=CreateDefaultSubobject<UStaticMeshComponent>("Mesh");Mesh->SetupAttachment(GetRootComponent());Mesh->SetStaticMesh(Cube.Object);Mesh->SetRelativeScale3D(FVector(.4f));
 Mesh->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
 Screen=CreateDefaultSubobject<UTextRenderComponent>("Screen");Screen->SetupAttachment(GetRootComponent());Screen->SetHorizontalAlignment(EHTA_Center);Screen->SetVerticalAlignment(EVRTA_TextCenter);Screen->SetWorldSize(9);Screen->SetRelativeLocation(FVector(25,0,0));
}
void AVFInteractable::BeginPlay(){Super::BeginPlay();OnConstruction(GetActorTransform());}
void AVFInteractable::OnConstruction(const FTransform& T){Super::OnConstruction(T);VF::Paint(Mesh,Color);Screen->SetText(FText::FromString(ScreenText));Screen->SetVisibility(!ScreenText.IsEmpty());}
void AVFInteractable::SetScreenText(const FString& Text,FLinearColor TextColor){ScreenText=Text;Screen->SetText(FText::FromString(Text));Screen->SetTextRenderColor(TextColor.ToFColor(true));Screen->SetVisibility(!Text.IsEmpty());}
bool AVFInteractable::UsableNow()const{
 if(!bEnabled||(bOneShot&&bUsed)||IsHidden())return false;
 const auto* M=AVFMissionController::Get(this);if(!M)return false;
 if(!ActiveStates.IsEmpty()&&!ActiveStates.Contains(M->State))return false;
 return M->CanUse(this);
}
FText AVFInteractable::GetPrompt(APawn* User)const{return UsableNow()?Prompt:FText();}
bool AVFInteractable::Activate_Implementation(APawn* User){
 if(!UsableNow())return false;auto* M=AVFMissionController::Get(this);if(!M)return false;
 bUsed=true;if(bHideWhenUsed){Mesh->SetVisibility(false,true);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Screen->SetVisibility(false);}
 M->OnInteract(this,InteractionId,User);return true;
}
void AVFInteractable::ResetUse(){bUsed=false;bEnabled=true;Mesh->SetVisibility(true,true);Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Screen->SetVisibility(!ScreenText.IsEmpty());}

AVFDefensePanel::AVFDefensePanel(){Color=FLinearColor(.12f,.13f,.14f);}
FText AVFDefensePanel::GetPrompt(APawn* User)const{
 if(!UsableNow())return FText();
 if(DefenseState==EVFDefenseState::Armed)return bTriggerReady?FText::FromString("TRIGGER"):FText();
 if(DefenseState==EVFDefenseState::Triggered)return FText();
 return Prompt;
}
void AVFDefensePanel::SetDefenseState(EVFDefenseState S){
 DefenseState=S;
 const bool Road=InteractionId==FName("panel.road");
 const FString Name=Road?TEXT("APPROACH ROAD"):TEXT("SHOP BAY");
 if(S==EVFDefenseState::Safe)SetScreenText(Name+TEXT("\nSAFE"),FLinearColor(.3f,1,.4f));
 else if(S==EVFDefenseState::Armed)SetScreenText(Name+TEXT("\nARMED"),FLinearColor(1,.25f,.15f));
 else SetScreenText(Name+TEXT("\nDISCHARGED"),FLinearColor(1,.7f,.2f));
}

// ---------------------------------------------------------------- damage zones and blasts
AVFScriptedDamageZone::AVFScriptedDamageZone(){
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 Box=CreateDefaultSubobject<UBoxComponent>("Box");SetRootComponent(Box);Box->SetBoxExtent(FVector(600,500,200));Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);Box->ShapeColor=FColor::Red;
 ArmedLight=CreateDefaultSubobject<UStaticMeshComponent>("ArmedLight");ArmedLight->SetupAttachment(Box);ArmedLight->SetStaticMesh(Cube.Object);ArmedLight->SetRelativeScale3D(FVector(.06f));ArmedLight->SetRelativeLocation(FVector(0,520,-190));ArmedLight->SetCollisionEnabled(ECollisionEnabled::NoCollision);ArmedLight->SetVisibility(false);
}
void AVFScriptedDamageZone::SetArmed(bool bArmed){ArmedLight->SetVisibility(bArmed);if(bArmed)VF::Paint(ArmedLight,FLinearColor(4,.1f,.05f),VF::EMat::Emissive);}
void AVFScriptedDamageZone::Trigger(AVFMissionController* Mission){
 SetArmed(false);
 FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[this,Mission]{
  if(!IsValid(Mission))return;const FVector C=GetActorLocation();const FVector E=Box->GetScaledBoxExtent();
  for(int32 i=-1;i<=1;++i)Mission->SpawnBlast(C+GetActorForwardVector()*E.X*.6f*i+FVector(0,0,-E.Z+60),2.2f,i==0,i==0);
  const FBox Area=FBox(C-E-FVector(250,250,0),C+E+FVector(250,250,400));
  for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It)if(It->Faction==EVFFaction::NCD&&It->IsAlive()&&Area.IsInside(It->GetActorLocation())){VF::Kill(It->Health);Mission->OnPawnKilled(*It);}
  for(TActorIterator<AVFScriptedVehicle> It(GetWorld());It;++It)if(!It->bWrecked&&Area.IsInside(It->GetActorLocation()))It->Wreck(Mission);
 }),FMath::Max(.01f,Delay),false);
}

AVFBlastEffect::AVFBlastEffect(){
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));
 Flash=Part(this,TEXT("Flash"),GetRootComponent(),Sphere.Object,FVector::ZeroVector,FVector(1),false);Flash->CastShadow=false;
 Light=CreateDefaultSubobject<UPointLightComponent>("Light");Light->SetupAttachment(GetRootComponent());Light->SetIntensity(0);Light->SetLightColor(FLinearColor(1,.55f,.25f));Light->SetAttenuationRadius(3000);Light->SetCastShadows(false);
 for(int32 i=0;i<6;++i){auto* S=Part(this,*FString::Printf(TEXT("Smoke%d"),i),GetRootComponent(),Sphere.Object,FVector::ZeroVector,FVector(.1f),false);S->CastShadow=false;Smoke.Add(S);}
}
void AVFBlastEffect::Init(float Scale,USoundBase* Sound,bool bLingeringFire){
 Size=Scale;bFire=bLingeringFire;VF::Paint(Flash,FLinearColor(30,12,3),VF::EMat::Emissive);
 for(UStaticMeshComponent* S:Smoke)VF::Paint(S,FLinearColor(.04f,.04f,.04f),VF::EMat::Smoke);
 if(Sound)UGameplayStatics::PlaySoundAtLocation(this,Sound,GetActorLocation(),FMath::Clamp(.6f+Scale*.2f,.5f,1.5f),FMath::FRandRange(.85f,1.05f)/FMath::Max(.7f,FMath::Sqrt(Scale)*.8f));
 GetWorldTimerManager().SetTimer(Timer,this,&AVFBlastEffect::Step,1.f/20.f,true);Step();
}
void AVFBlastEffect::Step(){
 Age+=1.f/20.f;const float Life=bFire?60.f:7.f;
 const float FlashA=FMath::Clamp(1-Age/.45f,0.f,1.f);Flash->SetRelativeScale3D(FVector((1.5f+Age*10)*Size*FlashA+.01f));Flash->SetVisibility(FlashA>0);
 const float Flicker=bFire?FMath::FRandRange(.6f,1.f):0;Light->SetIntensity(Size*(FlashA*400000+(bFire?Flicker*25000:0)));
 for(int32 i=0;i<Smoke.Num();++i){
  const float T=FMath::Fmod(Age+i*.9f,bFire?5.4f:7.f);const float Grow=(.6f+T*.7f)*Size;
  Smoke[i]->SetRelativeLocation(FVector(FMath::Sin(i*2.1f)*60*Size,FMath::Cos(i*1.7f)*60*Size,50+T*140*Size));Smoke[i]->SetRelativeScale3D(FVector(Grow));
  if(auto* M=Cast<UMaterialInstanceDynamic>(Smoke[i]->GetMaterial(0)))M->SetScalarParameterValue("Opacity",FMath::Clamp(.75f-T/7.f,0.f,1.f)*(bFire||Age<5?1.f:FMath::Clamp((7-Age)/2,0.f,1.f)));
 }
 if(Age>Life)Destroy();
}

// ---------------------------------------------------------------- vehicles
AVFScriptedVehicle::AVFScriptedVehicle(){
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));GetRootComponent()->SetMobility(EComponentMobility::Movable);
 Body=Part(this,TEXT("Body"),GetRootComponent(),Cube.Object,FVector(0,0,95),FVector(4.8f,2.0f,1.0f));
 Cabin=Part(this,TEXT("Cabin"),GetRootComponent(),Cube.Object,FVector(-40,0,180),FVector(3.0f,1.9f,.75f));
 Blood=Part(this,TEXT("Blood"),GetRootComponent(),Cube.Object,FVector(40,101,110),FVector(1.1f,.02f,.5f),false);
 Seat=CreateDefaultSubobject<USceneComponent>("Seat");Seat->SetupAttachment(GetRootComponent());Seat->SetRelativeLocationAndRotation(FVector(-215,0,240),FRotator(0,180,0));
 for(int32 i=0;i<4;++i){auto* W=Part(this,*FString::Printf(TEXT("Wheel%d"),i),GetRootComponent(),Cylinder.Object,FVector(i<2?160:-160,i%2?100:-100,45),FVector(.9f,.9f,.35f),false);W->SetRelativeRotation(FRotator(0,0,90));Wheels.Add(W);}
 Headlights=CreateDefaultSubobject<UPointLightComponent>("Headlights");Headlights->SetupAttachment(GetRootComponent());Headlights->SetRelativeLocation(FVector(280,0,100));Headlights->SetIntensity(6000);Headlights->SetAttenuationRadius(1600);Headlights->SetCastShadows(false);
 Engine=CreateDefaultSubobject<UAudioComponent>("Engine");Engine->SetupAttachment(GetRootComponent());Engine->bAutoActivate=false;
}
void AVFScriptedVehicle::OnConstruction(const FTransform& T){Super::OnConstruction(T);Paint();}
void AVFScriptedVehicle::BeginPlay(){Super::BeginPlay();Paint();}
void AVFScriptedVehicle::Paint(){
 const FLinearColor C=bWrecked?FLinearColor(.015f,.012f,.01f):Color;VF::Paint(Body,C);VF::Paint(Cabin,bWrecked?C:C*.6f);
 for(UStaticMeshComponent* W:Wheels)VF::Paint(W,FLinearColor(.01f,.01f,.01f));
 VF::Paint(Blood,FLinearColor(.25f,.005f,.004f));Blood->SetVisibility(bBloodied&&!bWrecked);
 Body->SetRelativeScale3D(bArmored?FVector(5.4f,2.3f,1.25f):FVector(4.8f,2.0f,1.0f));
}
void AVFScriptedVehicle::Drive(const TArray<FVector>& Points,float Speed,TFunction<void()> OnArrive,bool bSlideStop){
 Route=Points;SpeedCm=Speed;Arrive=MoveTemp(OnArrive);bSlide=bSlideStop;SlideYaw=0;if(bWrecked||Route.IsEmpty())return;
 if(Engine->Sound&&!Engine->IsPlaying())Engine->Play();
 GetWorldTimerManager().SetTimer(Timer,this,&AVFScriptedVehicle::Step,StepSeconds,true);
}
void AVFScriptedVehicle::Stop(){Route.Reset();GetWorldTimerManager().ClearTimer(Timer);if(Engine->IsPlaying())Engine->FadeOut(1.5f,0);}
void AVFScriptedVehicle::Step(){
 if(Route.IsEmpty()||bWrecked){Stop();return;}
 FVector P=GetActorLocation();FVector Goal=Route[0];Goal.Z=P.Z;
 float Remaining=0;FVector Prev=P;for(const FVector& R:Route){Remaining+=FVector::Dist2D(Prev,R);Prev=R;}
 float Speed=SpeedCm;if(Route.Num()==1)Speed*=FMath::Clamp(Remaining/900.f,.18f,1.f);
 const FVector Delta=Goal-P;const float Dist=Delta.Size2D();const float Move=Speed*StepSeconds;
 FRotator R=GetActorRotation();
 if(Dist<=Move){SetActorLocation(Goal);Route.RemoveAt(0);if(Route.IsEmpty()){if(bSlide)SetActorRotation(R+FRotator(0,SlideYaw,0));auto Done=MoveTemp(Arrive);Arrive=nullptr;Stop();if(Done)Done();return;}}
 else{
  SetActorLocation(P+Delta/Dist*Move);
  const float WantYaw=Delta.Rotation().Yaw;R.Yaw=FMath::FixedTurn(R.Yaw,WantYaw,120*StepSeconds);
  // Hard brake: swing the tail out over the last stretch.
  if(bSlide&&Route.Num()==1&&Remaining<1400){const float Swing=FMath::FInterpTo(SlideYaw,38.f,StepSeconds,2.5f);R.Yaw+=Swing-SlideYaw;SlideYaw=Swing;}
  SetActorRotation(R);
 }
 for(UStaticMeshComponent* W:Wheels)W->AddLocalRotation(FRotator(0,Speed*StepSeconds*.6f,0));
}
void AVFScriptedVehicle::Wreck(AVFMissionController* Mission){
 if(bWrecked)return;Stop();bWrecked=true;Paint();Headlights->SetVisibility(false);
 SetActorRotation(GetActorRotation()+FRotator(FMath::FRandRange(-4.f,4.f),FMath::FRandRange(-20.f,20.f),FMath::FRandRange(-8.f,8.f)));
 if(Mission)Mission->SpawnBlast(GetActorLocation()+FVector(0,0,120),2.6f,true,true);
}
void AVFScriptedVehicle::Restore(FTransform At){Stop();bWrecked=false;SetActorTransform(At);Headlights->SetVisibility(true);Paint();}
FVector AVFScriptedVehicle::Dismount(int32 Index)const{
 const FVector Side=GetActorRightVector()*((Index%2)?-230.f:230.f);const FVector Along=GetActorForwardVector()*(-180.f+120.f*(Index/2));
 return GetActorLocation()+Side+Along+FVector(0,0,100);
}

// ---------------------------------------------------------------- tree / fence lines
AVFGreyboxLine::AVFGreyboxLine(){
 SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));
 Primary=CreateDefaultSubobject<UInstancedStaticMeshComponent>("Primary");Primary->SetupAttachment(GetRootComponent());
 Secondary=CreateDefaultSubobject<UInstancedStaticMeshComponent>("Secondary");Secondary->SetupAttachment(GetRootComponent());
}
void AVFGreyboxLine::BeginPlay(){Super::BeginPlay();if(Primary->GetInstanceCount()==0)OnConstruction(GetActorTransform());else{VF::Paint(Primary,bTrees?FLinearColor(.09f,.06f,.04f):FLinearColor(.18f,.15f,.12f));VF::Paint(Secondary,bTrees?FLinearColor(.05f,.11f,.04f):FLinearColor(.25f,.22f,.18f));}}
void AVFGreyboxLine::OnConstruction(const FTransform& T){
 Super::OnConstruction(T);Primary->ClearInstances();Secondary->ClearInstances();
 const float Length=End.Size2D();if(Length<1)return;const FVector Dir=End.GetSafeNormal2D();const FVector Side(-Dir.Y,Dir.X,0);const float Yaw=Dir.Rotation().Yaw;
 FRandomStream Rand(Seed);
 if(bTrees){
  Primary->SetStaticMesh(EngineShape(TEXT("Cylinder")));Secondary->SetStaticMesh(EngineShape(TEXT("Sphere")));Secondary->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  VF::Paint(Primary,FLinearColor(.09f,.06f,.04f));VF::Paint(Secondary,FLinearColor(.05f,.11f,.04f));
  for(int32 Row=0;Row<FMath::Max(1,Rows);++Row)for(float D=Rand.FRandRange(0,Spacing*.5f);D<Length;D+=Spacing*Rand.FRandRange(.7f,1.3f)){
   const FVector P=Dir*D+Side*(Row*Spacing*.7f+Rand.FRandRange(-Jitter,Jitter))+Dir*Rand.FRandRange(-Jitter,Jitter)*.5f;
   const float H=Rand.FRandRange(6.f,10.f),W=Rand.FRandRange(.3f,.5f),Crown=Rand.FRandRange(4.5f,7.5f);
   Primary->AddInstance(FTransform(FRotator::ZeroRotator,P+FVector(0,0,H*50),FVector(W,W,H)));
   Secondary->AddInstance(FTransform(FRotator(0,Rand.FRandRange(0,360),0),P+FVector(0,0,H*100+Crown*25),FVector(Crown,Crown*Rand.FRandRange(.8f,1.2f),Crown*.8f)));
  }
 }else{
  Primary->SetStaticMesh(EngineShape(TEXT("Cube")));Secondary->SetStaticMesh(EngineShape(TEXT("Cube")));Secondary->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  VF::Paint(Primary,FLinearColor(.18f,.15f,.12f));VF::Paint(Secondary,FLinearColor(.25f,.22f,.18f));
  const int32 Posts=FMath::Max(1,FMath::CeilToInt(Length/Spacing));const float Step=Length/Posts;
  for(int32 i=0;i<=Posts;++i)Primary->AddInstance(FTransform(FRotator(0,Yaw,0),Dir*Step*i+FVector(0,0,65),FVector(.14f,.14f,1.3f)));
  for(int32 i=0;i<Posts;++i)for(float Z:{55.f,110.f})Secondary->AddInstance(FTransform(FRotator(0,Yaw,0),Dir*Step*(i+.5f)+FVector(0,0,Z),FVector(Step/100.f,.05f,.1f)));
 }
}
