#include "TrainingCharacter.h"
#include "TrainingMode.h"
#include "TrainingDodgeAbility.h"
#include "LightStrikeAbility.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"

ATrainingCharacter::ATrainingCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(38.f, 90.f);
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 600.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 440.f;

	auto* Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	Arm->SetupAttachment(RootComponent);
	Arm->TargetArmLength = 350.f;
	Arm->SetRelativeLocation(FVector(0, 0, 65));
	Arm->SocketOffset = FVector(0, 48, 15);
	Arm->bUsePawnControlRotation = true;
	auto* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	Camera->SetupAttachment(Arm, USpringArmComponent::SocketName);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Body(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> Animation(TEXT("/Game/Combat/ABP_TrainingCombat"));
	GetMesh()->SetSkeletalMesh(Body.Object);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -90), FRotator(0, -90, 0));
	GetMesh()->SetAnimInstanceClass(Animation.Class);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> Movement(TEXT("/Game/Input/IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> Mouse(TEXT("/Game/Input/IMC_MouseLook"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveInput(TEXT("/Game/Input/Actions/IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookInput(TEXT("/Game/Input/Actions/IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MouseInput(TEXT("/Game/Input/Actions/IA_MouseLook"));
	MovementContext = Movement.Object;
	MouseContext = Mouse.Object;
	MoveAction = MoveInput.Object;
	LookAction = LookInput.Object;
	MouseAction = MouseInput.Object;
	CombatContext = FSoftObjectPath(TEXT("/Game/Combat/IMC_Combat.IMC_Combat"));
	StrikeAction = FSoftObjectPath(TEXT("/Game/Combat/IA_LightStrike.IA_LightStrike"));
	HeavyStrikeAction = FSoftObjectPath(TEXT("/Game/Combat/IA_HeavyStrike.IA_HeavyStrike"));
	DodgeAction = FSoftObjectPath(TEXT("/Game/Combat/IA_Dodge.IA_Dodge"));
}

UAbilitySystemComponent* ATrainingCharacter::GetAbilitySystemComponent() const
{
	const auto* PS = GetPlayerState<ATrainingPlayerState>();
	return PS ? PS->GetAbilitySystemComponent() : nullptr;
}

void ATrainingCharacter::InitializeAbilitySystem()
{
	if (auto* ASC = GetAbilitySystemComponent())
	{
		ASC->InitAbilityActorInfo(GetPlayerState(), this);
		if (HasAuthority())
		{
			const TSubclassOf<UGameplayAbility> Abilities[] = {
				ULightStrikeAbility::StaticClass(), UTrainingDodgeAbility::StaticClass()
			};
			for (const auto Ability : Abilities)
			{
				if (!ASC->FindAbilitySpecFromClass(Ability)) ASC->GiveAbility(FGameplayAbilitySpec(Ability, 1));
			}
		}
	}
}

void ATrainingCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
}

void ATrainingCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeAbilitySystem();
}

void ATrainingCharacter::UnPossessed()
{
	if (auto* ASC = GetAbilitySystemComponent(); ASC && ASC->GetAvatarActor() == this)
	{
		ASC->CancelAbilities();
		ASC->ClearActorInfo();
	}
	Super::UnPossessed();
}

void ATrainingCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	auto* PC = Cast<APlayerController>(Controller);
	if (!PC) return;
	if (auto* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Input->AddMappingContext(MovementContext, 0);
		Input->AddMappingContext(MouseContext, 0);
		if (auto* Combat = CombatContext.LoadSynchronous()) Input->AddMappingContext(Combat, 1);
	}
}

void ATrainingCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);
	auto* Enhanced = CastChecked<UEnhancedInputComponent>(Input);
	Enhanced->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	Enhanced->BindAction(MouseAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	Enhanced->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	Input->BindKey(EKeys::K, IE_Pressed, this, &ThisClass::LearnHeavyStrike);
	if (auto* Attack = StrikeAction.LoadSynchronous())
	{
		Enhanced->BindAction(Attack, ETriggerEvent::Started, this, &ThisClass::Strike);
	}
	if (auto* Attack = HeavyStrikeAction.LoadSynchronous())
	{
		Enhanced->BindAction(Attack, ETriggerEvent::Started, this, &ThisClass::HeavyStrike);
	}
	if (auto* Evade = DodgeAction.LoadSynchronous())
	{
		Enhanced->BindAction(Evade, ETriggerEvent::Started, this, &ThisClass::Dodge);
	}
}

void ATrainingCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axes = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	AddMovementInput(Yaw.Vector(), Axes.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axes.X);
}

void ATrainingCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axes = Value.Get<FVector2D>();
	AddControllerYawInput(Axes.X);
	AddControllerPitchInput(Axes.Y);
}

void ATrainingCharacter::Strike()
{
	if (ULightStrikeAbility* Active = ActiveStrike.Get())
	{
		Active->ReceiveStrikeInput();
		return;
	}

	if (auto* ASC = GetAbilitySystemComponent())
	{
		ASC->TryActivateAbilityByClass(ULightStrikeAbility::StaticClass());
	}
}

void ATrainingCharacter::HeavyStrike()
{
	if (auto* ASC = GetAbilitySystemComponent())
	{
		ASC->TryActivateAbilityByClass(UHeavyStrikeAbility::StaticClass());
	}
}

void ATrainingCharacter::LearnHeavyStrike()
{
	if (auto* PS = GetPlayerState<ATrainingPlayerState>(); PS && PS->CanLearnHeavyStrike())
	{
		PS->ServerLearnHeavyStrike();
	}
}

void ATrainingCharacter::Dodge()
{
	if (auto* ASC = GetAbilitySystemComponent())
	{
		ASC->TryActivateAbilityByClass(UTrainingDodgeAbility::StaticClass());
	}
}
