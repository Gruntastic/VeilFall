#include "VFIntro.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// ---------------------------------------------------------------- HUD
UTextBlock* UVFIntroHUD::Text(UCanvasPanel* Canvas,int32 Size,FLinearColor Color,FVector2D Anchor,FVector2D Offset,FVector2D Align,bool bAutoSize,FVector2D Size2){
 auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetColorAndOpacity(FSlateColor(Color));auto Font=T->GetFont();Font.Size=Size;T->SetFont(Font);
 T->SetShadowOffset(FVector2D(1.5f,1.5f));T->SetShadowColorAndOpacity(FLinearColor(0,0,0,.85f));
 auto* CSlot=Canvas->AddChildToCanvas(T);CSlot->SetAnchors(FAnchors(Anchor.X,Anchor.Y));CSlot->SetAlignment(Align);CSlot->SetPosition(Offset);CSlot->SetAutoSize(bAutoSize);if(!bAutoSize)CSlot->SetSize(Size2);
 return T;
}
UBorder* UVFIntroHUD::Panel(UCanvasPanel* Canvas,FLinearColor Color,FAnchors Anchors,FMargin Offsets){
 auto* B=WidgetTree->ConstructWidget<UBorder>();B->SetBrushColor(Color);auto* CSlot=Canvas->AddChildToCanvas(B);CSlot->SetAnchors(Anchors);CSlot->SetOffsets(Offsets);return B;
}
void UVFIntroHUD::NativeOnInitialized(){
 Super::NativeOnInitialized();
 auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
 DamagePanel=Panel(Canvas,FLinearColor(.6f,0,0,0),FAnchors(0,0,1,1),FMargin(0));
 AlertPanel=Panel(Canvas,FLinearColor(.02f,.02f,.03f,.93f),FAnchors(.5f,.5f),FMargin(0,0,980,560));
 if(auto* CSlot=Cast<UCanvasPanelSlot>(AlertPanel->Slot))CSlot->SetAlignment(FVector2D(.5f));
 AlertText=WidgetTree->ConstructWidget<UTextBlock>();auto F=AlertText->GetFont();F.Size=24;AlertText->SetFont(F);AlertText->SetJustification(ETextJustify::Center);AlertText->SetAutoWrapText(true);
 AlertText->SetText(FText::FromString(TEXT("PHONE  |  EMERGENCY ALERT\n\nNATIONAL EMERGENCY ALERT\n\nSHELTER IN PLACE IMMEDIATELY.\nENTER THE NEAREST SECURE STRUCTURE.\nLOCK ALL EXTERIOR DOORS AND WINDOWS.\nDO NOT TRAVEL.\nDO NOT APPROACH UNKNOWN PERSONS OR VEHICLES.\nAWAIT FURTHER INSTRUCTIONS.\nDO NOT RELY ON INFORMATION FROM UNAUTHORIZED SOURCES.")));
 AlertPanel->SetContent(AlertText);AlertPanel->SetPadding(FMargin(40));AlertPanel->SetHorizontalAlignment(HAlign_Center);AlertPanel->SetVerticalAlignment(VAlign_Center);AlertPanel->SetVisibility(ESlateVisibility::Collapsed);
 Crosshair=Text(Canvas,14,FLinearColor(1,1,1,.8f),FVector2D(.5f),FVector2D::ZeroVector,FVector2D(.5f));Crosshair->SetText(FText::FromString(TEXT("+")));
 PromptText=Text(Canvas,18,FLinearColor(1,.95f,.8f),FVector2D(.5f),FVector2D(0,70),FVector2D(.5f,0));
 Status=Text(Canvas,16,FLinearColor(.9f,.92f,.88f),FVector2D(1,1),FVector2D(-28,-24),FVector2D(1,1));Status->SetJustification(ETextJustify::Right);
 ObjectiveText=Text(Canvas,17,FLinearColor(.92f,.9f,.78f),FVector2D(0,0),FVector2D(32,28),FVector2D(0,0));
 HintText=Text(Canvas,16,FLinearColor(.75f,.88f,1),FVector2D(0,1),FVector2D(32,-28),FVector2D(0,1));
 GrenadeText=Text(Canvas,34,FLinearColor(1,.25f,.15f),FVector2D(.5f,.32f),FVector2D::ZeroVector,FVector2D(.5f));GrenadeText->SetText(FText::FromString(TEXT("GRENADE")));GrenadeText->SetVisibility(ESlateVisibility::Collapsed);
 KeypadPanel=Panel(Canvas,FLinearColor(.03f,.035f,.04f,.95f),FAnchors(.5f,.5f),FMargin(0,0,560,250));
 if(auto* CSlot=Cast<UCanvasPanelSlot>(KeypadPanel->Slot))CSlot->SetAlignment(FVector2D(.5f));
 KeypadText=WidgetTree->ConstructWidget<UTextBlock>();F=KeypadText->GetFont();F.Size=22;KeypadText->SetFont(F);KeypadText->SetJustification(ETextJustify::Center);KeypadText->SetColorAndOpacity(FSlateColor(FLinearColor(.55f,1,.6f)));
 KeypadPanel->SetContent(KeypadText);KeypadPanel->SetHorizontalAlignment(HAlign_Center);KeypadPanel->SetVerticalAlignment(VAlign_Center);KeypadPanel->SetVisibility(ESlateVisibility::Collapsed);
 DebugText=Text(Canvas,13,FLinearColor(.7f,1,.7f),FVector2D(1,0),FVector2D(-24,24),FVector2D(1,0));DebugText->SetJustification(ETextJustify::Right);DebugText->SetVisibility(ESlateVisibility::Collapsed);
 FadePanel=Panel(Canvas,FLinearColor(0,0,0,0),FAnchors(0,0,1,1),FMargin(0));
 Title=Text(Canvas,110,FLinearColor(.88f,.86f,.8f),FVector2D(.5f,.46f),FVector2D::ZeroVector,FVector2D(.5f));Title->SetText(FText::FromString(TEXT("V E I L F A L L")));Title->SetVisibility(ESlateVisibility::Collapsed);
 Subtitle=Text(Canvas,21,FLinearColor::White,FVector2D(.5f,1),FVector2D(0,-110),FVector2D(.5f,1),false,FVector2D(1200,90));Subtitle->SetJustification(ETextJustify::Center);Subtitle->SetAutoWrapText(true);
 SetIsFocusable(true);SetVisibility(ESlateVisibility::SelfHitTestInvisible);
 ShowObjective(TEXT(""));
}
void UVFIntroHUD::ShowSubtitle(const FString& Speaker,const FString& Line,float Seconds){
 Subtitle->SetText(FText::FromString(Speaker.IsEmpty()?Line:Speaker.ToUpper()+TEXT(":  ")+Line));SubtitleUntil=GetWorld()->GetTimeSeconds()+Seconds;Subtitle->SetVisibility(ESlateVisibility::HitTestInvisible);
 UE_LOG(LogTemp,Display,TEXT("VF_SUBTITLE %s: %s"),*Speaker,*Line);
}
void UVFIntroHUD::ShowObjective(const FString& Line){ObjectiveText->SetText(FText::FromString(Line));ObjectiveUntil=Line.IsEmpty()?0:GetWorld()->GetTimeSeconds()+9;}
void UVFIntroHUD::ShowHint(const FString& Line,float Seconds){HintText->SetText(FText::FromString(Line));HintUntil=GetWorld()->GetTimeSeconds()+Seconds;}
void UVFIntroHUD::ShowAlert(bool b){AlertPanel->SetVisibility(b?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);}
void UVFIntroHUD::FadeTo(float Opacity,float Seconds){FadeTarget=Opacity;if(Seconds<=0){Fade=Opacity;FadeSpeed=100;}else FadeSpeed=FMath::Abs(Opacity-Fade)/Seconds+.01f;}
void UVFIntroHUD::ShowTitle(bool b){Title->SetVisibility(b?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);}
void UVFIntroHUD::ShowKeypad(bool b){bKeypad=b;Code.Reset();KeypadPanel->SetVisibility(b?ESlateVisibility::Visible:ESlateVisibility::Collapsed);}
void UVFIntroHUD::PulseDamage(float S){Damage=FMath::Max(Damage,S);}
void UVFIntroHUD::GrenadeWarning(float Seconds){GrenadeUntil=GetWorld()->GetTimeSeconds()+Seconds;}
void UVFIntroHUD::ShowDebug(bool b){bDebug=b;DebugText->SetVisibility(b?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);}

