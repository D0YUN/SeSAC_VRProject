#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRPlayer.generated.h"

UCLASS()
class VRPROJECT_API AVRPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	AVRPlayer();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Camera
public:
	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* VRCamera;

	// Motion Controller ���
	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class UMotionControllerComponent* LeftHand;

	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class UMotionControllerComponent* RightHand;

	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class UMotionControllerComponent* RightAim;

public:
	// IMC
	UPROPERTY(EditDefaultsOnly, Category="Input")
	class UInputMappingContext* IMC_VRInput;

	// Move
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_VRMove;
	void Move(const struct FInputActionValue& InValues);

	// Turn
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_VRMouse;
	void Turn(const struct FInputActionValue& InValues);
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bUsingMiouse = false;

	// Teleport - Line
public:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_VRTeleport;

	bool bIsDebugDraw = false;

	// �ܼ� ���â���� ���� ������ �Լ�
	UFUNCTION(Exec)
	void ActiveDebugDraw();

	// �ڷ���Ʈ�� ����ȭ�� �޽�
	//UPROPERTY(VisibleAnywhere)
	//class UStaticMeshComponent* TeleportCircle;	
	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* TeleportCircle;	

	// �ڷ���Ʈ ���� ����
	bool bTeleporting = false;	

	// �ڷ���Ʈ ����
	bool ResetTeleport();	

	// �ڷ���Ʈ ����
	void TeleportStart(const struct FInputActionValue& InValues);
	// �ڷ���Ʈ ��
	void TeleportEnd(const struct FInputActionValue& InValues);		

	// ���� �ڷ���Ʈ �׸��� - Line Trace �̿�
	void DrawTeleportStraight();	

	// �ڷ���Ʈ ��ġ
	FVector TeleportLocation;

	// Teleport - Curve
public:
	// P = P0 + vt	: ��� �
	// v = v0 + at	: ���� � �� velocity
	// F = ma		: ���� ��Ģ

	// ��� �̷�� ���� ���� (��� �ε巯�� ����)
	UPROPERTY(EditAnywhere, Category = Teleport)
	int32 LineSmooth = 40;	// for���� 40�� ���ڴ�

	// Curve�� �׸��� ���ư��� ���� ����
	UPROPERTY(EditAnywhere, Category = Teleport)
	float CurveForce = 1500;

	// �߷°��ӵ�
	UPROPERTY(EditAnywhere, Category = Teleport)
	float Gravity = -5000;

	// Delta time
	UPROPERTY(EditAnywhere, Category = Teleport)
	float SimulateTime = 0.02f;	

	// ����� �� ����Ʈ
	TArray<FVector> Lines;

	// �ڷ���Ʈ ��� ��ȯ (Curve�� �� �� �������� �� ��)
	UPROPERTY(EditAnywhere, Category = Teleport)
	bool bTeleportCurve = true;

	// � �ڷ���Ʈ �׸���
	void DrawTeleportCurve();

	// CurPos�� ������Ʈ�� �� �� �����Ƿ� ���� �������� ���� �޴´�
	bool CheckHitTeleport(FVector LastPos, FVector& CurPos);

	// Teleport UI
public:
	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* TeleportUIComponent;

	// Warp - Teleport�� ���� key�� ���� �ɷ�
private:
	// Warp Ȱ��ȭ ����
	UPROPERTY(EditAnywhere, Category = "Warp", meta = (AllowPrivateAccess=true))
	bool IsWarp = true;

	/* ���� �̵� - ���/��� � ���� ��� */
	// 1. ��� � : ������ �ð��� ���� �̵��ϱ� - Lerp�� �̿��غ���
	// ��� �ð�
	float CurrentTime = 0;
	// Warp Ÿ��
	UPROPERTY(EditAnywhere, Category = "Warp", meta = (AllowPrivateAccess=true))
	float WarpTime = 0.2f;

	// Warp ���࿡ �̿��� Ÿ�̸�
	FTimerHandle WarpTimer;

	// Warp ���� �Լ�
	void DoWarp();

	/* �� ��� */
public:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_VRFire;

	// bool PerformLineTrace(FVector InStartPos, FVector InEndPos, FHitResult& InHitResult);

	void FireInput(const struct FInputActionValue& InValues);

	UPROPERTY(VisibleAnywhere)
	class UChildActorComponent* CrosshairComp;

	// crosshair �׸���
	void DrawCrosshair();

	/* ��� */
public:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_VRGrab;

	// ��ü�� ��� �ִ��� ����
	bool bIsGrabbing = false;

	// �ʿ� �Ӽ� : ���� ����
	UPROPERTY(EditAnywhere, Category = "Grab")
	float GrabRadius = 100;

	// ���� ��ü�� ����� ����
	UPROPERTY()
	class UPrimitiveComponent* GrabbedObject;

	void TryGrab(const struct FInputActionValue& InValues);
	void TryUnGrab(const struct FInputActionValue& InValues);

	// ��ü�� ���� ���·� ��Ʈ���ϱ�
	void Grabbing();
};

/* meta = (AllowPrivateAccess=true)
  : �ڵ� �󿡼��� �ܺ� Ŭ���������� �ڵ� ���� �� ������ ����,
    �����Ϳ����� ������ �����ϰ� ��
    - �̷��� �ϴ� ���� : ���� ��Ʈ�� �ƴ� ����(=��ȹ ��Ʈ)�� �� �ɼ��� �ǵ���� �ϴ� ���
*/

