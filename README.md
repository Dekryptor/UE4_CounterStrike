# 목차
* [소개](#소개)
* [구현 내용](#구현-내용)

## 소개

* 카운터 스트라이크 온라인 리소스를 이용해 UE4로 제작
* 리슨 서버를 이용한 멀티플레이 게임 (최대 4인)
* 언리얼 UI 및 멀티플레이 튜토리얼 참고하여 제작
* 개발 기간 : 20. 1/1 ~ 21. 3/31
* 개발 인원 : 1인 
* **플레이 링크** : 준비중

## 구현 내용
* [총기 클래스 구조](#총기-클래스-구조)
* [총기 연사 및 단발 구현](#총기-연사-및-단발-구현)
* [스프레이 패턴 구현](#스프레이-패턴-구현)
* [월 샷 구현](#월-샷-구현)
* [히트박스 및 데미지 구현](#히트박스-및-데미지-구현)
* [근접 공격 구현](#근접-공격-구현)
* [데칼 표현](#데칼-표현)
* [RPC 함수](#RPC-함수)
* [크로스헤어 구현](#크로스헤어-구현)
* [기타 UI 및 애니메이션](#기타-UI-및-애니메이션)
___

### 총기 클래스 구조
* **WBase**라는 모든 무기 클래스에서 칼, 총, 폭탄으로 파생되어 생성 Base에는 무기에 따른 캐**릭터 스피드, 무기 사정거리 등** 존재
* 총기류는 라이플, 샷건, 권총, 스나이퍼, SMG로 파생되고, 소음기가 달린 무기는 추가로 상속 (M4A1, USP)
* 폭탄류는 데미지를 주는 Grenade와 섬광효과를 주는 Flash로 파생 (**현재 WSmoke 클래스는 삭제**)
<img src="https://user-images.githubusercontent.com/77636255/113281699-e1b67600-9320-11eb-960b-094108508016.PNG" width = "800">

___

### 총기 연사 및 단발 구현
* **bCanAutoFire** bool 변수를 이용하여 연사 무기와 단발 무기를 분류
* 타이머를 이용해 클릭을 유지 했을 경우 계속 공격이 나가도록 구현
* 타이머가 애니메이션 실행 후 특정 DelayTime에 실행되므로 **애니메이션 길이 - 딜레이 값**을 다음 애니메이션 실행 타이머 시간에 넣어준다.
* 클릭을 하지 않거나 단발 무기인 경우 StopFire 후 Idle로 이동 혹은 Idle로 이동
```C++
GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]() {
	// 타이머 초기화
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);

	// 애니메이션 공격을 멈추도록 false
	Player->AttackAnimCall = false;

	// RPC 함수를 호출 다른 클라이언트들에게 보이는 공격 애니메이션 설정
	if (Player->GetLocalRole() < ROLE_Authority && Player->IsLocallyControlled())
	{
		Player->SyncClientAttack(Player->AttackAnimCall, Player->DelayTime);
	}

	// 만약 마우스를 클릭하고 있다면...
	if (Player->IsAttackHeld)
	{
		// 연사가 가능한 무기라면..
		if (bCanAutoFire)
		{
			// 총을 들고 있고, 총의 현재탄약이 0이라면...
			if ((eWeaponNum == EWeaponNum::E_Rifle ||
				eWeaponNum == EWeaponNum::E_Sub) &&
				Player->GetFPSCharacterStatComponent()->GetCurrentGunWeapon()->GetCurrentAmmoCount() == 0)
			{
				// 현재 재생하고있는 애니메이션 길이와 무기 공격 후 딜레이를 빼서 Idle 애니메이션이 자연스럽게 이어지도록 WaitTime을 설정
				WaitTime = CurrentPlayingAnim->SequenceLength - GetAttackDelay();
				Idle();
			}

			else
			{
				Fire();
			}
		}

		// 아닌 경우..
		else
		{
			StopFire();
			WaitTime = CurrentPlayingAnim->SequenceLength - GetAttackDelay();
			Idle();
		}
	}

	// 클릭을 하지 않고 있다면...
	else
	{
		WaitTime = CurrentPlayingAnim->SequenceLength - GetAttackDelay();
		Idle();
	}

}, GetAttackDelay(), false);
```

___

### 스프레이 패턴 구현
<img src="https://user-images.githubusercontent.com/77636255/113293428-b471c400-9330-11eb-8ec3-926d43a51118.gif" width = "450"> AK-47| <img src="https://user-images.githubusercontent.com/77636255/113296331-4cbd7800-9334-11eb-87a6-05cbf7f44aef.gif" width = "450"> M4A1-S
:-------------------------:|:-------------------------:

* 총을 쏠 때마다 ShotCount가 하나씩 추가되며 ShotCount의 숫자에 따라 switch문을 따라
* Pitch값이 정해진 RealHitImpactLimit값을 초과 할 수 없으며 초과한 경우 Pitch값은 더 이상 올라가지 않음
* Pitch가 일정 수치에 도달한 경우 Yaw값이 왼쪽 오른쪽으로 이동하도록 만듬

```c++
	switch (ShotCount)
	{
	case 0:
		Player->AddControllerPitchInput(-RandomRecoil * 0.1f);
		break;
	case 1:
	case 2:
	case 3:
		RandomRecoil = FMath::RandRange(0.8f, 1.15f);
		Rotation.Pitch += RandomRecoil;
		RandomRecoil += RecoilWeight;
		Player->AddControllerPitchInput(-RandomRecoil * 0.15f);
		break;

	default:
		if (RandomRecoil < RealHitImpactLimit)
		{
			RandomRecoil += RecoilWeight;
			RecoilWeight += .1f;

			if (RandomRecoil > RealHitImpactLimit)
			{
				RandomRecoil = RealHitImpactLimit;
			}

			Player->AddControllerPitchInput(-RandomRecoil * 0.15f);
		}


		if (Player->IsCrouchHeld)
		{
			Rotation.Pitch += RandomRecoil * 0.6f;
		}

		else
		{
			Rotation.Pitch += RandomRecoil;
		}

		break;
	}

	if (ShotCount > 1)
	{
		float a = RandomHorizontalDirection();
		Rotation.Yaw += a;
		Player->AddControllerYawInput(a * 0.15f);
	}
```

___

### 월 샷 구현
<img src="https://user-images.githubusercontent.com/77636255/113298891-4381da80-9337-11eb-877f-89cf31e546ba.gif" width = "450"> | <img src="https://user-images.githubusercontent.com/77636255/113298950-51cff680-9337-11eb-9041-a109eecea6c2.gif" width = "450">
:-------------------------:|:-------------------------:

* 처음 총을 발사 했을 때 캐릭터, 물체에 맞았을 경우 실행 (**빨간 선**)
* 수치를 지정해서(**thickness**) 수치 만큼 이동한 벡터를 생성 (**Start**)
* 총알 쐈던 **반대 방향**으로 Trace를 실행 만약 맞았다면 관통에 성공했으므로 데칼을 생성
* 데칼을 생성한 지점에서 다시 Trace를 실행 (**파란 선**)

```c++
bool AWGun::CheckPenetrationShot(FHitResult Point, FVector Direction)
{
	// 총알이 물체 맞았다면 실행...

	// 물체의 크기가 정해진 두께보다 얇다면...
	float thickness = Point.GetActor()->GetActorScale3D().Size2D() * 150.f;
	FHitResult FinalHit;
	FVector Start = Point.ImpactPoint + Direction * thickness;

	bool bSuccess = false;

	bSuccess = GetWorld()->LineTraceSingleByChannel(FinalHit,
		Start, Start - Direction * thickness,
		ECollisionChannel::ECC_Visibility, FCollisionQueryParams());

	// 관통 성공.. 동시에 물체의 반대편에 데칼 생성
	if (bSuccess)
	{
		if (FinalHit.GetActor())
		{
			if (!FinalHit.GetActor()->IsA(AFPSCharacter::StaticClass()))
			{
				SpawnDecal(FinalHit, EDecalPoolList::EDP_BULLETHOLE);
			}

			else
			{
				SpawnDecal(FinalHit, EDecalPoolList::EDP_BLOOD);
			}

			return true;
		}
	}

	return false;
}
```

___

### 히트박스 및 데미지 구현
<img src="https://user-images.githubusercontent.com/77636255/113302987-952c6400-933b-11eb-8ea1-d4e0e4d66337.PNG" width = "500"> | <img src="https://user-images.githubusercontent.com/77636255/113303023-9e1d3580-933b-11eb-8b2d-724164e5ec53.PNG" width = "500">
:-------------------------:|:-------------------------:

* enum을 이용하여 총 4부위를 지정 (몸통, 머리, 하체, 급소)
```c++
UENUM(BlueprintType)
enum class EBoneHit : uint8
{
	EB_NONE = 0			UMETA(DisplayName = "HIT"),
	EB_HEAD = 1			UMETA(DisplayName = "HEAD_HIT"),
	EB_LEG = 2			UMETA(DisplayName = "LEG_HIT"),
	EB_GUTS = 3			UMETA(DisplayName = "GUTS_HIT"),
};
```
* 캐릭터가 맞았다면 **Hit의 BoneName**을 ToString으로 String 비교 체크
```
	if (HitBoneName.Equals(TEXT("Bip01-Head")))
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,TEXT("Head Shot!"));
		return EBoneHit::EB_HEAD;
	}

	else if (HitBoneName.Equals(TEXT("Bip01-Spine1")))
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("Shot!"));
		return EBoneHit::EB_NONE;
	}

	else if (HitBoneName.Equals(TEXT("Bip01-Pelvis")))
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("Guts Shot!"));
		return EBoneHit::EB_GUTS;
	}

	else
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("Leg or Arm Shot!"));
		return EBoneHit::EB_LEG;
	}
```
* 맞은 부위를 알아낸 후 CS의 데미지 공식에 따라 데미지를 적용
```
	switch (HitType)
	{
		case EBoneHit::EB_HEAD:
			CurrentHP -= FMath::RoundToInt(Damage * Penetration * 4.f);
			//UE_LOG(LogTemp, Warning, TEXT("%d"), FMath::RoundToInt(Damage * Penetration * 4.f));
			break;
		case EBoneHit::EB_LEG:
			CurrentHP -= FMath::RoundToInt(Damage * Penetration * 0.75f);
			//UE_LOG(LogTemp, Warning, TEXT("%d"), FMath::RoundToInt(Damage * Penetration * 0.75f));
			break;
		default:
			CurrentHP -= FMath::RoundToInt(Damage * Penetration);
			//UE_LOG(LogTemp, Warning, TEXT("%d"), FMath::RoundToInt(Damage * Penetration));
			break;
	}
```
___

### 근접 공격 구현
<img src="https://user-images.githubusercontent.com/77636255/113316751-a8dec700-9349-11eb-84c6-d55cd08bca8d.gif" width = "600">

* Anim Notify를 이용해 애니메이션 특정 구간에서 함수를 실행
```c++
void UKnifeCheckAttackAnimNotify::Notify(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation)
{
	Super::Notify(MeshComp, Animation);

	player = Cast<AFPSCharacter>(MeshComp->GetOwner());

	if (player)
	{
		Knife = Cast<AWKnife>(player->GetFPSCharacterStatComponent()->GetCurrentWeapon());

		if (Knife)
		{
			Knife->CheckAttack();
		}
	}
}
```
* 부채꼴 형태로 10개의 벡터를 생성해서 Trace를 실행
* 공격 방향에 따라 for문이 0 혹은 9부터 시작한다.
* 배열의 처음과 끝에 중앙으로 시작하는 Line을 넣어 **중앙선이 맨 처음 충돌**하도록 구현 (벽에 근접 공격 시 크로스헤어 쪽에 충돌 판정을 받아야 하기 )
* 중복히트를 방지하기 위해 for문으로 같은 액터인지 체크
```C++
	// 찌르기는 오른쪽에서 왼쪽... 일반 공격은 왼쪽에서 오른쪽...
	for (int i = 9; i >= 0; --i)
	{
		GetWorld()->LineTraceSingleByChannel(Hit, Location, vec[i], ECollisionChannel::ECC_Visibility, CollisionParams);

		if (Hit.GetActor())
		{
			if (HitPoint.Num() > 0)
			{
				bool flag = false;
				for (int j = 0; j < HitPoint.Num(); ++j)
				{
					// 다단히트 방지를 위해 액터 체크
					if (HitPoint[j].GetActor() == Hit.GetActor())
					{
						flag = true;
						if (flag)
						{
							break;
						}
					}
				}
				// 처음 충돌한 액터라면 배열에 삽입
				if (!flag)
				{
					HitPoint.Add(Hit);
				}
			}

			else
			{
				HitPoint.Add(Hit);
			}
		}

		//DrawDebugLine(GetWorld(), Location, vec[i], FColor::Red, false, 4.f, 0, 1.f);
	}

	if (HitPoint.Num() > 0)
	{
		// 만약 공격에 성공했다면 찌르는 애니메이션으로 재생시킨다.
		Player->FPSmesh[uint8(Player->GetFPSCharacterStatComponent()->GetCurrentWeaponNumber()) - 1]->PlayAnimation(ActionHitAnim, false);
	}
```
___

### 데칼 표현
<img src="https://user-images.githubusercontent.com/77636255/113320590-a0888b00-934d-11eb-96dd-96e01d14b567.gif" width = "500"> 벽이 있을 때| <img src="https://user-images.githubusercontent.com/77636255/113320629-aaaa8980-934d-11eb-9cbe-a1d53570b388.gif" width = "500"> 벽이 없을 때
:-------------------------:|:-------------------------:

* 총에 맞은 액터를 기점으로 총알이 날라온 방향으로 CheckWall 함수를 발동
* CheckWall 함수는 FHitResult, FVector, bool을 받아 Trace를 실행시켜 FHitResult를 반환한다.
```
FHitResult PenetrationResult = CheckWall(Hit, (End - Location).GetSafeNormal() * 200.f, true);
```

* Result값이 없는 경우 다시 한 번 **바닥 방향으로 CheckWall을 실행** 바닥이 있는 경우 바닥에 랜덤한 위치로 데칼 호출
```
PenetrationResult = CheckWall(Hit, -Hit.GetActor()->GetActorUpVector() * 1000.f, true);
	if (PenetrationResult.GetActor())
	{
		FVector Vec = FVector(FMath::RandRange(PenetrationResult.ImpactPoint.X - 50.f, PenetrationResult.ImpactPoint.X + 50.f),
			FMath::RandRange(PenetrationResult.ImpactPoint.Y - 50.f, PenetrationResult.ImpactPoint.Y + 50.f), PenetrationResult.ImpactPoint.Z);
	}
```
___

### RPC 함수
#### 서버로 보내는 함수를 호출 한 뒤, 멀티캐스트 함수(모든 클라이언트 전송)를 서버로 보내는 함수에서 실행
#### ⓐ 이펙트(총알 발사 이펙트) 와 스태틱 액터(탄피) 호출

<img src="https://user-images.githubusercontent.com/77636255/113413058-be5cfb00-93f4-11eb-9601-2f824e7c677a.gif" width = "500"> 1인칭 시점| <img src="https://user-images.githubusercontent.com/77636255/113413085-cd43ad80-93f4-11eb-86fc-c34f4bcf8fba.gif" width = "500"> 상대방 시점
:-------------------------:|:-------------------------:

* **IsLocallyControlled()** 함수를 이용해 **자신의 클라이언트에서는 1인칭 메쉬 소켓**에서 호출 되도록하며, **다른 플레이어가 보는 자신의 액터에서는 3인칭 메쉬 소켓**에서 호출하도록 함
```
	if (!IsLocallyControlled())
	{
		AStaticMeshActor* Shell = nullptr;
		Shell = GetWorld()->SpawnActor<AStaticMeshActor>(ShellBlueprint);

		if (Shell)
		{
			Shell->SetActorLocation(MeshComp->GetSocketLocation(TEXT("Shell")));
			Shell->GetStaticMeshComponent()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
			Shell->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
			Shell->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
			Shell->SetActorHiddenInGame(false);
			Shell->GetStaticMeshComponent()->AddImpulse(Impulse);
		}
	}
```

#### ⓑ 멀티캐스트를 이용한 데미지 함수 구현

* 멀티캐스트 함수를 호출 FConstPlayerControllerIterator를 이용하여 해당 플레이어를 찾아낸다
* 피격된 클라이언트에게 피격 효과 및 체력 감소, 방어구 감소를 적용
* 모든 플레이어에게 DamagedCharacter가 데미지를 입었다는 것을 알림
```
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (AFPSCharacter* DamagedCharacter = Cast<AFPSCharacter>(Iterator->Get()->GetCharacter()))
		{
			if (DamagedCharacter == Character && DamagedCharacter->GetFPSUIWidget())
			{
				DamagedCharacter->GetFPSUIWidget()->SetDamageUI(DirectionType);
				DamagedCharacter->GetFPSCharacterStatComponent()->SetHP(HP);
				DamagedCharacter->GetFPSCharacterStatComponent()->SetKevlar(Kevlar);
				DamagedCharacter->GetFPSUIWidget()->SetArmorAndHealth(DamagedCharacter);

				if (HP <= 0)
				{
					DamagedCharacter->SyncClientDeath(DamagedCharacter, Direction, HitType, Causer);
					DamagedCharacter->SyncClientRevive(DamagedCharacter, ReviveTime);

					AFPSCharacter* CauserFPS = Cast<AFPSCharacter>(Causer);
					if (CauserFPS)
					{
						CauserFPS->GetFPSCharacterStatComponent()->SetKillCount();
						DamagedCharacter->DoSomethingOnServer(CauserFPS->GetFPSCharacterStatComponent()->GetKillCount(), CauserFPS);
					}
				}
			}
		}
	}
```
___

### 크로스헤어 구현

* 순서대로 민감도, 색상 RGB값, 크기, 가운데 공간, 두께를 지정할 수 있다
<img src="https://user-images.githubusercontent.com/77636255/113444898-bb333080-942f-11eb-829f-21c25dffc24a.PNG">

* 사용자의 취향에 따라 다양한 크로스헤어 설정이 가능

<img src="https://user-images.githubusercontent.com/77636255/113445182-5af0be80-9430-11eb-8722-98c6e181f3f0.PNG" width = "500"> | <img src="https://user-images.githubusercontent.com/77636255/113445209-68a64400-9430-11eb-9237-2524614ae281.PNG" width = "500">
:-------------------------:|:-------------------------:

* 크로스헤어는 **Top, Bottom, Left, Right, Dot**의 5개의 Image 컴포넌트로 이루어져 있다
```
	// 색깔 지정 함수
	FLinearColor color;

	// 사용자가 정해주는 4개의 float값을 color에 넣어준다.
	color.A = Color_A;
	color.R = Color_R;
	color.G = Color_G;
	color.B = Color_B;

	Top->SetColorAndOpacity(color);
	Bottom->SetColorAndOpacity(color);
	Left->SetColorAndOpacity(color);
	Right->SetColorAndOpacity(color);
```
* 크로스 헤어 UI 컴포넌트 구성 (Overlay 컴포넌트가 5개의 Image 컴포넌트를 갖고 있음)
<img src="https://user-images.githubusercontent.com/77636255/113446987-f2a3dc00-9433-11eb-8518-99aab1a36efa.PNG">

* 사이즈를 늘릴 때 5가지 컴포넌트를 가지고 있는 부모 Overlay의 사이즈도 같이 늘려준다.
```
	if (!CanvasSlot)
	{
		CanvasSlot = Cast< UCanvasPanelSlot >(Crosshair->Slot);
	}

	FVector2D vector;

	// 사이즈 늘리기 함수
	
	// Size 변수는 사용자가 옵션에서 정하는 값
	vector.X = 6.f + Size;
	vector.Y = 6.f + Size;
	
	// Top, Bottom, Left, Right, Dot 이미지 컴포넌트를 자식으로 갖고 있는 부모 CanvasSlot의 사이즈도 같이 늘려준다.
	CanvasSlot->SetSize(vector);

	// 적절히 늘려준다.
	Top->Brush.ImageSize.Y = 3.f + Size * 0.5f;
	Bottom->Brush.ImageSize.Y = 3.f + Size * 0.5f;
	Left->Brush.ImageSize.X = 3.f + Size * 0.5f;
	Right->Brush.ImageSize.X = 3.f + Size * 0.5f;
```

* 또한 움직임 보정, 사격 보정 옵션을 추가

<img src="https://user-images.githubusercontent.com/77636255/113447185-53331900-9434-11eb-8d09-45701fbbf8e6.gif" width = "500">  움직임 보정| <img src="https://user-images.githubusercontent.com/77636255/113447207-5c23ea80-9434-11eb-988b-d793165640ea.gif" width = "500">  사격 보정
:-------------------------:|:-------------------------:

* 움직임 보정은 캐릭터의 CharacterMovement 컴포넌트의 Velocity값을 참조하여 보정
```
	if (bUseMoveDynamic)
	{
		TopVec = FVector2D(0, -Player->GetMovementComponent()->Velocity.Size() * 0.1f);
		BottomVec = FVector2D(0, Player->GetMovementComponent()->Velocity.Size() * 0.1f);
		RightVec = FVector2D(Player->GetMovementComponent()->Velocity.Size() * 0.1f, 0);
		LeftVec = FVector2D(-Player->GetMovementComponent()->Velocity.Size() * 0.1f, 0);
	}
```

* 사격 보정은 캐릭터의 현재 무기를 쏠 때마다 증가하는 ShotCount 변수를 참조하여 보정 (**StopFire 시 ShotCount가 초기화**) 
```
	if (bUseShootDynamic)
	{
		if (Player->GetFPSCharacterStatComponent())
		{
			if (Player->GetFPSCharacterStatComponent()->GetCurrentGunWeapon())
			{
				TopVec += FVector2D(0, -Player->GetFPSCharacterStatComponent()->GetCurrentGunWeapon()->GetShotCount() * 2);
				BottomVec += FVector2D(0, Player->GetFPSCharacterStatComponent()->GetCurrentGunWeapon()->GetShotCount() * 2);
				RightVec += FVector2D(Player->GetFPSCharacterStatComponent()->GetCurrentGunWeapon()->GetShotCount() * 2, 0);
				LeftVec += FVector2D(-Player->GetFPSCharacterStatComponent()->GetCurrentGunWeapon()->GetShotCount() * 2, 0);
			}
		}
	}
```

* 보정값을 전부 계산한 벡터를 **SetRenderTranslation** 함수를 이용하여 이미지를 이동시키도록 함
```
	Top->SetRenderTranslation(FMath::Vector2DInterpTo(Top->RenderTransform.Translation,
		TopVec, DeltaTime, 200.f));
	Bottom->SetRenderTranslation(FMath::Vector2DInterpTo(Bottom->RenderTransform.Translation,
		BottomVec, DeltaTime, 200.f));
	Left->SetRenderTranslation(FMath::Vector2DInterpTo(Left->RenderTransform.Translation,
		LeftVec, DeltaTime, 200.f));
	Right->SetRenderTranslation(FMath::Vector2DInterpTo(Right->RenderTransform.Translation,
		RightVec, DeltaTime, 200.f));
```
___

### 기타 UI 및 애니메이션

#### 방향에 따른 피격 UI 표시

<img src="https://user-images.githubusercontent.com/77636255/113484790-2559dd00-94e5-11eb-9ca5-b14a66323a44.gif"  width="500"> 후면 | <img src="https://user-images.githubusercontent.com/77636255/113484799-2ee34500-94e5-11eb-9888-b5648b6a14be.gif"  width="500"> 정면 |
:-------------------------:|:-------------------------:
<img src="https://user-images.githubusercontent.com/77636255/113484777-14a96700-94e5-11eb-95c3-cd1f3440f676.gif"  width="500"> 측면(왼쪽) | <img src="https://user-images.githubusercontent.com/77636255/113484783-1d01a200-94e5-11eb-80d4-f2ca9dcb9816.gif"  width="500"> 측면(오른쪽)

* 방향에 따른 enum class를 제작
* **내적으로 정면과 후면** 측정, **외적으로 측면** 측정
* 매개변수로 총을 맞은 상대 액터의 **ForwardVector와 총알의 방향** 전달
* 결과값에 따라 정해진 image의 SetVisibility 함수를 실행
```c++
	// 상대의 앞벡터와 총알의 방향 벡터를 내적 계산
	float DeathValue = FVector::DotProduct(Direction, DamagedActor->GetActorForwardVector());

	// 0.7보다 작거나, -0.7보다 작으면 왼쪽, 오른쪽으로 UI를 표시해야 하기 때문에 if문을 작성한다.
	
	// 양수일 경우 서로 평행이 되는 방향
	if (DeathValue > 0.7f)
	{
		// back..
		return EDamagedDirectionType::EDDT_BACK;
	}

	// 음수는 서로 교차가 되는 방향
	else if (DeathValue < -0.7f)
	{
		//	front..
		return EDamagedDirectionType::EDDT_FRONT;
	}

	// 그 외의 경우는 측면으로 결과값을 리턴한다.

	// 측면은 외적으로 계산
	FVector CrossVac = FVector::CrossProduct(DamagedActor->GetActorForwardVector(), -Direction);

	// 외적의 오른손 법칙에 따라 양수면..
	if (CrossVac.Z > 0)
	{
		return EDamagedDirectionType::EDDT_RIGHT;
	}

	// 음수면 왼쪽..
	else
	{
		return EDamagedDirectionType::EDDT_LEFT;
	}

	return EDamagedDirectionType::EDDT_ALL;
```

#### 변수 복제와 애님인스턴스

<img src="https://user-images.githubusercontent.com/77636255/113488942-395d0900-94fc-11eb-98ba-b575c6cd3079.gif"  width="500"> 앉기 | <img src="https://user-images.githubusercontent.com/77636255/113488950-437f0780-94fc-11eb-928d-de14a2418b5c.gif"  width="500"> 점프 |
:-------------------------:|:-------------------------:
<img src="https://user-images.githubusercontent.com/77636255/113488965-51cd2380-94fc-11eb-9ba1-dfccb8d55233.gif"  width="500"> 공격 | <img src="https://user-images.githubusercontent.com/77636255/113488985-6c9f9800-94fc-11eb-8092-61c1438c3c41.gif"  width="500"> 무기 교체

* 변수 리플렉션을 Replicated로 지정 후 GetLifetimeReplicatedProps 함수를 이용하여 변수 복제
```	
void AFPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// float..
	DOREPLIFETIME(AFPSCharacter, MoveForwardValue);
	DOREPLIFETIME(AFPSCharacter, MoveRightValue);
	DOREPLIFETIME(AFPSCharacter, AimOffsetPitch);
	DOREPLIFETIME(AFPSCharacter, AimOffsetYaw);
	DOREPLIFETIME(AFPSCharacter, IsWalkHeld);
	DOREPLIFETIME(AFPSCharacter, IsCrouchHeld);
	DOREPLIFETIME(AFPSCharacter, AttackAnimCall);
	DOREPLIFETIME(AFPSCharacter, bIsReloading);
	DOREPLIFETIME(AFPSCharacter, ReloadStartTime);
	DOREPLIFETIME(AFPSCharacter, CurrentAnimationWeaponNumber);
	DOREPLIFETIME(AFPSCharacter, DelayTime);
}
```

* AnimInstance의 NativeUpdateAnimation 함수를 이용해 ACharacter의 변수를 사용하여 애니메이션을 적용
```
	if (MyChar)
	{
		bStartJump = MyChar->IsJumpHeld;
		bCurrentFalling = MyChar->GetMovementComponent()->IsFalling();

		bIsCrouching = MyChar->IsCrouchHeld;
		bIsWalk = MyChar->IsWalkHeld;

		fForwardVal = MyChar->MoveForwardValue;
		fRightVal = MyChar->MoveRightValue;

		fCurrentLowerRotation = MyChar->CurrentLowerHipsRotation;
		fAimYaw = MyChar->AimOffsetYaw;
		fAimPitch = MyChar->AimOffsetPitch;

		ExitAnimDelayTime = MyChar->DelayTime;
		bIsAttack = MyChar->AttackAnimCall;

		bIsReload = MyChar->bIsReloading;

		eGunNumber = MyChar->CurrentAnimationWeaponNumber;
		fStartTime = MyChar->ReloadStartTime;

		deathNum = uint8(MyChar->GetFPSCharacterStatComponent()->GetDeathNum());
	}
```
#### 에임 오프셋 및 본 트랜스폼

* 액터의 회전값과 카메라의 회전값을 이용함
<img src="https://user-images.githubusercontent.com/77636255/113499655-abf8d380-9552-11eb-93c6-8eef89630df7.gif"  width="500"> | <img src="https://user-images.githubusercontent.com/77636255/113499660-b61ad200-9552-11eb-8ad3-e46c51d6128d.gif"  width="500">
:-------------------------:|:-------------------------:

* 90도 이상 꺾일 시 액터를 회전 (내적을 이용해 두 벡터가 수직일 경우 액터 회전)
<img src="https://user-images.githubusercontent.com/77636255/113499790-b5367000-9553-11eb-9969-9393ab2d2cfd.gif"  width="500">

* 본 트랜스폼을 이용해 하나의 애니메이션으로 여러 방향이동을 표현
<img src="https://user-images.githubusercontent.com/77636255/113499893-ac926980-9554-11eb-8ee1-bcf5f9dacb60.gif"  width="500"> 이동하는 모습 | <img src="https://user-images.githubusercontent.com/77636255/113499913-cd5abf00-9554-11eb-952a-9c81203298a3.PNG"  width="500"> 본을 정한 뒤 회전시킨다
___
