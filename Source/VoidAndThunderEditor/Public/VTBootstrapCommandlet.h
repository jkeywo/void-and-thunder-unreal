#pragma once
#include "Commandlets/Commandlet.h"
#include "VTBootstrapCommandlet.generated.h"
UCLASS()
class UVTBootstrapCommandlet : public UCommandlet {
 GENERATED_BODY()
public:
 UVTBootstrapCommandlet();
 virtual int32 Main(const FString& Params) override;
};
UCLASS()
class UVTBenchmarkCommandlet : public UCommandlet {
 GENERATED_BODY()
public:
 UVTBenchmarkCommandlet();
 virtual int32 Main(const FString& Params) override;
};
