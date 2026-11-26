#include "LightStrikeAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CombatAttributes.h"
#include "TrainingCharacter.h"
#include "TrainingMode.h"
#include "Engine/World.h"
#include "TimerManager.h"

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
	CostGameplayEffectClass = ULightStrikeCost::StaticClass();
	DamageEffect = ULightStrikeDamage::StaticClass();
	SetAssetTags(FGameplayTagContainer(TrainingAttack));
	ActivationBlockedTags.AddTag(TrainingHitStun);
	ActivationBlockedTags.AddTag(TrainingDodging);
}

void ULightStrikeAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event)
{
	Super::ActivateAbility(Handle, Info, Activation, Event);
	auto* Character = Cast<ATrainingCharacter>(Info->AvatarActor.Get());
	auto* Montage = AttackMontage.LoadSynchronous();
	if (!Character || !Montage || !Montage->IsValidSectionName(StartSection) || !CommitAbility(Handle, Info, Activation))
	{
		EndAbility(Handle, Info, Activation, true, true);
		return;
	}

	if (const auto* PreviousStrike = Character->ActiveStrike.Get(); PreviousStrike && PreviousStrike != this)
	{
		GetAbilitySystemComponentFromActorInfo()->CancelAbilityHandle(PreviousStrike->GetCurrentAbilitySpecHandle());
	}
	ResetCombo();
	Character->ActiveStrike = this;
	PlayStrikeSection(Montage, StartSection);
	ListenForStrikeInput();
}

bool ULightStrikeAbility::CanHeavyFollowUp() const
{
	return LastPlayedSection == StrikeSections[1] && BufferedSection != LastPlayedSection && IsRecovering();
}

bool ULightStrikeAbility::IsRecovering() const
{
	if (!GraceNextSection.IsNone()) return true;

	const auto* Montage = AttackMontage.Get();
	const auto* Anim = GetActorInfo().GetAnimInstance();
	const auto* Instance = Anim ? Anim->GetActiveInstanceForMontage(Montage) : nullptr;
	if (!Instance) return !LastPlayedSection.IsNone();

	const int32 SectionIndex = Montage->GetSectionIndex(LastPlayedSection);
	if (SectionIndex == INDEX_NONE) return false;
	float StartTime, EndTime;
	Montage->GetSectionStartAndEndTime(SectionIndex, StartTime, EndTime);
	// Both light punches allow cancels once their hit windows have closed.
	const float RecoveryStart = LastPlayedSection == StrikeSections[0] ? .48f
		: LastPlayedSection == StrikeSections[1] ? .58f
		: LastPlayedSection == StrikeSections[2] ? .91f
		: (EndTime - StartTime) * .80f;
	return Instance->GetPosition() - StartTime >= RecoveryStart;
}

void ULightStrikeAbility::PlayStrikeSection(UAnimMontage* Montage, FName Section)
{
	GetWorld()->GetTimerManager().ClearTimer(GraceTimer);
	GraceNextSection = NAME_None;
	LastPlayedSection = Section;

	auto* Play = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.f, Section, true, 1.f, 0.f, true);
	Play->OnCompleted.AddDynamic(this, &ThisClass::FinishStrike);
	Play->OnInterrupted.AddDynamic(this, &ThisClass::InterruptStrike);
	Play->OnCancelled.AddDynamic(this, &ThisClass::InterruptStrike);
	Play->ReadyForActivation();

	if (IsActive())
	{
		FOnMontageSectionChanged Changed;
		Changed.BindUObject(this, &ThisClass::OnStrikeSectionChanged);
		CurrentActorInfo->GetAnimInstance()->Montage_SetSectionChangedDelegate(Changed, Montage);
	}
}

void ULightStrikeAbility::OnStrikeSectionChanged(UAnimMontage* Montage, FName Section, bool bLooped)
{
	if (IsActive() && Montage == GetCurrentMontage() && Section != LastPlayedSection)
	{
		LastPlayedSection = Section;
		if (!CommitAbilityCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
		{
			CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		}
	}
}

bool ULightStrikeAbility::TryContinueStrike(FName Section)
{
	if (!CommitAbilityCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo)) return false;
	PlayStrikeSection(AttackMontage.Get(), Section);
	return true;
}

FName ULightStrikeAbility::GetPlayingSection(const UAnimSequenceBase* Animation) const
{
	const UAnimMontage* Montage = AttackMontage.Get();
	if (!Montage || Animation != Montage || GetCurrentMontage() != Montage) return NAME_None;

	const UAnimInstance* Anim = CurrentActorInfo->GetAnimInstance();
	return Anim && Anim->Montage_IsPlaying(Montage) ? Anim->Montage_GetCurrentSection(Montage) : NAME_None;
}

