# 从 Aura 原项目的提交学习 GAS 架构

打开 DruidMech 的 Aura 仓库，C++ 声明在 `Source/Aura/Public`，实现在 `Source/Aura/Private`，火球等技能的蓝图在 `Content/Blueprints/AbilitySystem/Aura/Abilities`。阅读时可以开两个标签页：一个看课程提交引入了什么，另一个看这个设计在最终版本中承担什么职责。这一章取自原仓库的 `main` 分支，固定到 UE 5.3 提交 `cd631e4`。[原版 C++ 源码目录](https://github.com/DruidMech/GameplayAbilitySystem_Aura/tree/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura) · [技能蓝图目录](https://github.com/DruidMech/GameplayAbilitySystem_Aura/tree/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Content/Blueprints/AbilitySystem/Aura/Abilities)

## 原版和本地副本的差异

忽略行尾差异后，本地外层 `GameplayAbilitySystem_Aura-main` 的 143 个 `Source` 文件中，138 个与上游一致。5 个有差异的文件是 `AuraGameModeBase.cpp` 以及 `MVVM_LoadScreen`、`MVVM_LoadSlot` 各自的头文件和实现。`Config` 和 `Aura.uproject` 也与上游一致。之前总结的角色初始化、火球、伤害和技能树都可以直接在这个 GitHub 版本里逐行对照。内层同名目录缺少源码。

一个可见的版本差异是存档槽名称。上游的 `UMVVM_LoadSlot` 给 `LoadSlotName` 配置了 FieldNotify 和访问函数，GameMode 通过 Getter 读取；本地副本直接访问字段。因此，研究读档界面如何响应字段变化时，应以上游这次兼容性修改为准。上述比对覆盖源码和配置；蓝图执行引脚与多人表现仍需在编辑器里验证。[上游 UE 5.3 改动明细](https://github.com/DruidMech/GameplayAbilitySystem_Aura/commit/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b)

## 六个设计决定和相应的学习练习

上游保留了 394 个提交。以下从中选择了六个适合在 LearnGas 单独实现、单独验收的阶段；每个阶段都能在原项目中找到对应的设计转折。

### 1. 先定状态的寿命，再选 ASC 挂在哪里

玩家的 PlayerState 持有 ASC 和属性集，受控角色把自己注册为 Avatar。敌人的 ASC 则直接挂在敌人身上。两种对象都能成为伤害目标，但玩家重新附身时可以保留已授予能力，普通敌人的能力可以随敌人销毁。把两个初始化函数并排读，重点看传给同一个 GAS API 的两个 Actor。**练习**：更换玩家 Pawn，记录 Owner、Avatar 和已授予的技能句柄，分别判断哪些应保留、哪些应变化。[引入 ActorInfo 的提交](https://github.com/DruidMech/GameplayAbilitySystem_Aura/commit/091575d7ad487bea3d4279d65e1753e8163d3b3b) · [玩家初始化](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/Character/AuraCharacter.cpp#L328) · [敌人初始化](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/Character/AuraEnemy.cpp#L148)

### 2. 把效果的定义与这次应用分开

`AAuraEffectActor` 让区域分别配置进入、离开时施加的效果。对于指定为离开时移除的无限效果，它保存本次应用返回的 ActiveEffectHandle 和目标 ASC，离开时按句柄撤销。两个区域都加护甲，它们授予的效果仍要能分别追踪。这里自然引出一个后续设计问题：哪些效果应独立，哪些应按来源堆叠？**练习**：让两个区域各授予一份独立增益，离开其中一个后，检查剩余那份的来源、数值和句柄。[课程提交](https://github.com/DruidMech/GameplayAbilitySystem_Aura/commit/7e373159224e1247991fce6eb2ab36f1cb5edabf) · [效果应用与撤销](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/Actor/AuraEffectActor.cpp#L65)

### 3. 将异步瞄准封装成技能任务

`TargetDataUnderMouse` 在本地收集光标命中并通过预测窗口传递；远端按技能句柄和预测键查找数据。它同时处理“服务器已经等着数据”和“数据在监听注册之前到达”两种顺序。这样技能蓝图可以围绕一份目标结果继续编排，网络时序集中在任务里。**练习**：让两次连续施法指向不同位置，分别记录各次预测键、目标数据和服务端生成的投射物，确认目标对应正确。[接收目标数据的提交](https://github.com/DruidMech/GameplayAbilitySystem_Aura/commit/04b42850529345f1d8a59b0ad4b59887852a8130) · [任务实现](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/AbilitySystem/AbilityTasks/TargetDataUnderMouse.cpp)

### 4. 给“伤害是多少”和“受伤后发生什么”各设一个入口

课程先引入 `IncomingDamage` 中间属性，后来才加入 Execution，以便在源和目标的属性之间计算抗性、格挡和护甲。最终的执行类输出伤害；目标属性集消费该值、清零、扣血并处理生死。这一层划分让“新加一种伤害计算”和“所有死亡都触发经验奖励”可以独立改动。**练习**：把一次直接伤害和一跳灼烧分别送到同一目标，跟踪两条路径是否都经过同一个扣血入口；随后用主路线里的 40 点例子复算输出。[中间属性提交](https://github.com/DruidMech/GameplayAbilitySystem_Aura/commit/ca9d9c614456fd87215a37955b38e5b606960342) · [Execution 提交](https://github.com/DruidMech/GameplayAbilitySystem_Aura/commit/8314c0abffa92c8bf2ea0b503100e1484b409375) · [伤害消费位置](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/AbilitySystem/AuraAttributeSet.cpp#L159)

### 5. 从公共目录和个人技能规格组装界面

`BroadcastAbilityInfo` 遍历玩家已授予的 AbilitySpec，按能力标签查出目录记录的副本，再把这名玩家的槽位与状态填进去，最后广播给界面。图标、能力类型和所需等级可以共享；槽位、等级和解锁状态属于每名玩家。锁定技能的预览还需要单独查询公共目录，因为此时可能根本没有对应 Spec。**练习**：让两个玩家把同一技能放在不同槽位，同时验证未解锁的技能也能显示说明。[引入技能目录](https://github.com/DruidMech/GameplayAbilitySystem_Aura/commit/735735655075c60e5726109d5f4b8116eafdc3d1) · [技能点操作](https://github.com/DruidMech/GameplayAbilitySystem_Aura/commit/baa579d54884550130b5c6dd76e07c4f2f8f60a3) · [展示数据组装](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/UI/WidgetController/AuraWidgetController.cpp#L30)

### 6. 用持久数据重建技能，而非保存一次运行的句柄

存档记录技能类、身份标签、等级、状态、槽位和类型。读档时重新构造 Spec，并按被动技能的装备状态决定是否激活。设计上的关键是：同一角色通过“首次授予”或“从磁盘恢复”进入游戏时，应建立等价的玩法状态；这一轮重新生成的运行时句柄可以不同。**练习**：保存一个已装备被动和一个解锁未装备的主动技能，重进后分别检查 UI、特效、输入和技能等级。[加载技能的提交](https://github.com/DruidMech/GameplayAbilitySystem_Aura/commit/78f19b6ce40621066a7e1cea21c63a8ddc4a0e79) · [保存入口](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/Character/AuraCharacter.cpp#L217) · [技能重建](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp#L21)

## 用火球和雷电光束检验复用边界

火球在服务端从战斗接口取得施法插槽，按技能等级与投射物上限决定发射数量，为每个投射物准备伤害参数。之后的目标由飞行碰撞确定。雷电光束则先追踪主目标，再以它为中心，在 850 个世界单位内搜寻候选目标；额外目标数取“技能等级减一”与配置上限的较小值，并对选中的目标订阅死亡事件。因此，两者真正不同的是目标如何产生、目标能保持多久，以及何时触发效果。[火球的发射策略](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/AbilitySystem/Abilities/AuraFireBolt.cpp#L103) · [光束的目标选择与死亡订阅](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/AbilitySystem/Abilities/AuraBeamSpell.cpp#L32)

把这个边界带到 LearnGas 的近战练习中：普通轻击由角色动作的有效窗口判定命中，未来的飞行技能可由投射物碰撞判定；两者分别产生目标，再复用同一个伤害 Spec 与属性结算入口。可以用一项改动检查拆分是否有效：调整敌人的物理抗性时，三段轻击应一起受影响；调整第二段的接招窗口时，只改变操作手感。[效果构建](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/AbilitySystem/AuraAbilitySystemLibrary.cpp#L512) · [伤害计算](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/AbilitySystem/ExecCalc/ExecCalc_Damage.cpp#L107)

## 带着三个问题复核原版

- **命中上下文复制后还剩哪些字段？** 对照派生类型返回值、Duplicate 分配类型和序列化位数，再检查非零范围中心标记占用哪一位。这能解释为何单机命中正确还不足以证明网络结果正确。[类型与复制](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Public/AuraAbilityTypes.h#L113) · [序列化标记](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/AuraAbilityTypes.cpp#L93)
- **换槽时谁记录被替换的技能？** 跟踪新旧两份 Spec，检查状态、槽位和标脏位置，再对照客户端收到装备通知的位置。UI 显示、服务端真值、读档恢复三者应能相互印证。[服务端换槽](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp#L336)
- **谁负责回调与临时数据的寿命？** 范围伤害分支把局部变量按引用绑定到目标委托；异常效果分支动态分配 Spec 后按引用应用。这两处都适合在复用前明确对象的释放与解绑责任。[范围伤害回调](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/AbilitySystem/ExecCalc/ExecCalc_Damage.cpp#L188) · [异常效果的创建与应用](https://github.com/DruidMech/GameplayAbilitySystem_Aura/blob/cd631e409fcb7f78f3a6d8bc3a58496a93c8d52b/Source/Aura/Private/AbilitySystem/AuraAttributeSet.cpp#L218)

完整的伤害公式、六项实现问题及对应验收场景见[主学习路线](Aura-GAS-Architecture.md)。
