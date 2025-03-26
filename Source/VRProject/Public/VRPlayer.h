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

	// Motion Controller 등록
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

	// 콘솔 명령창에서 실행 가능한 함수
	UFUNCTION(Exec)
	void ActiveDebugDraw();

	// 텔레포트를 가시화할 메쉬
	//UPROPERTY(VisibleAnywhere)
	//class UStaticMeshComponent* TeleportCircle;	
	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* TeleportCircle;	

	// 텔레포트 진행 여부
	bool bTeleporting = false;	

	// 텔레포트 리셋
	bool ResetTeleport();	

	// 텔레포트 시작
	void TeleportStart(const struct FInputActionValue& InValues);
	// 텔레포트 끝
	void TeleportEnd(const struct FInputActionValue& InValues);		

	// 직선 텔레포트 그리기 - Line Trace 이용
	void DrawTeleportStraight();	

	// 텔레포트 위치
	FVector TeleportLocation;

	// Teleport - Curve
public:
	// P = P0 + vt	: 등가속 운동
	// v = v0 + at	: 가속 운동 시 velocity
	// F = ma		: 힘의 법칙

	// 곡선을 이루는 점의 개수 (곡선의 부드러운 정도)
	UPROPERTY(EditAnywhere, Category = Teleport)
	int32 LineSmooth = 40;	// for문을 40번 돌겠다

	// Curve를 그리며 날아가는 힘의 세기
	UPROPERTY(EditAnywhere, Category = Teleport)
	float CurveForce = 1500;

	// 중력가속도
	UPROPERTY(EditAnywhere, Category = Teleport)
	float Gravity = -5000;

	// Delta time
	UPROPERTY(EditAnywhere, Category = Teleport)
	float SimulateTime = 0.02f;	

	// 기억할 점 리스트
	TArray<FVector> Lines;

	// 텔레포트 모드 전환 (Curve로 할 지 직선으로 할 지)
	UPROPERTY(EditAnywhere, Category = Teleport)
	bool bTeleportCurve = true;

	// 곡선 텔레포트 그리기
	void DrawTeleportCurve();

	// CurPos는 업데이트가 될 수 있으므로 참조 형식으로 값을 받는다
	bool CheckHitTeleport(FVector LastPos, FVector& CurPos);

	// Teleport UI
public:
	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* TeleportUIComponent;

	// Warp - Teleport와 같은 key를 쓰는 걸로
private:
	// Warp 활성화 여부
	UPROPERTY(EditAnywhere, Category = "Warp", meta = (AllowPrivateAccess=true))
	bool IsWarp = true;

	/* 빨리 이동 - 등속/등가속 운동 원리 사용 */
	// 1. 등속 운동 : 정해진 시간에 맞춰 이동하기 - Lerp를 이용해보자
	// 경과 시간
	float CurrentTime = 0;
	// Warp 타임
	UPROPERTY(EditAnywhere, Category = "Warp", meta = (AllowPrivateAccess=true))
	float WarpTime = 0.2f;

	// Warp 수행에 이용할 타이머
	FTimerHandle WarpTimer;

	// Warp 수행 함수
	void DoWarp();

	/* 총 쏘기 */
public:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_VRFire;

	// bool PerformLineTrace(FVector InStartPos, FVector InEndPos, FHitResult& InHitResult);

	void FireInput(const struct FInputActionValue& InValues);

	UPROPERTY(VisibleAnywhere)
	class UChildActorComponent* CrosshairComp;

	// crosshair 그리기
	void DrawCrosshair();

	/* 잡기 */
public:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_VRGrab;

	// 물체를 잡고 있는지 여부
	bool bIsGrabbing = false;

	// 필요 속성 : 잡을 범위
	UPROPERTY(EditAnywhere, Category = "Grab")
	float GrabRadius = 100;

	// 잡은 물체를 기억할 변수
	UPROPERTY()
	class UPrimitiveComponent* GrabbedObject;

	void TryGrab(const struct FInputActionValue& InValues);
	void TryUnGrab(const struct FInputActionValue& InValues);

	// 물체를 잡은 상태로 컨트롤하기
	void Grabbing();
};

/* meta = (AllowPrivateAccess=true)
  : 코드 상에서만 외부 클래스에서의 코드 수정 및 접근을 막고,
    에디터에서는 수정이 가능하게 함
    - 이렇게 하는 이유 : 개발 파트가 아닌 직군(=기획 파트)이 이 옵션을 건드려야 하는 경우
*/