void UVFIntroHUD::NativeTick(const FGeometry& G,float Dt){
 Super::NativeTick(G,Dt);const double Now=GetWorld()->GetTimeSeconds();
 Fade=FMath::FInterpConstantTo(Fade,FadeTarget,Dt,FadeSpeed);FadePanel->SetBrushColor(FLinearColor(0,0,0,Fade));
 Damage=FMath::FInterpTo(Damage,0,Dt,2.5f);DamagePanel->SetBrushColor(FLinearColor(.55f,0,0,Damage*.45f));
 Subtitle->SetVisibility(Now<SubtitleUntil?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
 HintText->SetRenderOpacity(FMath::Clamp(float(HintUntil-Now),0.f,1.f));
 ObjectiveText->SetRenderOpacity(FMath::Clamp(float(ObjectiveUntil-Now)/1.5f,0.f,1.f));
 GrenadeText->SetVisibility(Now<GrenadeUntil&&FMath::Fmod(Now,.4)<.25?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
 if(AlertPanel->GetVisibility()!=ESlateVisibility::Collapsed){AlertFlash+=Dt;AlertText->SetColorAndOpacity(FSlateColor(FMath::Fmod(AlertFlash,1.2f)<.6f?FLinearColor(1,.25f,.2f):FLinearColor(1,1,1)));}
 if(bKeypad){FString Dots;for(int32 i=0;i<4;++i)Dots+=i<Code.Len()?FString(TEXT(" *")):FString(TEXT(" _"));KeypadText->SetText(FText::FromString(TEXT("SECURITY PANEL\n\n")+Dots+TEXT("\n\nType the code with 0-9.  Enter to confirm.\nBackspace to clear.  Esc to step away.")));}
 auto* Pawn=Cast<AVFIntroCharacter>(GetOwningPlayerPawn());auto* Mission=AVFMissionController::Get(this);
 // Interaction prompt (same 3 m view trace the interaction component uses).
 FString Prompt;
 if(Mission&&!Mission->ForcedPrompt.IsEmpty())Prompt=Mission->ForcedPrompt;
 else if(Pawn&&!bKeypad){FVector Eye;FRotator R;Pawn->GetActorEyesViewPoint(Eye,R);FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(VFPrompt),false,Pawn);
  if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+R.Vector()*300,ECC_Visibility,Q))if(auto* Src=Cast<IVFPromptSource>(Hit.GetActor()))Prompt=Src->GetPrompt(Pawn).ToString();}
 PromptText->SetText(FText::FromString(Prompt.IsEmpty()?FString():TEXT("[F]  ")+Prompt));
 if(Pawn){
  const bool Armed=Pawn->bArmed&&Pawn->Health->State.Life==EVeilLife::Alive;Crosshair->SetVisibility(Armed&&!Pawn->Combat->IsADS()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
  if(Armed){const auto* A=Pawn->Combat->CurrentAmmo();const auto& CS=Pawn->Combat->State;const auto Spec=Pawn->Combat->Spec(Pawn->Combat->EquippedId);
   Status->SetText(FText::FromString(FString::Printf(TEXT("%s  %d / %d%s\nHealth %.0f   Med %d"),*Spec.DisplayName,A?A->Magazine:0,A?A->Reserve:0,CS.bReloading?TEXT("  RELOADING"):TEXT(""),Pawn->Health->State.Health,CS.MedicalCharges)));
   // Player gunshot audio: the combat component has no native sound hook, so watch the magazine.
   if(A&&Pawn->Combat->EquippedId==LastWeapon&&A->Magazine<LastMagazine&&!CS.bReloading)if(Mission&&Mission->Gunshot())UGameplayStatics::PlaySound2D(this,Mission->Gunshot(),.45f,FMath::FRandRange(.95f,1.05f));
   LastMagazine=A?A->Magazine:-1;LastWeapon=Pawn->Combat->EquippedId;
  }else Status->SetText(FText());
  static int32 SeenRevision=-1;if(SeenRevision>=0&&Pawn->Health->State.DamageRevision!=SeenRevision)PulseDamage(.7f);SeenRevision=Pawn->Health->State.DamageRevision;
 }
 if(bDebug&&Mission){
  FString D=TEXT("DEV STATE MENU  (F10 close, PgUp/PgDn jump, F9 checkpoint)\n");const UEnum* E=StaticEnum<EVFIntroState>();
  for(int32 i=0;i<E->NumEnums()-1;++i)D+=(int32(Mission->State)==i?TEXT("> "):TEXT("  "))+E->GetNameStringByIndex(i)+TEXT("\n");
  D+=TEXT("console: VFState <NAME>, VFSkip ncd|firefight|vanguard|extract|scuttle,\nVFResetFight, VFOpenVault, VFResetVault, VFCheckpoint");DebugText->SetText(FText::FromString(D));
 }
}
FReply UVFIntroHUD::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& Event){
 if(!bKeypad)return Super::NativeOnKeyDown(G,Event);
 auto* PC=Cast<AVFIntroPlayerController>(GetOwningPlayer());const FKey K=Event.GetKey();
 static const FKey Digits[]={EKeys::Zero,EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::Nine};
 static const FKey Pad[]={EKeys::NumPadZero,EKeys::NumPadOne,EKeys::NumPadTwo,EKeys::NumPadThree,EKeys::NumPadFour,EKeys::NumPadFive,EKeys::NumPadSix,EKeys::NumPadSeven,EKeys::NumPadEight,EKeys::NumPadNine};
 for(int32 i=0;i<10;++i)if((K==Digits[i]||K==Pad[i])&&Code.Len()<4){Code.AppendInt(i);if(Code.Len()==4&&PC){const FString Done=Code;PC->CloseKeypad(true,Done);}return FReply::Handled();}
 if(K==EKeys::BackSpace){Code.Reset();return FReply::Handled();}
 if(K==EKeys::Enter&&PC){const FString Done=Code;PC->CloseKeypad(true,Done);return FReply::Handled();}
 if(K==EKeys::Escape&&PC){PC->CloseKeypad(false,FString());return FReply::Handled();}
 return FReply::Handled();
}