void ULightStrikeAbility::OpenBufferWindow(const UAnimSequenceBase* Animation)
{
	const FName Section = GetPlayingSection(Animation);
	if (!Section.IsNone())
	{
		OpenBufferSection = Section;
	}
}

void ULightStrikeAbility::ReceiveStrikeInput()
{
	GetAbilitySystemComponentFromActorInfo()->InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed,
		GetCurrentAbilitySpecHandle(), GetCurrentActivationInfo().GetActivationPredictionKey());
}

void ULightStrikeAbility::ListenForStrikeInput()
{
	if (IsActive())
	{
		auto* Input = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
		Input->OnPress.AddDynamic(this, &ThisClass::OnStrikePressed);
		Input->ReadyForActivation();
	}
}

void ULightStrikeAbility::OnStrikePressed(float TimeWaited)
{
	const FName Section = LastPlayedSection;
	const FName NextSection = NextStrikeSection(Section);
	if (!GraceNextSection.IsNone())
	{
		TryContinueStrike(GraceNextSection);
	}
	else if (!NextSection.IsNone() && OpenBufferSection == Section && BufferedSection != Section)
	{
		BufferedSection = Section;
		if (DecisionSection == Section && GetPlayingSection(AttackMontage.Get()) == Section
			&& CheckCost(CurrentSpecHandle, CurrentActorInfo))
		{
			MontageSetNextSectionName(Section, NextSection);
		}
	}

	ListenForStrikeInput();
}

void ULightStrikeAbility::ConsumeBufferedStrike(const UAnimSequenceBase* Animation)
{
	const FName Section = GetPlayingSection(Animation);
	const FName NextSection = NextStrikeSection(Section);
	DecisionSection = Section;
	if (!NextSection.IsNone() && BufferedSection == Section && CheckCost(CurrentSpecHandle, CurrentActorInfo))
	{
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

		auto Damage = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
		if (Damage.IsValid())
		{
			HitActors.Add(Target);
			if (const auto* State = Character->GetPlayerState<ATrainingPlayerState>())
			{
				Damage.Data->SetSetByCallerMagnitude(TrainingWeaponMultiplier, State->GetWeaponMultiplier());
			}
			Damage.Data->GetContext().AddHitResult(Hit);
			GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*Damage.Data, TargetASC);
		}
	}
}

void ULightStrikeAbility::FinishStrike()
{
	const FName NextSection = NextStrikeSection(LastPlayedSection);
	if (NextSection.IsNone())
	{
		EndStrike();
	}
	else if (BufferedSection == LastPlayedSection && TryContinueStrike(NextSection))
	{
		// An input during blend-out can no longer link the old montage instance.
		return;
	}
	else if (ContinuationGracePeriod > 0.f)
	{
		GraceNextSection = NextSection;
		GetWorld()->GetTimerManager().SetTimer(GraceTimer, this, &ThisClass::EndStrike, ContinuationGracePeriod);
	}
	else
	{
		EndStrike();
	}
}

UHeavyStrikeAbility::UHeavyStrikeAbility()
{
	StartSection = TEXT("Heavy");
	DamageEffect = UHeavyStrikeDamage::StaticClass();
	CostGameplayEffectClass = UHeavyStrikeCost::StaticClass();
}

bool UHeavyStrikeAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* RelevantTags) const
{
	const auto* Character = Info ? Cast<ATrainingCharacter>(Info->AvatarActor.Get()) : nullptr;
	const auto* Strike = Character ? Character->ActiveStrike.Get() : nullptr;
	return Strike && Strike->CanHeavyFollowUp()
		&& Super::CanActivateAbility(Handle, Info, SourceTags, TargetTags, RelevantTags);
}

void ULightStrikeAbility::EndStrike()
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
	GetWorld()->GetTimerManager().ClearTimer(GraceTimer);
	ResetCombo();
	Super::EndAbility(Handle, Info, Activation, bReplicate, bCancelled);
}

void ULightStrikeAbility::ResetCombo()
{
	HitActors.Reset();
	HitSection = NAME_None;
	LastPlayedSection = NAME_None;
	OpenBufferSection = NAME_None;
	BufferedSection = NAME_None;
	DecisionSection = NAME_None;
	GraceNextSection = NAME_None;
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
	if (auto* Strike = GetActiveStrike(Mesh)) Strike->OpenBufferWindow(Animation);
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
