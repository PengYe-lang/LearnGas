# LearnGas

用 Unreal Engine 5.8.4 和 C++ 学习 Gameplay Ability System（GAS），实践方向是第三人称连招 RPG。当前可玩里程碑是一段轻击：第三人称角色出拳命中训练靶，通过 GAS 扣血。

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
4. 打开默认关卡 `Content/Maps/MeleeTraining` 并点击播放。WASD 按镜头方向移动，鼠标转动镜头，左键出一拳。角色初始朝向训练靶，移动可以改变角色朝向。命中扣 20 点生命，长按只起手一次；训练靶生命归零后约两秒恢复。最初的连接测试在 `Content/Maps/MCPConnectionTest`。

## 一段轻击的代码入口

1. `ATrainingPlayerState` 持有 ASC 和属性集。`ATrainingCharacter` 绑定 Avatar，由服务端授予 `ULightStrikeAbility`。Enhanced Input 的 Started 事件发起这次激活。
2. 能力播放 `AM_LightStrike`。其 Strike 通知轨道在动作第 0.10 至 0.46 秒开放命中采样，权威端沿右手轨迹检测。每次能力激活独立记录已命中的目标。
3. `ULightStrikeDamage` 是即时 Gameplay Effect，写入 `IncomingDamage`。`UCombatAttributes` 消费并清零中间值，将结果扣到 Health；靶上方的血条读取同一属性。角色、基础移动和第一段攻击动画来自本机 UE 5.8 第三人称模板。

## 学习记录

[Aura 的 GAS 架构与 LearnGas 学习路线](Aura-GAS-Architecture.md) 前五节用火球分析技能激活、伤害和属性反馈。第六节将这些职责映射到第三人称近战，并给出三段轻击的输入、衔接、打断和验收规则。

[从 Aura 原项目的提交学习 GAS 架构](Aura-Upstream-Reading.md) 按六个设计阶段链接 DruidMech 的课程提交与最终 C++ 实现，并对照火球与雷电光束的职责边界。

[迭代记录](CHANGELOG.md) 按时间列出改动和验证结果。[协作约定](AGENTS.md) 说明每次迭代如何提交、推送和回退。
