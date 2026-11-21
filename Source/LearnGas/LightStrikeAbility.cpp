#include "LightStrikeAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CombatAttributes.h"
#include "TrainingCharacter.h"
#include "Engine/World.h"

namespace
{
	const FName StrikeSections[] = { TEXT("Light1"), TEXT("Light2"), TEXT("Light3") };

	FName NextStrikeSection(FName Current)
	{
		if (Current == StrikeSections[0]) return StrikeSections[1];
		if (Current == StrikeSections[1]) return StrikeSections[2];
		return NAME_None;
	}

	ULightStrikeAbility* GetActiveStrike(const USkeletalMeshComponent* Mesh)
	{
		const auto* Character = Cast<ATrainingCharacter>(Mesh->GetOwner());
		return Character ? Character->ActiveStrike.Get() : nullptr;
	}
}

ULightStrikeAbility::ULightStrikeAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AttackMontage = FSoftObjectPath(TEXT("/Game/Combat/AM_LightStrike.AM_LightStrike"));
}

void ULightStrikeAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event)
{
	Super::ActivateAbility(Handle, Info, Activation, Event);
	auto* Character = Cast<ATrainingCharacter>(Info->AvatarActor.Get());
	auto* Montage = AttackMontage.LoadSynchronous();
	if (!Character || !Montage || !CommitAbility(Handle, Info, Activation))
	{
		EndAbility(Handle, Info, Activation, true, true);
		return;
	}

	ResetCombo();
	Character->ActiveStrike = this;
	auto* Play = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.f, StrikeSections[0], true, 1.f, 0.f, true);
	Play->OnCompleted.AddDynamic(this, &ThisClass::FinishStrike);
	Play->OnInterrupted.AddDynamic(this, &ThisClass::InterruptStrike);
	Play->OnCancelled.AddDynamic(this, &ThisClass::InterruptStrike);
	Play->ReadyForActivation();
}

FName ULightStrikeAbility::GetPlayingSection(const UAnimSequenceBase* Animation) const
{
	const UAnimMontage* Montage = AttackMontage.Get();
	if (!Montage || Animation != Montage || GetCurrentMontage() != Montage) return NAME_None;

	const UAnimInstance* Anim = CurrentActorInfo->GetAnimInstance();
	return Anim && Anim->Montage_IsPlaying(Montage) ? Anim->Montage_GetCurrentSection(Montage) : NAME_None;
}

void ULightStrikeAbility::SetBufferWindow(const UAnimSequenceBase* Animation, bool bOpen)
{
	const FName Section = GetPlayingSection(Animation);
	if (!Section.IsNone())
	{
		OpenBufferSection = bOpen ? Section : NAME_None;
	}
}

void ULightStrikeAbility::BufferNextStrike()
{
	const FName Section = GetPlayingSection(AttackMontage.Get());
	if (Section == OpenBufferSection && !NextStrikeSection(Section).IsNone() && BufferedSection.IsNone())
	{
		BufferedSection = Section;
	}
}

void ULightStrikeAbility::ConsumeBufferedStrike(const UAnimSequenceBase* Animation)
{
	const FName Section = GetPlayingSection(Animation);
	const FName NextSection = NextStrikeSection(Section);
	if (!NextSection.IsNone() && BufferedSection == Section)
	{
		BufferedSection = NAME_None;
		MontageSetNextSectionName(Section, NextSection);
	}
}

void ULightStrikeAbility::SampleStrike(const UAnimSequenceBase* Animation, FName HitSocket, bool bFirstSample)
{
	auto* Character = Cast<ATrainingCharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority()) return;

	const FName Section = GetPlayingSection(Animation);
	if (Section.IsNone()) return;

	const bool bNewSection = HitSection != Section;
	if (bNewSection)
	{
		HitSection = Section;
		HitActors.Reset();
	}

	const FVector SocketPosition = Character->GetMesh()->GetSocketLocation(HitSocket);
	const FVector Start = bFirstSample || bNewSection ? SocketPosition : PreviousStrikePosition;
	PreviousStrikePosition = SocketPosition;
	TArray<FHitResult> Hits;
	FCollisionObjectQueryParams Objects(ECC_Pawn);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(LightStrike), false, Character);
	GetWorld()->SweepMultiByObjectType(Hits, Start, SocketPosition, FQuat::Identity, Objects, FCollisionShape::MakeSphere(22.f), Query);

	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!TargetASC || HitActors.Contains(Target)) continue;
		const FVector ToTarget = Target->GetActorLocation() - Character->GetActorLocation();
		if (FVector::DotProduct(Character->GetActorForwardVector(), ToTarget) <= 0.f) continue;

		auto Damage = MakeOutgoingGameplayEffectSpec(ULightStrikeDamage::StaticClass(), GetAbilityLevel());
		if (Damage.IsValid())
		{
			HitActors.Add(Target);
			Damage.Data->GetContext().AddHitResult(Hit);
			GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*Damage.Data, TargetASC);
		}
	}
}

void ULightStrikeAbility::FinishStrike()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULightStrikeAbility::InterruptStrike()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void ULightStrikeAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, bool bReplicate, bool bCancelled)
{
	if (auto* Character = Cast<ATrainingCharacter>(Info->AvatarActor.Get()); Character && Character->ActiveStrike.Get() == this)
	{
		Character->ActiveStrike.Reset();
	}
	ResetCombo();
	Super::EndAbility(Handle, Info, Activation, bReplicate, bCancelled);
}

void ULightStrikeAbility::ResetCombo()
{
	HitActors.Reset();
	HitSection = NAME_None;
	OpenBufferSection = NAME_None;
	BufferedSection = NAME_None;
}

void ULightStrikeWindow::NotifyBegin(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float Duration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(Mesh, Animation, Duration, EventReference);
	if (auto* Strike = GetActiveStrike(Mesh))
	{
		Strike->SampleStrike(Animation, HitSocket, true);
	}
}

void ULightStrikeWindow::NotifyTick(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float DeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(Mesh, Animation, DeltaTime, EventReference);
	if (auto* Strike = GetActiveStrike(Mesh))
	{
		Strike->SampleStrike(Animation, HitSocket, false);
	}
}

ULightStrikeBufferWindow::ULightStrikeBufferWindow()
{
	bIsNativeBranchingPoint = true;
}

void ULightStrikeBufferWindow::NotifyBegin(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float Duration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(Mesh, Animation, Duration, EventReference);
	if (auto* Strike = GetActiveStrike(Mesh)) Strike->SetBufferWindow(Animation, true);
}

void ULightStrikeBufferWindow::NotifyEnd(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(Mesh, Animation, EventReference);
	if (auto* Strike = GetActiveStrike(Mesh)) Strike->SetBufferWindow(Animation, false);
}

ULightStrikeComboDecision::ULightStrikeComboDecision()
{
	bIsNativeBranchingPoint = true;
}

void ULightStrikeComboDecision::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(Mesh, Animation, EventReference);
	if (auto* Strike = GetActiveStrike(Mesh)) Strike->ConsumeBufferedStrike(Animation);
}
