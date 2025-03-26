#include "VRPlayer.h"
#include "Camera/CameraComponent.h"
#include "../../../../Plugins/EnhancedInput/Source/EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "../../../../Plugins/EnhancedInput/Source/EnhancedInput/Public/EnhancedInputComponent.h"
#include "../../../../../../../Plugins/EnhancedInput/Source/EnhancedInput/Public/InputMappingContext.h"
#include "../../../../../../../Plugins/EnhancedInput/Source/EnhancedInput/Public/InputAction.h"
#include "../../../../../../../Source/Runtime/Engine/Classes/Engine/HitResult.h"
#include "../../../../../../../Source/Runtime/HeadMountedDisplay/Public/MotionControllerComponent.h"
#include "../../../../../../../Plugins/FX/Niagara/Source/Niagara/Public/NiagaraComponent.h"
#include "../../../../../../../Plugins/FX/Niagara/Source/Niagara/Classes/NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "../../../../../../../Source/Runtime/Engine/Classes/Components/CapsuleComponent.h"
#include "../../../../../../../Source/Runtime/Engine/Public/TimerManager.h"
#include "../../../../../../../Source/Runtime/Engine/Classes/Camera/CameraComponent.h"


// 정적 로딩 - 컴파일 시 로딩
// 동적 로딩 - 런타임 시 로딩
AVRPlayer::AVRPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	/* Camera */ 
	VRCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VRCamera"));
	VRCamera->SetupAttachment(RootComponent);


	/* Input Mappig Context */ 
	ConstructorHelpers::FObjectFinder<UInputMappingContext> tmpIMC(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/Inputs/IMC_VRInput.IMC_VRInput'"));
	if (tmpIMC.Succeeded()) IMC_VRInput = tmpIMC.Object;


	/* Input Actions */ 
	ConstructorHelpers::FObjectFinder<UInputAction> tmpIAMove(TEXT("/Script/EnhancedInput.InputAction'/Game/Inputs/IA_VRMove.IA_VRMove'"));
	if(tmpIAMove.Succeeded()) IA_VRMove = tmpIAMove.Object;

	ConstructorHelpers::FObjectFinder<UInputAction> tmpIAMouse(TEXT("/Script/EnhancedInput.InputAction'/Game/Inputs/IA_VRMouse.IA_VRMouse'"));
	if (tmpIAMouse.Succeeded()) IA_VRMouse = tmpIAMouse.Object;

	ConstructorHelpers::FObjectFinder<UInputAction> tmpIATeleport(TEXT("/Script/EnhancedInput.InputAction'/Game/Inputs/IA_VRTeleport.IA_VRTeleport'"));
	if (tmpIATeleport.Succeeded()) IA_VRTeleport = tmpIATeleport.Object;

	ConstructorHelpers::FObjectFinder<UInputAction> tmpIAFire(TEXT("/Script/EnhancedInput.InputAction'/Game/Inputs/IA_VRFire.IA_VRFire'"));
	if (tmpIATeleport.Succeeded()) IA_VRFire = tmpIAFire.Object;

	ConstructorHelpers::FObjectFinder<UInputAction> tmpIAGrab(TEXT("/Script/EnhancedInput.InputAction'/Game/Inputs/IA_VRGrab.IA_VRGrab'"));
	if (tmpIAGrab.Succeeded()) IA_VRGrab = tmpIAGrab.Object;


	/* Motion Controller */
	LeftHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftHand"));
	LeftHand->SetupAttachment(RootComponent);
	LeftHand->SetTrackingMotionSource(TEXT("Left"));

	RightHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightHand"));
	RightHand->SetupAttachment(RootComponent);
	RightHand->SetTrackingMotionSource(TEXT("Right"));

	RightAim = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightAim"));
	RightAim->SetupAttachment(RootComponent);
	RightAim->SetTrackingMotionSource(TEXT("RightAim"));


	/* Teleport Circle */
	// TeleportCircle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeleportCircle"));
	TeleportCircle = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TeleportCircle"));
	TeleportCircle->SetupAttachment(RootComponent);

	/* Teleport UI Component */
	TeleportUIComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TeleportUIComponent"));
	TeleportUIComponent->SetupAttachment(RootComponent);

	// TIPS) string 데이터를 int형으로 바꾼 게 FName -> 그래서 FString이 FName보다 빠르다
	// 해시키 형태로 저장 - Look Up table

	/* Crosshair */
	CrosshairComp = CreateDefaultSubobject<UChildActorComponent>(TEXT("CrosshairComp"));
	CrosshairComp->SetupAttachment(RootComponent);
	ConstructorHelpers::FClassFinder<AActor> tmpCrosshair(TEXT("/Script/Engine.Blueprint'/Game/Blueprints/BP_Crosshair.BP_Crosshair'_C"));
	if(tmpCrosshair.Succeeded()) CrosshairComp->SetChildActorClass(tmpCrosshair.Class);
}


