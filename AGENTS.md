# Agent Instructions

## Language

- Unless the user asks otherwise, respond in Simplified Chinese.
- Use UTF-8 encoded text.

## Project Context

- This is an Unreal Engine project.
- Before UE development work, read `.agents/ue-project-context.md`.
- Keep `.agents/ue-project-context.md` updated when gameplay systems, workflow assumptions, or known issues change.

## Local Environment

- Project path: `D:\workspace\test\ActionRoguelike`
- Engine path: `D:\EpicGame\UE_5.7`
- Full C++ verification command:

```powershell
& 'D:\EpicGame\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe' ActionRoguelikeEditor Win64 Development -NoLiveCoding -Project='D:\workspace\test\ActionRoguelike\ActionRoguelike.uproject' -WaitMutex -FromMsBuild
```

## Workflow Notes

- Prefer small, verified C++ changes.
- Do not blindly edit `.uasset` or `.umap` files unless the user explicitly asks for editor asset work.
- Do not rely on the old `C:\Users\Hejiaqi\.codex\superpowers` bootstrap path; that folder is no longer present on this machine.
- Use the available Codex skills under `C:\Users\Hejiaqi\.codex\skills` when they match the task.
- Ignore the local PowerShell profile execution-policy warning unless it blocks the actual command.
