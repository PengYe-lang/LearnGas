# LearnGas 迭代记录

## 2026-11-21 · 将实践路线调整为第三人称连招 RPG

- 在[主学习路线](Aura-GAS-Architecture.md)第六节把 Aura 的角色、技能、结算和养成设计映射到第三人称近战，加入三段轻击、输入缓冲、命中去重、取消策略和分阶段验收。
- 同步 README 与原版源码导读的练习方向，核对 UE 5.8 蒙太奇 API 和文档链接。本次交付为设计文档。

## 2026-11-21 · 将 Aura 原版源码接入学习路线

- 对照 DruidMech 原仓库 UE 5.3 提交 `cd631e4` 与本地副本，核实核心 GAS 源码一致，并记录读档 MVVM 相关差异。
- 在[主路线](Aura-GAS-Architecture.md)接入[上游源码导读](Aura-Upstream-Reading.md)，用课程提交、最终实现与练习串起六个阶段，增加火球与光束的设计对照。验证范围为源码与 Git 历史。

## 2026-11-21 · 分析 Aura 的 GAS 架构

- 新增 [Aura 的 GAS 架构与 LearnGas 学习路线](Aura-GAS-Architecture.md)，从本地 UE 5.3 工程追踪玩家与敌人的 ASC 初始化、火球结算、技能树、UI 和存档。
- 对照 C++、蓝图包内引用、Epic 的 Lyra 文档和 UE 5.8 头文件，记录效果上下文、范围伤害、换槽和网络表现等问题，以及对应的验证方法。本轮为静态分析，未运行 Aura 多人 PIE。

## 2026-11-21 · 建立可回退的学习基线

- **目标**：从一个已经能编译、能运行的 UE 5.8.4 C++ 工程开始学习 GAS。
- **工程内容**：保存 `MCPConnectionTest` 关卡和 `AMCPConnectionMarker`。编辑器通过 Epic 官方 MCP 放置 Actor，立方体上方使用 C++ 创建的世界空间控件显示“连接mcp成功”。
- **版本管理**：使用 Git 跟踪源码与项目配置，使用 Git LFS 跟踪关卡和 Unreal 二进制资产。将本机 Android 调试令牌移入个人配置，并在 `AGENTS.md` 中约定每次迭代的验证、记录和推送流程。
- **验证**：`LearnGasEditor Win64 Development` 编译通过。已在编辑器中运行并重新加载 `/Game/Maps/MCPConnectionTest`，确认中央立方体与中文标签可见。