void AVRPlayer::BeginPlay()
{
	Super::BeginPlay();

	ResetTeleport();
}


// TIP) Tick()에서 입력이 씹히는 경우 - 커스텀 입력 큐를 만들어서 입력을 큐에 담았다가 하나하나 수동 처리해준다
void AVRPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DrawCrosshair();

	// 텔레포트 활성화 시
	if (bTeleporting == true)
	{
		// 텔레포트 그리기 - 곡선 방식
		if(bTeleportCurve)
			DrawTeleportCurve();
		// 텔레포트 그리기 - 직선 방식
		else
			DrawTeleportStraight();
	}

	// Niagara Curve가 보여지면 데이터를 세팅하자
	// TEXT에 들어갈 내용 : 변수명
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(TeleportUIComponent, TEXT("User.PointArray"), Lines);
}


void AVRPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto pc = GetWorld()->GetFirstPlayerController();
	if (pc)
	{
		auto localPlayer = pc->GetLocalPlayer();
		auto ss = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(localPlayer);	// subsystem
		if (ss)
		{
			ss->AddMappingContext(IMC_VRInput, 1);
		}
	}

	auto inputSystem = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (inputSystem)
	{
		inputSystem->BindAction(IA_VRMove, ETriggerEvent::Triggered, this, &AVRPlayer::Move);
		inputSystem->BindAction(IA_VRMouse, ETriggerEvent::Triggered, this, &AVRPlayer::Turn);

		// Teleport
		inputSystem->BindAction(IA_VRTeleport, ETriggerEvent::Started, this, &AVRPlayer::TeleportStart);
		inputSystem->BindAction(IA_VRTeleport, ETriggerEvent::Completed, this, &AVRPlayer::TeleportEnd);

		// Fire
		inputSystem->BindAction(IA_VRFire, ETriggerEvent::Started, this, &AVRPlayer::FireInput);

		// Grab
		inputSystem->BindAction(IA_VRGrab, ETriggerEvent::Started, this, &AVRPlayer::TryGrab);
		inputSystem->BindAction(IA_VRGrab, ETriggerEvent::Completed, this, &AVRPlayer::TryUnGrab);
	}

	/*
	Cast와 CastChecked
	- Cast : Static cast - 없으면 null
	- CastChecked : 없으면 crash를 냄
	*/
}


void AVRPlayer::Move(const FInputActionValue& InValues)
{
	// 방법1 : 이 방법이 효율적
	FVector2D Scale = InValues.Get<FVector2D>();
	FVector direction = VRCamera->GetForwardVector() * Scale.X + VRCamera->GetRightVector() * Scale.Y;

	// 방법2
	/*AddMovementInput(VRCamera->GetForwardVector(), Scale.X);
	AddMovementInput(VRCamera->GetForwardVector(), Scale.Y);*/

	AddMovementInput(direction);

	//// cf) 카메라가 바라보는 방식으로 회전
	//FVector direction = FVector(Scale.X, Scale.Y, 0);
	//VRCamera->GetComponentTransform().TransformVector(direction);
}

void AVRPlayer::Turn(const struct FInputActionValue& InValues)
{
	if(bUsingMiouse == false) return;

	FVector2d scale = InValues.Get<FVector2d>();
	AddControllerPitchInput(scale.Y);
	AddControllerYawInput(scale.X);
}

void AVRPlayer::ActiveDebugDraw()
{
	bIsDebugDraw = !bIsDebugDraw;
}

// 텔레포트 설정을 초기화한다
// 현재 텔레포트가 가능한지 결과로 넘겨준다
bool AVRPlayer::ResetTeleport()
{
	// 현재 텔레포트 써클이 화면에 보이면 이동이 가능하다
	// 그렇지 않으면 이동 불가
	bool bCanTeleport = TeleportCircle->GetVisibleFlag();

	// 텔레포트 종료
	bTeleporting = false;	// Teleport가 종료되었다고 알려준다
	TeleportCircle->SetVisibility(false);	// UI를 꺼주고
	TeleportUIComponent->SetVisibility(false);

	// 텔레포트 가능 여부를 결과로 넘겨준다
	return bCanTeleport;
}

