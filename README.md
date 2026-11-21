# LearnGas

用 Unreal Engine 5.8.4 和 C++ 学习 Gameplay Ability System（GAS），实践方向是第三人称连招 RPG。当前可在训练关卡打出三段轻击，每段命中通过 GAS 结算。

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
4. 打开默认关卡 `Content/Maps/MeleeTraining` 并点击播放。WASD 按镜头方向移动，鼠标转动镜头，点击左键起手。前两段各自在动作开始约半秒后再点一次，即可接到下一段。长按只起手一次。每段命中扣 20 点生命，训练靶生命归零后约两秒恢复。最初的连接测试在 `Content/Maps/MCPConnectionTest`。

## 三段轻击的代码入口

1. `ATrainingPlayerState` 持有 ASC 和属性集。`ATrainingCharacter` 绑定 Avatar，由服务端授予 `ULightStrikeAbility`。Enhanced Input 的 Started 事件发起这次激活。
2. 一次能力激活从 `AM_LightStrike` 的 `Light1` 开始。`Light1` 和 `Light2` 的 Combo 轨道各在段内 0.34 至 0.72 秒开放一格输入缓冲，在 0.74 秒决定是否衔接。只有当前段缓存的按下事件才能设置下一段。`Light3` 收招后结束激活。
3. Strike 轨道为每段指定命中窗口和采样的骨骼。权威端沿骨骼轨迹扫掠，进入新段时重置已命中集合，同一目标每段最多结算一次。攻击挥空时也可以接下一段。
4. `ULightStrikeDamage` 是即时 Gameplay Effect，写入 `IncomingDamage`。`UCombatAttributes` 消费并清零中间值，将结果扣到 Health。靶上方的血条读取同一属性。角色、基础移动和前两段出拳动画来自 UE 5.8 第三人称模板。

按住 WASD 也可以接招。`ABP_TrainingCombat` 的前两段只在脊柱以上混入攻击姿势，腰腿继续走跑。第三段播放 `MM_Attack_03`，在原动作的命中帧保留全身姿势，随后淡回走跑。第三段沿 `hand_l` 采样命中，角色位移仍由移动输入控制。

## 学习记录

[Aura 的 GAS 架构与 LearnGas 学习路线](Aura-GAS-Architecture.md) 前五节用火球分析技能激活、伤害和属性反馈。第六节将这些职责映射到第三人称近战，并给出三段轻击的输入、衔接、打断和验收规则。

[从 Aura 原项目的提交学习 GAS 架构](Aura-Upstream-Reading.md) 按六个设计阶段链接 DruidMech 的课程提交与最终 C++ 实现，并对照火球与雷电光束的职责边界。

[迭代记录](CHANGELOG.md) 按时间列出改动和验证结果。[协作约定](AGENTS.md) 说明每次迭代如何提交、推送和回退。
