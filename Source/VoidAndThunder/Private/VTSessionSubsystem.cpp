#include "VTSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "VTGameplay.h"
#include "VTSaveSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UVTSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
 Super::Initialize(Collection); if(auto* Online=IOnlineSubsystem::Get()) Sessions=Online->GetSessionInterface();
 if(Sessions.IsValid()) {CreateHandle=Sessions->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this,&UVTSessionSubsystem::Created)); FindHandle=Sessions->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this,&UVTSessionSubsystem::Found)); JoinHandle=Sessions->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this,&UVTSessionSubsystem::Joined));}
}
void UVTSessionSubsystem::Deinitialize() {
 if(Sessions.IsValid()) {Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle); Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle); Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle); if(Sessions->GetNamedSession(NAME_GameSession)) Sessions->DestroySession(NAME_GameSession);}
 Super::Deinitialize();
}
void UVTSessionSubsystem::CreateWorld(bool Continue,const FString& Name) {
 if(!Sessions.IsValid()||Sessions->GetNamedSession(NAME_GameSession)) {Status=TEXT("A session is already running."); return;}
 FString Slot; for(TCHAR C:Name) if(FChar::IsAlnum(C)||C==TCHAR('-')||C==TCHAR('_')) Slot.AppendChar(C); Slot=Slot.Left(32); if(Slot.IsEmpty()) Slot=TEXT("Campaign");
 GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->Slot=Slot;
 auto* GI=CastChecked<UVTGameInstance>(GetGameInstance()); GI->PlayMode=TEXT("sandbox"); GI->ContinueWorld=Continue;
 auto* Data=GetWorld()->GetSubsystem<UVTSimulation>()->Data.Get(); const int32* Population=Data->PopulationProfiles.Find(GI->PopulationProfile); GI->NewWorldPopulation=Continue ? -1 : Population ? *Population : 500;
 if(!Continue) {auto* Save=GI->GetSubsystem<UVTSaveSubsystem>(); Save->ResetWorldIdentity();}
 FOnlineSessionSettings Settings; Settings.bIsLANMatch=true; Settings.NumPublicConnections=4; Settings.bShouldAdvertise=true; Settings.bAllowJoinInProgress=true; Settings.bUsesPresence=false; Settings.bAllowInvites=false;
 Settings.Set(FName("VT_WORLD"),Slot,EOnlineDataAdvertisementType::ViaOnlineServiceAndPing); Settings.Set(FName("VT_VERSION"),FString(TEXT("1")),EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
 Status=TEXT("Starting world..."); if(!Sessions->CreateSession(0,NAME_GameSession,Settings)) Status=TEXT("Could not start the LAN session.");
}
void UVTSessionSubsystem::Created(FName Name,bool Success) {Status=Success ? TEXT("World ready") : TEXT("Could not create session"); if(Success) UGameplayStatics::OpenLevel(this,TEXT("/Game/Maps/Sandbox"),true,TEXT("listen"));}
void UVTSessionSubsystem::Discover() {
 if(!Sessions.IsValid()) return; Search=MakeShared<FOnlineSessionSearch>(); Search->bIsLanQuery=true; Search->MaxSearchResults=32; Worlds.Reset(); Status=TEXT("Looking for LAN worlds..."); if(!Sessions->FindSessions(0,Search.ToSharedRef())) Status=TEXT("Discovery could not start.");
}
void UVTSessionSubsystem::Found(bool Success) {
 Worlds.Reset(); if(Success&&Search.IsValid()) for(const auto& Result:Search->SearchResults) {FString Name; Result.Session.SessionSettings.Get(FName("VT_WORLD"),Name); Worlds.Add(Name.IsEmpty() ? TEXT("Sandbox world") : Name);}
 Status=Worlds.IsEmpty() ? TEXT("No LAN worlds found. You can also join by address.") : TEXT("Choose a world to join.");
}
void UVTSessionSubsystem::JoinWorld(int32 Index) {if(Sessions.IsValid()&&Search.IsValid()&&Search->SearchResults.IsValidIndex(Index)) {Status=TEXT("Joining..."); Sessions->JoinSession(0,NAME_GameSession,Search->SearchResults[Index]);}}
void UVTSessionSubsystem::Joined(FName Name,EOnJoinSessionCompleteResult::Type Result) {FString Address; if(Result==EOnJoinSessionCompleteResult::Success&&Sessions->GetResolvedConnectString(Name,Address)) CastChecked<UVTGameInstance>(GetGameInstance())->Join(Address); else Status=TEXT("Could not join this world.");}
