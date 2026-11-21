# LearnGas

用 Unreal Engine 5.8.4 和 C++ 逐步学习 Gameplay Ability System（GAS）。首个版本是可运行的工程基线，包含通过 Epic 官方 MCP 创建的测试关卡。后续技能练习按迭代保存在 Git 历史中。

## 在 Windows 上打开

1. 安装 Unreal Engine 5.8.4、支持该版本的 Visual Studio C++ 工具链和 Git LFS。
2. 克隆工程并获取关卡资产。

   ```powershell
   git lfs install
   git clone https://github.com/PengYe-lang/LearnGas.git
   cd LearnGas
   git lfs pull
   ```

3. 若系统提示找不到关联引擎，右键 `LearnGas.uproject`，使用 **Switch Unreal Engine version** 选择本机的 UE 5.8.4。生成 Visual Studio 工程文件并以 **Development Editor / Win64** 编译 `LearnGasEditor`。
4. 打开工程，在内容浏览器中双击 `Content/Maps/MCPConnectionTest`。点击播放，中央立方体上方会显示“连接mcp成功”。该文字记录首次 MCP 操作测试的结果。

## 学习记录

[迭代记录](CHANGELOG.md) 按时间列出改动和验证结果。[协作约定](AGENTS.md) 说明每次迭代如何提交、推送和回退。
