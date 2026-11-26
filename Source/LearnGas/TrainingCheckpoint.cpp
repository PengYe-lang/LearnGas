#include "TrainingCheckpoint.h"
#include "CombatAttributes.h"
#include "TrainingMode.h"
#include "TrainingProgressionAttributes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"

namespace
{
	const FString Slots[] = { TEXT("LearnGas_Training_A"), TEXT("LearnGas_Training_B") };
}

bool UTrainingCheckpoint::IsValidProgress() const
{
	const int32 EarnedPoints = Experience / UTrainingProgressionAttributes::ExperiencePerLevel;
	const int32 SpentPoints = bLearnedHeavy ? ATrainingPlayerState::HeavyStrikeSkillPointCost : 0;
	return Version == 1 && Generation > 0 && Experience >= 0 && SkillPoints >= 0
		&& SkillPoints == EarnedPoints - SpentPoints
		&& (!bLearnedHeavy || EarnedPoints + 1 >= ATrainingPlayerState::HeavyStrikeRequiredLevel)
		&& FMath::IsFinite(Health) && Health >= 0.f && FMath::IsFinite(Stamina)
		&& Stamina >= 0.f && Stamina <= UCombatAttributes::StaminaCapacity;
}

UTrainingCheckpoint* UTrainingCheckpoint::LoadLatest(int32& SlotIndex)
{
	UTrainingCheckpoint* Latest = nullptr;
	SlotIndex = INDEX_NONE;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slots); ++Index)
	{
		if (!UGameplayStatics::DoesSaveGameExist(Slots[Index], 0)) continue;
		TArray<uint8> Bytes;
		if (!UGameplayStatics::LoadDataFromSlot(Bytes, Slots[Index], 0) || Bytes.Num() <= sizeof(uint32)) continue;
		const int32 PayloadSize = Bytes.Num() - sizeof(uint32);
		uint32 StoredChecksum;
		FMemory::Memcpy(&StoredChecksum, Bytes.GetData() + PayloadSize, sizeof(StoredChecksum));
		if (FCrc::MemCrc32(Bytes.GetData(), PayloadSize) != StoredChecksum) continue;
		Bytes.SetNum(PayloadSize);
		auto* Candidate = Cast<UTrainingCheckpoint>(UGameplayStatics::LoadGameFromMemory(Bytes));
		if (Candidate && Candidate->IsValidProgress() && (!Latest || Candidate->Generation > Latest->Generation))
		{
			Latest = Candidate;
			SlotIndex = Index;
		}
	}
	return Latest;
}

bool UTrainingCheckpoint::WriteNext()
{
	int32 LatestIndex;
	const auto* Latest = LoadLatest(LatestIndex);
	if (Latest && Latest->Generation == MAX_int64) return false;
	Generation = Latest ? Latest->Generation + 1 : 1;
	TArray<uint8> Bytes;
	if (!IsValidProgress() || !UGameplayStatics::SaveGameToMemory(this, Bytes)) return false;
	const uint32 Checksum = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
	Bytes.Append(reinterpret_cast<const uint8*>(&Checksum), sizeof(Checksum));
	return UGameplayStatics::SaveDataToSlot(Bytes, Slots[LatestIndex == 0 ? 1 : 0], 0);
}