// ---------------------------------------------------------------- player controller
void AVFIntroPlayerController::BeginPlay(){
 Super::BeginPlay();
 if(IsLocalController()&&!IsRunningCommandlet()&&!FParse::Param(FCommandLine::Get(),TEXT("nullrhi"))){HUD=CreateWidget<UVFIntroHUD>(this);if(HUD)HUD->AddToViewport(10);}
 SetInputMode(FInputModeGameOnly());
}
void AVFIntroPlayerController::SetupInputComponent(){
 Super::SetupInputComponent();
 InputComponent->BindKey(EKeys::F,IE_Pressed,this,&AVFIntroPlayerController::UsePressed);
#if !UE_BUILD_SHIPPING
 InputComponent->BindKey(EKeys::F10,IE_Pressed,this,&AVFIntroPlayerController::ToggleDebug);
 InputComponent->BindKey(EKeys::PageDown,IE_Pressed,this,&AVFIntroPlayerController::DebugNext);
 InputComponent->BindKey(EKeys::PageUp,IE_Pressed,this,&AVFIntroPlayerController::DebugPrev);
 InputComponent->BindKey(EKeys::F9,IE_Pressed,this,&AVFIntroPlayerController::VFCheckpoint);
#endif
}
void AVFIntroPlayerController::UsePressed(){if(auto* M=AVFMissionController::Get(this))M->OnPlayerUse();}
void AVFIntroPlayerController::OpenKeypad(){
 if(!HUD)return;HUD->ShowKeypad(true);FInputModeUIOnly Mode;Mode.SetWidgetToFocus(HUD->TakeWidget());SetInputMode(Mode);
}
void AVFIntroPlayerController::CloseKeypad(bool bSubmitted,const FString& Code){
 if(HUD)HUD->ShowKeypad(false);SetInputMode(FInputModeGameOnly());
 if(bSubmitted)if(auto* M=AVFMissionController::Get(this))M->OnKeypadResult(Code==M->VaultCode);
}
void AVFIntroPlayerController::ToggleDebug(){bDebug=!bDebug;if(HUD)HUD->ShowDebug(bDebug);}
void AVFIntroPlayerController::DebugNext(){if(auto* M=AVFMissionController::Get(this))if(M->State!=EVFIntroState::MISSION_COMPLETE)M->JumpToState(EVFIntroState(uint8(M->State)+1));}
void AVFIntroPlayerController::DebugPrev(){if(auto* M=AVFMissionController::Get(this))if(M->State!=EVFIntroState::NORMAL_HOME)M->JumpToState(EVFIntroState(uint8(M->State)-1));}
void AVFIntroPlayerController::VFState(const FString& Name){
 auto* M=AVFMissionController::Get(this);const int64 V=StaticEnum<EVFIntroState>()->GetValueByNameString(Name.ToUpper());
 if(M&&V!=INDEX_NONE)M->JumpToState(EVFIntroState(V));else ClientMessage(TEXT("Unknown state ")+Name);
}
void AVFIntroPlayerController::VFSkip(const FString& What){
 const FString W=What.ToLower();
 VFState(W==TEXT("ncd")?TEXT("NCD_ARRIVAL"):W==TEXT("firefight")||W==TEXT("fight")?TEXT("HOLD_SHOP"):W==TEXT("vanguard")?TEXT("VANGUARD_ARRIVAL"):W==TEXT("extract")?TEXT("EXTRACT_FAMILY"):W==TEXT("scuttle")?TEXT("EVACUATE"):What);
}
void AVFIntroPlayerController::VFResetFight(){if(auto* M=AVFMissionController::Get(this))M->ResetPropertyFight();}
void AVFIntroPlayerController::VFOpenVault(){if(auto* M=AVFMissionController::Get(this))M->DebugVault(true);}
void AVFIntroPlayerController::VFResetVault(){if(auto* M=AVFMissionController::Get(this))M->DebugVault(false);}
void AVFIntroPlayerController::VFCheckpoint(){if(auto* M=AVFMissionController::Get(this))M->RestartCheckpoint();}

AVFIntroGameMode::AVFIntroGameMode(){DefaultPawnClass=AVFIntroCharacter::StaticClass();PlayerControllerClass=AVFIntroPlayerController::StaticClass();}
