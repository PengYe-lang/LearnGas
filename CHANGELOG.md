# LearnGas 迭代记录

## 2026-11-21 · 建立可回退的学习基线

- **目标**：从一个已经能编译、能运行的 UE 5.8.4 C++ 工程开始学习 GAS。
- **工程内容**：保存 `MCPConnectionTest` 关卡和 `AMCPConnectionMarker`。编辑器通过 Epic 官方 MCP 放置 Actor，立方体上方使用 C++ 创建的世界空间控件显示“连接mcp成功”。
- **版本管理**：使用 Git 跟踪源码与项目配置，使用 Git LFS 跟踪关卡和 Unreal 二进制资产。将本机 Android 调试令牌移入个人配置，并在 `AGENTS.md` 中约定每次迭代的验证、记录和推送流程。
- **验证**：`LearnGasEditor Win64 Development` 编译通过。已在编辑器中运行并重新加载 `/Game/Maps/MCPConnectionTest`，确认中央立方体与中文标签可见。