void AVRPlayer::TeleportStart(const struct FInputActionValue& InValues)
{
	// 텔레포트 시작
	bTeleporting = true;
	TeleportUIComponent->SetVisibility(true);
	UE_LOG(LogTemp, Warning, TEXT(">>> Teleport Start"));
}

void AVRPlayer::TeleportEnd(const FInputActionValue& InValues)
{
	UE_LOG(LogTemp, Warning, TEXT(">>> Teleport End"));
	// 텔레포트가 가능하지 않으면 아무 처리를 하지 않는다
	if (ResetTeleport() == false)
	{
		return;
	}

	// Warp가 가능하면 Warp로 이동하고
	if(IsWarp) DoWarp();
	// 그렇지 않으면 텔레포트로 이동한다
	else SetActorLocation(TeleportLocation);
}

// Line Trace 이용한 직선 방식의 Teleport - 시작점과 끝 점이 필요하다
void AVRPlayer::DrawTeleportStraight()
{
	// 1. Line을 만든다
	FVector StartPoint = RightHand->GetComponentLocation();
	FVector EndPoint = StartPoint + RightHand->GetForwardVector() * 1000;	// 카메라가 바라보는 방향으로부터 10m

	// 2. 만들어진 Line을 쏜다	
	bool bHit = CheckHitTeleport(StartPoint, EndPoint);

	// 방법1. 선 그리기 - Debug Line 이용
	//DrawDebugLine(GetWorld(), StartPoint, EndPoint, FColor::Red, false, -1, 0, 1);
	//if(bIsDebugDraw)
	//	DrawDebugSphere(GetWorld(), StartPoint, 200, 20, FColor::Yellow);

	// 방법2. 선 그리기 - spline 이용
	
	// 방법3. 선 그리기 - Niagara Effect 이용
	Lines.Empty();
	Lines.Add(StartPoint);
	Lines.Add(EndPoint);
}

void AVRPlayer::DrawTeleportCurve()
{
	// 비워준다 - 사용자 입력이 들어올 때마다 실행되므로
	Lines.Empty();

	// 선이 진행될 힘(방향)
	FVector velocity = RightHand->GetForwardVector() * CurveForce;

	// P0 - 시작점
	FVector pos = RightHand->GetComponentLocation();
	Lines.Add(pos);
	
	// FMath::GetReflectionVector() - 입사각 반사각에 의해 어디로 튕길지 계산해준다
	
	// 이 과정을 LineSmooth를 구성하는 (점의 개수 - 1)만큼 진행하겠다.
	for (int i = 0; i < LineSmooth; i++)
	{
		FVector lastPos = pos;

		// v = v0 + at
		velocity += FVector::UpVector * Gravity * SimulateTime;
		
		// P = P0 + vt
		pos += velocity * SimulateTime;

		// 이전 점(lastPos)과 현재 점(pos)까지의 충돌 체크를 진행한다
		bool bHit = CheckHitTeleport(lastPos, pos);
		Lines.Add(pos);

		// 부딪혔다면 라인 그리기를 종료한다
		if (bHit) break;
	}

	// Line을 그려준다
	// Debug Draw를 이용하는 방법
	/*int lineCount = Lines.Num();
	for (int i = 0; i < lineCount - 1; i++)
	{
		DrawDebugLine(GetWorld(), Lines[i], Lines[i + 1], FColor::Red, false, -1, 0, 1);
	}*/
}

// 선 또는 곡선이 그려질때 충돌이 생기는 부분까지만 그려주고 싶다
bool AVRPlayer::CheckHitTeleport(FVector LastPos, FVector& CurPos)
{
	// Trace channel - visible, camera 두 개 있음
	FHitResult hitInfo;
	FCollisionQueryParams params;	// 충돌 체크 옵션
	params.AddIgnoredActor(this);	// 나는 충돌 체크에서 제외한다

	bool bHit = GetWorld()->LineTraceSingleByChannel(hitInfo, LastPos, CurPos, ECC_Visibility, params);

	// 3. Line과 부딪혔고, 부딪힌 곳의 이름에 GroundFloor가 있는 경우
	if (bHit && hitInfo.GetActor()->GetActorNameOrLabel().Contains("GroundFloor") == true)
	{
		// 텔레포트 UI를 활성화한다
		TeleportCircle->SetVisibility(true);
		// 부딪힌 지점에 텔레포트 써클을 위치시킨다
		TeleportCircle->SetWorldLocation(hitInfo.Location);

		// 텔레포트 위치를 지정한다
		TeleportLocation = hitInfo.Location;
		UE_LOG(LogTemp, Warning, TEXT("Hit Floor Succeeded!"));

		CurPos = TeleportLocation;
	}
	// 4. 부딪히지 않았다면 써클을 위치시키지 않는다
	else
	{
		// 텔레포트 UI를 비활성화한다
		TeleportCircle->SetVisibility(false);
		UE_LOG(LogTemp, Warning, TEXT("Hit Other Succeeded!"));
	}

	/*
		hitInfo.GetActor()->GetName().Contains("Floor")가 if문 안에 있어야 하는 이유
		: 충돌했는데 Floor가 아닌 경우, 해당 조건이 if문 안의 if문으로 들어가면
		  텔레포트 UI를 비활성화할 수 없음
	*/

	return false;
}

