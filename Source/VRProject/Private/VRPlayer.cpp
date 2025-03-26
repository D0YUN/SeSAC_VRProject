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


// ���� �ε� - ������ �� �ε�
// ���� �ε� - ��Ÿ�� �� �ε�
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

	// TIPS) string �����͸� int������ �ٲ� �� FName -> �׷��� FString�� FName���� ������
	// �ؽ�Ű ���·� ���� - Look Up table

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


// TIP) Tick()���� �Է��� ������ ��� - Ŀ���� �Է� ť�� ���� �Է��� ť�� ��Ҵٰ� �ϳ��ϳ� ���� ó�����ش�
void AVRPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DrawCrosshair();

	// �ڷ���Ʈ Ȱ��ȭ ��
	if (bTeleporting == true)
	{
		// �ڷ���Ʈ �׸��� - � ���
		if(bTeleportCurve)
			DrawTeleportCurve();
		// �ڷ���Ʈ �׸��� - ���� ���
		else
			DrawTeleportStraight();
	}

	// Niagara Curve�� �������� �����͸� ��������
	// TEXT�� �� ���� : ������
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
	Cast�� CastChecked
	- Cast : Static cast - ������ null
	- CastChecked : ������ crash�� ��
	*/
}


void AVRPlayer::Move(const FInputActionValue& InValues)
{
	// ���1 : �� ����� ȿ����
	FVector2D Scale = InValues.Get<FVector2D>();
	FVector direction = VRCamera->GetForwardVector() * Scale.X + VRCamera->GetRightVector() * Scale.Y;

	// ���2
	/*AddMovementInput(VRCamera->GetForwardVector(), Scale.X);
	AddMovementInput(VRCamera->GetForwardVector(), Scale.Y);*/

	AddMovementInput(direction);

	//// cf) ī�޶� �ٶ󺸴� ������� ȸ��
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

// �ڷ���Ʈ ������ �ʱ�ȭ�Ѵ�
// ���� �ڷ���Ʈ�� �������� ����� �Ѱ��ش�
bool AVRPlayer::ResetTeleport()
{
	// ���� �ڷ���Ʈ ��Ŭ�� ȭ�鿡 ���̸� �̵��� �����ϴ�
	// �׷��� ������ �̵� �Ұ�
	bool bCanTeleport = TeleportCircle->GetVisibleFlag();

	// �ڷ���Ʈ ����
	bTeleporting = false;	// Teleport�� ����Ǿ��ٰ� �˷��ش�
	TeleportCircle->SetVisibility(false);	// UI�� ���ְ�
	TeleportUIComponent->SetVisibility(false);

	// �ڷ���Ʈ ���� ���θ� ����� �Ѱ��ش�
	return bCanTeleport;
}

void AVRPlayer::TeleportStart(const struct FInputActionValue& InValues)
{
	// �ڷ���Ʈ ����
	bTeleporting = true;
	TeleportUIComponent->SetVisibility(true);
	UE_LOG(LogTemp, Warning, TEXT(">>> Teleport Start"));
}

void AVRPlayer::TeleportEnd(const FInputActionValue& InValues)
{
	UE_LOG(LogTemp, Warning, TEXT(">>> Teleport End"));
	// �ڷ���Ʈ�� �������� ������ �ƹ� ó���� ���� �ʴ´�
	if (ResetTeleport() == false)
	{
		return;
	}

	// Warp�� �����ϸ� Warp�� �̵��ϰ�
	if(IsWarp) DoWarp();
	// �׷��� ������ �ڷ���Ʈ�� �̵��Ѵ�
	else SetActorLocation(TeleportLocation);
}

// Line Trace �̿��� ���� ����� Teleport - �������� �� ���� �ʿ��ϴ�
void AVRPlayer::DrawTeleportStraight()
{
	// 1. Line�� �����
	FVector StartPoint = RightHand->GetComponentLocation();
	FVector EndPoint = StartPoint + RightHand->GetForwardVector() * 1000;	// ī�޶� �ٶ󺸴� �������κ��� 10m

	// 2. ������� Line�� ���	
	bool bHit = CheckHitTeleport(StartPoint, EndPoint);

	// ���1. �� �׸��� - Debug Line �̿�
	//DrawDebugLine(GetWorld(), StartPoint, EndPoint, FColor::Red, false, -1, 0, 1);
	//if(bIsDebugDraw)
	//	DrawDebugSphere(GetWorld(), StartPoint, 200, 20, FColor::Yellow);

	// ���2. �� �׸��� - spline �̿�
	
	// ���3. �� �׸��� - Niagara Effect �̿�
	Lines.Empty();
	Lines.Add(StartPoint);
	Lines.Add(EndPoint);
}

void AVRPlayer::DrawTeleportCurve()
{
	// ����ش� - ����� �Է��� ���� ������ ����ǹǷ�
	Lines.Empty();

	// ���� ����� ��(����)
	FVector velocity = RightHand->GetForwardVector() * CurveForce;

	// P0 - ������
	FVector pos = RightHand->GetComponentLocation();
	Lines.Add(pos);
	
	// FMath::GetReflectionVector() - �Ի簢 �ݻ簢�� ���� ���� ƨ���� ������ش�
	
	// �� ������ LineSmooth�� �����ϴ� (���� ���� - 1)��ŭ �����ϰڴ�.
	for (int i = 0; i < LineSmooth; i++)
	{
		FVector lastPos = pos;

		// v = v0 + at
		velocity += FVector::UpVector * Gravity * SimulateTime;
		
		// P = P0 + vt
		pos += velocity * SimulateTime;

		// ���� ��(lastPos)�� ���� ��(pos)������ �浹 üũ�� �����Ѵ�
		bool bHit = CheckHitTeleport(lastPos, pos);
		Lines.Add(pos);

		// �ε����ٸ� ���� �׸��⸦ �����Ѵ�
		if (bHit) break;
	}

	// Line�� �׷��ش�
	// Debug Draw�� �̿��ϴ� ���
	/*int lineCount = Lines.Num();
	for (int i = 0; i < lineCount - 1; i++)
	{
		DrawDebugLine(GetWorld(), Lines[i], Lines[i + 1], FColor::Red, false, -1, 0, 1);
	}*/
}

// �� �Ǵ� ��� �׷����� �浹�� ����� �κб����� �׷��ְ� �ʹ�
bool AVRPlayer::CheckHitTeleport(FVector LastPos, FVector& CurPos)
{
	// Trace channel - visible, camera �� �� ����
	FHitResult hitInfo;
	FCollisionQueryParams params;	// �浹 üũ �ɼ�
	params.AddIgnoredActor(this);	// ���� �浹 üũ���� �����Ѵ�

	bool bHit = GetWorld()->LineTraceSingleByChannel(hitInfo, LastPos, CurPos, ECC_Visibility, params);

	// 3. Line�� �ε�����, �ε��� ���� �̸��� GroundFloor�� �ִ� ���
	if (bHit && hitInfo.GetActor()->GetActorNameOrLabel().Contains("GroundFloor") == true)
	{
		// �ڷ���Ʈ UI�� Ȱ��ȭ�Ѵ�
		TeleportCircle->SetVisibility(true);
		// �ε��� ������ �ڷ���Ʈ ��Ŭ�� ��ġ��Ų��
		TeleportCircle->SetWorldLocation(hitInfo.Location);

		// �ڷ���Ʈ ��ġ�� �����Ѵ�
		TeleportLocation = hitInfo.Location;
		UE_LOG(LogTemp, Warning, TEXT("Hit Floor Succeeded!"));

		CurPos = TeleportLocation;
	}
	// 4. �ε����� �ʾҴٸ� ��Ŭ�� ��ġ��Ű�� �ʴ´�
	else
	{
		// �ڷ���Ʈ UI�� ��Ȱ��ȭ�Ѵ�
		TeleportCircle->SetVisibility(false);
		UE_LOG(LogTemp, Warning, TEXT("Hit Other Succeeded!"));
	}

	/*
		hitInfo.GetActor()->GetName().Contains("Floor")�� if�� �ȿ� �־�� �ϴ� ����
		: �浹�ߴµ� Floor�� �ƴ� ���, �ش� ������ if�� ���� if������ ����
		  �ڷ���Ʈ UI�� ��Ȱ��ȭ�� �� ����
	*/

	return false;
}

void AVRPlayer::DoWarp()
{
	// 1. Warp�� Ȱ��ȭ�Ǿ� ���� ���� �����Ѵ�
	if(IsWarp == false) return;

	/* 2. ���� �ð����� ������ ��ġ�� �̵��ϰ� �ʹ� */
	CurrentTime = 0;
	FVector curPos = GetActorLocation();
	// �浹 ó���� ���ش�
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWorldTimerManager().SetTimer(WarpTimer, FTimerDelegate::CreateLambda(
		[this]()->void
		{
			//// 2-1. �ð��� �귯�� 
			//CurrentTime += GetWorld()->DeltaTimeSeconds;
			//// 2-2. Warp Time ���� ���� ��ġ���� target����
			//FVector targetPos = TeleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			//FVector curPos = GetActorLocation();

			//// 2-3. �̵��ϰ� �ʹ�
			//curPos = FMath::Lerp(curPos, targetPos, CurrentTime / WarpTime);
			//SetActorLocation(curPos);

			//// 3. �������� �����ߴٸ� 
			//if (CurrentTime - WarpTime >= 0)
			//{
			//	// ������ ��ġ�� ��ġ�� �������ش�
			//	// ������ ��ġ�� �����ϴ� ���� : ������ �ϸ� ��Ȯ�� targetPos�� �����ϱ⺸��
			//	SetActorLocation(targetPos);

			//	// Ÿ�̸Ӹ� ����
			//	GetWorldTimerManager().ClearTimer(WarpTimer);

			//	// �浹 ó���� ���ش�
			//	GetCapsuleComponent()->SetCollisionEnabled((ECollisionEnabled::QueryAndPhysics));
			//}

			// From->To�� Percent�� �̿��� �̵��Ѵ�
			// ���� �������� ���� �����ϸ� ������ ������ ó���Ѵ�
			// 2-1. �ð��� �귯�� 
			//CurrentTime += GetWorld()->DeltaTimeSeconds;
			// 2-2. Warp Time ���� ���� ��ġ���� target����
			FVector targetPos = TeleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			FVector curPos = GetActorLocation();

			// 2-3. �̵��ϰ� �ʹ�
			curPos = FMath::Lerp(curPos, targetPos, 10 * GetWorld()->DeltaTimeSeconds);
			SetActorLocation(curPos);

			// 3. �������� �����ߴٸ� 
			// -> ���� ���� �ȿ� ���Դٸ� - �� �浹(3d), �� �浹(2d)
			//if (CurrentTime - WarpTime >= 0)
			float distance = FVector::Distance(curPos, targetPos);
			if(distance < 10)
			{
				// ������ ��ġ�� ��ġ�� �������ش�
				// ������ ��ġ�� �����ϴ� ���� : ������ �ϸ� ��Ȯ�� targetPos�� �����ϱ⺸��
				SetActorLocation(targetPos);

				// Ÿ�̸Ӹ� ����
				GetWorldTimerManager().ClearTimer(WarpTimer);

				// �浹 ó���� ���ش�
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
		// �ε��� ����� ���� ����� Ȱ��ȭ�Ǿ� ������ ����������
		auto hitComp = hitInfo.GetComponent();
		if (hitComp && hitComp->IsSimulatingPhysics())
			hitComp->AddImpulseAtLocation( RightAim->GetForwardVector() * 1000, hitInfo.Location );
	}

}

// crosshair �׸���
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

	// �ε����� ���
	if (bHit)
	{
		CrosshairComp->SetWorldLocation(hitInfo.Location);
		distance = FVector::Distance(VRCamera->GetComponentLocation(), hitInfo.Location);	// Crosshair : �� ������ �ٶ󺸴ϱ�
	}

	// �׷��� ���� ���
	else 
	{
		CrosshairComp->SetWorldLocation(endPos);
		distance = FVector::Distance(VRCamera->GetComponentLocation(), endPos);
	}

	// distance ���� �̿��Ͽ� ũ�� ������ ���ش�
	// �ּ� ���� 1, �ִ�� ������ ���� ��

	distance = FMath::Max(1, distance);
	CrosshairComp->SetWorldScale3D(FVector(distance));

	// ������ : ī�޶������� �ٶ󺸰� �Ѵ�
	FVector direction = CrosshairComp->GetComponentLocation() - VRCamera->GetComponentLocation();
	CrosshairComp->SetWorldRotation(direction.Rotation());
}

// ���� ���� �ȿ� �ִ� ��ü�� ��� �ʹ�
// �ʿ� �Ӽ� : ���� ����
void AVRPlayer::TryGrab(const FInputActionValue& InValues)
{
	// �̹� ��� ���� ���� �׸� ��� �Ѵ�


	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	FVector handPos = RightHand->GetComponentLocation();
	TArray<FOverlapResult> hitObjects;

	bool bHit = GetWorld()->OverlapMultiByChannel(hitObjects, handPos, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(GrabRadius), params);

	// �浹�� ��ü�� ������ �ƹ� ó������ �ʴ´�
	if(!bHit) return;

	// ���� ����� ��ü�� ��������
	int closest = -1;

	// �հ� ���� ����� ��ü�� ã�´�
	for (int i = 0; i < hitObjects.Num(); i++)
	{
		// ��ü �����⸦ �ϰ� �ʹ�
		// ������ Ȱ��ȭ�Ǿ� �ִ� ��ü�� ����
		auto hitComp = hitObjects[i].GetComponent();

		if(hitComp->IsSimulatingPhysics() == false) continue;

		if (closest == -1)
		{
			closest = i;
		}
		// ��ü�� ��� �ִ� ���·� �����Ѵ�
		bIsGrabbing = true;

		// ���� ���� �հ� ����� ��ġ�� ã�´�
		FVector closestPos = hitObjects[closest].GetActor()->GetActorLocation();
		float closestDistance = FVector::Distance(closestPos, handPos);

		// ���� ��ü���� �Ÿ�
		FVector nextPos = hitObjects[closest].GetActor()->GetActorLocation();
		float nextDistance = FVector::Distance(nextPos, handPos);

		// ���� ��ü�� �� �հ� ������
		// ���� ����� ��ü �ε��� ��ü
		if(nextDistance < closestDistance)
			closest = i;

	}

	// ��ü�� ��Ҵٸ�
	if (bIsGrabbing)
	{
		GrabbedObject = hitObjects[closest].GetComponent();

		// ���̱� ���� ���� ����� ���ش�
		GrabbedObject->SetSimulatePhysics(false);

		// �浹 ó���� ���ش�
		GrabbedObject->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// ��ü�� ��ġ�� �� ��ġ�� �ٲ��ش�
		hitObjects[closest].GetActor()->SetActorLocation(handPos);

		// �տ� �ٿ��ش�
		GrabbedObject->AttachToComponent(RightHand, FAttachmentTransformRules::KeepWorldTransform);

		UE_LOG(LogTemp, Warning, TEXT(">>>>> Grab"));
	}
}

void AVRPlayer::TryUnGrab(const FInputActionValue& InValues)
{
	// ��ü�� ��� ���� �ʴٸ� �ƹ� ó������ �ʴ´�
	if(bIsGrabbing == false) return;

	// ��� ���� ������ ����Ѵ�
	bIsGrabbing = false;

	// ��ü�� ��� �ִٸ� �տ� �پ� �ִ� ��ü�� �����ش�
	GrabbedObject->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	// ���� ��� Ȱ��ȯ
	GrabbedObject->SetSimulatePhysics(true);

	// �浹ü Ȱ��ȭ
	GrabbedObject->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AVRPlayer::Grabbing()
{
	
}
