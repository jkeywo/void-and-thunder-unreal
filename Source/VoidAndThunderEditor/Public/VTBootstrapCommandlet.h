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

UCLASS()
class UVTContentCommandlet : public UCommandlet {
 GENERATED_BODY()
public:
 UVTContentCommandlet();
 virtual int32 Main(const FString& Params) override;
};

UCLASS()
class UVTPlayabilityCommandlet : public UCommandlet {
 GENERATED_BODY()
public:
 UVTPlayabilityCommandlet();
 virtual int32 Main(const FString& Params) override;
};

UCLASS()
class UVTHUDStyleCommandlet : public UCommandlet {
 GENERATED_BODY()
public:
 UVTHUDStyleCommandlet();
 virtual int32 Main(const FString& Params) override;
};