void AVRPlayer::DoWarp()
{
	// 1. Warp가 활성화되어 있을 때만 수행한다
	if(IsWarp == false) return;

	/* 2. 일정 시간동안 정해진 위치로 이동하고 싶다 */
	CurrentTime = 0;
	FVector curPos = GetActorLocation();
	// 충돌 처리를 꺼준다
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWorldTimerManager().SetTimer(WarpTimer, FTimerDelegate::CreateLambda(
		[this]()->void
		{
			//// 2-1. 시간이 흘러서 
			//CurrentTime += GetWorld()->DeltaTimeSeconds;
			//// 2-2. Warp Time 동안 현재 위치에서 target까지
			//FVector targetPos = TeleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			//FVector curPos = GetActorLocation();

			//// 2-3. 이동하고 싶다
			//curPos = FMath::Lerp(curPos, targetPos, CurrentTime / WarpTime);
			//SetActorLocation(curPos);

			//// 3. 목적지에 도착했다면 
			//if (CurrentTime - WarpTime >= 0)
			//{
			//	// 목적지 위치로 위치를 보정해준다
			//	// 목적지 위치를 보정하는 이유 : 보간을 하면 정확히 targetPos에 도착하기보다
			//	SetActorLocation(targetPos);

			//	// 타이머를 끈다
			//	GetWorldTimerManager().ClearTimer(WarpTimer);

			//	// 충돌 처리를 켜준다
			//	GetCapsuleComponent()->SetCollisionEnabled((ECollisionEnabled::QueryAndPhysics));
			//}

			// From->To로 Percent를 이용해 이동한다
			// 만약 목적지에 거의 도착하면 도착한 것으로 처리한다
			// 2-1. 시간이 흘러서 
			//CurrentTime += GetWorld()->DeltaTimeSeconds;
			// 2-2. Warp Time 동안 현재 위치에서 target까지
			FVector targetPos = TeleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			FVector curPos = GetActorLocation();

			// 2-3. 이동하고 싶다
			curPos = FMath::Lerp(curPos, targetPos, 10 * GetWorld()->DeltaTimeSeconds);
			SetActorLocation(curPos);

			// 3. 목적지에 도착했다면 
			// -> 일정 범위 안에 들어왔다면 - 구 충돌(3d), 원 충돌(2d)
			//if (CurrentTime - WarpTime >= 0)
			float distance = FVector::Distance(curPos, targetPos);
			if(distance < 10)
			{
				// 목적지 위치로 위치를 보정해준다
				// 목적지 위치를 보정하는 이유 : 보간을 하면 정확히 targetPos에 도착하기보다
				SetActorLocation(targetPos);

				// 타이머를 끈다
				GetWorldTimerManager().ClearTimer(WarpTimer);

				// 충돌 처리를 켜준다
				GetCapsuleComponent()->SetCollisionEnabled((ECollisionEnabled::QueryAndPhysics));
			}
		}
	), 0.02f, true);
}

//bool AVRPlayer::PerformLineTrace(FVector InStartPos, FVector InEndPos, FHitResult& InHitResult)
//{
//	FVector startPos = RightHand->GetComponentLocation();
//	FVector endPos = startPos + RightHand->GetForwardVector() * 100;
//	FHitResult hitInfo;
//	FCollisionQueryParams params;
//	params.AddIgnoredActor(this);
//
//	bool bHit = GetWorld()->LineTraceSingleByChannel(hitInfo, startPos, endPos, ECC_Visibility, params);
//}

void AVRPlayer::FireInput(const FInputActionValue& InValues)
{
	FVector startPos = RightAim->GetComponentLocation();
	FVector endPos = startPos + RightAim->GetForwardVector() * 100;
	FHitResult hitInfo;
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(hitInfo, startPos, endPos, ECC_Visibility, params);

	if (bHit)
	{
		// 부딪힌 녀셕이 물리 기능이 활성화되어 있으면 날려보내자
		auto hitComp = hitInfo.GetComponent();
		if (hitComp && hitComp->IsSimulatingPhysics())
			hitComp->AddImpulseAtLocation( RightAim->GetForwardVector() * 1000, hitInfo.Location );
	}

}

