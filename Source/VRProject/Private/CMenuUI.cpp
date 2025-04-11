#include "CMenuUI.h"
#include "Kismet/KismetSystemLibrary.h"

void UCMenuUI::QuitVRGame()
{
    auto pc = GetWorld()->GetFirstPlayerController();

#if WITH_EDITOR
    UKismetSystemLibrary::QuitGame(GetWorld(), pc, EQuitPreference::Quit, false);
#else
    UKismetSystemLibrary::QuitGame(GetWorld(), pc, EQuitPreference::Quit, true);
#endif


}
