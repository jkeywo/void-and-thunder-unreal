#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "VTSessionSubsystem.generated.h"
UCLASS()
class VOIDANDTHUNDER_API UVTSessionSubsystem : public UGameInstanceSubsystem {
 GENERATED_BODY()
public:
 IOnlineSessionPtr Sessions;
 TSharedPtr<FOnlineSessionSearch> Search;
 FDelegateHandle CreateHandle,FindHandle,JoinHandle;
 UPROPERTY(BlueprintReadOnly) TArray<FString> Worlds;
 UPROPERTY(BlueprintReadOnly) FString Status;
 UFUNCTION(BlueprintCallable) void CreateWorld(bool Continue,const FString& Name);
 UFUNCTION(BlueprintCallable) void Discover();
 UFUNCTION(BlueprintCallable) void JoinWorld(int32 Index);
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;
 void Created(FName Name,bool Success);
 void Found(bool Success);
 void Joined(FName Name,EOnJoinSessionCompleteResult::Type Result);
};
