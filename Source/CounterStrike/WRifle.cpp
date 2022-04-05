// Fill out your copyright notice in the Description page of Project Settings.


#include "WRifle.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "ActorPool.h"

AWRifle::AWRifle()
{
	eWeaponNum = EWeaponNum::E_Rifle;

	bCanAutoFire = true;
	//eGunNumber = EGunNumber::AR_AK;

	RunSpeedRatio = 0.9f;

	PenatrateDecreaseDistanceRatio = 2.5f;
}


void AWRifle::ShuffleShotAnim()
{
	uint8 random = FMath::RandRange(0, 3);

	switch (random)
	{
	case 1:
		CurrentPlayingAnim = AttackAnim_2;
		break;
	case 2:
		CurrentPlayingAnim = AttackAnim_3;
		break;
	default:
		CurrentPlayingAnim = AttackAnim;
		break;
	}
	++ShotCount;
	Super::ShuffleShotAnim();
}


void AWRifle::StopFire()
{
	Super::StopFire();


	RecoilWeight = 0.7f;
	RandomRecoil = 0.f;

	RandomHorizontalRecoil = 0.f;
	RecoilHorizontalWeight = 1.f;
	dir = EC_Direction::RIGHT;
	//WeightSquare = false;
}

void AWRifle::Reload()
{
	if (AmmoCount == 0)
		return;

	Super::Reload();

	RecoilWeight = 0.7f;
	RandomRecoil = 0.f;

	RandomHorizontalRecoil = 0.f;
	RecoilHorizontalWeight = 1.f;
	dir = EC_Direction::RIGHT;
	//WeightSquare = false;
}

void AWRifle::ChangeRecoilDirection()
{
	switch(dir)
	{
	case EC_Direction::LEFT:
		dir = EC_Direction::RIGHT;
		break;
	case EC_Direction::RIGHT:
		dir = EC_Direction::LEFT;
		break;
	}

	/*GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, 
				FString::Printf(TEXT("Direction : %d"), dir));*/
	
}



void AWRifle::RandomHorizontalDirection()
{
	float retval = 0.f;

	// MouseFocusingLimit�� �ѱ� �� ���� �������� ����ݵ��� Ƥ��.
	if (RandomRecoil < MouseFocusingLimit)
	{
		retval = FMath::RandRange(-1.f, 1.f);
		Rotation.Yaw += retval;
		// ī�޶��� ��ġ �� ����
		Player->AddControllerYawInput(retval * 0.15f);
	}

	else
	{
		// ���⿡ ���� �������� �ݵ��� Ƣ���� ��.
		switch(dir)
		{
		case EC_Direction::MIDDLE:
			break;
		case EC_Direction::LEFT:
			RandomHorizontalRecoil -= FMath::RandRange(HorizontalRandomValue * 1.5f,
				HorizontalRandomValue * 1.7f);
			Player->AddControllerYawInput(RandomHorizontalRecoil * 0.15f);
			break;
		case EC_Direction::RIGHT:
			// ī�޶��� ��ġ �� ����
			RandomHorizontalRecoil += FMath::RandRange(HorizontalRandomValue * 1.3f,
				HorizontalRandomValue * 1.5f);
			Player->AddControllerYawInput(RandomHorizontalRecoil * 0.15f);
			break;
		}
		
		Rotation.Yaw += RandomHorizontalRecoil;
	}

	// ���� �ٲٱ�
	if(RandomHorizontalRecoil > RealHitImpackHorizontalLimit && dir == EC_Direction::RIGHT)
	{
		// �ܼ� dir ���� �ٲٱ� �Լ�
		ChangeRecoilDirection();
	}

	else if(RandomHorizontalRecoil < -RealHitImpackHorizontalLimit * 0.8f && dir == EC_Direction::LEFT)
	{
		ChangeRecoilDirection();
	}
}

void AWRifle::RecoilEndVec()
{
	if(!RandomRecoilFlag)
	{
		// ����ݵ��� ������ �����ϴ� �Լ�
		RandomHorizontalDirection();
		
		switch (ShotCount)
		{
			// 3�߱��� �����ݵ��� �����ϰ� Ƣ���� ����.
		case 0:
			Player->AddControllerPitchInput(-RandomRecoil * 0.1f);
			break;
		case 1:
		case 2:
		case 3:
			RandomRecoil = FMath::RandRange(0.8f, 1.15f);
			Rotation.Pitch += RandomRecoil;
			// ����ġ ���ϱ�
			RandomRecoil += RecoilWeight;
			Player->AddControllerPitchInput(-RandomRecoil * 0.15f);
			break;

		default:
			// �� ���ĺ��� ������ ���⿡ ���� �����ϰ� (RecoilWeight��ŭ) �����ݵ��� Ƣ���� ����
			if(RandomRecoil < RealHitImpactLimit)
			{
				// �ݵ� ���� ����ġ�� ���Ѵ�
				RandomRecoil += RecoilWeight;
				// ����ġ�� ����
				RecoilWeight += .1f;
				Player->AddControllerPitchInput(-RandomRecoil * 0.15f);
			}
			
			// ī�޶��� ��ġ �� ����
			if (Player->IsCrouchHeld)
			{
				RandomRecoil *= 0.6f;
			}
			
			Rotation.Pitch += RandomRecoil;
			//Player->AddControllerPitchInput(-RandomRecoil * 0.15f);
			break;
		}
	}


	Super::RecoilEndVec();
}


void AWRifle::SpawnShell()
{
	SpawnedShell = nullptr;
	BulletDecalBluePrint = nullptr;

	if (GetActorPool())
	{
		BulletDecalBluePrint = GetActorPool()->Get762Shell();
		SpawnedShell = GetWorld()->SpawnActor<AStaticMeshActor>(GetActorPool()->Get762Shell());
	}

	Super::SpawnShell();
}