// crosshair 그리기
void AVRPlayer::DrawCrosshair()
{
	FVector startPos = RightAim->GetComponentLocation();
	FVector endPos = startPos + RightAim->GetForwardVector() * 100;
	FHitResult hitInfo;
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	DrawDebugLine(GetWorld(), startPos, endPos, FColor::Magenta);

	bool bHit = GetWorld()->LineTraceSingleByChannel(hitInfo, startPos, endPos, ECC_Visibility, params);

	float distance = 0;

	// 부딪혔을 경우
	if (bHit)
	{
		CrosshairComp->SetWorldLocation(hitInfo.Location);
		distance = FVector::Distance(VRCamera->GetComponentLocation(), hitInfo.Location);	// Crosshair : 내 눈에서 바라보니까
	}

	// 그렇지 않을 경우
	else 
	{
		CrosshairComp->SetWorldLocation(endPos);
		distance = FVector::Distance(VRCamera->GetComponentLocation(), endPos);
	}

	// distance 값을 이용하여 크기 설정을 해준다
	// 최소 값은 1, 최대는 위에서 구한 값

	distance = FMath::Max(1, distance);
	CrosshairComp->SetWorldScale3D(FVector(distance));

	// 빌보딩 : 카메라쪽으로 바라보게 한다
	FVector direction = CrosshairComp->GetComponentLocation() - VRCamera->GetComponentLocation();
	CrosshairComp->SetWorldRotation(direction.Rotation());
}

// 일정 범위 안에 있는 물체를 잡고 싶다
// 필요 속성 : 잡을 범위
void AVRPlayer::TryGrab(const FInputActionValue& InValues)
{
	// 이미 잡고 있을 때는 그만 잡게 한다


	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	FVector handPos = RightHand->GetComponentLocation();
	TArray<FOverlapResult> hitObjects;

	bool bHit = GetWorld()->OverlapMultiByChannel(hitObjects, handPos, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(GrabRadius), params);

	// 충돌한 물체가 없으면 아무 처리하지 않는다
	if(!bHit) return;

	// 가장 가까운 물체를 검출하자
	int closest = -1;

	// 손과 가장 가까운 물체를 찾는다
	for (int i = 0; i < hitObjects.Num(); i++)
	{
		// 물체 던지기를 하고 싶다
		// 물리가 활성화되어 있는 물체만 검출
		auto hitComp = hitObjects[i].GetComponent();

		if(hitComp->IsSimulatingPhysics() == false) continue;

		if (closest == -1)
		{
			closest = i;
		}
		// 물체를 잡고 있는 상태로 설정한다
		bIsGrabbing = true;

		// 현재 가장 손과 가까운 위치를 찾는다
		FVector closestPos = hitObjects[closest].GetActor()->GetActorLocation();
		float closestDistance = FVector::Distance(closestPos, handPos);

		// 다음 물체와의 거리
		FVector nextPos = hitObjects[closest].GetActor()->GetActorLocation();
		float nextDistance = FVector::Distance(nextPos, handPos);

		// 다음 물체가 더 손과 가까우면
		// 가장 가까운 물체 인덱스 교체
		if(nextDistance < closestDistance)
			closest = i;

	}

	// 물체를 잡았다면
	if (bIsGrabbing)
	{
		GrabbedObject = hitObjects[closest].GetComponent();

		// 붙이기 전에 물리 기능을 꺼준다
		GrabbedObject->SetSimulatePhysics(false);

		// 충돌 처리도 꺼준다
		GrabbedObject->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// 물체의 위치를 손 위치로 바꿔준다
		hitObjects[closest].GetActor()->SetActorLocation(handPos);

		// 손에 붙여준다
		GrabbedObject->AttachToComponent(RightHand, FAttachmentTransformRules::KeepWorldTransform);

		UE_LOG(LogTemp, Warning, TEXT(">>>>> Grab"));
	}
}

void AVRPlayer::TryUnGrab(const FInputActionValue& InValues)
{
	// 물체를 잡고 있지 않다면 아무 처리하지 않는다
	if(bIsGrabbing == false) return;

	// 잡고 있지 않음을 명시한다
	bIsGrabbing = false;

	// 물체를 잡고 있다면 손에 붙어 있는 물체를 떼어준다
	GrabbedObject->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	// 물리 기능 활성환
	GrabbedObject->SetSimulatePhysics(true);

	// 충돌체 활성화
	GrabbedObject->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AVRPlayer::Grabbing()
{
	
}
