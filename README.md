# LearnGas

用 Unreal Engine 5.8.4 和 C++ 学习 Gameplay Ability System（GAS），实践方向是第三人称连招 RPG。训练关卡包含三段轻击、重击派生、后摇闪步和受击打断。

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
4. 打开默认关卡 `Content/Maps/MeleeTraining` 并点击播放。WASD 按镜头方向移动，鼠标转动镜头，点击左键起手。前两段约在动作开始 0.34 秒后可再次点按，收招播完后还有 0.30 秒可以接下一段。长按只起手一次。每段命中扣 20 点生命，训练靶生命归零后约两秒恢复。最初的连接测试在 `Content/Maps/MCPConnectionTest`。

## 重击、闪步与受击

- **轻二接重击**。学会重击后，左键打出前两段，在第二拳收招时点击右键。重击消耗 30 点体力，徒手命中扣训练靶 45 点生命。徒手打完整套后，满血训练靶剩 15 点生命。
- **后摇闪步**。待机时或攻击结束命中窗口后按左 Shift，沿移动输入方向闪步。松开方向键时向面朝方向闪步。`AM_Dodge` 使用全身姿势和根运动，空地上约 0.6 秒滑出 2.3 米。闪步消耗 24 点体力，起手的 0.24 秒内免受训练脉冲伤害。
- **体力与受击**。每段轻击消耗 12 点，停止消耗约 1.3 秒后开始回复。走进训练场旁的圆形脉冲区，等提示「脉冲即将释放」时出招，即可观察受击如何取消当前攻击和已缓存的接招。

## 经验、升级与招式解锁

按 Q 可在徒手和训练拳套之间切换。武器栏显示当前伤害倍率。训练拳套默认使用 1.5 倍，数值在 `Config/DefaultGame.ini` 的 `GauntletMultiplier` 中配置。出招和接招宽限期间需先收招才能换装。

1. 在 `MeleeTraining` 用轻击击倒训练靶。每次击倒奖励 50 经验，训练靶约两秒后恢复。
2. 累计 100 经验后升到 2 级并获得 1 点技能点。按 `K` 花费 1 点学习重击，界面会提示轻二收招时按右键。
3. 经验计入 `PlayerState` 上的 `UTrainingProgressionAttributes`，每累计 100 经验升一级并获得 1 点技能点。已学招式以 GAS 的 Ability Spec 为准。同一场游戏换 Pawn 会继续使用该玩家的进度。

### 从训练靶到实战陪练

在脉冲区旁交叠的蓝色平台上，每层获得 25 护甲和 20% 火抗。训练拳套的轻三命中会点燃仍存活的目标。按 Tab 打开技能册，2 级起可以用 1 点学习重击（K）、守势（J）或焰弹（B）。P 装卸守势，R 向屏幕准星发射焰弹。可分别瞄准出生点前的近靶与右前方的远靶，比较近战命中窗口和投射物飞行碰撞。

从出生点向左前方靠近陪练守卫。看到蓄力提示可以闪到侧面，或退到出拳距离之外。守卫脱战后走回原位，每次击倒奖励 50 经验，约 5 秒后复起。

在守卫试炼圆台按 E 开始两波挑战。第一波一人，第二波两人。离开圆台约 8.5 米或倒地会结束本轮；回到圆台即可重试。击倒每个守卫获得 50 经验，首次通关另获 100；F6 保存首通记录。

### 单机训练存档

按 F6 保存当前经验、技能点、已学技能、被动槽、装备、生命和体力。按 F9 可在训练中恢复，角色从待机重新出招。重新进入游戏时会自动读取最近的有效存档，等级按累计经验恢复。训练靶和场景按关卡初始状态生成。

`Saved/SaveGames` 下的 `LearnGas_Training_A.sav` 和 `LearnGas_Training_B.sav` 交替写入。读取前先校验文件完整性和数据版本，最近一次写入损坏时可恢复上一次有效进度。

## 三段轻击的代码入口

1. `ATrainingPlayerState` 持有 ASC 和属性集。`ATrainingCharacter` 绑定 Avatar，由服务端授予 `ULightStrikeAbility`。Enhanced Input 的 Started 事件发起这次激活。
2. 一次能力激活从 `AM_LightStrike` 的 `Light1` 开始。前两段在段内 0.34 秒开放一格输入缓冲，蒙太奇仍可设置下一段时走段间衔接。遇到收招混出则在该段播完后接续。自然收招后仍有 `ContinuationGracePeriod`（0.30 秒）可从下一段播放；超过宽限或三段打完后，下次出招从 `Light1` 重新开始。
3. Strike 轨道为每段指定命中窗口和采样的骨骼。权威端沿骨骼轨迹扫掠，进入新段时重置已命中集合，同一目标每段最多结算一次。攻击挥空时也可以接下一段。
4. `ULightStrikeDamage` 是即时 Gameplay Effect，由伤害 Execution 捕获目标防御并写入 `IncomingDamage`。`UCombatAttributes` 消费并清零中间值，将结果扣到 Health。靶上方的血条读取同一属性。角色、基础移动和前两段出拳动画来自 UE 5.8 第三人称模板。

按住 WASD 也可以接招。`ABP_TrainingCombat` 的前两段只在脊柱以上混入攻击姿势，腰腿继续走跑。第三段播放 `MM_Attack_03`，在原动作的命中帧保留全身姿势，随后淡回走跑。第三段沿 `hand_l` 采样命中，角色位移仍由移动输入控制。

## 学习记录

[Aura 的 GAS 架构与 LearnGas 学习路线](Aura-GAS-Architecture.md) 前五节用火球分析技能激活、伤害和属性反馈。第六节将这些职责映射到第三人称近战，并给出三段轻击的输入、衔接、打断和验收规则。

[从 Aura 原项目的提交学习 GAS 架构](Aura-Upstream-Reading.md) 按六个设计阶段链接 DruidMech 的课程提交与最终 C++ 实现，并对照火球与雷电光束的职责边界。

[迭代记录](CHANGELOG.md) 按时间列出改动和验证结果。[协作约定](AGENTS.md) 说明每次迭代如何提交、推送和回退。
